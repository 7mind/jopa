#include "jast_semantic.h"
#include "control.h"
#include "stream.h"
#include "option.h"

namespace Jopa {

//=============================================================================
// CONSTRUCTOR / DESTRUCTOR
//=============================================================================

JastSemantic::JastSemantic(Control& control, FileSymbol* file_symbol)
    : control_(control)
    , file_symbol_(file_symbol)
    , lex_stream_(file_symbol->lex_stream)
    , this_package_(file_symbol->package)
    , compilation_unit_(nullptr)
    , current_scope_(nullptr)
{
}

JastSemantic::~JastSemantic() {
    while (current_scope_) {
        popScope();
    }
}

//=============================================================================
// MAIN ENTRY POINT
//=============================================================================

bool JastSemantic::analyze(jast2::CompilationUnit* unit) {
    if (!unit) return false;

    compilation_unit_ = unit;
    unit->lex_stream = lex_stream_;

    // Phase 1: Process imports
    processImports();

    // Phase 2: Build type declarations (create TypeSymbols)
    processTypeDeclarations();

    // Phase 3: Resolve type headers (extends, implements)
    resolveTypeHeaders();

    // Phase 4: Process members (create MethodSymbols, VariableSymbols)
    processMembers();

    // Phase 5: Process method bodies
    processBodies();

    unit->has_errors = hasErrors();
    return !hasErrors();
}

//=============================================================================
// ERROR REPORTING
//=============================================================================

void JastSemantic::reportError(jast2::Node* node, const std::string& message) {
    int line = node ? node->location.line : 0;
    int column = node ? node->location.column : 0;
    reportError(line, column, message);
}

void JastSemantic::reportError(int line, int column, const std::string& message) {
    JastSemanticError err;
    err.severity = JastErrorSeverity::ERROR;
    err.line = line;
    err.column = column;
    err.message = message;
    if (file_symbol_ && file_symbol_->FileName()) {
        err.filename = file_symbol_->FileName();
    }
    errors_.push_back(err);
}

void JastSemantic::reportWarning(jast2::Node* node, const std::string& message) {
    JastSemanticError err;
    err.severity = JastErrorSeverity::WARNING;
    err.line = node ? node->location.line : 0;
    err.column = node ? node->location.column : 0;
    err.message = message;
    errors_.push_back(err);
}

//=============================================================================
// SCOPE MANAGEMENT
//=============================================================================

void JastSemantic::pushScope() {
    JastScope* new_scope = new JastScope(current_scope_);
    current_scope_ = new_scope;
}

void JastSemantic::popScope() {
    if (current_scope_) {
        JastScope* old = current_scope_;
        current_scope_ = current_scope_->parent;
        delete old;
    }
}

void JastSemantic::enterType(TypeSymbol* type) {
    pushScope();
    current_scope_->enclosing_type = type;
    this_type_ = type;
}

void JastSemantic::exitType() {
    popScope();
    this_type_ = current_scope_ ? current_scope_->enclosing_type : nullptr;
}

void JastSemantic::enterMethod(MethodSymbol* method) {
    pushScope();
    current_scope_->enclosing_method = method;
    current_scope_->is_static = method->ACC_STATIC();
}

void JastSemantic::exitMethod() {
    popScope();
}

//=============================================================================
// PHASE 1: IMPORTS
//=============================================================================

void JastSemantic::processImports() {
    // Add java.lang.* implicitly
    PackageSymbol* java_lang = control_.LangPackage();
    if (java_lang) {
        type_import_on_demand_.push_back(java_lang);
    }

    // Process explicit imports
    for (auto& import : compilation_unit_->imports) {
        processImport(import.get());
    }
}

void JastSemantic::processImport(jast2::ImportDeclaration* import) {
    if (!import || !import->name) return;

    std::string full_name = nameToString(import->name.get());

    if (import->isStatic) {
        // Static import
        if (import->isOnDemand) {
            // import static com.foo.Bar.*;
            TypeSymbol* type = resolveQualifiedType(full_name);
            if (type) {
                static_import_on_demand_.push_back(type);
                import->imported_type = type;
            } else {
                reportError(import, "Cannot resolve type: " + full_name);
            }
        } else {
            // import static com.foo.Bar.baz;
            // Need to resolve the type and find the member
            size_t last_dot = full_name.rfind('.');
            if (last_dot != std::string::npos) {
                std::string type_name = full_name.substr(0, last_dot);
                std::string member_name = full_name.substr(last_dot + 1);
                TypeSymbol* type = resolveQualifiedType(type_name);
                if (type) {
                    import->imported_type = type;
                    // Check for field
                    VariableSymbol* field = findField(type, member_name);
                    if (field && field->ACC_STATIC()) {
                        single_static_imports_[member_name] = field;
                    }
                    // Check for method (simplified - just note the type for now)
                } else {
                    reportError(import, "Cannot resolve type: " + type_name);
                }
            }
        }
    } else {
        // Type import
        if (import->isOnDemand) {
            // import com.foo.*;
            // Try to find as package
            PackageSymbol* pkg = resolvePackage(full_name);
            if (pkg) {
                type_import_on_demand_.push_back(pkg);
                import->imported_package = pkg;
            } else {
                // Could be a type for nested classes
                TypeSymbol* type = resolveQualifiedType(full_name);
                if (type) {
                    static_import_on_demand_.push_back(type);
                    import->imported_type = type;
                } else {
                    reportError(import, "Cannot resolve package or type: " + full_name);
                }
            }
        } else {
            // import com.foo.Bar;
            TypeSymbol* type = resolveQualifiedType(full_name);
            if (type) {
                size_t last_dot = full_name.rfind('.');
                std::string simple_name = (last_dot != std::string::npos)
                    ? full_name.substr(last_dot + 1) : full_name;

                // Check for conflict
                auto it = single_type_imports_.find(simple_name);
                if (it != single_type_imports_.end() && it->second != type) {
                    reportError(import, "Conflicting import: " + full_name);
                } else {
                    single_type_imports_[simple_name] = type;
                }
                import->imported_type = type;
            } else {
                reportError(import, "Cannot resolve type: " + full_name);
            }
        }
    }
}

PackageSymbol* JastSemantic::resolvePackage(const std::string& name) {
    // Convert dot-separated name to package lookup
    PackageSymbol* pkg = control_.UnnamedPackage();
    size_t start = 0;
    size_t pos;

    while ((pos = name.find('.', start)) != std::string::npos) {
        std::string part = name.substr(start, pos - start);
        std::wstring wpart(part.begin(), part.end());
        NameSymbol* name_sym = control_.FindOrInsertName(wpart.c_str(), wpart.length());
        pkg = pkg->FindPackageSymbol(name_sym);
        if (!pkg) return nullptr;
        start = pos + 1;
    }

    std::string part = name.substr(start);
    std::wstring wpart(part.begin(), part.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wpart.c_str(), wpart.length());
    return pkg->FindPackageSymbol(name_sym);
}

//=============================================================================
// PHASE 2: TYPE DECLARATIONS
//=============================================================================

void JastSemantic::processTypeDeclarations() {
    for (auto& type_decl : compilation_unit_->types) {
        processTypeDeclaration(type_decl.get(), nullptr);
    }
}

TypeSymbol* JastSemantic::processTypeDeclaration(jast2::Declaration* decl, TypeSymbol* enclosing) {
    if (!decl) return nullptr;

    if (auto* cls = dynamic_cast<jast2::ClassDeclaration*>(decl)) {
        return processClassDeclaration(cls, enclosing);
    } else if (auto* iface = dynamic_cast<jast2::InterfaceDeclaration*>(decl)) {
        return processInterfaceDeclaration(iface, enclosing);
    } else if (auto* enm = dynamic_cast<jast2::EnumDeclaration*>(decl)) {
        return processEnumDeclaration(enm, enclosing);
    } else if (auto* ann = dynamic_cast<jast2::AnnotationDeclaration*>(decl)) {
        return processAnnotationDeclaration(ann, enclosing);
    }

    return nullptr;
}

TypeSymbol* JastSemantic::processClassDeclaration(jast2::ClassDeclaration* decl, TypeSymbol* enclosing) {
    if (!decl || decl->name.empty()) return nullptr;

    // Create the TypeSymbol
    std::wstring wname(decl->name.begin(), decl->name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    TypeSymbol* type = enclosing
        ? enclosing->InsertNestedTypeSymbol(name_sym)
        : this_package_->InsertOuterTypeSymbol(name_sym);

    if (!type) {
        reportError(decl, "Duplicate type declaration: " + decl->name);
        return nullptr;
    }

    // Set up the type
    type->file_symbol = file_symbol_;
    type->SetOwner(enclosing ? static_cast<Symbol*>(enclosing) : static_cast<Symbol*>(this_package_));
    type->outermost_type = enclosing ? enclosing->outermost_type : type;
    type->supertypes_closure = new SymbolSet;
    type->subtypes = new SymbolSet;
    type->SetACC_PUBLIC();  // TODO: process modifiers properly

    if (decl->modifiers) {
        if (decl->modifiers->isAbstract()) type->SetACC_ABSTRACT();
        if (decl->modifiers->isFinal()) type->SetACC_FINAL();
        if (decl->modifiers->isPublic()) type->SetACC_PUBLIC();
        if (decl->modifiers->isProtected()) type->SetACC_PROTECTED();
        if (decl->modifiers->isPrivate()) type->SetACC_PRIVATE();
        if (decl->modifiers->isStatic()) type->SetACC_STATIC();
    }

    // Link jast2 <-> symbol
    type->jast2_class_decl = decl;
    decl->symbol = type;

    // Process nested types
    if (decl->body) {
        for (auto& member : decl->body->declarations) {
            processTypeDeclaration(member.get(), type);
        }
    }

    // Add to file's type list
    file_symbol_->types.Next() = type;

    return type;
}

TypeSymbol* JastSemantic::processInterfaceDeclaration(jast2::InterfaceDeclaration* decl, TypeSymbol* enclosing) {
    if (!decl || decl->name.empty()) return nullptr;

    std::wstring wname(decl->name.begin(), decl->name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    TypeSymbol* type = enclosing
        ? enclosing->InsertNestedTypeSymbol(name_sym)
        : this_package_->InsertOuterTypeSymbol(name_sym);

    if (!type) {
        reportError(decl, "Duplicate type declaration: " + decl->name);
        return nullptr;
    }

    type->file_symbol = file_symbol_;
    type->SetOwner(enclosing ? static_cast<Symbol*>(enclosing) : static_cast<Symbol*>(this_package_));
    type->outermost_type = enclosing ? enclosing->outermost_type : type;
    type->supertypes_closure = new SymbolSet;
    type->subtypes = new SymbolSet;
    type->SetACC_INTERFACE();
    type->SetACC_ABSTRACT();

    if (decl->modifiers) {
        if (decl->modifiers->isPublic()) type->SetACC_PUBLIC();
        if (decl->modifiers->isProtected()) type->SetACC_PROTECTED();
        if (decl->modifiers->isPrivate()) type->SetACC_PRIVATE();
        if (decl->modifiers->isStatic()) type->SetACC_STATIC();
    }

    type->jast2_interface_decl = decl;
    decl->symbol = type;

    // Process nested types
    if (decl->body) {
        for (auto& member : decl->body->declarations) {
            processTypeDeclaration(member.get(), type);
        }
    }

    file_symbol_->types.Next() = type;

    return type;
}

TypeSymbol* JastSemantic::processEnumDeclaration(jast2::EnumDeclaration* decl, TypeSymbol* enclosing) {
    if (!decl || decl->name.empty()) return nullptr;

    std::wstring wname(decl->name.begin(), decl->name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    TypeSymbol* type = enclosing
        ? enclosing->InsertNestedTypeSymbol(name_sym)
        : this_package_->InsertOuterTypeSymbol(name_sym);

    if (!type) {
        reportError(decl, "Duplicate type declaration: " + decl->name);
        return nullptr;
    }

    type->file_symbol = file_symbol_;
    type->SetOwner(enclosing ? static_cast<Symbol*>(enclosing) : static_cast<Symbol*>(this_package_));
    type->outermost_type = enclosing ? enclosing->outermost_type : type;
    type->supertypes_closure = new SymbolSet;
    type->subtypes = new SymbolSet;
    type->MarkEnum();
    type->SetACC_ENUM();
    type->SetACC_FINAL();

    if (decl->modifiers) {
        if (decl->modifiers->isPublic()) type->SetACC_PUBLIC();
        if (decl->modifiers->isProtected()) type->SetACC_PROTECTED();
        if (decl->modifiers->isPrivate()) type->SetACC_PRIVATE();
        if (decl->modifiers->isStatic()) type->SetACC_STATIC();
    }

    type->jast2_enum_decl = decl;
    decl->symbol = type;

    file_symbol_->types.Next() = type;

    return type;
}

TypeSymbol* JastSemantic::processAnnotationDeclaration(jast2::AnnotationDeclaration* decl, TypeSymbol* enclosing) {
    if (!decl || decl->name.empty()) return nullptr;

    std::wstring wname(decl->name.begin(), decl->name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    TypeSymbol* type = enclosing
        ? enclosing->InsertNestedTypeSymbol(name_sym)
        : this_package_->InsertOuterTypeSymbol(name_sym);

    if (!type) {
        reportError(decl, "Duplicate type declaration: " + decl->name);
        return nullptr;
    }

    type->file_symbol = file_symbol_;
    type->SetOwner(enclosing ? static_cast<Symbol*>(enclosing) : static_cast<Symbol*>(this_package_));
    type->outermost_type = enclosing ? enclosing->outermost_type : type;
    type->supertypes_closure = new SymbolSet;
    type->subtypes = new SymbolSet;
    type->SetACC_INTERFACE();
    type->SetACC_ABSTRACT();
    type->SetACC_ANNOTATION();

    if (decl->modifiers) {
        if (decl->modifiers->isPublic()) type->SetACC_PUBLIC();
    }

    type->jast2_annotation_decl = decl;
    decl->symbol = type;

    file_symbol_->types.Next() = type;

    return type;
}

//=============================================================================
// PHASE 3: TYPE HEADERS
//=============================================================================

void JastSemantic::resolveTypeHeaders() {
    for (unsigned i = 0; i < file_symbol_->types.Length(); i++) {
        resolveTypeHeader(file_symbol_->types[i]);
    }
}

void JastSemantic::resolveTypeHeader(TypeSymbol* type) {
    if (!type) return;

    if (type->jast2_class_decl) {
        resolveClassHeader(type, type->jast2_class_decl);
    } else if (type->jast2_interface_decl) {
        resolveInterfaceHeader(type, type->jast2_interface_decl);
    } else if (type->jast2_enum_decl) {
        resolveEnumHeader(type, type->jast2_enum_decl);
    }
}

void JastSemantic::resolveClassHeader(TypeSymbol* type, jast2::ClassDeclaration* decl) {
    // Resolve superclass
    if (decl->superclass) {
        TypeSymbol* super = resolveTypeName(decl->superclass.get());
        if (super) {
            type->super = super;
        } else {
            reportError(decl->superclass.get(), "Cannot resolve superclass");
            type->super = objectType();
        }
    } else {
        // Default superclass is java.lang.Object
        type->super = objectType();
    }

    // Resolve interfaces
    for (auto& iface : decl->interfaces) {
        TypeSymbol* iface_type = resolveTypeName(iface.get());
        if (iface_type) {
            type->AddInterface(iface_type);
        } else {
            reportError(iface.get(), "Cannot resolve interface");
        }
    }
}

void JastSemantic::resolveInterfaceHeader(TypeSymbol* type, jast2::InterfaceDeclaration* decl) {
    // Interfaces extend java.lang.Object
    type->super = objectType();

    // Resolve extended interfaces
    for (auto& ext : decl->extendsTypes) {
        TypeSymbol* ext_type = resolveTypeName(ext.get());
        if (ext_type) {
            type->AddInterface(ext_type);
        } else {
            reportError(ext.get(), "Cannot resolve interface");
        }
    }
}

void JastSemantic::resolveEnumHeader(TypeSymbol* type, jast2::EnumDeclaration* decl) {
    // Enums extend java.lang.Enum
    TypeSymbol* enum_type = control_.Enum();
    if (enum_type) {
        type->super = enum_type;
    } else {
        type->super = objectType();
    }

    // Resolve interfaces
    for (auto& iface : decl->interfaces) {
        TypeSymbol* iface_type = resolveTypeName(iface.get());
        if (iface_type) {
            type->AddInterface(iface_type);
        } else {
            reportError(iface.get(), "Cannot resolve interface");
        }
    }
}

//=============================================================================
// PHASE 4: MEMBERS
//=============================================================================

void JastSemantic::processMembers() {
    for (unsigned i = 0; i < file_symbol_->types.Length(); i++) {
        processTypeMembers(file_symbol_->types[i]);
    }
}

void JastSemantic::processTypeMembers(TypeSymbol* type) {
    if (!type) return;

    enterType(type);

    if (type->jast2_class_decl && type->jast2_class_decl->body) {
        processClassBodyMembers(type, type->jast2_class_decl->body.get());
    } else if (type->jast2_interface_decl && type->jast2_interface_decl->body) {
        processClassBodyMembers(type, type->jast2_interface_decl->body.get());
    } else if (type->jast2_enum_decl) {
        // Process enum constants
        for (auto& constant : type->jast2_enum_decl->constants) {
            // Create field for each enum constant
            std::wstring wname(constant->name.begin(), constant->name.end());
            NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

            VariableSymbol* field = type->InsertVariableSymbol(name_sym);
            if (field) {
                field->SetType(type);
                field->SetOwner(type);
                field->SetACC_PUBLIC();
                field->SetACC_STATIC();
                field->SetACC_FINAL();
                field->SetACC_ENUM();
                field->jast2_declarator = nullptr;
                constant->symbol = field;
            }
        }

        if (type->jast2_enum_decl->body) {
            processClassBodyMembers(type, type->jast2_enum_decl->body.get());
        }
    } else if (type->jast2_annotation_decl && type->jast2_annotation_decl->body) {
        processClassBodyMembers(type, type->jast2_annotation_decl->body.get());
    }

    exitType();
}

void JastSemantic::processClassBodyMembers(TypeSymbol* type, jast2::ClassBody* body) {
    if (!body) return;

    for (auto& decl : body->declarations) {
        if (auto* field = dynamic_cast<jast2::FieldDeclaration*>(decl.get())) {
            processFieldDeclaration(type, field);
        } else if (auto* method = dynamic_cast<jast2::MethodDeclaration*>(decl.get())) {
            processMethodDeclaration(type, method);
        } else if (auto* ctor = dynamic_cast<jast2::ConstructorDeclaration*>(decl.get())) {
            processConstructorDeclaration(type, ctor);
        }
        // Nested types already processed in processTypeDeclarations
    }
}

void JastSemantic::processFieldDeclaration(TypeSymbol* type, jast2::FieldDeclaration* decl) {
    if (!decl || !decl->type) return;

    TypeSymbol* field_type = resolveType(decl->type.get());
    if (!field_type) {
        reportError(decl->type.get(), "Cannot resolve field type");
        field_type = objectType();
    }

    for (auto& var : decl->declarators) {
        if (!var || var->name.empty()) continue;

        std::wstring wname(var->name.begin(), var->name.end());
        NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

        VariableSymbol* field = type->InsertVariableSymbol(name_sym);
        if (!field) {
            reportError(var.get(), "Duplicate field: " + var->name);
            continue;
        }

        // Handle extra dimensions
        TypeSymbol* actual_type = field_type;
        if (var->extraDims > 0) {
            actual_type = getArrayType(field_type, var->extraDims);
        }

        field->SetType(actual_type);
        field->SetOwner(type);

        if (decl->modifiers) {
            if (decl->modifiers->isPublic()) field->SetACC_PUBLIC();
            if (decl->modifiers->isProtected()) field->SetACC_PROTECTED();
            if (decl->modifiers->isPrivate()) field->SetACC_PRIVATE();
            if (decl->modifiers->isStatic()) field->SetACC_STATIC();
            if (decl->modifiers->isFinal()) field->SetACC_FINAL();
            if (decl->modifiers->isVolatile()) field->SetACC_VOLATILE();
            if (decl->modifiers->isTransient()) field->SetACC_TRANSIENT();
        }

        // Link jast2 <-> symbol
        field->jast2_declarator = var.get();
        field->jast2_field_decl = decl;
        var->symbol = field;
    }
}

void JastSemantic::processMethodDeclaration(TypeSymbol* type, jast2::MethodDeclaration* decl) {
    if (!decl || decl->name.empty()) return;

    std::wstring wname(decl->name.begin(), decl->name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    MethodSymbol* method = type->InsertMethodSymbol(name_sym);
    if (!method) {
        reportError(decl, "Failed to create method: " + decl->name);
        return;
    }

    // Return type
    if (decl->returnType) {
        TypeSymbol* ret_type = resolveType(decl->returnType.get());
        method->SetType(ret_type ? ret_type : voidType());
    } else {
        method->SetType(voidType());
    }

    // Parameters
    for (auto& param : decl->parameters) {
        if (!param) continue;

        TypeSymbol* param_type = resolveType(param->type.get());
        if (!param_type) param_type = objectType();

        if (param->isVarargs) {
            param_type = getArrayType(param_type, 1);
            method->SetACC_VARARGS();
        }

        std::wstring wpname(param->name.begin(), param->name.end());
        NameSymbol* pname_sym = control_.FindOrInsertName(wpname.c_str(), wpname.length());

        VariableSymbol* param_sym = new VariableSymbol(pname_sym);
        param_sym->SetType(param_type);
        param_sym->SetOwner(method);
        param_sym->jast2_parameter = param.get();
        param_sym->MarkComplete();
        param->symbol = param_sym;
        method->AddFormalParameter(param_sym);
    }

    // Modifiers
    method->SetContainingType(type);

    if (decl->modifiers) {
        if (decl->modifiers->isPublic()) method->SetACC_PUBLIC();
        if (decl->modifiers->isProtected()) method->SetACC_PROTECTED();
        if (decl->modifiers->isPrivate()) method->SetACC_PRIVATE();
        if (decl->modifiers->isStatic()) method->SetACC_STATIC();
        if (decl->modifiers->isFinal()) method->SetACC_FINAL();
        if (decl->modifiers->isAbstract()) method->SetACC_ABSTRACT();
        if (decl->modifiers->isNative()) method->SetACC_NATIVE();
        if (decl->modifiers->isSynchronized()) method->SetACC_SYNCHRONIZED();
        if (decl->modifiers->isStrictfp()) method->SetACC_STRICTFP();
    }

    // Link jast2 <-> symbol
    method->jast2_method_decl = decl;
    decl->symbol = method;
}

void JastSemantic::processConstructorDeclaration(TypeSymbol* type, jast2::ConstructorDeclaration* decl) {
    if (!decl) return;

    NameSymbol* init_sym = control_.init_name_symbol;
    MethodSymbol* method = type->InsertMethodSymbol(init_sym);
    if (!method) {
        reportError(decl, "Failed to create constructor");
        return;
    }

    method->SetType(voidType());

    // Parameters
    for (auto& param : decl->parameters) {
        if (!param) continue;

        TypeSymbol* param_type = resolveType(param->type.get());
        if (!param_type) param_type = objectType();

        if (param->isVarargs) {
            param_type = getArrayType(param_type, 1);
            method->SetACC_VARARGS();
        }

        std::wstring wpname(param->name.begin(), param->name.end());
        NameSymbol* pname_sym = control_.FindOrInsertName(wpname.c_str(), wpname.length());

        VariableSymbol* param_sym = new VariableSymbol(pname_sym);
        param_sym->SetType(param_type);
        param_sym->SetOwner(method);
        param_sym->jast2_parameter = param.get();
        param_sym->MarkComplete();
        param->symbol = param_sym;
        method->AddFormalParameter(param_sym);
    }

    method->SetContainingType(type);

    if (decl->modifiers) {
        if (decl->modifiers->isPublic()) method->SetACC_PUBLIC();
        if (decl->modifiers->isProtected()) method->SetACC_PROTECTED();
        if (decl->modifiers->isPrivate()) method->SetACC_PRIVATE();
    }

    // Link jast2 <-> symbol
    method->jast2_constructor_decl = decl;
    decl->symbol = method;
}

//=============================================================================
// PHASE 5: BODIES
//=============================================================================

void JastSemantic::processBodies() {
    for (unsigned i = 0; i < file_symbol_->types.Length(); i++) {
        TypeSymbol* type = file_symbol_->types[i];
        if (!type) continue;

        enterType(type);

        // Process method bodies
        jast2::ClassBody* body = nullptr;
        if (type->jast2_class_decl) body = type->jast2_class_decl->body.get();
        else if (type->jast2_interface_decl) body = type->jast2_interface_decl->body.get();
        else if (type->jast2_enum_decl) body = type->jast2_enum_decl->body.get();
        else if (type->jast2_annotation_decl) body = type->jast2_annotation_decl->body.get();

        if (body) {
            for (auto& decl : body->declarations) {
                if (auto* method = dynamic_cast<jast2::MethodDeclaration*>(decl.get())) {
                    if (method->body && method->symbol) {
                        enterMethod(method->symbol);
                        processMethodBody(method->symbol, method->body.get());
                        exitMethod();
                    }
                } else if (auto* ctor = dynamic_cast<jast2::ConstructorDeclaration*>(decl.get())) {
                    if (ctor->body && ctor->symbol) {
                        enterMethod(ctor->symbol);
                        processMethodBody(ctor->symbol, ctor->body.get());
                        exitMethod();
                    }
                } else if (auto* init = dynamic_cast<jast2::InitializerDeclaration*>(decl.get())) {
                    processInitializerBody(type, init);
                }
            }
        }

        // Process field initializers
        if (body) {
            for (auto& decl : body->declarations) {
                if (auto* field = dynamic_cast<jast2::FieldDeclaration*>(decl.get())) {
                    for (auto& var : field->declarators) {
                        if (var->initializer) {
                            processExpression(var->initializer.get());
                        }
                    }
                }
            }
        }

        exitType();
    }
}

void JastSemantic::processMethodBody(MethodSymbol* method, jast2::MethodBody* body) {
    if (!body) return;

    // Add parameters to scope
    for (unsigned i = 0; i < method->NumFormalParameters(); i++) {
        VariableSymbol* param = method->FormalParameter(i);
        if (param) {
            std::wstring wname(param->Name());
            std::string name(wname.begin(), wname.end());
            current_scope_->variables[name] = param;
        }
    }

    // Process explicit constructor call
    if (body->explicitConstructorCall) {
        processStatement(body->explicitConstructorCall.get());
    }

    // Process statements
    for (auto& stmt : body->statements) {
        processStatement(stmt.get());
    }
}

void JastSemantic::processInitializerBody(TypeSymbol* /*type*/, jast2::InitializerDeclaration* init) {
    if (!init || !init->body) return;

    pushScope();
    current_scope_->is_static = init->isStatic;

    for (auto& stmt : init->body->statements) {
        processStatement(stmt.get());
    }

    popScope();
}

//=============================================================================
// TYPE RESOLUTION
//=============================================================================

TypeSymbol* JastSemantic::resolveType(jast2::Type* type) {
    if (!type) return nullptr;

    if (auto* prim = dynamic_cast<jast2::PrimitiveType*>(type)) {
        return resolvePrimitiveType(prim);
    } else if (auto* arr = dynamic_cast<jast2::ArrayType*>(type)) {
        return resolveArrayType(arr);
    } else if (auto* name = dynamic_cast<jast2::TypeName*>(type)) {
        return resolveTypeName(name);
    }

    return nullptr;
}

TypeSymbol* JastSemantic::resolvePrimitiveType(jast2::PrimitiveType* type) {
    if (!type) return nullptr;

    TypeSymbol* result = nullptr;

    if (type->type == "int") result = intType();
    else if (type->type == "long") result = longType();
    else if (type->type == "float") result = floatType();
    else if (type->type == "double") result = doubleType();
    else if (type->type == "boolean") result = booleanType();
    else if (type->type == "char") result = charType();
    else if (type->type == "byte") result = byteType();
    else if (type->type == "short") result = shortType();
    else if (type->type == "void") result = voidType();

    if (result) {
        type->resolved_type = result;
    }

    return result;
}

TypeSymbol* JastSemantic::resolveArrayType(jast2::ArrayType* type) {
    if (!type || !type->elementType) return nullptr;

    TypeSymbol* element = resolveType(type->elementType.get());
    if (!element) return nullptr;

    TypeSymbol* result = getArrayType(element, type->dimensions);
    type->resolved_type = result;
    return result;
}

TypeSymbol* JastSemantic::resolveTypeName(jast2::TypeName* type) {
    if (!type) return nullptr;

    std::string name;
    if (type->name) {
        name = nameToString(type->name.get());
    }

    TypeSymbol* result = resolveTypeByName(name);
    if (result) {
        type->resolved_type = result;
    }

    return result;
}

TypeSymbol* JastSemantic::resolveTypeByName(const std::string& name) {
    if (name.empty()) return nullptr;

    // Check for qualified name
    if (name.find('.') != std::string::npos) {
        return resolveQualifiedType(name);
    }

    // Simple name - check various scopes

    // 1. Check enclosing types for nested type
    TypeSymbol* enclosing = this_type_;
    while (enclosing) {
        TypeSymbol* nested = findTypeInType(enclosing, name);
        if (nested) return nested;
        enclosing = dynamic_cast<TypeSymbol*>(enclosing->owner);
    }

    // 2. Check single-type imports
    auto it = single_type_imports_.find(name);
    if (it != single_type_imports_.end()) {
        return it->second;
    }

    // 3. Check same package
    TypeSymbol* pkg_type = findTypeInPackage(this_package_, name);
    if (pkg_type) return pkg_type;

    // 4. Check type-import-on-demand
    TypeSymbol* found = nullptr;
    for (PackageSymbol* pkg : type_import_on_demand_) {
        TypeSymbol* t = findTypeInPackage(pkg, name);
        if (t) {
            if (found && found != t) {
                reportError(0, 0, "Ambiguous type: " + name);
                return found;
            }
            found = t;
        }
    }
    if (found) return found;

    return nullptr;
}

TypeSymbol* JastSemantic::resolveQualifiedType(const std::string& qualified_name) {
    // Try to resolve as package.Type
    size_t last_dot = qualified_name.rfind('.');
    if (last_dot == std::string::npos) {
        // Simple name - check in unnamed package
        return findTypeInPackage(control_.UnnamedPackage(), qualified_name);
    }

    // Try treating prefix as package
    std::string prefix = qualified_name.substr(0, last_dot);
    std::string suffix = qualified_name.substr(last_dot + 1);

    PackageSymbol* pkg = resolvePackage(prefix);
    if (pkg) {
        TypeSymbol* type = findTypeInPackage(pkg, suffix);
        if (type) return type;
    }

    // Try treating prefix as type (for nested types)
    TypeSymbol* outer = resolveQualifiedType(prefix);
    if (outer) {
        return findTypeInType(outer, suffix);
    }

    return nullptr;
}

TypeSymbol* JastSemantic::findTypeInPackage(PackageSymbol* package, const std::string& name) {
    if (!package) return nullptr;

    std::wstring wname(name.begin(), name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    return package->FindTypeSymbol(name_sym);
}

TypeSymbol* JastSemantic::findTypeInType(TypeSymbol* type, const std::string& name) {
    if (!type) return nullptr;

    std::wstring wname(name.begin(), name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    // Check member types
    for (unsigned i = 0; i < type->NumTypeSymbols(); i++) {
        TypeSymbol* nested = type->TypeSym(i);
        if (nested && nested->name_symbol == name_sym) {
            return nested;
        }
    }

    return nullptr;
}

//=============================================================================
// EXPRESSION PROCESSING
//=============================================================================

TypeSymbol* JastSemantic::processExpression(jast2::Expression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* result = nullptr;

    if (auto* lit = dynamic_cast<jast2::LiteralExpression*>(expr)) {
        result = processLiteral(lit);
    } else if (auto* name = dynamic_cast<jast2::NameExpression*>(expr)) {
        result = processNameExpression(name);
    } else if (auto* this_expr = dynamic_cast<jast2::ThisExpression*>(expr)) {
        result = processThisExpression(this_expr);
    } else if (auto* super_expr = dynamic_cast<jast2::SuperExpression*>(expr)) {
        result = processSuperExpression(super_expr);
    } else if (auto* paren = dynamic_cast<jast2::ParenthesizedExpression*>(expr)) {
        result = processParenthesizedExpression(paren);
    } else if (auto* binary = dynamic_cast<jast2::BinaryExpression*>(expr)) {
        result = processBinaryExpression(binary);
    } else if (auto* unary = dynamic_cast<jast2::UnaryExpression*>(expr)) {
        result = processUnaryExpression(unary);
    } else if (auto* assign = dynamic_cast<jast2::AssignmentExpression*>(expr)) {
        result = processAssignmentExpression(assign);
    } else if (auto* cond = dynamic_cast<jast2::ConditionalExpression*>(expr)) {
        result = processConditionalExpression(cond);
    } else if (auto* instof = dynamic_cast<jast2::InstanceofExpression*>(expr)) {
        result = processInstanceofExpression(instof);
    } else if (auto* cast = dynamic_cast<jast2::CastExpression*>(expr)) {
        result = processCastExpression(cast);
    } else if (auto* field = dynamic_cast<jast2::FieldAccessExpression*>(expr)) {
        result = processFieldAccess(field);
    } else if (auto* method = dynamic_cast<jast2::MethodInvocationExpression*>(expr)) {
        result = processMethodInvocation(method);
    } else if (auto* arr = dynamic_cast<jast2::ArrayAccessExpression*>(expr)) {
        result = processArrayAccess(arr);
    } else if (auto* create = dynamic_cast<jast2::ClassCreationExpression*>(expr)) {
        result = processClassCreation(create);
    } else if (auto* arr_create = dynamic_cast<jast2::ArrayCreationExpression*>(expr)) {
        result = processArrayCreation(arr_create);
    } else if (auto* arr_init = dynamic_cast<jast2::ArrayInitializer*>(expr)) {
        result = processArrayInitializer(arr_init, nullptr);
    } else if (auto* cls_lit = dynamic_cast<jast2::ClassLiteralExpression*>(expr)) {
        result = processClassLiteral(cls_lit);
    }

    if (result) {
        expr->resolved_type = result;
    }

    return result;
}

TypeSymbol* JastSemantic::processLiteral(jast2::LiteralExpression* expr) {
    if (!expr) return nullptr;

    if (expr->literalKind == "Integer") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::INT;
        return intType();
    } else if (expr->literalKind == "Long") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::LONG;
        return longType();
    } else if (expr->literalKind == "Float") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::FLOAT;
        return floatType();
    } else if (expr->literalKind == "Double") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::DOUBLE;
        return doubleType();
    } else if (expr->literalKind == "Boolean") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::BOOLEAN;
        return booleanType();
    } else if (expr->literalKind == "Character") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::INT;
        return charType();
    } else if (expr->literalKind == "String") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::STRING;
        expr->string_constant = expr->value;
        return stringType();
    } else if (expr->literalKind == "Null") {
        expr->is_constant = true;
        expr->constant_kind = jast2::Expression::ConstantKind::NULL_VAL;
        return nullType();
    }

    return nullptr;
}

TypeSymbol* JastSemantic::processNameExpression(jast2::NameExpression* expr) {
    if (!expr || !expr->name) return nullptr;

    std::string name = nameToString(expr->name.get());

    // Check for simple name first
    if (name.find('.') == std::string::npos) {
        // Check local variables
        VariableSymbol* var = current_scope_ ? current_scope_->findVariable(name) : nullptr;
        if (var) {
            expr->resolved_variable = var;
            return var->Type();
        }

        // Check fields in this type
        if (this_type_) {
            VariableSymbol* field = findField(this_type_, name);
            if (field) {
                expr->resolved_variable = field;
                return field->Type();
            }
        }

        // Check static imports
        auto it = single_static_imports_.find(name);
        if (it != single_static_imports_.end()) {
            expr->resolved_variable = it->second;
            return it->second->Type();
        }

        // Check if it's a type name
        TypeSymbol* type = resolveTypeByName(name);
        if (type) {
            expr->resolved_type_name = type;
            return type;
        }
    } else {
        // Qualified name - could be package.Class or Type.field
        // Try as type first
        TypeSymbol* type = resolveQualifiedType(name);
        if (type) {
            expr->resolved_type_name = type;
            return type;
        }

        // Try as type.field
        size_t last_dot = name.rfind('.');
        std::string prefix = name.substr(0, last_dot);
        std::string suffix = name.substr(last_dot + 1);

        TypeSymbol* prefix_type = resolveQualifiedType(prefix);
        if (prefix_type) {
            VariableSymbol* field = findField(prefix_type, suffix);
            if (field) {
                expr->resolved_variable = field;
                return field->Type();
            }
        }
    }

    reportError(expr, "Cannot resolve name: " + name);
    return nullptr;
}

TypeSymbol* JastSemantic::processThisExpression(jast2::ThisExpression* expr) {
    if (!expr) return nullptr;

    if (current_scope_ && current_scope_->is_static) {
        reportError(expr, "'this' cannot be used in static context");
        return nullptr;
    }

    return this_type_;
}

TypeSymbol* JastSemantic::processSuperExpression(jast2::SuperExpression* expr) {
    if (!expr) return nullptr;

    if (current_scope_ && current_scope_->is_static) {
        reportError(expr, "'super' cannot be used in static context");
        return nullptr;
    }

    if (!this_type_ || !this_type_->super) {
        reportError(expr, "No superclass available");
        return nullptr;
    }

    return this_type_->super;
}

TypeSymbol* JastSemantic::processParenthesizedExpression(jast2::ParenthesizedExpression* expr) {
    if (!expr || !expr->expression) return nullptr;
    return processExpression(expr->expression.get());
}

TypeSymbol* JastSemantic::processBinaryExpression(jast2::BinaryExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* left_type = processExpression(expr->left.get());
    TypeSymbol* right_type = processExpression(expr->right.get());

    if (!left_type || !right_type) return nullptr;

    const std::string& op = expr->op;

    // String concatenation
    if (op == "+" && (left_type == stringType() || right_type == stringType())) {
        return stringType();
    }

    // Numeric operators
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        if (!isNumeric(left_type) || !isNumeric(right_type)) {
            reportError(expr, "Numeric operands required for operator: " + op);
            return nullptr;
        }
        return binaryNumericPromotion(left_type, right_type);
    }

