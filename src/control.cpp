#include "control.h"
#include "scanner.h"
#include "parser.h"
#include "semantic.h"
#include "error.h"
#include "bytecode.h"
#include "case.h"
#include "option.h"
#include "paramtype.h"


namespace Jopa { // Open namespace Jopa block
Control::Control(char** arguments, Option& option_)
    : return_code(0)
    , option(option_)
    , dot_classpath_index(0)
    , system_table(NULL)
    , system_semantic(NULL)
    , semantic(1024)
    , needs_body_work(1024)
    , type_trash_bin(1024)
    , input_java_file_set(1021)
    , input_class_file_set(1021)
    , expired_file_set()
    , recompilation_file_set(1021)
    // Type and method cache. These variables are assigned in control.h
    // accessors, but must be NULL at startup.
    , Annotation_type(NULL)
    , AssertionError_type(NULL)
    , AssertionError_Init_method(NULL)
    , AssertionError_InitWithChar_method(NULL)
    , AssertionError_InitWithBoolean_method(NULL)
    , AssertionError_InitWithInt_method(NULL)
    , AssertionError_InitWithLong_method(NULL)
    , AssertionError_InitWithFloat_method(NULL)
    , AssertionError_InitWithDouble_method(NULL)
    , AssertionError_InitWithObject_method(NULL)
    , AutoCloseable_type(NULL)
    , AutoCloseable_close_method(NULL)
    , Boolean_type(NULL)
    , Boolean_TYPE_field(NULL)
    , Byte_type(NULL)
    , Byte_TYPE_field(NULL)
    , Character_type(NULL)
    , Character_TYPE_field(NULL)
    , Class_type(NULL)
    , Class_forName_method(NULL)
    , Class_getComponentType_method(NULL)
    , Class_desiredAssertionStatus_method(NULL)
    , ClassNotFoundException_type(NULL)
    , Cloneable_type(NULL)
    , Comparable_type(NULL)
    , Double_type(NULL)
    , Double_TYPE_field(NULL)
    , ElementType_type(NULL)
    , ElementType_TYPE_field(NULL)
    , ElementType_FIELD_field(NULL)
    , ElementType_METHOD_field(NULL)
    , ElementType_PARAMETER_field(NULL)
    , ElementType_CONSTRUCTOR_field(NULL)
    , ElementType_LOCAL_VARIABLE_field(NULL)
    , ElementType_ANNOTATION_TYPE_field(NULL)
    , ElementType_PACKAGE_field(NULL)
    , Enum_type(NULL)
    , Enum_Init_method(NULL)
    , Enum_ordinal_method(NULL)
    , Enum_valueOf_method(NULL)
    , Error_type(NULL)
    , Exception_type(NULL)
    , Float_type(NULL)
    , Float_TYPE_field(NULL)
    , Integer_type(NULL)
    , Integer_TYPE_field(NULL)
    , Iterable_type(NULL)
    , Iterable_iterator_method(NULL)
    , Iterator_type(NULL)
    , Iterator_hasNext_method(NULL)
    , Iterator_next_method(NULL)
    , Long_type(NULL)
    , Long_TYPE_field(NULL)
    , NoClassDefFoundError_type(NULL)
    , NoClassDefFoundError_Init_method(NULL)
    , NoClassDefFoundError_InitString_method(NULL)
    , Object_type(NULL)
    , Object_getClass_method(NULL)
    , Overrides_type(NULL)
    , Retention_type(NULL)
    , RetentionPolicy_type(NULL)
    , RetentionPolicy_SOURCE_field(NULL)
    , RetentionPolicy_CLASS_field(NULL)
    , RetentionPolicy_RUNTIME_field(NULL)
    , RuntimeException_type(NULL)
    , Serializable_type(NULL)
    , Short_type(NULL)
    , Short_TYPE_field(NULL)
    , String_type(NULL)
    , String_hashCode_method(NULL)
    , String_equals_method(NULL)
    , StringBuffer_type(NULL)
    , StringBuffer_Init_method(NULL)
    , StringBuffer_InitWithString_method(NULL)
    , StringBuffer_toString_method(NULL)
    , StringBuffer_append_char_method(NULL)
    , StringBuffer_append_boolean_method(NULL)
    , StringBuffer_append_int_method(NULL)
    , StringBuffer_append_long_method(NULL)
    , StringBuffer_append_float_method(NULL)
    , StringBuffer_append_double_method(NULL)
    , StringBuffer_append_string_method(NULL)
    , StringBuffer_append_object_method(NULL)
    , StringBuilder_type(NULL)
    , StringBuilder_Init_method(NULL)
    , StringBuilder_InitWithString_method(NULL)
    , StringBuilder_toString_method(NULL)
    , StringBuilder_append_char_method(NULL)
    , StringBuilder_append_boolean_method(NULL)
    , StringBuilder_append_int_method(NULL)
    , StringBuilder_append_long_method(NULL)
    , StringBuilder_append_float_method(NULL)
    , StringBuilder_append_double_method(NULL)
    , StringBuilder_append_string_method(NULL)
    , StringBuilder_append_object_method(NULL)
    , Target_type(NULL)
    , Throwable_type(NULL)
    , Throwable_getMessage_method(NULL)
    , Throwable_initCause_method(NULL)
    , Throwable_addSuppressed_method(NULL)
    , Void_type(NULL)
    , Void_TYPE_field(NULL)
    // storage for all literals seen in source
    , int_pool(&bad_value)
    , long_pool(&bad_value)
    , float_pool(&bad_value)
    , double_pool(&bad_value)
    , Utf8_pool(&bad_value)
#ifdef JOPA_DEBUG
    , input_files_processed(0)
    , class_files_read(0)
    , class_files_written(0)
    , line_count(0)
#endif // JOPA_DEBUG
    // Package cache.  unnamed and lang are initialized in constructor body.
    , annotation_package(NULL)
    , io_package(NULL)
    , util_package(NULL)
{
    ProcessGlobals();
    ProcessUnnamedPackage();
    ProcessPath();
    ProcessSystemInformation();

    //
    // Instantiate a scanner and a parser and initialize the static members
    // for the semantic processors.
    //
    scanner = new Scanner(*this);
    parser = new Parser();
    SemanticError::StaticInitializer();

    //
    // Process all file names specified in command line
    //
    ProcessNewInputFiles(input_java_file_set, arguments);

    //
    // For each input file, copy it into the input_files array and process
    // its package declaration. Estimate we need 64 tokens.
    //
    StoragePool* ast_pool = new StoragePool(64);
    FileSymbol** input_files = new FileSymbol*[input_java_file_set.Size() + 1];
    int num_files = 0;
    FileSymbol* file_symbol;
    for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
         file_symbol;
         file_symbol = (FileSymbol*) input_java_file_set.NextElement())
    {
        input_files[num_files++] = file_symbol;
#ifdef JOPA_DEBUG
        input_files_processed++;
#endif
        errno = 0;
        scanner -> Scan(file_symbol);
        if (file_symbol -> lex_stream) // did we have a successful scan!
        {
            AstPackageDeclaration* package_declaration =
                parser -> PackageHeaderParse(file_symbol -> lex_stream,
                                             ast_pool);
            ProcessPackageDeclaration(file_symbol, package_declaration);
            ast_pool -> Reset();
        }
        else
        {
            const char* std_err = strerror(errno);
            ErrorString err_str;
            err_str << '"' << std_err << '"' << " while trying to open "
                    << file_symbol -> FileName();
            general_io_errors.Next() = err_str.SafeArray();
        }
    }

