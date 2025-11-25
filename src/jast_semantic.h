#ifndef jast_semantic_INCLUDED
#define jast_semantic_INCLUDED

#include "platform.h"
#include "jast2.h"
#include "symbol.h"
#include "control.h"
#include "lookup.h"
#include "tuple.h"
#include "set.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace Jopa {

class Control;
class LexStream;
class PackageSymbol;
class TypeSymbol;
class MethodSymbol;
class VariableSymbol;
class BlockSymbol;
class LabelSymbol;

// Error severity levels
enum class JastErrorSeverity {
    ERROR,
    WARNING,
    NOTE
};

// Semantic error record
struct JastSemanticError {
    JastErrorSeverity severity;
    int line;
    int column;
    std::string message;
    std::string filename;
};

// Scope for symbol lookup during semantic analysis
class JastScope {
public:
    JastScope* parent = nullptr;
    std::unordered_map<std::string, VariableSymbol*> variables;
    std::unordered_map<std::string, LabelSymbol*> labels;
    TypeSymbol* enclosing_type = nullptr;
    MethodSymbol* enclosing_method = nullptr;
    bool is_static = false;

    JastScope() = default;
    explicit JastScope(JastScope* parent_) : parent(parent_) {
        if (parent) {
            enclosing_type = parent->enclosing_type;
            enclosing_method = parent->enclosing_method;
            is_static = parent->is_static;
        }
    }

    VariableSymbol* findVariable(const std::string& name) const {
        auto it = variables.find(name);
        if (it != variables.end()) return it->second;
        if (parent) return parent->findVariable(name);
        return nullptr;
    }

    LabelSymbol* findLabel(const std::string& name) const {
        auto it = labels.find(name);
        if (it != labels.end()) return it->second;
        if (parent) return parent->findLabel(name);
        return nullptr;
    }
};

// Main semantic analyzer for jast2
class JastSemantic {
public:
    JastSemantic(Control& control, FileSymbol* file_symbol);
    ~JastSemantic();

    // Main entry point - analyze a compilation unit
    bool analyze(jast2::CompilationUnit* unit);

    // Get errors
    const std::vector<JastSemanticError>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    Control& control_;
    FileSymbol* file_symbol_;
    LexStream* lex_stream_;
    PackageSymbol* this_package_;
    jast2::CompilationUnit* compilation_unit_;
    JastScope* current_scope_;
    std::vector<JastSemanticError> errors_;

    // Import tracking
    std::unordered_map<std::string, TypeSymbol*> single_type_imports_;
    std::vector<PackageSymbol*> type_import_on_demand_;
    std::unordered_map<std::string, VariableSymbol*> single_static_imports_;
    std::unordered_map<std::string, MethodSymbol*> single_static_method_imports_;
    std::vector<TypeSymbol*> static_import_on_demand_;

    // Type being processed
    TypeSymbol* this_type_ = nullptr;

    // Error reporting
    void reportError(jast2::Node* node, const std::string& message);
    void reportError(int line, int column, const std::string& message);
    void reportWarning(jast2::Node* node, const std::string& message);

    // Scope management
    void pushScope();
    void popScope();
    void enterType(TypeSymbol* type);
    void exitType();
    void enterMethod(MethodSymbol* method);
    void exitMethod();

    // Phase 1: Process imports
    void processImports();
    void processImport(jast2::ImportDeclaration* import);

    // Phase 2: Build type declarations (create TypeSymbols)
    void processTypeDeclarations();
    TypeSymbol* processTypeDeclaration(jast2::Declaration* decl, TypeSymbol* enclosing = nullptr);
    TypeSymbol* processClassDeclaration(jast2::ClassDeclaration* decl, TypeSymbol* enclosing);
    TypeSymbol* processInterfaceDeclaration(jast2::InterfaceDeclaration* decl, TypeSymbol* enclosing);
    TypeSymbol* processEnumDeclaration(jast2::EnumDeclaration* decl, TypeSymbol* enclosing);
    TypeSymbol* processAnnotationDeclaration(jast2::AnnotationDeclaration* decl, TypeSymbol* enclosing);