    // Shift operators
    if (op == "<<" || op == ">>" || op == ">>>") {
        if (!isIntegral(left_type) || !isIntegral(right_type)) {
            reportError(expr, "Integral operands required for shift operator");
            return nullptr;
        }
        return unaryNumericPromotion(left_type);
    }

    // Comparison operators
    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        if (!isNumeric(left_type) || !isNumeric(right_type)) {
            reportError(expr, "Numeric operands required for comparison");
            return nullptr;
        }
        return booleanType();
    }

    // Equality operators
    if (op == "==" || op == "!=") {
        return booleanType();
    }

    // Logical operators
    if (op == "&&" || op == "||") {
        if (!isBoolean(left_type) || !isBoolean(right_type)) {
            reportError(expr, "Boolean operands required for logical operator");
            return nullptr;
        }
        return booleanType();
    }

    // Bitwise operators
    if (op == "&" || op == "|" || op == "^") {
        if (isBoolean(left_type) && isBoolean(right_type)) {
            return booleanType();
        }
        if (isIntegral(left_type) && isIntegral(right_type)) {
            return binaryNumericPromotion(left_type, right_type);
        }
        reportError(expr, "Invalid operands for bitwise operator");
        return nullptr;
    }

    return nullptr;
}

TypeSymbol* JastSemantic::processUnaryExpression(jast2::UnaryExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* operand_type = processExpression(expr->operand.get());
    if (!operand_type) return nullptr;

    const std::string& op = expr->op;

    if (op == "+" || op == "-") {
        if (!isNumeric(operand_type)) {
            reportError(expr, "Numeric operand required");
            return nullptr;
        }
        return unaryNumericPromotion(operand_type);
    }

    if (op == "~") {
        if (!isIntegral(operand_type)) {
            reportError(expr, "Integral operand required");
            return nullptr;
        }
        return unaryNumericPromotion(operand_type);
    }

    if (op == "!") {
        if (!isBoolean(operand_type)) {
            reportError(expr, "Boolean operand required");
            return nullptr;
        }
        return booleanType();
    }

    if (op == "++" || op == "--") {
        if (!isNumeric(operand_type)) {
            reportError(expr, "Numeric operand required");
            return nullptr;
        }
        return operand_type;
    }

    return nullptr;
}