    //
    // Handle parse-only mode
    //
    if (option.parse_only)
    {
        bool parse_success = true;
        int total_errors = 0;

        // Parse each file fully
        for (int j = 0; j < num_files; j++)
        {
            FileSymbol* file_symbol = input_files[j];
            if (file_symbol -> lex_stream)
            {
                file_symbol -> lex_stream -> Reset();
                file_symbol -> compilation_unit =
                    parser -> HeaderParse(file_symbol -> lex_stream, ast_pool);
                if (! file_symbol -> compilation_unit ||
                    file_symbol -> compilation_unit -> BadCompilationUnitCast())
                {
                    parse_success = false;
                }
                total_errors += file_symbol -> lex_stream -> NumBadTokens();
            }
            else
            {
                parse_success = false;
            }
        }

        // Also check for IO errors
        if (general_io_errors.Length() > 0)
        {
            parse_success = false;
        }

        // Write the result to the output file
        FILE* outfile = SystemFopen(option.parse_only_output, "w");
        if (outfile)
        {
            if (parse_success && total_errors == 0)
            {
                fprintf(outfile, "OK\n");
                return_code = 0;
            }
            else
            {
                fprintf(outfile, "FAIL\n");
                fprintf(outfile, "Errors: %d\n", total_errors);
                return_code = 1;
            }
            fclose(outfile);
        }
        else
        {
            Coutput << "*** Cannot open parse-only output file "
                    << option.parse_only_output << endl;
            return_code = 1;
        }

        // Clean up compilation units and their ast_pools
        for (int j = 0; j < num_files; j++)
        {
            FileSymbol* file_symbol = input_files[j];
            if (file_symbol -> compilation_unit)
            {
                delete file_symbol -> compilation_unit -> ast_pool;
                file_symbol -> compilation_unit = NULL;
            }
        }
        delete ast_pool;
        delete [] input_files;
        return;
    }

    //
    //
    //
    FileSymbol* main_file_clone;
    if (num_files > 0)
        main_file_clone = input_files[0] -> Clone();
    else
    {
        //
        // Some name, any name !!! We use dot_name_symbol as a bad file name
        // because no file can be named ".".
        //
        FileSymbol* file_symbol = classpath[dot_classpath_index] ->
            RootDirectory() -> InsertFileSymbol(dot_name_symbol);
        file_symbol -> directory_symbol = classpath[dot_classpath_index] ->
            RootDirectory();
        file_symbol -> SetJava();

        main_file_clone = file_symbol -> Clone();
    }

    main_file_clone -> semantic = new Semantic(*this, main_file_clone);
    system_semantic = main_file_clone -> semantic;
    scanner -> SetUp(main_file_clone);


    unsigned i;
    for (i = 0; i < bad_dirnames.Length(); i++)
    {
        system_semantic ->
            ReportSemError(SemanticError::CANNOT_OPEN_PATH_DIRECTORY,
                           BAD_TOKEN, bad_dirnames[i]);
    }
    for (i = 0; i < bad_zip_filenames.Length(); i++)
    {
        system_semantic -> ReportSemError(SemanticError::CANNOT_OPEN_ZIP_FILE,
                                          BAD_TOKEN, bad_zip_filenames[i]);
    }
    for (i = 0; i < general_io_warnings.Length(); i++)
    {
        system_semantic -> ReportSemError(SemanticError::IO_WARNING, BAD_TOKEN,
                                          general_io_warnings[i]);
        delete [] general_io_warnings[i];
    }
    for (i = 0; i < general_io_errors.Length(); i++)
    {
        system_semantic -> ReportSemError(SemanticError::IO_ERROR, BAD_TOKEN,
                                          general_io_errors[i]);
        delete [] general_io_errors[i];
    }