    // Phase 3: Resolve type headers (extends, implements)
    void resolveTypeHeaders();
    void resolveTypeHeader(TypeSymbol* type);
    void resolveClassHeader(TypeSymbol* type, jast2::ClassDeclaration* decl);
    void resolveInterfaceHeader(TypeSymbol* type, jast2::InterfaceDeclaration* decl);
    void resolveEnumHeader(TypeSymbol* type, jast2::EnumDeclaration* decl);

    // Phase 4: Process members (create MethodSymbols, VariableSymbols)
    void processMembers();
    void processTypeMembers(TypeSymbol* type);
    void processClassBodyMembers(TypeSymbol* type, jast2::ClassBody* body);
    void processFieldDeclaration(TypeSymbol* type, jast2::FieldDeclaration* decl);
    void processMethodDeclaration(TypeSymbol* type, jast2::MethodDeclaration* decl);
    void processConstructorDeclaration(TypeSymbol* type, jast2::ConstructorDeclaration* decl);

    // Phase 5: Process method bodies
    void processBodies();
    void processMethodBody(MethodSymbol* method, jast2::MethodBody* body);
    void processInitializerBody(TypeSymbol* type, jast2::InitializerDeclaration* init);

    // Type resolution
    TypeSymbol* resolveType(jast2::Type* type);
    TypeSymbol* resolvePrimitiveType(jast2::PrimitiveType* type);
    TypeSymbol* resolveArrayType(jast2::ArrayType* type);
    TypeSymbol* resolveTypeName(jast2::TypeName* type);
    TypeSymbol* resolveTypeByName(const std::string& name);
    TypeSymbol* resolveQualifiedType(const std::string& qualified_name);
    PackageSymbol* resolvePackage(const std::string& name);
    TypeSymbol* findTypeInPackage(PackageSymbol* package, const std::string& name);
    TypeSymbol* findTypeInType(TypeSymbol* type, const std::string& name);

    // Expression processing
    TypeSymbol* processExpression(jast2::Expression* expr);
    TypeSymbol* processLiteral(jast2::LiteralExpression* expr);
    TypeSymbol* processNameExpression(jast2::NameExpression* expr);
    TypeSymbol* processThisExpression(jast2::ThisExpression* expr);
    TypeSymbol* processSuperExpression(jast2::SuperExpression* expr);
    TypeSymbol* processParenthesizedExpression(jast2::ParenthesizedExpression* expr);
    TypeSymbol* processBinaryExpression(jast2::BinaryExpression* expr);
    TypeSymbol* processUnaryExpression(jast2::UnaryExpression* expr);
    TypeSymbol* processAssignmentExpression(jast2::AssignmentExpression* expr);
    TypeSymbol* processConditionalExpression(jast2::ConditionalExpression* expr);
    TypeSymbol* processInstanceofExpression(jast2::InstanceofExpression* expr);
    TypeSymbol* processCastExpression(jast2::CastExpression* expr);
    TypeSymbol* processFieldAccess(jast2::FieldAccessExpression* expr);
    TypeSymbol* processMethodInvocation(jast2::MethodInvocationExpression* expr);
    TypeSymbol* processArrayAccess(jast2::ArrayAccessExpression* expr);
    TypeSymbol* processClassCreation(jast2::ClassCreationExpression* expr);
    TypeSymbol* processArrayCreation(jast2::ArrayCreationExpression* expr);
    TypeSymbol* processArrayInitializer(jast2::ArrayInitializer* expr, TypeSymbol* expected_type);
    TypeSymbol* processClassLiteral(jast2::ClassLiteralExpression* expr);