TypeSymbol* JastSemantic::processAssignmentExpression(jast2::AssignmentExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* left_type = processExpression(expr->left.get());
    TypeSymbol* right_type = processExpression(expr->right.get());

    if (!left_type || !right_type) return nullptr;

    // Check assignability
    if (!isAssignable(left_type, right_type)) {
        reportError(expr, "Incompatible types in assignment");
    }

    return left_type;
}

TypeSymbol* JastSemantic::processConditionalExpression(jast2::ConditionalExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* cond_type = processExpression(expr->condition.get());
    TypeSymbol* then_type = processExpression(expr->thenExpr.get());
    TypeSymbol* else_type = processExpression(expr->elseExpr.get());

    if (cond_type && !isBoolean(cond_type)) {
        reportError(expr->condition.get(), "Condition must be boolean");
    }

    if (!then_type || !else_type) return nullptr;

    return commonType(then_type, else_type);
}

TypeSymbol* JastSemantic::processInstanceofExpression(jast2::InstanceofExpression* expr) {
    if (!expr) return nullptr;

    processExpression(expr->expression.get());
    resolveType(expr->type.get());

    return booleanType();
}

TypeSymbol* JastSemantic::processCastExpression(jast2::CastExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* cast_type = resolveType(expr->type.get());
    processExpression(expr->expression.get());

    return cast_type;
}