    //
    // Require the existence of java.lang.
    //
    if (lang_package -> directory.Length() == 0)
    {
        system_semantic -> ReportSemError(SemanticError::PACKAGE_NOT_FOUND,
                                          BAD_TOKEN,
                                          StringConstant::US_java_SL_lang);
    }

    //
    // When the -d option is specified, create the relevant
    // directories if they don't already exist.
    //
    if (option.directory)
    {
        if (! SystemIsDirectory(option.directory))
        {
            for (char* ptr = option.directory; *ptr; ptr++)
            {
                char delimiter = *ptr;
                if (delimiter == U_SLASH)
                {
                    *ptr = U_NULL;

                    if (! SystemIsDirectory(option.directory))
                        SystemMkdir(option.directory);

                    *ptr = delimiter;
                }
            }

            SystemMkdir(option.directory);

            if (! SystemIsDirectory(option.directory))
            {
                int length = strlen(option.directory);
                wchar_t* name = new wchar_t[length + 1];
                for (int j = 0; j < length; j++)
                    name[j] = option.directory[j];
                name[length] = U_NULL;
                system_semantic -> ReportSemError(SemanticError::CANNOT_OPEN_DIRECTORY,
                                                  BAD_TOKEN, name);
                delete [] name;
            }
        }
    }

    //
    //
    //
    for (i = 0; i < bad_input_filenames.Length(); i++)
    {
        system_semantic -> ReportSemError(SemanticError::BAD_INPUT_FILE,
                                          BAD_TOKEN, bad_input_filenames[i]);
    }

    //
    //
    //
    for (i = 0; i < unreadable_input_filenames.Length(); i++)
    {
        system_semantic -> ReportSemError(SemanticError::UNREADABLE_INPUT_FILE,
                                          BAD_TOKEN,
                                          unreadable_input_filenames[i]);
    }

    //
    //
    //
    if (system_semantic -> NumErrors() > 0)
    {
        system_semantic -> PrintMessages();
        return_code = system_semantic -> return_code;
    }
    else
    {
        //
        // There might be some warnings we want to print.
        //
        system_semantic -> PrintMessages();
        input_java_file_set.SetEmpty();
        for (int j = 0; j < num_files; j++)
        {
            FileSymbol* file_symbol = input_files[j];
            if (! input_java_file_set.IsElement(file_symbol))
                ProcessFile(file_symbol);
        }

        //
        // Clean up all the files that have just been compiled in this new
        // batch.
        //
        FileSymbol* file_symbol;
        for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
             file_symbol;
             file_symbol = (FileSymbol*) input_java_file_set.NextElement())
        {
            CleanUp(file_symbol);
        }

        //
        // If more messages were added to system_semantic, print them...
        //
        system_semantic -> PrintMessages();
        if (system_semantic -> return_code > 0 ||
            bad_input_filenames.Length() > 0 ||
            unreadable_input_filenames.Length() > 0)
        {
            return_code = 1;
        }

