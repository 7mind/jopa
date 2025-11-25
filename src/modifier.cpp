#include "platform.h"
#include "semantic.h"
#include "option.h"
#include "javasym.h"
#include "stream.h"


namespace Jopa { // Open namespace Jopa block
//
// Process modifiers, returning the AccessFlags they represent.
//
// Diagnostics are emitted in the following cases:
// . Invalid modifiers based on context.
// . Duplicated mutually exclusive modifiers (public, protected, private).
// . Duplicated general modifiers.
// . Modifier combinations not legal (such as final abstract).
// . Modifiers not in the recommended order (controlled by +Pmodifier-order).
// . Explicit specification of implicit modifier (controlled by
//   +Predundant-modifiers).
//
AccessFlags Semantic::ProcessModifiers(AstModifiers* modifiers,
                                       const wchar_t* declaration_kind_name,
                                       u2 valid_flags, u2 implicit_flags)
{
    if (! modifiers)
        return AccessFlags(implicit_flags);
    AccessFlags access_flags;
    bool seen_keyword = false;
    for (unsigned i = 0; i < modifiers -> NumModifiers(); i++)
    {
        Ast* mod = modifiers -> Modifier(i);
        AstAnnotation* annotation = mod -> AnnotationCast();
        AstModifierKeyword* keyword = mod -> ModifierKeywordCast();
        if (annotation)
        {
            if (seen_keyword)
            {
                ReportSemError(SemanticError::RECOMMENDED_ANNOTATION_ORDER,
                               mod,
                               lex_stream -> NameString(annotation -> name ->
                                                        identifier_token));
            }
            // Annotations are allowed in Java 5+. They will be processed
            // separately for semantic checking and bytecode generation.
            // For now, we just allow them to be parsed without error.
            continue;
        }
        assert(keyword);
        seen_keyword = true;
        u2 flag = 0;
        switch (lex_stream -> Kind(keyword -> modifier_token))
        {
        case TK_abstract: flag = AccessFlags::ACCESS_ABSTRACT; break;
        case TK_final: flag = AccessFlags::ACCESS_FINAL; break;
        case TK_native: flag = AccessFlags::ACCESS_NATIVE; break;
        case TK_public: flag = AccessFlags::ACCESS_PUBLIC; break;
        case TK_protected: flag = AccessFlags::ACCESS_PROTECTED; break;
        case TK_private: flag = AccessFlags::ACCESS_PRIVATE; break;
        case TK_static: flag = AccessFlags::ACCESS_STATIC; break;
        case TK_strictfp: flag = AccessFlags::ACCESS_STRICTFP; break;
        case TK_synchronized: flag = AccessFlags::ACCESS_SYNCHRONIZED; break;
        case TK_transient: flag = AccessFlags::ACCESS_TRANSIENT; break;
        case TK_volatile: flag = AccessFlags::ACCESS_VOLATILE; break;
        default:
            assert(false && "token was not modifier");
        }
        if ((flag & valid_flags) == 0)
        {
            ReportSemError(SemanticError::INVALID_MODIFIER, mod,
                           lex_stream -> NameString(keyword -> modifier_token),
                           declaration_kind_name);
        }
        else if ((flag & AccessFlags::ACCESS_ACCESS) &&
                 (access_flags.Flags() & AccessFlags::ACCESS_ACCESS))
        {
            ReportSemError(SemanticError::DUPLICATE_ACCESS_MODIFIER, mod);
        }
        else if (access_flags.IsSet(flag))
        {
            ReportSemError(SemanticError::DUPLICATE_MODIFIER, mod,
                           lex_stream -> NameString(keyword ->
                                                    modifier_token));
        }
        else
        {
            // We have a valid flag if it is alone.
            if ((flag & implicit_flags) != 0)
            {
                ReportSemError(SemanticError::REDUNDANT_MODIFIER, mod,
                               lex_stream -> NameString(keyword ->
                                                        modifier_token),
                               declaration_kind_name);
            }
            if (! access_flags.RecommendedOrder(flag))
            {
                ReportSemError(SemanticError::RECOMMENDED_MODIFIER_ORDER, mod,
                               lex_stream -> NameString(keyword ->
                                                        modifier_token));
            }
            access_flags.SetFlags(flag);
            if (access_flags.ACC_FINAL())
            {
                if (access_flags.ACC_VOLATILE())
                {
                    // We know it's a field because of volatile.
                    ReportSemError(SemanticError::VOLATILE_FINAL_FIELD, mod,
                                   lex_stream -> NameString(keyword ->
                                                            modifier_token));
                    access_flags.ResetFlags(flag);
                }
                else if (access_flags.ACC_ABSTRACT())
                {
                    ReportSemError(SemanticError::FINAL_ABSTRACT_ENTITY, mod,
                                   declaration_kind_name);
                    access_flags.ResetFlags(flag);
                }
            }
            else if (access_flags.ACC_ABSTRACT() &&
                     (valid_flags & AccessFlags::ACCESS_NATIVE))
            {
                //
                // Classes can be private static abstract strictfp, but methods
                // cannot. Hence, we checked that we are dealing with a method
                // by looking for NATIVE in the legal flags.
                //
                if (access_flags.ACC_SYNCHRONIZED() ||
                    access_flags.ACC_NATIVE() || access_flags.ACC_PRIVATE() ||
                    access_flags.ACC_STATIC() || access_flags.ACC_STRICTFP())
                {
                    ReportSemError(SemanticError::BAD_ABSTRACT_METHOD_MODIFIER,
                                   mod);
                    access_flags.ResetFlags(flag);
                }
            }
            else if (access_flags.ACC_STRICTFP() && access_flags.ACC_NATIVE())
            {
                ReportSemError(SemanticError::STRICTFP_NATIVE_METHOD, mod);
                access_flags.ResetFlags(flag);
            }
        }
    }
    access_flags.SetFlags(implicit_flags);
    return access_flags;
}

//
// Process modifiers of packages (annotations only, added in 1.5).
//
AccessFlags Semantic::ProcessPackageModifiers(AstPackageDeclaration* package)
{
    return ProcessModifiers(package -> modifiers_opt, L"a package", 0);
}


//
// Process modifiers of top-level types.
//
AccessFlags Semantic::ProcessTopLevelTypeModifiers(AstDeclaredType* decl)
{
    AstClassDeclaration* class_decl = decl -> ClassDeclarationCast();
    AstEnumDeclaration* enum_decl = decl -> EnumDeclarationCast();
    AstInterfaceDeclaration* interface_decl =
        decl -> InterfaceDeclarationCast();
    AstAnnotationDeclaration* annotation_decl =
        decl -> AnnotationDeclarationCast();
    (void)annotation_decl;
    u2 valid_flags;
    u2 implicit_flags = 0;
    const wchar_t* context;
    if (class_decl)
    {
        context = L"a top-level class";
        valid_flags = AccessFlags::ACCESS_ABSTRACT |
            AccessFlags::ACCESS_FINAL | AccessFlags::ACCESS_PUBLIC |
            AccessFlags::ACCESS_STRICTFP;
    }
    else if (enum_decl)
    {
        valid_flags = AccessFlags::ACCESS_ABSTRACT |
            AccessFlags::ACCESS_FINAL | AccessFlags::ACCESS_PUBLIC |
            AccessFlags::ACCESS_STRICTFP;
        implicit_flags = AccessFlags::ACCESS_FINAL;
        for (unsigned i = 0; i < enum_decl -> NumEnumConstants(); i++)
        {
            if (enum_decl -> EnumConstant(i) -> class_body_opt)
            {
                valid_flags &= ~ AccessFlags::ACCESS_FINAL;
                implicit_flags = 0;
                break;
            }
        }
        context = implicit_flags
            ? L"a top-level enumeration (all enum constants without a body)"
            : L"a top-level enumeration (some enum constants with a body)";
    }
    else
    {
        valid_flags = AccessFlags::ACCESS_ABSTRACT |
            AccessFlags::ACCESS_PUBLIC | AccessFlags::ACCESS_STRICTFP;
        implicit_flags = AccessFlags::ACCESS_INTERFACE |
            AccessFlags::ACCESS_ABSTRACT;
        if (interface_decl)
            context = L"a top-level interface";
        else
        {
            assert(annotation_decl);
            context = L"a top-level annotation type";
            implicit_flags |= AccessFlags::ACCESS_ANNOTATION;
        }
    }
    return ProcessModifiers(decl -> modifiers_opt, context, valid_flags,
                            implicit_flags);
}


//
// Process modifiers of local classes declared as a statement in a method.
//
AccessFlags Semantic::ProcessLocalClassModifiers(AstDeclaredType* decl)
{
    AstClassDeclaration* class_decl = decl -> ClassDeclarationCast();
    AstEnumDeclaration* enum_decl = decl -> EnumDeclarationCast();
    u2 valid_flags = AccessFlags::ACCESS_ABSTRACT |
        AccessFlags::ACCESS_FINAL | AccessFlags::ACCESS_STRICTFP;
    u2 implicit_flags = 0;
    const wchar_t* context;
    if (class_decl)
        context = L"a local inner class";
    else
    {
        assert(enum_decl);
        implicit_flags = AccessFlags::ACCESS_FINAL;
        for (unsigned i = 0; i < enum_decl -> NumEnumConstants(); i++)
        {
            if (enum_decl -> EnumConstant(i) -> class_body_opt)
            {
                valid_flags &= ~ AccessFlags::ACCESS_FINAL;
                implicit_flags = 0;
                break;
            }
        }
        context = implicit_flags
            ? L"a local enumeration (all enum constants without a body)"
            : L"a local enumeration (some enum constants with a body)";
    }
    return ProcessModifiers(decl -> modifiers_opt, context, valid_flags,
                            implicit_flags);
}


//
// Process modifiers of nested and inner types.
//
AccessFlags Semantic::ProcessNestedTypeModifiers(TypeSymbol* containing_type,
                                                 AstDeclaredType* decl)
{
    AstClassDeclaration* class_decl = decl -> ClassDeclarationCast();
    AstEnumDeclaration* enum_decl = decl -> EnumDeclarationCast();
    AstInterfaceDeclaration* interface_decl =
        decl -> InterfaceDeclarationCast();
    AstAnnotationDeclaration* annotation_decl =
        decl -> AnnotationDeclarationCast();
    (void)annotation_decl;
    u2 valid_flags;
    u2 implicit_flags = 0;
    const wchar_t* context;
    if (class_decl || enum_decl)
    {
        if (containing_type -> ACC_INTERFACE())
        {
            valid_flags = AccessFlags::ACCESS_ABSTRACT |
                AccessFlags::ACCESS_FINAL | AccessFlags::ACCESS_PUBLIC |
                AccessFlags::ACCESS_STATIC | AccessFlags::ACCESS_STRICTFP;
            implicit_flags = AccessFlags::ACCESS_STATIC |
                AccessFlags::ACCESS_PUBLIC;
        }
        else
        {
            valid_flags = AccessFlags::ACCESS_ABSTRACT |
                AccessFlags::ACCESS_FINAL | AccessFlags::ACCESS_ACCESS |
                AccessFlags::ACCESS_STATIC | AccessFlags::ACCESS_STRICTFP;
        }
        if (class_decl)
            context = L"a member class";
        else
        {
            implicit_flags |= AccessFlags::ACCESS_FINAL |
                AccessFlags::ACCESS_STATIC;
            for (unsigned i = 0; i < enum_decl -> NumEnumConstants(); i++)
            {
                if (enum_decl -> EnumConstant(i) -> class_body_opt)
                {
                    valid_flags &= ~ AccessFlags::ACCESS_FINAL;
                    implicit_flags &= ~ AccessFlags::ACCESS_FINAL;
                    break;
                }
            }
            context = (implicit_flags & AccessFlags::ACCESS_FINAL)
                ? L"a member enumeration (all enum constants without a body)"
                : L"a member enumeration (some enum constants with a body)";
        }
    }
    else
    {
        if (containing_type -> ACC_INTERFACE())
        {
            valid_flags = AccessFlags::ACCESS_ABSTRACT |
                AccessFlags::ACCESS_PUBLIC | AccessFlags::ACCESS_STATIC |
                AccessFlags::ACCESS_STRICTFP;
            implicit_flags = AccessFlags::ACCESS_INTERFACE |
                AccessFlags::ACCESS_ABSTRACT | AccessFlags::ACCESS_STATIC |
                AccessFlags::ACCESS_PUBLIC;
        }
        else
        {
            valid_flags = AccessFlags::ACCESS_ABSTRACT |
                AccessFlags::ACCESS_ACCESS | AccessFlags::ACCESS_STATIC |
                AccessFlags::ACCESS_STRICTFP;
            implicit_flags = AccessFlags::ACCESS_INTERFACE |
                AccessFlags::ACCESS_ABSTRACT | AccessFlags::ACCESS_STATIC;
        }
        if (interface_decl)
            context = L"a member interface";
        else
        {
            assert(annotation_decl);
            context = L"a member annotation type";
            implicit_flags |= AccessFlags::ACCESS_ANNOTATION;
        }
    }
    return ProcessModifiers(decl -> modifiers_opt, context, valid_flags,
                            implicit_flags);
}


//
// Process modifiers of fields declared in classes.
//
AccessFlags Semantic::ProcessFieldModifiers(AstFieldDeclaration* field_declaration)
{
    return ProcessModifiers(field_declaration -> modifiers_opt,
                            L"a class's member field",
                            (AccessFlags::ACCESS_ACCESS |
                             AccessFlags::ACCESS_STATIC |
                             AccessFlags::ACCESS_FINAL |
                             AccessFlags::ACCESS_TRANSIENT |
                             AccessFlags::ACCESS_VOLATILE));
}


//
// Process modifiers of local variables.
//
AccessFlags Semantic::ProcessLocalModifiers(AstLocalVariableStatement* decl)
{
    return ProcessModifiers(decl -> modifiers_opt, L"a local variable",
                            AccessFlags::ACCESS_FINAL);
}


//
// Process modifiers of parameters.
//
AccessFlags Semantic::ProcessFormalModifiers(AstFormalParameter* decl)
{
    return ProcessModifiers(decl -> modifiers_opt, L"a formal parameter",
                            AccessFlags::ACCESS_FINAL);
}


//
// Process modifiers of methods declared in classes.
//
AccessFlags Semantic::ProcessMethodModifiers(AstMethodDeclaration* decl)
{
    return ProcessModifiers(decl -> modifiers_opt, L"a class's member method",
                            (AccessFlags::ACCESS_ACCESS |
                             AccessFlags::ACCESS_STATIC |
                             AccessFlags::ACCESS_STRICTFP |
                             AccessFlags::ACCESS_ABSTRACT |
                             AccessFlags::ACCESS_FINAL |
                             AccessFlags::ACCESS_NATIVE |
                             AccessFlags::ACCESS_SYNCHRONIZED));
}


//
// Process modifiers of methods declared in interfaces.
//
AccessFlags Semantic::ProcessInterfaceMethodModifiers(AstMethodDeclaration* method_declaration)
{
    return ProcessModifiers(method_declaration -> modifiers_opt,
                            L"an interface's member method",
                            (AccessFlags::ACCESS_PUBLIC |
                             AccessFlags::ACCESS_ABSTRACT),
                            (AccessFlags::ACCESS_PUBLIC |
                             AccessFlags::ACCESS_ABSTRACT));
}


//
// Process modifiers of constructors.
//
AccessFlags Semantic::ProcessConstructorModifiers(AstConstructorDeclaration* constructor_declaration)
{
    return ProcessModifiers(constructor_declaration -> modifiers_opt,
                            L"a constructor", AccessFlags::ACCESS_ACCESS);
}


//
// Process modifiers of fields declared in interfaces.
//
AccessFlags Semantic::ProcessInterfaceFieldModifiers(AstFieldDeclaration* decl)
{
    return ProcessModifiers(decl -> modifiers_opt,
                            L"an interface's member field",
                            (AccessFlags::ACCESS_PUBLIC |
                             AccessFlags::ACCESS_STATIC |
                             AccessFlags::ACCESS_FINAL),
                            (AccessFlags::ACCESS_PUBLIC |
                             AccessFlags::ACCESS_STATIC |
                             AccessFlags::ACCESS_FINAL));
}


//
// Process static and instance initializer modifiers.
//
AccessFlags Semantic::ProcessInitializerModifiers(AstInitializerDeclaration* initializer)
{
    return ProcessModifiers(initializer -> modifiers_opt,
                            L"an initializer block",
                            AccessFlags::ACCESS_STATIC);
}


//
// Process static and instance initializer modifiers.
//
AccessFlags Semantic::ProcessEnumConstantModifiers(AstEnumConstant* decl)
{
    return ProcessModifiers(decl -> modifiers_opt, L"an enum constant", 0,
                            (AccessFlags::ACCESS_PUBLIC |
                             AccessFlags::ACCESS_STATIC |
                             AccessFlags::ACCESS_FINAL |
                             AccessFlags::ACCESS_ENUM));
}


} // Close namespace Jopa block