TypeSymbol* JastSemantic::processFieldAccess(jast2::FieldAccessExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* base_type = processExpression(expr->base.get());
    if (!base_type) return nullptr;

    VariableSymbol* field = findField(base_type, expr->identifier);
    if (!field) {
        reportError(expr, "Cannot resolve field: " + expr->identifier);
        return nullptr;
    }

    expr->resolved_field = field;
    return field->Type();
}

TypeSymbol* JastSemantic::processMethodInvocation(jast2::MethodInvocationExpression* expr) {
    if (!expr) return nullptr;

    // Process arguments
    std::vector<TypeSymbol*> arg_types;
    for (auto& arg : expr->arguments) {
        TypeSymbol* arg_type = processExpression(arg.get());
        arg_types.push_back(arg_type ? arg_type : objectType());
    }

    // Determine the type to search in
    TypeSymbol* search_type = nullptr;
    if (expr->base) {
        search_type = processExpression(expr->base.get());
    } else {
        search_type = this_type_;
    }

    if (!search_type) {
        reportError(expr, "Cannot determine receiver type");
        return nullptr;
    }

    // Find the method
    MethodSymbol* method = findMethod(search_type, expr->methodName, arg_types);
    if (!method) {
        reportError(expr, "Cannot resolve method: " + expr->methodName);
        return nullptr;
    }

    expr->resolved_method = method;
    return method->Type();
}