        //
        // If the incremental flag is on, check to see if the user wants us
        // to recompile.
        //
        if (option.incremental)
        {
            // The depend flag should only be in effect in the first pass
            option.depend = false;

            for (bool recompile = IncrementalRecompilation();
                 recompile; recompile = IncrementalRecompilation())
            {
                // Reset the return code as we may compile clean in this pass.
                return_code = 0;
                system_semantic -> return_code = 0;

                //
                //
                //
                for (i = 0; i < bad_input_filenames.Length(); i++)
                {
                    system_semantic ->
                        ReportSemError(SemanticError::BAD_INPUT_FILE,
                                       BAD_TOKEN, bad_input_filenames[i]);
                }

                //
                //
                //
                for (i = 0; i < unreadable_input_filenames.Length(); i++)
                {
                    system_semantic ->
                        ReportSemError(SemanticError::UNREADABLE_INPUT_FILE,
                                       BAD_TOKEN,
                                       unreadable_input_filenames[i]);
                }

                FileSymbol* file_symbol;

                num_files = 0;
                delete [] input_files; // delete previous copy
                input_files = new FileSymbol*[recompilation_file_set.Size()];
                for (file_symbol = (FileSymbol*) recompilation_file_set.FirstElement();
                     file_symbol;
                     file_symbol = (FileSymbol*) recompilation_file_set.NextElement())
                {
                    input_java_file_set.RemoveElement(file_symbol);
                    input_files[num_files++] = file_symbol;

                    LexStream* lex_stream = file_symbol -> lex_stream;
                    if (lex_stream)
                    {
                        AstPackageDeclaration* package_declaration = parser ->
                            PackageHeaderParse(lex_stream, ast_pool);
                        ProcessPackageDeclaration(file_symbol,
                                                  package_declaration);
                        ast_pool -> Reset();
                    }
                }

                //
                // If a file was erased, remove it from the input file set.
                //
                for (file_symbol = (FileSymbol*) expired_file_set.FirstElement();
                     file_symbol;
                     file_symbol = (FileSymbol*) expired_file_set.NextElement())
                {
                    input_java_file_set.RemoveElement(file_symbol);
                }

                //
                // Reset the global objects before recompiling this new batch.
                //
                expired_file_set.SetEmpty();
                recompilation_file_set.SetEmpty();
                type_trash_bin.Reset();

                //
                // For each file that should be recompiled, process it if it
                // has not already been dragged in by dependence.
                //
                for (int j = 0; j < num_files; j++)
                {
                    FileSymbol* file_symbol = input_files[j];
                    if (! input_java_file_set.IsElement(file_symbol))
                        ProcessFile(file_symbol);
                }

                //
                // Clean up all the files that have just been compiled in
                // this new batch.
                //
                for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
                    // delete file_symbol
                     file_symbol;
                     file_symbol = (FileSymbol*) input_java_file_set.NextElement())
                {
                    // delete file_symbol
                    CleanUp(file_symbol);
                }

                //
                // If any system error or warning was detected, print it...
                //
                system_semantic -> PrintMessages();
                if (system_semantic -> return_code > 0 ||
                    bad_input_filenames.Length() > 0 ||
                    unreadable_input_filenames.Length() > 0)
                {
                    return_code = 1;
                }
            }
        }

        //
        // Are we supposed to generate Makefiles?
        //
        if (option.makefile)
        {
            if (option.dependence_report)
            {
                FILE* outfile = SystemFopen(option.dependence_report_name,
                                            "w");
                if (outfile == NULL)
                    Coutput << "*** Cannot open dependence output file "
                            << option.dependence_report_name << endl;
                else
                {
                    SymbolSet types_in_new_files;
                    FileSymbol* file_symbol;
                    for (file_symbol = (FileSymbol*) input_java_file_set.FirstElement();
                         file_symbol;
                         file_symbol = (FileSymbol*) input_java_file_set.NextElement())
                    {
                        char* java_name = file_symbol -> FileName();

                        for (i = 0; i < file_symbol -> types.Length(); i++)
                        {
                            TypeSymbol* type = file_symbol -> types[i];
                            fprintf(outfile, "%s : %s\n", java_name,
                                    type -> SignatureString());

                            TypeSymbol* static_parent;
                            for (static_parent = (TypeSymbol*) type -> static_parents -> FirstElement();
                                 static_parent;
                                 static_parent = (TypeSymbol*) type -> static_parents -> NextElement())
                            {
                                if (! type -> parents ->
                                    IsElement(static_parent))
                                {
                                    // Only a static ref to static_parent?
                                    fprintf(outfile, "   !%s\n",
                                            static_parent -> SignatureString());

                                    //
                                    // If the type is contained in a type that
                                    // is not one of the input files, save it
                                    //
                                    if (static_parent -> file_symbol &&
                                        static_parent -> file_symbol -> IsClass())
                                    {
                                        types_in_new_files.AddElement(static_parent);
                                    }
                                }
                            }

                            TypeSymbol* parent;
                            for (parent = (TypeSymbol*) type -> parents -> FirstElement();
                                 parent;
                                 parent = (TypeSymbol*) type -> parents -> NextElement())
                            {
                                fprintf(outfile, "    %s\n",
                                        parent -> SignatureString());

                                //
                                // If the type is contained in a type that is
                                // not one of the input files, save it
                                //
                                if (parent -> file_symbol &&
                                    parent -> file_symbol -> IsClass())
                                {
                                    types_in_new_files.AddElement(parent);
                                }
                            }
                        }
                    }

                    //
                    // Print the list of class files that are referenced.
                    //
                    TypeSymbol* type;
                    for (type = (TypeSymbol*) types_in_new_files.FirstElement();
                         type;
                         type = (TypeSymbol*) types_in_new_files.NextElement())
                    {
                        char* class_name = type -> file_symbol -> FileName();
                        fprintf(outfile, "%s : %s\n", class_name,
                                type -> SignatureString());
                    }

                    fclose(outfile);
                }
            }
            else
            {
                SymbolSet* candidates =
                    new SymbolSet(input_java_file_set.Size() +
                                  input_class_file_set.Size());
                *candidates = input_java_file_set;
                candidates -> Union(input_class_file_set);

                TypeDependenceChecker* dependence_checker =
                    new TypeDependenceChecker(this, *candidates,
                                              type_trash_bin);
                dependence_checker -> PartialOrder();
                dependence_checker -> OutputDependences();
                delete dependence_checker;

                delete candidates;
            }
        }
    }

    delete ast_pool;
    delete main_file_clone; // delete the clone of the main source file...
    delete [] input_files;
}


Control::~Control()
{
    unsigned i;
    for (i = 0; i < bad_zip_filenames.Length(); i++)
        delete [] bad_zip_filenames[i];
    for (i = 0; i < bad_input_filenames.Length(); i++)
        delete [] bad_input_filenames[i];
    for (i = 0; i < unreadable_input_filenames.Length(); i++)
        delete [] unreadable_input_filenames[i];
    for (i = 0; i < system_directories.Length(); i++)
        delete system_directories[i];

    delete scanner;
    delete parser;
    delete system_semantic;
    delete system_table;

    // Clean up globally-stored ParameterizedType objects
    for (ParameterizedType* ptype : parameterized_types)
        delete ptype;

#ifdef JOPA_DEBUG
    if (option.debug_dump_lex || option.debug_dump_ast ||
        option.debug_unparse_ast)
    {
        Coutput << line_count << " source lines read" << endl
                << class_files_read << " \".class\" files read" << endl
                << class_files_written << " \".class\" files written" << endl
                << input_files_processed << " \".java\" files processed"
                << endl;
    }
#endif // JOPA_DEBUG
}