    // Statement processing
    void processStatement(jast2::Statement* stmt);
    void processBlockStatement(jast2::BlockStatement* stmt);
    void processExpressionStatement(jast2::ExpressionStatement* stmt);
    void processLocalVariableStatement(jast2::LocalVariableStatement* stmt);
    void processIfStatement(jast2::IfStatement* stmt);
    void processWhileStatement(jast2::WhileStatement* stmt);
    void processDoStatement(jast2::DoStatement* stmt);
    void processForStatement(jast2::ForStatement* stmt);
    void processForeachStatement(jast2::ForeachStatement* stmt);
    void processSwitchStatement(jast2::SwitchStatement* stmt);
    void processBreakStatement(jast2::BreakStatement* stmt);
    void processContinueStatement(jast2::ContinueStatement* stmt);
    void processReturnStatement(jast2::ReturnStatement* stmt);
    void processThrowStatement(jast2::ThrowStatement* stmt);
    void processSynchronizedStatement(jast2::SynchronizedStatement* stmt);
    void processTryStatement(jast2::TryStatement* stmt);
    void processAssertStatement(jast2::AssertStatement* stmt);
    void processThisCall(jast2::ThisCall* stmt);
    void processSuperCall(jast2::SuperCall* stmt);
    void processLabeledStatement(jast2::LabeledStatement* stmt);
    void processLocalClassStatement(jast2::LocalClassStatement* stmt);

    // Method resolution
    MethodSymbol* findMethod(TypeSymbol* type, const std::string& name,
                            const std::vector<TypeSymbol*>& arg_types);
    MethodSymbol* findConstructor(TypeSymbol* type,
                                 const std::vector<TypeSymbol*>& arg_types);
    bool isMethodApplicable(MethodSymbol* method, const std::vector<TypeSymbol*>& arg_types);
    MethodSymbol* selectMostSpecific(const std::vector<MethodSymbol*>& candidates,
                                    const std::vector<TypeSymbol*>& arg_types);

    // Field resolution
    VariableSymbol* findField(TypeSymbol* type, const std::string& name);

    // Type checking utilities
    bool isAssignable(TypeSymbol* target, TypeSymbol* source);
    bool isSubtype(TypeSymbol* sub, TypeSymbol* super);
    bool isNumeric(TypeSymbol* type);
    bool isIntegral(TypeSymbol* type);
    bool isBoolean(TypeSymbol* type);
    bool isReference(TypeSymbol* type);
    TypeSymbol* binaryNumericPromotion(TypeSymbol* left, TypeSymbol* right);
    TypeSymbol* unaryNumericPromotion(TypeSymbol* type);
    TypeSymbol* commonType(TypeSymbol* t1, TypeSymbol* t2);
    TypeSymbol* boxType(TypeSymbol* primitive);
    TypeSymbol* unboxType(TypeSymbol* wrapper);

    // Helper to get array type
    TypeSymbol* getArrayType(TypeSymbol* element_type, int dimensions);

    // Access control
    bool isAccessible(Symbol* member, TypeSymbol* from_type);
    bool isAccessibleType(TypeSymbol* type, TypeSymbol* from_type);

    // Modifier checking
    void checkModifiers(jast2::Modifiers* mods, bool is_class, bool is_interface,
                       bool is_method, bool is_field, bool is_local);

    // Utility to convert jast2 name to string
    std::string nameToString(jast2::Name* name);

    // Create primitive types cache reference
    TypeSymbol* intType();
    TypeSymbol* longType();
    TypeSymbol* floatType();
    TypeSymbol* doubleType();
    TypeSymbol* booleanType();
    TypeSymbol* charType();
    TypeSymbol* byteType();
    TypeSymbol* shortType();
    TypeSymbol* voidType();
    TypeSymbol* nullType();
    TypeSymbol* stringType();
    TypeSymbol* objectType();
    TypeSymbol* classType();
};

} // namespace Jopa

#endif // jast_semantic_INCLUDED