TypeSymbol* JastSemantic::processArrayAccess(jast2::ArrayAccessExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* array_type = processExpression(expr->array.get());
    TypeSymbol* index_type = processExpression(expr->index.get());

    if (!array_type || !array_type->IsArray()) {
        reportError(expr->array.get(), "Array type required");
        return nullptr;
    }

    if (index_type && !isIntegral(index_type)) {
        reportError(expr->index.get(), "Integer index required");
    }

    return array_type->base_type;
}

TypeSymbol* JastSemantic::processClassCreation(jast2::ClassCreationExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* type = resolveTypeName(expr->type.get());
    if (!type) {
        reportError(expr->type.get(), "Cannot resolve type");
        return nullptr;
    }

    // Process arguments
    std::vector<TypeSymbol*> arg_types;
    for (auto& arg : expr->arguments) {
        TypeSymbol* arg_type = processExpression(arg.get());
        arg_types.push_back(arg_type ? arg_type : objectType());
    }

    // Find constructor
    MethodSymbol* ctor = findConstructor(type, arg_types);
    if (!ctor) {
        reportError(expr, "Cannot find matching constructor");
    } else {
        expr->resolved_constructor = ctor;
    }

    return type;
}

TypeSymbol* JastSemantic::processArrayCreation(jast2::ArrayCreationExpression* expr) {
    if (!expr) return nullptr;

    TypeSymbol* element_type = resolveType(expr->elementType.get());
    if (!element_type) return nullptr;

    // Process dimension expressions
    int dims = 0;
    for (auto& dim : expr->dimensionExprs) {
        TypeSymbol* dim_type = processExpression(dim.get());
        if (dim_type && !isIntegral(dim_type)) {
            reportError(dim.get(), "Integer dimension required");
        }
        dims++;
    }
    dims += expr->extraDims;

    // Process initializer if present
    if (expr->initializer) {
        processArrayInitializer(expr->initializer.get(), getArrayType(element_type, dims));
    }

    return getArrayType(element_type, dims);
}