PackageSymbol* Control::ProcessPackage(const wchar_t* name)
{
    int name_length = wcslen(name);
    wchar_t* package_name = new wchar_t[name_length];
    int length;
    for (length = 0;
         length < name_length && name[length] != U_SLASH; length++)
    {
         package_name[length] = name[length];
    }
    NameSymbol* name_symbol = FindOrInsertName(package_name, length);

    PackageSymbol* package_symbol =
        external_table.FindPackageSymbol(name_symbol);
    if (! package_symbol)
    {
        package_symbol = external_table.InsertPackageSymbol(name_symbol, NULL);
        FindPathsToDirectory(package_symbol);
    }

    while (length < name_length)
    {
        int start = ++length;
        for (int i = 0;
             length < name_length && name[length] != U_SLASH;
             i++, length++)
        {
             package_name[i] = name[length];
        }
        name_symbol = FindOrInsertName(package_name, length - start);
        PackageSymbol* subpackage_symbol =
            package_symbol -> FindPackageSymbol(name_symbol);
        if (! subpackage_symbol)
        {
            subpackage_symbol =
                package_symbol -> InsertPackageSymbol(name_symbol);
            FindPathsToDirectory(subpackage_symbol);
        }
        package_symbol = subpackage_symbol;
    }

    delete [] package_name;
    return package_symbol;
}


//
// When searching for a subdirectory in a zipped file, it must already be
// present in the hierarchy.
//
DirectorySymbol* Control::FindSubdirectory(PathSymbol* path_symbol,
                                           wchar_t* name, int name_length)
{
    wchar_t* directory_name = new wchar_t[name_length + 1];

    DirectorySymbol* directory_symbol = path_symbol -> RootDirectory();
    for (int start = 0, end;
         directory_symbol && start < name_length;
         start = end + 1)
    {
        end = start;
        for (int i = 0; end < name_length && name[end] != U_SLASH; i++, end++)
             directory_name[i] = name[end];
        NameSymbol* name_symbol = FindOrInsertName(directory_name,
                                                   end - start);
        directory_symbol =
            directory_symbol -> FindDirectorySymbol(name_symbol);
    }

    delete [] directory_name;
    return directory_symbol;
}


//
// When searching for a directory in the system, if it is not already present
// in the hierarchy insert it and attempt to read it from the system...
//
#ifdef UNIX_FILE_SYSTEM
DirectorySymbol* Control::ProcessSubdirectories(wchar_t* source_name,
                                                int source_name_length,
                                                bool source_dir)
{
    int name_length = (source_name_length < 0 ? 0 : source_name_length);
    char* input_name = new char[name_length + 1];
    for (int i = 0; i < name_length; i++)
        input_name[i] = source_name[i];
    input_name[name_length] = U_NULL;

    DirectorySymbol* directory_symbol = NULL;
    struct stat status;
    if (SystemStat(input_name, &status) == 0 &&
        (status.st_mode & JOPA_STAT_S_IFDIR))
    {
        directory_symbol = system_table ->
            FindDirectorySymbol(status.st_dev, status.st_ino);
    }

    if (! directory_symbol)
    {
        if (input_name[0] == U_SLASH) // file name starts with '/'
        {
            directory_symbol =
                new DirectorySymbol(FindOrInsertName(source_name, name_length),
                                    classpath[dot_classpath_index],
                                    source_dir);
            directory_symbol -> ReadDirectory();
            system_directories.Next() = directory_symbol;
            system_table -> InsertDirectorySymbol(status.st_dev,
                                                  status.st_ino,
                                                  directory_symbol);
        }
        else
        {
            wchar_t* name = new wchar_t[name_length + 1];
            for (int i = 0; i < name_length; i++)
                name[i] = source_name[i];
            name[name_length] = U_NULL;

            // Start at the dot directory.
            directory_symbol =
                classpath[dot_classpath_index] -> RootDirectory();

            wchar_t* directory_name = new wchar_t[name_length];
            int end = 0;
            for (int start = end; start < name_length; start = end)
            {
                int length;
                for (length = 0;
                     end < name_length && name[end] != U_SLASH;
                     length++, end++)
                {
                    directory_name[length] = name[end];
                }

                if (length != 1 || directory_name[0] != U_DOT)
                {
                    // Not the current directory.
                    if (length == 2 && directory_name[0] == U_DOT &&
                        directory_name[1] == U_DOT)
                    {
                        // keep the current directory
                        if (directory_symbol -> Identity() == dot_name_symbol ||
                            directory_symbol -> Identity() == dot_dot_name_symbol)
                        {
                            DirectorySymbol* subdirectory_symbol =
                                directory_symbol -> FindDirectorySymbol(dot_dot_name_symbol);
                            if (! subdirectory_symbol)
                                subdirectory_symbol =
                                    directory_symbol -> InsertDirectorySymbol(dot_dot_name_symbol,
                                                                              source_dir);
                            directory_symbol = subdirectory_symbol;
                        }
                        else directory_symbol = directory_symbol -> owner -> DirectoryCast();
                    }
                    else
                    {
                        NameSymbol* name_symbol =
                            FindOrInsertName(directory_name, length);
                        DirectorySymbol* subdirectory_symbol =
                            directory_symbol -> FindDirectorySymbol(name_symbol);
                        if (! subdirectory_symbol)
                            subdirectory_symbol =
                                directory_symbol -> InsertDirectorySymbol(name_symbol,
                                                                          source_dir);
                        directory_symbol = subdirectory_symbol;
                    }
                }

                for (end++;
                     end < name_length && name[end] == U_SLASH;
                     end++); // skip all extra '/'
            }

            //
            // Insert the new directory into the system table to avoid
            // duplicates, in case the same directory is specified with
            // a different name.
            //
            if (directory_symbol !=
                classpath[dot_classpath_index] -> RootDirectory())
            {
                // Not the dot directory.
                system_table -> InsertDirectorySymbol(status.st_dev,
                                                      status.st_ino,
                                                      directory_symbol);
                directory_symbol -> ReadDirectory();
            }

            delete [] directory_name;
            delete [] name;
        }
    }

    delete [] input_name;
    return directory_symbol;
}
#endif


void Control::ProcessNewInputFiles(SymbolSet& file_set, char** arguments)
{
    unsigned i;
    for (i = 0; i < bad_input_filenames.Length(); i++)
        delete [] bad_input_filenames[i];
    bad_input_filenames.Reset();
    for (i = 0; i < unreadable_input_filenames.Length(); i++)
        delete [] unreadable_input_filenames[i];
    unreadable_input_filenames.Reset();

    //
    // Process all file names specified in command line. By this point, only
    // filenames should remain in arguments - constructing the Option should
    // have filtered out all options and expanded @files.
    //
    if (arguments)
    {
        int j = 0;
        while (arguments[j])
        {
            char* file_name = arguments[j++];
            unsigned file_name_length = strlen(file_name);

            wchar_t* name = new wchar_t[file_name_length + 1];
            for (unsigned i = 0; i < file_name_length; i++)
                name[i] = (file_name[i] != U_BACKSLASH ? file_name[i]
                           : (wchar_t) U_SLASH); // Change '\' to '/'.
            name[file_name_length] = U_NULL;

            //
            // File must be of the form xxx.java where xxx is a
            // character string consisting of at least one character.
            //
            if (file_name_length < FileSymbol::java_suffix_length ||
                (! FileSymbol::IsJavaSuffix(&file_name[file_name_length - FileSymbol::java_suffix_length])))
            {
                bad_input_filenames.Next() = name;
            }
            else
            {
                FileSymbol* file_symbol =
                    FindOrInsertJavaInputFile(name,
                                              file_name_length - FileSymbol::java_suffix_length);

                if (! file_symbol)
                    unreadable_input_filenames.Next() = name;
                else
                {
                    delete [] name;
                    file_set.AddElement(file_symbol);
                }
            }
        }
    }
}


FileSymbol* Control::FindOrInsertJavaInputFile(DirectorySymbol* directory_symbol,
                                               NameSymbol* file_name_symbol)
{
    FileSymbol* file_symbol = NULL;

    int length = file_name_symbol -> Utf8NameLength() +
        FileSymbol::java_suffix_length;
    char* java_name = new char[length + 1]; // +1 for \0
    strcpy(java_name, file_name_symbol -> Utf8Name());
    strcat(java_name, FileSymbol::java_suffix);

    DirectoryEntry* entry = directory_symbol -> FindEntry(java_name, length);
    if (entry)
    {
        file_symbol = directory_symbol -> FindFileSymbol(file_name_symbol);

        if (! file_symbol)
        {
            file_symbol =
                directory_symbol -> InsertFileSymbol(file_name_symbol);
            file_symbol -> directory_symbol = directory_symbol;
            file_symbol -> SetJava();
        }

        file_symbol -> mtime = entry -> Mtime();
    }

    delete [] java_name;
    return file_symbol;
}


FileSymbol* Control::FindOrInsertJavaInputFile(wchar_t* name, int name_length)
{
    FileSymbol* file_symbol = NULL;

    //
    // The name has been preprocessed so that if it contains any
    // slashes it is a forward slash. In the loop below we look
    // for the occurrence of the first slash (if any) that separates
    // the file name from its directory name.
    //
    DirectorySymbol* directory_symbol;
    NameSymbol* file_name_symbol;
#ifdef UNIX_FILE_SYSTEM
    int len;
    for (len = name_length - 1; len >= 0 && name[len] != U_SLASH; len--)
        ;
    directory_symbol = ProcessSubdirectories(name, len, true);
    file_name_symbol = FindOrInsertName(&name[len + 1],
                                        name_length - (len + 1));
#endif

    for (unsigned i = 1; i < classpath.Length(); i++)
    {
        if (i == dot_classpath_index) // the current directory (.).
        {
            file_symbol = FindOrInsertJavaInputFile(directory_symbol,
                                                    file_name_symbol);
            if (file_symbol)
                break;
        }
        else if (classpath[i] -> IsZip())
        {
            DirectorySymbol* directory_symbol = FindSubdirectory(classpath[i],
                                                                 name, len);
            if (directory_symbol)
            {
                file_symbol =
                    directory_symbol -> FindFileSymbol(file_name_symbol);
                if (file_symbol && file_symbol -> IsJava())
                     break;
                else file_symbol = NULL;
            }
        }
    }

    //
    // If the file was found, return it; otherwise, in case the (.) directory
    // was not specified in the classpath, search for the file in it...
    //
    return file_symbol ? file_symbol
        : FindOrInsertJavaInputFile(directory_symbol, file_name_symbol);
}


PackageSymbol* Control::FindOrInsertPackage(LexStream* lex_stream,
                                            AstName* name)
{
    PackageSymbol* package;

    if (name -> base_opt)
    {
        package = FindOrInsertPackage(lex_stream, name -> base_opt);
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        PackageSymbol* subpackage = package -> FindPackageSymbol(name_symbol);
        if (! subpackage)
            subpackage = package -> InsertPackageSymbol(name_symbol);
        package = subpackage;
    }
    else
    {
        NameSymbol* name_symbol =
            lex_stream -> NameSymbol(name -> identifier_token);
        package = external_table.FindPackageSymbol(name_symbol);
        if (! package)
            package = external_table.InsertPackageSymbol(name_symbol, NULL);
    }

    FindPathsToDirectory(package);
    return package;
}