TypeSymbol* JastSemantic::processArrayInitializer(jast2::ArrayInitializer* expr, TypeSymbol* expected_type) {
    if (!expr) return nullptr;

    TypeSymbol* element_type = expected_type && expected_type->IsArray()
        ? expected_type->base_type : objectType();

    for (auto& elem : expr->elements) {
        if (auto* nested = dynamic_cast<jast2::ArrayInitializer*>(elem.get())) {
            processArrayInitializer(nested, element_type);
        } else {
            processExpression(elem.get());
        }
    }

    return expected_type;
}

TypeSymbol* JastSemantic::processClassLiteral(jast2::ClassLiteralExpression* expr) {
    if (!expr) return nullptr;

    resolveType(expr->type.get());
    return classType();
}

//=============================================================================
// STATEMENT PROCESSING
//=============================================================================

void JastSemantic::processStatement(jast2::Statement* stmt) {
    if (!stmt) return;

    if (auto* block = dynamic_cast<jast2::BlockStatement*>(stmt)) {
        processBlockStatement(block);
    } else if (auto* expr_stmt = dynamic_cast<jast2::ExpressionStatement*>(stmt)) {
        processExpressionStatement(expr_stmt);
    } else if (auto* local = dynamic_cast<jast2::LocalVariableStatement*>(stmt)) {
        processLocalVariableStatement(local);
    } else if (auto* if_stmt = dynamic_cast<jast2::IfStatement*>(stmt)) {
        processIfStatement(if_stmt);
    } else if (auto* while_stmt = dynamic_cast<jast2::WhileStatement*>(stmt)) {
        processWhileStatement(while_stmt);
    } else if (auto* do_stmt = dynamic_cast<jast2::DoStatement*>(stmt)) {
        processDoStatement(do_stmt);
    } else if (auto* for_stmt = dynamic_cast<jast2::ForStatement*>(stmt)) {
        processForStatement(for_stmt);
    } else if (auto* foreach_stmt = dynamic_cast<jast2::ForeachStatement*>(stmt)) {
        processForeachStatement(foreach_stmt);
    } else if (auto* switch_stmt = dynamic_cast<jast2::SwitchStatement*>(stmt)) {
        processSwitchStatement(switch_stmt);
    } else if (auto* break_stmt = dynamic_cast<jast2::BreakStatement*>(stmt)) {
        processBreakStatement(break_stmt);
    } else if (auto* cont_stmt = dynamic_cast<jast2::ContinueStatement*>(stmt)) {
        processContinueStatement(cont_stmt);
    } else if (auto* ret_stmt = dynamic_cast<jast2::ReturnStatement*>(stmt)) {
        processReturnStatement(ret_stmt);
    } else if (auto* throw_stmt = dynamic_cast<jast2::ThrowStatement*>(stmt)) {
        processThrowStatement(throw_stmt);
    } else if (auto* sync_stmt = dynamic_cast<jast2::SynchronizedStatement*>(stmt)) {
        processSynchronizedStatement(sync_stmt);
    } else if (auto* try_stmt = dynamic_cast<jast2::TryStatement*>(stmt)) {
        processTryStatement(try_stmt);
    } else if (auto* assert_stmt = dynamic_cast<jast2::AssertStatement*>(stmt)) {
        processAssertStatement(assert_stmt);
    } else if (auto* this_call = dynamic_cast<jast2::ThisCall*>(stmt)) {
        processThisCall(this_call);
    } else if (auto* super_call = dynamic_cast<jast2::SuperCall*>(stmt)) {
        processSuperCall(super_call);
    } else if (auto* labeled = dynamic_cast<jast2::LabeledStatement*>(stmt)) {
        processLabeledStatement(labeled);
    } else if (auto* local_class = dynamic_cast<jast2::LocalClassStatement*>(stmt)) {
        processLocalClassStatement(local_class);
    }
    // EmptyStatement requires no processing
}

void JastSemantic::processBlockStatement(jast2::BlockStatement* stmt) {
    pushScope();
    for (auto& s : stmt->statements) {
        processStatement(s.get());
    }
    popScope();
}

void JastSemantic::processExpressionStatement(jast2::ExpressionStatement* stmt) {
    if (stmt && stmt->expression) {
        processExpression(stmt->expression.get());
    }
}

void JastSemantic::processLocalVariableStatement(jast2::LocalVariableStatement* stmt) {
    if (!stmt || !stmt->type) return;

    TypeSymbol* var_type = resolveType(stmt->type.get());
    if (!var_type) {
        reportError(stmt->type.get(), "Cannot resolve type");
        var_type = objectType();
    }

    for (auto& decl : stmt->declarators) {
        if (!decl || decl->name.empty()) continue;

        // Create variable symbol
        std::wstring wname(decl->name.begin(), decl->name.end());
        NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

        MethodSymbol* method = current_scope_ ? current_scope_->enclosing_method : nullptr;
        VariableSymbol* var = method
            ? method->block_symbol->InsertVariableSymbol(name_sym)
            : new VariableSymbol(name_sym);

        if (!var) {
            reportError(decl.get(), "Duplicate local variable: " + decl->name);
            continue;
        }

        TypeSymbol* actual_type = var_type;
        if (decl->extraDims > 0) {
            actual_type = getArrayType(var_type, decl->extraDims);
        }

        var->SetType(actual_type);
        var->SetOwner(method);
        var->jast2_declarator = decl.get();
        decl->symbol = var;

        // Add to current scope
        if (current_scope_) {
            current_scope_->variables[decl->name] = var;
        }

        // Process initializer
        if (decl->initializer) {
            TypeSymbol* init_type = processExpression(decl->initializer.get());
            if (init_type && !isAssignable(actual_type, init_type)) {
                reportError(decl->initializer.get(), "Incompatible types in initialization");
            }
        }
    }
}