void Control::ProcessFile(FileSymbol* file_symbol)
{
    ProcessHeaders(file_symbol);

    //
    // As long as there are new bodies, ...
    //
    for (unsigned i = 0; i < needs_body_work.Length(); i++)
    {
        assert(semantic.Length() == 0);

        //
        // These bodies are not necessarily in file_symbol; they
        // might be in another FileSymbol used by file_symbol.
        //
        ProcessBodies(needs_body_work[i]);
    }
    needs_body_work.Reset();
}


void Control::ProcessHeaders(FileSymbol* file_symbol)
{
    if (file_symbol -> semantic)
        return;
    input_java_file_set.AddElement(file_symbol);

    bool initial_invocation = (semantic.Length() == 0);

    if (option.verbose)
    {
        Coutput << "[read "
                << file_symbol -> FileName()
                << "]" << endl;
    }

    if (! file_symbol -> lex_stream)
         scanner -> Scan(file_symbol);
    else file_symbol -> lex_stream -> Reset();

    if (file_symbol -> lex_stream) // do we have a successful scan!
    {
        if (! file_symbol -> compilation_unit)
            file_symbol -> compilation_unit =
                parser -> HeaderParse(file_symbol -> lex_stream);
        //
        // If we have a compilation unit, analyze it, process its types.
        //
        if (file_symbol -> compilation_unit)
        {
            assert(! file_symbol -> semantic);

            if (! file_symbol -> package)
                ProcessPackageDeclaration(file_symbol,
                                          file_symbol -> compilation_unit -> package_declaration_opt);
            file_symbol -> semantic = new Semantic(*this, file_symbol);
            semantic.Next() = file_symbol -> semantic;
            file_symbol -> semantic -> ProcessTypeNames();
        }
    }

    if (initial_invocation)
        ProcessMembers();
}


void Control::ProcessMembers()
{
    Tuple<TypeSymbol*> partially_ordered_types(1024);
    SymbolSet needs_member_work(101);
    TypeCycleChecker cycle_checker(partially_ordered_types);
    TopologicalSort topological_sorter(needs_member_work,
                                       partially_ordered_types);

    unsigned start = 0;
    while (start < semantic.Length())
    {
        needs_member_work.SetEmpty();

        do
        {
            //
            // Check whether or not there are cycles in this new batch of
            // types. Create a partial order of the types (cycles are ordered
            // arbitrarily) and place the result in partially_ordered_types.
            //
            cycle_checker.PartialOrder(semantic, start);
            start = semantic.Length(); // next starting point

            //
            // Process the extends and implements clauses.
            //
            for (unsigned j = 0; j < partially_ordered_types.Length(); j++)
            {
                TypeSymbol* type = partially_ordered_types[j];
                needs_member_work.AddElement(type);
                type -> ProcessTypeHeaders();
                type -> semantic_environment -> sem ->
                    types_to_be_processed.AddElement(type);
            }
        } while (start < semantic.Length());

        //
        // Partially order the collection of types in needs_member_work and
        // place the result in partially_ordered_types. This reordering is
        // based on the complete "supertype" information computed in
        // ProcessTypeHeaders.
        //
        topological_sorter.Sort();
        for (unsigned i = 0; i < partially_ordered_types.Length(); i++)
        {
            TypeSymbol* type = partially_ordered_types[i];
            needs_body_work.Next() = type;
            type -> ProcessMembers();
        }
    }

    semantic.Reset();
}


void Control::CollectTypes(TypeSymbol* type, Tuple<TypeSymbol*>& types)
{
    types.Next() = type;

    for (unsigned j = 0; j < type -> NumAnonymousTypes(); j++)
        CollectTypes(type -> AnonymousType(j), types);

    if (type -> local)
    {
        for (TypeSymbol* local_type = (TypeSymbol*) type -> local -> FirstElement();
             local_type;
             local_type = (TypeSymbol*) type -> local -> NextElement())
        {
            CollectTypes(local_type, types);
        }
    }

    if (type -> non_local)
    {
        for (TypeSymbol* non_local_type = (TypeSymbol*) type -> non_local -> FirstElement();
             non_local_type;
             non_local_type = (TypeSymbol*) type -> non_local -> NextElement())
        {
            CollectTypes(non_local_type, types);
        }
    }
}


void Control::ProcessBodies(TypeSymbol* type)
{
    Semantic* sem = type -> semantic_environment -> sem;

    if (type -> declaration &&
        ! sem -> compilation_unit -> BadCompilationUnitCast())
    {

        if (type -> declaration -> UnparsedClassBodyCast())
        {
            if (! parser -> InitializerParse(sem -> lex_stream,
                                             type -> declaration))
            {
                // Mark that syntax errors were detected.
                sem -> compilation_unit -> MarkBad();
            }
            else
            {
                type -> CompleteSymbolTable();
                if (! parser -> BodyParse(sem -> lex_stream, type -> declaration))
                {
                    // Mark that syntax errors were detected.
                    sem -> compilation_unit -> MarkBad();
                }
                else type -> ProcessExecutableBodies();
            }
        }
        else
        {
            // Already parsed (e.g., anonymous class)
            type -> CompleteSymbolTable();
            type -> ProcessExecutableBodies();
        }

        if (sem -> NumErrors() == 0 &&
            sem -> lex_stream -> NumBadTokens() == 0 &&
            ! sem -> compilation_unit -> BadCompilationUnitCast())
        {
            Tuple<TypeSymbol*>* types = new Tuple<TypeSymbol*>(1024);
            CollectTypes(type, *types);

            //
            // If we are supposed to generate code, do so now !!!
            //
            if (option.bytecode)
            {
                for (unsigned k = 0; k < types -> Length(); k++)
                {
                    TypeSymbol* type = (*types)[k];
                    // Make sure the literal is available for bytecode.
                    type -> file_symbol -> SetFileNameLiteral(this);
                    ByteCode* code = new ByteCode(type);
                    code -> GenerateCode();
                    delete code;
                }
            }

            //
            // If no error was detected while generating code, then
            // start cleaning up.
            //
            if (! option.nocleanup)
            {
                if (sem -> NumErrors() == 0)
                {
                    for (unsigned k = 0; k < types -> Length(); k++)
                    {
                        TypeSymbol* type = (*types)[k];
                        delete type -> semantic_environment;
                        type -> semantic_environment = NULL;
                        type -> declaration -> semantic_environment = NULL;
                    }
                }
                delete types;
            }
        }
    }

    sem -> types_to_be_processed.RemoveElement(type);

    if (sem -> types_to_be_processed.Size() == 0)
    {
        // All types belonging to this compilation unit have been processed.
        CheckForUnusedImports(sem);
        if (! option.nocleanup)
        {
            CleanUp(sem -> source_file_symbol);
        }
    }
}

void Control::CheckForUnusedImports(Semantic* sem)
{
    if (sem -> NumErrors() != 0 ||
        sem -> lex_stream -> NumBadTokens() != 0 ||
        sem -> compilation_unit -> BadCompilationUnitCast())
    {
        //
        // It's not worth checking for unused imports if compilation
        // wasn't successful; we may well have just not got round to
        // compiling the relevant code, and if there were errors, the
        // user has more important things to worry about than unused
        // imports!
        //
        return;
    }

    for (unsigned i = 0;
         i < sem -> compilation_unit -> NumImportDeclarations(); ++i)
    {
        AstImportDeclaration* import_declaration =
            sem -> compilation_unit -> ImportDeclaration(i);

        // Java 5: Skip static imports for now (not fully implemented yet)
        if (import_declaration -> static_token_opt)
            continue;

        Symbol* symbol = import_declaration -> name -> symbol;

        // Safety check: symbol might be NULL if import processing failed
        if (!symbol)
            continue;

        if (import_declaration -> star_token_opt)
        {
            PackageSymbol* package = symbol -> PackageCast();
            if (package &&
                ! sem -> referenced_package_imports.IsElement(package))
            {
                sem -> ReportSemError(SemanticError::UNUSED_PACKAGE_IMPORT,
                                      import_declaration,
                                      package -> PackageName());
            }
        }
        else
        {
            TypeSymbol* import_type = symbol -> TypeCast();
            if (import_type &&
                ! sem -> referenced_type_imports.IsElement(import_type))
            {
                sem -> ReportSemError(SemanticError::UNUSED_TYPE_IMPORT,
                                      import_declaration,
                                      import_type -> ContainingPackage() -> PackageName(),
                                      import_type -> ExternalName());
            }
        }
    }
}

//
// Introduce the main package and the current package.
// This procedure is invoked directly only while doing
// an incremental compilation.
//
void Control::ProcessPackageDeclaration(FileSymbol* file_symbol,
                                        AstPackageDeclaration* package_declaration)
{
    file_symbol -> package = (package_declaration
                              ? FindOrInsertPackage(file_symbol -> lex_stream,
                                                    package_declaration -> name)
                              : unnamed_package);

    for (unsigned i = 0; i < file_symbol -> lex_stream -> NumTypes(); i++)
    {
        TokenIndex identifier_token = file_symbol -> lex_stream ->
            Next(file_symbol -> lex_stream -> Type(i));
        if (file_symbol -> lex_stream -> Kind(identifier_token) ==
            TK_Identifier)
        {
            NameSymbol* name_symbol =
                file_symbol -> lex_stream -> NameSymbol(identifier_token);
            if (! file_symbol -> package -> FindTypeSymbol(name_symbol))
            {
                TypeSymbol* type = file_symbol -> package ->
                    InsertOuterTypeSymbol(name_symbol);
                type -> file_symbol = file_symbol;
                type -> outermost_type = type;
                type -> supertypes_closure = new SymbolSet;
                type -> subtypes = new SymbolSet;
                type -> SetOwner(file_symbol -> package);
                type -> SetSignature(*this);
                type -> MarkSourcePending();

                //
                // If this type is contained in the unnamed package add it to
                // the set unnamed_package_types if a type of similar name was
                // not already there.
                //
                if (! package_declaration &&
                    unnamed_package_types.Image(type -> Identity()) == NULL)
                {
                    unnamed_package_types.AddElement(type);
                }
            }
        }
    }
}


void Control::CleanUp(FileSymbol* file_symbol)
{
    Semantic* sem = file_symbol -> semantic;

    if (sem)
    {
#ifdef JOPA_DEBUG
        if (option.debug_dump_lex)
        {
            sem -> lex_stream -> Reset(); // rewind input and ...
            sem -> lex_stream -> Dump();  // dump it!
        }
        if (option.debug_dump_ast)
            sem -> compilation_unit -> Print(*sem -> lex_stream);
        if (option.debug_unparse_ast)
        {
            if (option.debug_unparse_ast_debug)
              {
                // which of these is correct?
                sem -> compilation_unit -> debug_unparse = true;
                Ast::debug_unparse = true;
              }
            sem -> compilation_unit -> Unparse(sem -> lex_stream,
                                               "unparsed/");
        }
#endif // JOPA_DEBUG
        sem -> PrintMessages();
        if (sem -> return_code > 0)
            return_code = 1;

        file_symbol -> CleanUp();
    }
}



} // Close namespace Jopa block