void JastSemantic::processIfStatement(jast2::IfStatement* stmt) {
    if (!stmt) return;

    TypeSymbol* cond_type = processExpression(stmt->condition.get());
    if (cond_type && !isBoolean(cond_type)) {
        reportError(stmt->condition.get(), "Condition must be boolean");
    }

    processStatement(stmt->thenStatement.get());
    if (stmt->elseStatement) {
        processStatement(stmt->elseStatement.get());
    }
}

void JastSemantic::processWhileStatement(jast2::WhileStatement* stmt) {
    if (!stmt) return;

    TypeSymbol* cond_type = processExpression(stmt->condition.get());
    if (cond_type && !isBoolean(cond_type)) {
        reportError(stmt->condition.get(), "Condition must be boolean");
    }

    processStatement(stmt->body.get());
}

void JastSemantic::processDoStatement(jast2::DoStatement* stmt) {
    if (!stmt) return;

    processStatement(stmt->body.get());

    TypeSymbol* cond_type = processExpression(stmt->condition.get());
    if (cond_type && !isBoolean(cond_type)) {
        reportError(stmt->condition.get(), "Condition must be boolean");
    }
}

void JastSemantic::processForStatement(jast2::ForStatement* stmt) {
    if (!stmt) return;

    pushScope();

    for (auto& init : stmt->init) {
        processStatement(init.get());
    }

    if (stmt->condition) {
        TypeSymbol* cond_type = processExpression(stmt->condition.get());
        if (cond_type && !isBoolean(cond_type)) {
            reportError(stmt->condition.get(), "Condition must be boolean");
        }
    }

    for (auto& update : stmt->update) {
        processExpression(update.get());
    }

    processStatement(stmt->body.get());

    popScope();
}

void JastSemantic::processForeachStatement(jast2::ForeachStatement* stmt) {
    if (!stmt) return;

    pushScope();

    TypeSymbol* var_type = resolveType(stmt->type.get());
    processExpression(stmt->iterable.get());  // TODO: validate iterable type

    // Create loop variable
    if (!stmt->variableName.empty()) {
        std::wstring wname(stmt->variableName.begin(), stmt->variableName.end());
        NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

        MethodSymbol* method = current_scope_ ? current_scope_->enclosing_method : nullptr;
        VariableSymbol* var = method
            ? method->block_symbol->InsertVariableSymbol(name_sym)
            : new VariableSymbol(name_sym);

        if (var) {
            var->SetType(var_type);
            var->SetOwner(method);
            stmt->variable_symbol = var;
            current_scope_->variables[stmt->variableName] = var;
        }
    }

    processStatement(stmt->body.get());

    popScope();
}

void JastSemantic::processSwitchStatement(jast2::SwitchStatement* stmt) {
    if (!stmt) return;

    processExpression(stmt->expression.get());  // TODO: validate switch expression type

    for (auto& block : stmt->blocks) {
        for (auto& label : block.labels) {
            if (label.expression) {
                processExpression(label.expression.get());
            }
        }
        for (auto& s : block.statements) {
            processStatement(s.get());
        }
    }
}

void JastSemantic::processBreakStatement(jast2::BreakStatement* /*stmt*/) {
    // Label resolution would go here
}

void JastSemantic::processContinueStatement(jast2::ContinueStatement* /*stmt*/) {
    // Label resolution would go here
}

void JastSemantic::processReturnStatement(jast2::ReturnStatement* stmt) {
    if (!stmt) return;

    MethodSymbol* method = current_scope_ ? current_scope_->enclosing_method : nullptr;
    TypeSymbol* return_type = method ? method->Type() : voidType();

    if (stmt->expression) {
        TypeSymbol* expr_type = processExpression(stmt->expression.get());
        if (return_type == voidType()) {
            reportError(stmt, "Cannot return value from void method");
        } else if (expr_type && !isAssignable(return_type, expr_type)) {
            reportError(stmt->expression.get(), "Incompatible return type");
        }
    } else {
        if (return_type != voidType()) {
            reportError(stmt, "Missing return value");
        }
    }
}

void JastSemantic::processThrowStatement(jast2::ThrowStatement* stmt) {
    if (stmt && stmt->expression) {
        processExpression(stmt->expression.get());
    }
}

void JastSemantic::processSynchronizedStatement(jast2::SynchronizedStatement* stmt) {
    if (!stmt) return;

    TypeSymbol* expr_type = processExpression(stmt->expression.get());
    if (expr_type && !isReference(expr_type)) {
        reportError(stmt->expression.get(), "Reference type required for synchronized");
    }

    if (stmt->body) {
        processBlockStatement(stmt->body.get());
    }
}

void JastSemantic::processTryStatement(jast2::TryStatement* stmt) {
    if (!stmt) return;

    if (stmt->tryBlock) {
        processBlockStatement(stmt->tryBlock.get());
    }

    for (auto& clause : stmt->catchClauses) {
        pushScope();

        // Create catch variable
        if (!clause.variableName.empty()) {
            std::wstring wname(clause.variableName.begin(), clause.variableName.end());
            NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

            MethodSymbol* method = current_scope_ ? current_scope_->enclosing_method : nullptr;
            VariableSymbol* var = method
                ? method->block_symbol->InsertVariableSymbol(name_sym)
                : new VariableSymbol(name_sym);

            if (var && !clause.types.empty()) {
                TypeSymbol* catch_type = resolveType(clause.types[0].get());
                var->SetType(catch_type ? catch_type : objectType());
                var->SetOwner(method);
                clause.variable_symbol = var;
                current_scope_->variables[clause.variableName] = var;
            }
        }

        if (clause.block) {
            processBlockStatement(clause.block.get());
        }

        popScope();
    }

    if (stmt->finallyBlock) {
        processBlockStatement(stmt->finallyBlock.get());
    }
}

void JastSemantic::processAssertStatement(jast2::AssertStatement* stmt) {
    if (!stmt) return;

    TypeSymbol* cond_type = processExpression(stmt->condition.get());
    if (cond_type && !isBoolean(cond_type)) {
        reportError(stmt->condition.get(), "Assertion condition must be boolean");
    }

    if (stmt->message) {
        processExpression(stmt->message.get());
    }
}

void JastSemantic::processThisCall(jast2::ThisCall* stmt) {
    if (!stmt) return;

    std::vector<TypeSymbol*> arg_types;
    for (auto& arg : stmt->arguments) {
        TypeSymbol* arg_type = processExpression(arg.get());
        arg_types.push_back(arg_type ? arg_type : objectType());
    }

    if (this_type_) {
        MethodSymbol* ctor = findConstructor(this_type_, arg_types);
        if (!ctor) {
            reportError(stmt, "Cannot find matching constructor");
        } else {
            stmt->resolved_constructor = ctor;
        }
    }
}

void JastSemantic::processSuperCall(jast2::SuperCall* stmt) {
    if (!stmt) return;

    std::vector<TypeSymbol*> arg_types;
    for (auto& arg : stmt->arguments) {
        TypeSymbol* arg_type = processExpression(arg.get());
        arg_types.push_back(arg_type ? arg_type : objectType());
    }

    if (this_type_ && this_type_->super) {
        MethodSymbol* ctor = findConstructor(this_type_->super, arg_types);
        if (!ctor) {
            reportError(stmt, "Cannot find matching super constructor");
        } else {
            stmt->resolved_constructor = ctor;
        }
    }
}

void JastSemantic::processLabeledStatement(jast2::LabeledStatement* stmt) {
    if (!stmt) return;

    // TODO: Create label symbol

    processStatement(stmt->statement.get());
}

void JastSemantic::processLocalClassStatement(jast2::LocalClassStatement* stmt) {
    if (!stmt || !stmt->declaration) return;

    // Process the local class declaration
    processClassDeclaration(stmt->declaration.get(), this_type_);
    // TODO: Handle local class semantics (captured variables, etc.)
}

//=============================================================================
// METHOD/FIELD RESOLUTION
//=============================================================================

MethodSymbol* JastSemantic::findMethod(TypeSymbol* type, const std::string& name,
                                       const std::vector<TypeSymbol*>& arg_types) {
    if (!type) return nullptr;

    std::wstring wname(name.begin(), name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    std::vector<MethodSymbol*> candidates;

    // Search in type and superclasses
    TypeSymbol* search = type;
    while (search) {
        for (unsigned i = 0; i < search->NumMethodSymbols(); i++) {
            MethodSymbol* method = search->MethodSym(i);
            if (method && method->name_symbol == name_sym) {
                if (isMethodApplicable(method, arg_types)) {
                    candidates.push_back(method);
                }
            }
        }
        search = search->super;
    }

    if (candidates.empty()) return nullptr;
    if (candidates.size() == 1) return candidates[0];

    return selectMostSpecific(candidates, arg_types);
}

MethodSymbol* JastSemantic::findConstructor(TypeSymbol* type,
                                           const std::vector<TypeSymbol*>& arg_types) {
    if (!type) return nullptr;

    NameSymbol* init_sym = control_.init_name_symbol;

    std::vector<MethodSymbol*> candidates;

    for (unsigned i = 0; i < type->NumMethodSymbols(); i++) {
        MethodSymbol* method = type->MethodSym(i);
        if (method && method->name_symbol == init_sym) {
            if (isMethodApplicable(method, arg_types)) {
                candidates.push_back(method);
            }
        }
    }

    if (candidates.empty()) return nullptr;
    if (candidates.size() == 1) return candidates[0];

    return selectMostSpecific(candidates, arg_types);
}

bool JastSemantic::isMethodApplicable(MethodSymbol* method,
                                      const std::vector<TypeSymbol*>& arg_types) {
    if (!method) return false;

    unsigned num_params = method->NumFormalParameters();

    // Check varargs
    if (method->ACC_VARARGS()) {
        if (arg_types.size() < num_params - 1) return false;
    } else {
        if (arg_types.size() != num_params) return false;
    }

    // Check each argument
    for (unsigned i = 0; i < num_params && i < arg_types.size(); i++) {
        TypeSymbol* param_type = method->FormalParameter(i)->Type();
        TypeSymbol* arg_type = arg_types[i];

        if (!isAssignable(param_type, arg_type)) {
            return false;
        }
    }

    return true;
}

MethodSymbol* JastSemantic::selectMostSpecific(const std::vector<MethodSymbol*>& candidates,
                                               const std::vector<TypeSymbol*>& /*arg_types*/) {
    // Simple implementation - just return first
    // TODO: A proper implementation would compare method signatures for most specific
    return candidates.empty() ? nullptr : candidates[0];
}

VariableSymbol* JastSemantic::findField(TypeSymbol* type, const std::string& name) {
    if (!type) return nullptr;

    std::wstring wname(name.begin(), name.end());
    NameSymbol* name_sym = control_.FindOrInsertName(wname.c_str(), wname.length());

    // Search in type and superclasses
    TypeSymbol* search = type;
    while (search) {
        VariableSymbol* field = search->FindVariableSymbol(name_sym);
        if (field) return field;
        search = search->super;
    }

    return nullptr;
}

//=============================================================================
// TYPE UTILITIES
//=============================================================================

bool JastSemantic::isAssignable(TypeSymbol* target, TypeSymbol* source) {
    if (!target || !source) return false;
    if (target == source) return true;

    // null is assignable to any reference type
    if (source == nullType() && isReference(target)) return true;

    // Primitive widening
    if (target->Primitive() && source->Primitive()) {
        if (target == doubleType()) {
            return source == floatType() || source == longType() ||
                   source == intType() || source == charType() ||
                   source == shortType() || source == byteType();
        }
        if (target == floatType()) {
            return source == longType() || source == intType() ||
                   source == charType() || source == shortType() || source == byteType();
        }
        if (target == longType()) {
            return source == intType() || source == charType() ||
                   source == shortType() || source == byteType();
        }
        if (target == intType()) {
            return source == charType() || source == shortType() || source == byteType();
        }
        if (target == shortType()) {
            return source == byteType();
        }
    }

    // Reference assignment (subtype)
    if (isReference(target) && isReference(source)) {
        return isSubtype(source, target);
    }

    // Boxing/unboxing
    TypeSymbol* boxed_source = boxType(source);
    if (boxed_source && isAssignable(target, boxed_source)) return true;

    TypeSymbol* unboxed_source = unboxType(source);
    if (unboxed_source && isAssignable(target, unboxed_source)) return true;

    return false;
}

bool JastSemantic::isSubtype(TypeSymbol* sub, TypeSymbol* super) {
    if (!sub || !super) return false;
    if (sub == super) return true;

    // Check superclass chain
    TypeSymbol* s = sub->super;
    while (s) {
        if (s == super) return true;
        s = s->super;
    }

    // Check interfaces
    for (unsigned i = 0; i < sub->NumInterfaces(); i++) {
        if (isSubtype(sub->Interface(i), super)) return true;
    }

    // Array covariance
    if (sub->IsArray() && super->IsArray()) {
        return isSubtype(sub->base_type, super->base_type);
    }

    return false;
}

bool JastSemantic::isNumeric(TypeSymbol* type) {
    return type && (type == intType() || type == longType() ||
                   type == floatType() || type == doubleType() ||
                   type == charType() || type == shortType() || type == byteType());
}

bool JastSemantic::isIntegral(TypeSymbol* type) {
    return type && (type == intType() || type == longType() ||
                   type == charType() || type == shortType() || type == byteType());
}

bool JastSemantic::isBoolean(TypeSymbol* type) {
    return type == booleanType();
}

bool JastSemantic::isReference(TypeSymbol* type) {
    return type && !type->Primitive() && type != voidType();
}

TypeSymbol* JastSemantic::binaryNumericPromotion(TypeSymbol* left, TypeSymbol* right) {
    if (left == doubleType() || right == doubleType()) return doubleType();
    if (left == floatType() || right == floatType()) return floatType();
    if (left == longType() || right == longType()) return longType();
    return intType();
}

TypeSymbol* JastSemantic::unaryNumericPromotion(TypeSymbol* type) {
    if (type == byteType() || type == shortType() || type == charType()) {
        return intType();
    }
    return type;
}

TypeSymbol* JastSemantic::commonType(TypeSymbol* t1, TypeSymbol* t2) {
    if (!t1) return t2;
    if (!t2) return t1;
    if (t1 == t2) return t1;

    // Numeric promotion
    if (isNumeric(t1) && isNumeric(t2)) {
        return binaryNumericPromotion(t1, t2);
    }

    // Reference types - find common supertype
    if (isReference(t1) && isReference(t2)) {
        if (isSubtype(t1, t2)) return t2;
        if (isSubtype(t2, t1)) return t1;
        return objectType();
    }

    return objectType();
}

TypeSymbol* JastSemantic::boxType(TypeSymbol* primitive) {
    if (!primitive || !primitive->Primitive()) return nullptr;

    if (primitive == intType()) return control_.Integer();
    if (primitive == longType()) return control_.Long();
    if (primitive == floatType()) return control_.Float();
    if (primitive == doubleType()) return control_.Double();
    if (primitive == booleanType()) return control_.Boolean();
    if (primitive == charType()) return control_.Character();
    if (primitive == byteType()) return control_.Byte();
    if (primitive == shortType()) return control_.Short();

    return nullptr;
}

TypeSymbol* JastSemantic::unboxType(TypeSymbol* wrapper) {
    if (!wrapper) return nullptr;

    if (wrapper == control_.Integer()) return intType();
    if (wrapper == control_.Long()) return longType();
    if (wrapper == control_.Float()) return floatType();
    if (wrapper == control_.Double()) return doubleType();
    if (wrapper == control_.Boolean()) return booleanType();
    if (wrapper == control_.Character()) return charType();
    if (wrapper == control_.Byte()) return byteType();
    if (wrapper == control_.Short()) return shortType();

    return nullptr;
}

TypeSymbol* JastSemantic::getArrayType(TypeSymbol* element_type, int dimensions) {
    if (!element_type || dimensions <= 0) return element_type;

    return element_type->GetArrayType(control_.system_semantic, dimensions);
}

//=============================================================================
// UTILITIES
//=============================================================================

std::string JastSemantic::nameToString(jast2::Name* name) {
    if (!name) return "";
    return name->getFullName();
}

TypeSymbol* JastSemantic::intType() { return control_.int_type; }
TypeSymbol* JastSemantic::longType() { return control_.long_type; }
TypeSymbol* JastSemantic::floatType() { return control_.float_type; }
TypeSymbol* JastSemantic::doubleType() { return control_.double_type; }
TypeSymbol* JastSemantic::booleanType() { return control_.boolean_type; }
TypeSymbol* JastSemantic::charType() { return control_.char_type; }
TypeSymbol* JastSemantic::byteType() { return control_.byte_type; }
TypeSymbol* JastSemantic::shortType() { return control_.short_type; }
TypeSymbol* JastSemantic::voidType() { return control_.void_type; }
TypeSymbol* JastSemantic::nullType() { return control_.null_type; }
TypeSymbol* JastSemantic::stringType() { return control_.String(); }
TypeSymbol* JastSemantic::objectType() { return control_.Object(); }
TypeSymbol* JastSemantic::classType() { return control_.Class(); }

} // namespace Jopa
