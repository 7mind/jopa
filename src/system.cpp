#include "platform.h"
#include "control.h"
#include "semantic.h"
#include "zip.h"
#include "option.h"
#include "case.h"

#ifdef HAVE_JIKES_NAMESPACE
namespace Jikes { // Open namespace Jikes block
#endif

//
// Convert the null terminated Unicode string source into its Utf8
// representation pointed to by target. The char string target is presumed
// to have been allocated and to be large enough to accomodate the conversion.
//
int Control::ConvertUnicodeToUtf8(const wchar_t* source, char* target)
{
    int length = 0;

    for ( ; *source; source++)
    {
        int ch = *source;

        if (ch == 0)
        {
             target[length++] = (char) 0xC0;
             target[length++] = (char) 0x80;
        }
        else if (ch <= 0x007F)
             target[length++] = (char) ch;
        else if (ch <= 0x07FF)
        {

            target[length++] = (char) (0xC0 | ((ch >> 6) & 0x1F));
            target[length++] = (char) (0x80 | (ch & 0x3F));
        }
        else
        {
            target[length++] = (char) (0xE0 | ((ch >> 12) & 0x0F));
            target[length++] = (char) (0x80 | ((ch >> 6) & 0x3F));
            target[length++] = (char) (0x80 | (ch & 0x3F));
        }
    }
    target[length] = U_NULL;
    return length;
}


//
// Turn a C string literal into a NameSymbol. Because it works even on
// non-ASCII systems (where 'a' != U_a), it is slightly inefficient. Hence it
// is private and only called from caching accessor methods.
//
NameSymbol* Control::FindOrInsertSystemName(const char* name)
{
    int len = strlen(name);
    wchar_t* wname = new wchar_t[len + 1];
    for (int i = 0; i < len; i++)
    {
        // Only list the characters we expect in system names.
        switch (name[i])
        {
        case 'a': wname[i] = U_a; break;
        case 'b': wname[i] = U_b; break;
        case 'c': wname[i] = U_c; break;
        case 'd': wname[i] = U_d; break;
        case 'e': wname[i] = U_e; break;
        case 'f': wname[i] = U_f; break;
        case 'g': wname[i] = U_g; break;
        case 'h': wname[i] = U_h; break;
        case 'i': wname[i] = U_i; break;
        case 'j': wname[i] = U_j; break;
        case 'k': wname[i] = U_k; break;
        case 'l': wname[i] = U_l; break;
        case 'm': wname[i] = U_m; break;
        case 'n': wname[i] = U_n; break;
        case 'o': wname[i] = U_o; break;
        case 'p': wname[i] = U_p; break;
        case 'q': wname[i] = U_q; break;
        case 'r': wname[i] = U_r; break;
        case 's': wname[i] = U_s; break;
        case 't': wname[i] = U_t; break;
        case 'u': wname[i] = U_u; break;
        case 'v': wname[i] = U_v; break;
        case 'w': wname[i] = U_w; break;
        case 'x': wname[i] = U_x; break;
        case 'y': wname[i] = U_y; break;
        case 'z': wname[i] = U_z; break;
        case 'A': wname[i] = U_A; break;
        case 'B': wname[i] = U_B; break;
        case 'C': wname[i] = U_C; break;
        case 'D': wname[i] = U_D; break;
        case 'E': wname[i] = U_E; break;
        case 'F': wname[i] = U_F; break;
        case 'G': wname[i] = U_G; break;
        case 'H': wname[i] = U_H; break;
        case 'I': wname[i] = U_I; break;
        case 'J': wname[i] = U_J; break;
        case 'K': wname[i] = U_K; break;
        case 'L': wname[i] = U_L; break;
        case 'M': wname[i] = U_M; break;
        case 'N': wname[i] = U_N; break;
        case 'O': wname[i] = U_O; break;
        case 'P': wname[i] = U_P; break;
        case 'Q': wname[i] = U_Q; break;
        case 'R': wname[i] = U_R; break;
        case 'S': wname[i] = U_S; break;
        case 'T': wname[i] = U_T; break;
        case 'U': wname[i] = U_U; break;
        case 'V': wname[i] = U_V; break;
        case 'W': wname[i] = U_W; break;
        case 'X': wname[i] = U_X; break;
        case 'Y': wname[i] = U_Y; break;
        case 'Z': wname[i] = U_Z; break;
        case '0': wname[i] = U_0; break;
        case '1': wname[i] = U_1; break;
        case '2': wname[i] = U_2; break;
        case '3': wname[i] = U_3; break;
        case '4': wname[i] = U_4; break;
        case '5': wname[i] = U_5; break;
        case '6': wname[i] = U_6; break;
        case '7': wname[i] = U_7; break;
        case '8': wname[i] = U_8; break;
        case '9': wname[i] = U_9; break;
        case '_': wname[i] = U_UN; break;
        case '$': wname[i] = U_DS; break;
        case '(': wname[i] = U_LP; break;
        case ')': wname[i] = U_RP; break;
        case ';': wname[i] = U_SC; break;
        case '<': wname[i] = U_LT; break;
        case '>': wname[i] = U_GT; break;
        case '/': wname[i] = U_SL; break;
        case '[': wname[i] = U_LB; break;
        case ']': wname[i] = U_RB; break;
        case '-': wname[i] = U_MI; break;
        case '.': wname[i] = U_DO; break;
        case '?': wname[i] = U_QU; break;
        default: assert(false && "bad character in system name");
        }
    }
    wname[len] = U_NULL;
    NameSymbol* name_symbol = name_table.FindOrInsertName(wname, len);
    delete [] wname;
    if (! name_symbol -> Utf8_literal)
        name_symbol -> Utf8_literal =
            ConvertUnicodeToUtf8(name_symbol -> Name());
    return name_symbol;
}


//
// Convert the Utf8 string of length len pointed to by source into its unicode
// representation pointed to by target. The wchar_t string target is presumed
// to have been allocated and to be large enough (at least len + 1) to
// accomodate the conversion.
//
int Control::ConvertUtf8ToUnicode(wchar_t* target, const char* source, int len)
{
    wchar_t* ptr = target;
    for (int i = 0; i < len; i++, ptr++)
    {
        u1 ch = source[i];

        if ((ch & 0x80) == 0)
            *ptr = ch;
        else if ((ch & 0xE0) == 0xC0)
        {
            *ptr = ch & 0x1F;
            *ptr <<= 6;
            i++;
            ch = source[i] & 0x3F;
            *ptr += ch;
        }
        else if ((ch & 0xF0) == 0xE0)
        {
            *ptr = ch & 0x0F;
            *ptr <<= 6;
            i++;
            ch = source[i] & 0x3F;
            *ptr += ch;

            *ptr <<= 6;
            i++;
            ch = source[i] & 0x3F;
            *ptr += ch;
        }
        else assert(false && "invalid character encoding");
    }
    *ptr = U_NULL;
    return ptr - target;
}


void Control::FindPathsToDirectory(PackageSymbol* package)
{
    if (package -> directory.Length() == 0)
    {
        PackageSymbol* owner_package = package -> owner;
        if (owner_package) // package is a subpackage?
        {
            for (unsigned i = 0; i < owner_package -> directory.Length(); i++)
            {
                DirectorySymbol* owner_directory_symbol =
                    owner_package -> directory[i];
                DirectorySymbol* subdirectory_symbol =
                    owner_directory_symbol -> FindDirectorySymbol(package -> Identity());
                if (! owner_directory_symbol -> IsZip())
                {
                    if (! subdirectory_symbol)
                    {
                        int length =
                            owner_directory_symbol -> DirectoryNameLength() +
                            package -> Utf8NameLength() + 1; // +1 for '/'
                        char* directory_name = new char[length + 1];

                        strcpy(directory_name,
                               owner_directory_symbol -> DirectoryName());
                        if (owner_directory_symbol -> DirectoryName()[owner_directory_symbol -> DirectoryNameLength() - 1] != U_SLASH)
                            strcat(directory_name, StringConstant::U8S_SL);
                        strcat(directory_name, package -> Utf8Name());

                        if (SystemIsDirectory(directory_name))
                            subdirectory_symbol = owner_directory_symbol ->
                                InsertDirectorySymbol(package -> Identity(),
                                                      owner_directory_symbol -> IsSourceDirectory());

                        delete [] directory_name;
                    }

                    if (subdirectory_symbol)
                        subdirectory_symbol -> ReadDirectory();
                }

                if (subdirectory_symbol)
                    package -> directory.Next() = subdirectory_symbol;
            }
        }
        else
        {
            //
            // Recall that since classpath[0] contains the default directory,
            // we always start searching at location 1.
            //
            for (unsigned k = 1; k < classpath.Length(); k++)
            {
                PathSymbol* path_symbol = classpath[k];
                DirectorySymbol* directory_symbol =
                    path_symbol -> RootDirectory() -> FindDirectorySymbol(package -> Identity());
                if (! path_symbol -> IsZip())
                {
                    if (! directory_symbol)
                    {
                        int length = path_symbol -> Utf8NameLength() +
                            package -> Utf8NameLength() + 1; // +1 for '/'
                        char* directory_name = new char[length + 1];
                        strcpy(directory_name, path_symbol -> Utf8Name());
                        char tail = path_symbol -> Utf8Name()[path_symbol -> Utf8NameLength() - 1];
                        if (tail != U_SLASH && tail != U_BACKSLASH)
                            strcat(directory_name, StringConstant::U8S_SL);
                        strcat(directory_name, package -> Utf8Name());

                        if (SystemIsDirectory(directory_name))
                            directory_symbol = path_symbol -> RootDirectory() ->
                                InsertDirectorySymbol(package -> Identity(),
                                                      path_symbol -> RootDirectory() -> IsSourceDirectory());
                        delete [] directory_name;
                    }

                    if (directory_symbol)
                        directory_symbol -> ReadDirectory();
                }

                if (directory_symbol)
                    package -> directory.Next() = directory_symbol;
            }
        }
    }
}


void Control::ProcessGlobals()
{
    // Some names are conditional on 1.5 VMs, which expanded the set of legal
    // VM names to include non-Java identifiers.
    access_name_symbol =
        FindOrInsertSystemName(option.source < JikesOption::SDK1_5
                               ? "access$" : "-");
    array_name_symbol = FindOrInsertSystemName("array");
    assert_name_symbol = FindOrInsertSystemName("assert");
    block_init_name_symbol = FindOrInsertSystemName("this");
    class_name_symbol = FindOrInsertSystemName("class");
    clinit_name_symbol = FindOrInsertSystemName("<clinit>");
    clone_name_symbol = FindOrInsertSystemName("clone");
    dot_name_symbol = FindOrInsertSystemName(".");
    dot_dot_name_symbol = FindOrInsertSystemName("..");
    Enum_name_symbol = FindOrInsertSystemName("Enum");
    equals_name_symbol = FindOrInsertSystemName("equals");
    false_name_symbol = FindOrInsertSystemName("false");
    hashCode_name_symbol = FindOrInsertSystemName("hashCode");
    init_name_symbol = FindOrInsertSystemName("<init>");
    length_name_symbol = FindOrInsertSystemName("length");
    null_name_symbol = FindOrInsertSystemName("null");
    Object_name_symbol = FindOrInsertSystemName("Object");
    package_info_name_symbol = FindOrInsertSystemName("package-info");
    question_name_symbol = FindOrInsertSystemName("??");
    serialPersistentFields_name_symbol =
        FindOrInsertSystemName("serialPersistentFields");
    serialVersionUID_name_symbol = FindOrInsertSystemName("serialVersionUID");
    this_name_symbol = FindOrInsertSystemName("this");
    true_name_symbol = FindOrInsertSystemName("true");
    val_name_symbol =
        FindOrInsertSystemName(option.source < JikesOption::SDK1_5
                               ? "val$" : "-");

    ConstantValue_literal = Utf8_pool.FindOrInsert(U8S_ConstantValue,
                                                   strlen(U8S_ConstantValue));
    Exceptions_literal = Utf8_pool.FindOrInsert(U8S_Exceptions,
                                                strlen(U8S_Exceptions));
    InnerClasses_literal = Utf8_pool.FindOrInsert(U8S_InnerClasses,
                                                  strlen(U8S_InnerClasses));
    Synthetic_literal = Utf8_pool.FindOrInsert(U8S_Synthetic,
                                               strlen(U8S_Synthetic));
    Deprecated_literal = Utf8_pool.FindOrInsert(U8S_Deprecated,
                                                strlen(U8S_Deprecated));
    LineNumberTable_literal =
        Utf8_pool.FindOrInsert(U8S_LineNumberTable,
                               strlen(U8S_LineNumberTable));
    LocalVariableTable_literal =
        Utf8_pool.FindOrInsert(U8S_LocalVariableTable,
                               strlen(U8S_LocalVariableTable));
    Code_literal = Utf8_pool.FindOrInsert(U8S_Code, strlen(U8S_Code));
    SourceFile_literal = Utf8_pool.FindOrInsert(U8S_SourceFile,
                                                strlen(U8S_SourceFile));
    EnclosingMethod_literal =
        Utf8_pool.FindOrInsert(U8S_EnclosingMethod,
                               strlen(U8S_EnclosingMethod));
    Signature_literal = Utf8_pool.FindOrInsert(U8S_Signature,
                                               strlen(U8S_Signature));
}


//
// Create the unnamed package and set up global names.
//
void Control::ProcessUnnamedPackage()
{
    unnamed_package = external_table
        .InsertPackageSymbol(FindOrInsertName(US_EMPTY, 0), NULL);

    //
    // Create an entry for no_type. no_type is used primarily to signal an
    // error.
    //
    no_type = unnamed_package -> InsertSystemTypeSymbol(question_name_symbol);
    no_type -> SetSignature(dot_name_symbol -> Utf8_literal);
    no_type -> outermost_type = no_type;
    no_type -> SetOwner(unnamed_package);
    no_type -> subtypes = new SymbolSet();
    no_type -> MarkBad();

    //
    // Create an entry for the null type.
    //
    null_type = unnamed_package -> InsertSystemTypeSymbol(null_name_symbol);
    null_type -> outermost_type = null_type;
    null_type -> SetOwner(unnamed_package);
    null_type -> SetACC_PUBLIC();
}


void Control::ProcessPath()
{
#ifdef UNIX_FILE_SYSTEM
    NameSymbol* dot_path_name_symbol = dot_name_symbol;

    //
    // We need a place to start. Allocate a "." directory with no owner
    // initially. (Hence, the null argument.) Allocate a "." path whose
    // associated directory is the "." directory. Identify the "." path as
    // the owner of the "." directory. It's not a sourcepath, so pass false.
    //
    DirectorySymbol* default_directory = new DirectorySymbol(dot_name_symbol,
                                                             NULL, false);
    // Since the "." directory may not be the first directory, set
    // dot_classpath_index to the first instance in classpath.
    dot_classpath_index = classpath.Length();
    classpath.Next() = classpath_table.InsertPathSymbol(dot_path_name_symbol,
                                                        default_directory);
    // Note that the default_directory is reset after it has been assigned
    // the owner above.
    default_directory -> ReadDirectory();
    system_directories.Next() = default_directory;

    system_table = new SystemTable();
    struct stat status;
    //FIXME: need to check for stat errors
    if (SystemStat(dot_name_symbol -> Utf8Name(), &status) == 0 &&
        (status.st_mode & JIKES_STAT_S_IFDIR))
        system_table -> InsertDirectorySymbol(status.st_dev, status.st_ino,
                                              default_directory);

#elif defined(WIN32_FILE_SYSTEM)

    char* main_current_directory = option.GetMainCurrentDirectory();
    int dot_path_name_length = strlen(main_current_directory);
    wchar_t* dot_path_name = new wchar_t[dot_path_name_length + 1];
    for (int i = 0; i < dot_path_name_length; i++)
        dot_path_name[i] = main_current_directory[i];
    dot_path_name[dot_path_name_length] = U_NULL;
    NameSymbol* dot_path_name_symbol = FindOrInsertName(dot_path_name,
                                                        dot_path_name_length);
    delete [] dot_path_name;

    //
    // We need a place to start. Allocate a "." directory with no owner
    // initially. (Hence, the null argument.) Allocate a "." path whose
    // associated directory is the "." directory. Identify the "." path as
    // the owner of the "." directory. It's not a sourcepath, so pass false.
    //
    DirectorySymbol* default_directory = new DirectorySymbol(dot_name_symbol,
                                                             NULL, false);
    // Since the "." directory may not be the first directory, set
    // dot_classpath_index to the first instance in classpath.
    dot_classpath_index = classpath.Length();
    classpath.Next() = classpath_table.InsertPathSymbol(dot_path_name_symbol,
                                                        default_directory);
    // Note that the default_directory is reset after it has been assigned
    // the owner above.
    default_directory -> ReadDirectory();
    system_directories.Next() = default_directory;
#endif // WIN32_FILE_SYSTEM
    //
    //
    //

    ProcessBootClassPath();
    ProcessExtDirs();
    ProcessClassPath();
    ProcessSourcePath();

    //
    // TODO: If the user did not specify "." in the class path we assume it.
    // javac makes that assumption also. Is that correct?
    //
    if (dot_classpath_index == 0)
        unnamed_package -> directory.Next() = classpath[0] -> RootDirectory();
}

void Control::ProcessBootClassPath()
{
    if (option.bootclasspath)
    {
        // The longest possible path name we can encounter
        int max_path_name_length = strlen(option.bootclasspath) + 1;
        wchar_t* path_name = new wchar_t[max_path_name_length + 1];

        wchar_t* input_name = NULL;
#ifdef WIN32_FILE_SYSTEM
        char* full_directory_name = NULL;
#endif

        for (char* path = option.bootclasspath,
                 * path_tail = &path[strlen(path)];
             path < path_tail; path++)
        {
#ifdef WIN32_FILE_SYSTEM
            delete [] full_directory_name;
            delete [] input_name;
#endif
            char* head;
            for (head = path; path < path_tail && *path != PathSeparator();
                 path++);

            // If a separator was encountered, replace it by \0 to terminate
            // the string.
            *path = U_NULL;
            int input_name_length = path - head;

            int path_name_length = input_name_length;
            for (int i = 0; i < path_name_length; i++)
                path_name[i] = head[i];
            path_name[path_name_length] = U_NULL;

#ifdef UNIX_FILE_SYSTEM

            input_name = path_name;

#elif defined(WIN32_FILE_SYSTEM)

            input_name = NULL;
            full_directory_name = NULL;
            char disk = (input_name_length >= 2 &&
                         Case::IsAsciiAlpha(head[0]) &&
                         head[1] == U_COLON ? head[0] : 0);

            //
            // Look for the directory. If it is found, update input_name and
            // head.
            //
            option.SaveCurrentDirectoryOnDisk(disk);
            if (SetCurrentDirectory(head))
            {
                char tmp[1];
                // First, get the right size.
                DWORD directory_length = GetCurrentDirectory(0, tmp);
                full_directory_name = new char[directory_length + 1];
                DWORD length = GetCurrentDirectory(directory_length,
                                                   full_directory_name);
                if (length <= directory_length)
                {
                    for (char* ptr = full_directory_name; *ptr; ptr++)
                    {
                        *ptr = (*ptr != U_BACKSLASH
                                ? *ptr : (char) U_SLASH); // turn '\' to '/'.
                    }

                    input_name_length = length;
                    input_name = new wchar_t[input_name_length + 1];
                    for (int k = 0; k < input_name_length; k++)
                        input_name[k] = full_directory_name[k];
                    input_name[input_name_length] = U_NULL;
                    head = full_directory_name;
                }
            }

            //
            // Default input_name, in case we do not succeed in finding the
            // directory.
            //
            if (! input_name)
            {
                input_name = new wchar_t[input_name_length + 1];
                for (int j = 0; j < input_name_length; j++)
                    input_name[j] = path_name[j];
                input_name[input_name_length] = U_NULL;
            }

            // Reset the current directory on disk.
            option.ResetCurrentDirectoryOnDisk(disk);
            // Reset the real current directory...
            option.SetMainCurrentDirectory();
#endif // WIN32_FILE_SYSTEM

            if (input_name_length > 0)
            {
                NameSymbol* name_symbol = FindOrInsertName(path_name,
                                                           path_name_length);

                //
                // If a directory is specified more than once, ignore the
                // duplicates.
                //
                if (classpath_table.FindPathSymbol(name_symbol))
                {
                    if (name_symbol == dot_name_symbol)
                    {
                        // The next index
                        dot_classpath_index = classpath.Length();
                        // Share the "." directory.
                        classpath.Next() = classpath[0];
                        unnamed_package -> directory.Next() =
                            classpath[0] -> RootDirectory();
                    }

                    continue;
                }

                //
                // Check whether or not the path points to a system directory.
                // If not, assume it's a zip file.
                //
                if (SystemIsDirectory(head))
                {
                    // This is the bootclasspath so it's not sourcepath, pass
                    // false
                    DirectorySymbol* dot_directory =
                        ProcessSubdirectories(input_name, input_name_length,
                                              false);
                    unnamed_package -> directory.Next() = dot_directory;
                    classpath.Next() =
                        classpath_table.InsertPathSymbol(name_symbol,
                                                         dot_directory);
                }
                else
                {
                    errno = 0;
                    Zip* zipinfo = new Zip(*this, head);
                    if (! zipinfo -> IsValid())
                    {
                        // If the zipfile is all screwed up, give up here !!!
                        wchar_t* name = new wchar_t[input_name_length + 1];
                        for (int i = 0; i < input_name_length; i++)
                            name[i] = input_name[i];
                        name[input_name_length] = U_NULL;
                        if (errno)
                        {
                            const char* std_err = strerror(errno);
                            ErrorString err_str;
                            err_str << '"' << std_err << '"'
                                    << " while trying to open " << name;
                            general_io_warnings.Next() = err_str.SafeArray();
                        }
                        else
                        {
                            wchar_t* tail = &name[input_name_length - 3];
                            if (Case::StringSegmentEqual(tail, US_zip, 3) ||
                               Case::StringSegmentEqual(tail, US_jar, 3))
                            {
                                bad_zip_filenames.Next() = name;
                            }
                            else bad_dirnames.Next() = name;
                        }
                    }

                    unnamed_package -> directory.Next() =
                        zipinfo -> RootDirectory();

                    //
                    // Create the new path symbol and update the class path
                    // with it.
                    //
                    PathSymbol* path_symbol =
                        classpath_table.InsertPathSymbol(name_symbol,
                                                         zipinfo -> RootDirectory());
                    path_symbol -> zipfile = zipinfo;
                    classpath.Next() = path_symbol;
                }
            }
        }

#ifdef WIN32_FILE_SYSTEM
        delete [] full_directory_name;
        delete [] input_name;
#endif // WIN32_FILE_SYSTEM
        delete [] path_name;
    }
}

void Control::ProcessExtDirs()
{
    SymbolSet extdirs_set;
    if (option.extdirs)
    {
        // The longest possible path name we can encounter
        int max_path_name_length = strlen(option.extdirs) + 1;
        wchar_t* path_name = new wchar_t[max_path_name_length + 1];

        wchar_t* input_name = NULL;
#ifdef WIN32_FILE_SYSTEM
        char* full_directory_name = NULL;
#endif

        for (char* path = option.extdirs, *path_tail = &path[strlen(path)];
             path < path_tail; path++)
        {
#ifdef WIN32_FILE_SYSTEM
            delete [] full_directory_name;
            delete [] input_name;
#endif
            char* head;
            for (head = path; path < path_tail && *path != PathSeparator();
                 path++);

            // If a separator was encountered, replace it by \0 to terminate
            // the string.
            *path = U_NULL;

            int input_name_length = path - head;

            int path_name_length = input_name_length;
            for (int i = 0; i < path_name_length; i++)
                path_name[i] = head[i];
            path_name[path_name_length] = U_NULL;

#ifdef UNIX_FILE_SYSTEM

            input_name = path_name;

#elif defined(WIN32_FILE_SYSTEM)

            input_name = NULL;
            full_directory_name = NULL;
            char disk = (input_name_length >= 2 &&
                         Case::IsAsciiAlpha(head[0]) &&
                         head[1] == U_COLON ? head[0] : 0);

            //
            // Look for the directory. If it is found, update input_name and
            // head.
            //
            option.SaveCurrentDirectoryOnDisk(disk);
            if (SetCurrentDirectory(head))
            {
                char tmp[1];
                // First, get the right size.
                DWORD directory_length = GetCurrentDirectory(0, tmp);
                full_directory_name = new char[directory_length + 1];
                DWORD length = GetCurrentDirectory(directory_length,
                                                   full_directory_name);
                if (length <= directory_length)
                {
                    for (char* ptr = full_directory_name; *ptr; ptr++)
                        *ptr = (*ptr != U_BACKSLASH
                                ? *ptr : (char) U_SLASH); // turn '\' to '/'.

                    input_name_length = length;
                    input_name = new wchar_t[input_name_length + 1];
                    for (int k = 0; k < input_name_length; k++)
                        input_name[k] = full_directory_name[k];
                    input_name[input_name_length] = U_NULL;
                    head = full_directory_name;
                }
            }

            //
            // Default input_name, in case we do not succeed in finding the
            // directory.
            //
            if (! input_name)
            {
                input_name = new wchar_t[input_name_length + 1];
                for (int j = 0; j < input_name_length; j++)
                    input_name[j] = path_name[j];
                input_name[input_name_length] = U_NULL;
            }

            // Reset the current directory on disk.
            option.ResetCurrentDirectoryOnDisk(disk);
            // Reset the real current directory...
            option.SetMainCurrentDirectory();
#endif // WIN32_FILE_SYSTEM

            if (input_name_length > 0)
            {
                NameSymbol* name_symbol = FindOrInsertName(path_name,
                                                           path_name_length);

                //
                // If a directory is specified more than once, ignore the
                // duplicates.
                //
                if (extdirs_set.IsElement(name_symbol))
                    continue;

                extdirs_set.AddElement(name_symbol);

                //
                // Check whether or not the path points to a system
                // directory. TODO If not, should we print a warning ??
                //
                if (SystemIsDirectory(head))
                {
//FIXME: should be in platform.cpp??
#ifdef UNIX_FILE_SYSTEM

                    DIR* extdir = opendir(head);

                    if (extdir)
                    {
                        for (dirent* entry = readdir(extdir); entry; entry =
                             readdir(extdir))
                        {
                            int entry_length = strlen(entry -> d_name);
                            // + 1 for possible '/' between path and file.
                            int fullpath_length = input_name_length +
                                entry_length + 1;
                            char* ending = &(entry->d_name[entry_length-3]);
                            // skip ., .., non-zip, and non-jar
                            if (! strcmp(entry -> d_name, ".") ||
                                ! strcmp(entry -> d_name, "..") ||
                                (strcasecmp(ending, "zip") &&
                                 strcasecmp(ending, "jar")))
                            {
                                continue;
                            }

                            char* extdir_entry = new char[fullpath_length + 1];
                            // First put on path.
                            strcpy(extdir_entry, head);

                            // Add '/' if it's not already there
                            if (head[input_name_length - 1] != U_SLASH)
                                strcat(extdir_entry, U8S_SL);

                            // Then add the filename.
                            strcat(extdir_entry, entry -> d_name);

                            wchar_t* extdir_entry_name =
                                new wchar_t[fullpath_length + 1];
                            for (int i = 0; i < fullpath_length; ++i)
                                extdir_entry_name[i] = extdir_entry[i];

                            errno = 0;
                            Zip* zipinfo = new Zip(*this, extdir_entry);
                            if (! zipinfo -> IsValid())
                            {
                                wchar_t* name =
                                    new wchar_t[fullpath_length + 1];
                                for (int i = 0; i < fullpath_length; ++i)
                                    name[i] = extdir_entry_name[i];
                                name[fullpath_length] = U_NULL;
                                if (errno)
                                {
                                    const char* std_err = strerror(errno);
                                    ErrorString err_str;
                                    err_str << '"' << std_err << '"'
                                            << " while trying to open "
                                            << name;
                                    general_io_warnings.Next() =
                                        err_str.SafeArray();
                                }
                                else bad_zip_filenames.Next() = name;
                            }

                            unnamed_package->directory.Next() =
                                zipinfo -> RootDirectory();

                            //
                            // Make a new PathSymbol to add to the classpath.
                            //
                            NameSymbol* extdir_entry_symbol =
                                FindOrInsertName(extdir_entry_name,
                                                 fullpath_length);
                            PathSymbol* path_symbol =
                                classpath_table.InsertPathSymbol(extdir_entry_symbol,
                                                                 zipinfo -> RootDirectory());
                            path_symbol -> zipfile = zipinfo;
                            classpath.Next() = path_symbol;
                        }
                        closedir(extdir);
                    }
#elif defined(WIN32_FILE_SYSTEM)

                    // +2 for "/*" +1 for '\0'
                    char* directory_name = new char[input_name_length + 3];
                    strcpy(directory_name, head);
                    if (directory_name[input_name_length - 1] != U_SLASH)
                        directory_name[input_name_length++] = U_SLASH;
                    directory_name[input_name_length++] = U_STAR;
                    directory_name[input_name_length] = U_NULL;

                    WIN32_FIND_DATA entry;
                    HANDLE file_handle = FindFirstFile(directory_name, &entry);
                    if (file_handle != INVALID_HANDLE_VALUE)
                    {
                        do
                        {
                            int entry_length = strlen(entry.cFileName);
                            // + 1 for possible '/' between path and file.
                            int fullpath_length = input_name_length +
                                entry_length + 1;
                            char* ending = &(entry.cFileName[entry_length-3]);
                            // skip ., .., and not zip or jar
                            if ((! strcmp(entry.cFileName, "." )) ||
                                (! strcmp(entry.cFileName, "..")) ||
                                ( strcasecmp(ending, "zip") &&
                                  strcasecmp(ending, "jar")))
                            {
                                continue;
                            }

                            char* extdir_entry = new char[fullpath_length + 1];
                            wchar_t* extdir_entry_name =
                                new wchar_t[fullpath_length + 1];
                            // First put path
                            strcpy(extdir_entry, head);

                            // If no slash, add slash before copying filename.
                            if (head[input_name_length - 1] != U_SLASH)
                            {
                                int path_length = input_name_length + 1;
                                strcat(extdir_entry, U8S_SL);

                                for (int i = 0; i < entry_length; i++)
                                {
                                    extdir_entry[i + path_length] =
                                        entry.cFileName[i] == U_BACKSLASH
                                        ? (char) U_SLASH : entry.cFileName[i];
                                }
                            }
                            else
                            { // If it's there, just append filename.
                                for (int i = 0; i < entry_length; i++)
                                {
                                    extdir_entry[i + input_name_length] =
                                        entry.cFileName[i] == U_BACKSLASH
                                        ? (char) U_SLASH : entry.cFileName[i];
                                }
                            }

                            for (int i = 0; i < fullpath_length; ++i)
                                extdir_entry_name[i] = extdir_entry[i];

                            errno = 0;
                            Zip* zipinfo = new Zip(*this, extdir_entry);
                            if (! zipinfo -> IsValid())
                            {
                                wchar_t* name =
                                    new wchar_t[fullpath_length + 1];
                                for (int i = 0; i < fullpath_length; ++i)
                                    name[i] = extdir_entry_name[i];
                                name[fullpath_length] = U_NULL;
                                if (errno)
                                {
                                    const char* std_err = strerror(errno);
                                    ErrorString err_str;
                                    err_str << '"' << std_err << '"'
                                            << " while trying to open "
                                            << name;
                                    general_io_warnings.Next() =
                                        err_str.SafeArray();
                                }
                                else bad_zip_filenames.Next() = name;
                            }

                            unnamed_package -> directory.Next() =
                                zipinfo -> RootDirectory();

                            NameSymbol* extdir_entry_symbol =
                                FindOrInsertName(extdir_entry_name,
                                                 fullpath_length);
                            //
                            // Make a new PathSymbol to add to the classpath.
                            //
                            PathSymbol* path_symbol =
                                classpath_table.InsertPathSymbol(extdir_entry_symbol,
                                                                 zipinfo -> RootDirectory());
                            path_symbol -> zipfile = zipinfo;
                            classpath.Next() = path_symbol;

                        } while (FindNextFile(file_handle, &entry));
                        FindClose(file_handle);
                    }

                    delete [] directory_name;
#endif // WIN32_FILE_SYSTEM
                }
                else
                {
                    wchar_t* name = new wchar_t[input_name_length + 1];
                    for (int i = 0; i < input_name_length; ++i)
                        name[i] = input_name[i];
                    name[input_name_length] = U_NULL;
                    bad_dirnames.Next() = name;
                }
            }
        }

#ifdef WIN32_FILE_SYSTEM
        delete [] full_directory_name;
        delete [] input_name;
#endif

        delete [] path_name;
    }
}

void Control::ProcessClassPath()
{
    if (option.classpath)
    {
        // The longest possible path name we can encounter.
        int max_path_name_length = strlen(option.classpath) + 1;
        wchar_t* path_name = new wchar_t[max_path_name_length + 1];

        wchar_t* input_name = NULL;
#ifdef WIN32_FILE_SYSTEM
        char* full_directory_name = NULL;
#endif

        for (char* path = option.classpath, *path_tail = &path[strlen(path)];
             path < path_tail; path++)
        {
#ifdef WIN32_FILE_SYSTEM
            delete [] full_directory_name;
            delete [] input_name;
#endif
            char* head;
            for (head = path; path < path_tail && *path != PathSeparator();
                 path++);

            // If a separator was encountered, replace it by \0 to terminate
            // the string.
            *path = U_NULL;
            int input_name_length = path - head;

            int path_name_length = input_name_length;
            for (int i = 0; i < path_name_length; i++)
                path_name[i] = head[i];
            path_name[path_name_length] = U_NULL;

#ifdef UNIX_FILE_SYSTEM

            input_name = path_name;

#elif defined(WIN32_FILE_SYSTEM)

            input_name = NULL;
            full_directory_name = NULL;
            char disk = (input_name_length >= 2 &&
                         Case::IsAsciiAlpha(head[0]) &&
                         head[1] == U_COLON ? head[0] : 0);

            //
            // Look for the directory. If it is found, update input_name and
            // head.
            //
            option.SaveCurrentDirectoryOnDisk(disk);
            if (SetCurrentDirectory(head))
            {
                char tmp[1];
                // First, get the right size.
                DWORD directory_length = GetCurrentDirectory(0, tmp);
                full_directory_name = new char[directory_length + 1];
                DWORD length = GetCurrentDirectory(directory_length,
                                                   full_directory_name);
                if (length <= directory_length)
                {
                    for (char* ptr = full_directory_name; *ptr; ptr++)
                        *ptr = (*ptr != U_BACKSLASH
                                ? *ptr : (char) U_SLASH); // turn '\' to '/'.

                    input_name_length = length;
                    input_name = new wchar_t[input_name_length + 1];
                    for (int k = 0; k < input_name_length; k++)
                        input_name[k] = full_directory_name[k];
                    input_name[input_name_length] = U_NULL;
                    head = full_directory_name;
                }
            }

            //
            // Default input_name, in case we do not succeed in finding the
            // directory.
            //
            if (! input_name)
            {
                input_name = new wchar_t[input_name_length + 1];
                for (int j = 0; j < input_name_length; j++)
                    input_name[j] = path_name[j];
                input_name[input_name_length] = U_NULL;
            }

            // Reset the current directory on disk.
            option.ResetCurrentDirectoryOnDisk(disk);
            // Reset the real current directory...
            option.SetMainCurrentDirectory();
#endif // WIN32_FILE_SYSTEM

            if (input_name_length > 0)
            {
                NameSymbol* name_symbol = FindOrInsertName(path_name,
                                                           path_name_length);

                //
                // If a directory is specified more than once, ignore the
                // duplicates.
                //
                if (classpath_table.FindPathSymbol(name_symbol))
                {
                    if (name_symbol == dot_name_symbol)
                    {
                        // The next index
                        dot_classpath_index = classpath.Length();
                        // Share the "." directory
                        classpath.Next() = classpath[0];
                        unnamed_package -> directory.Next() =
                            classpath[0] -> RootDirectory();
                    }

                    continue;
                }

                //
                // Check whether or not the path points to a system directory.
                // If not, assume it's a zip file.
                //
                if (SystemIsDirectory(head))
                {
                    // This is the classpath so it's not sourcepath, pass false
                    DirectorySymbol* dot_directory =
                        ProcessSubdirectories(input_name, input_name_length,
                                              false);
                    unnamed_package -> directory.Next() = dot_directory;
                    classpath.Next() =
                        classpath_table.InsertPathSymbol(name_symbol,
                                                         dot_directory);
                }
                else
                {
                    errno = 0;
                    Zip* zipinfo = new Zip(*this, head);
                    // If the zipfile is all screwed up, give up here !!!
                    if (! zipinfo -> IsValid())
                    {
                        wchar_t* name = new wchar_t[input_name_length + 1];
                        for (int i = 0; i < input_name_length; i++)
                            name[i] = input_name[i];
                        name[input_name_length] = U_NULL;
                        if (errno)
                        {
                            const char* std_err = strerror(errno);
                            ErrorString err_str;
                            err_str << '"' << std_err << '"'
                                    << " while trying to open " << name;
                            general_io_warnings.Next() = err_str.SafeArray();
                        }
                        else
                        {
                            wchar_t* tail = &name[input_name_length - 3];
                            if (Case::StringSegmentEqual(tail, US_zip, 3) ||
                                Case::StringSegmentEqual(tail, US_jar, 3))
                            {
                                bad_zip_filenames.Next() = name;
                            }
                            else bad_dirnames.Next() = name;
                        }
                    }

                    unnamed_package -> directory.Next() =
                        zipinfo -> RootDirectory();

                    //
                    // Create the new path symbol and update the class path
                    // with it.
                    //
                    PathSymbol* path_symbol =
                        classpath_table.InsertPathSymbol(name_symbol,
                                                         zipinfo -> RootDirectory());
                    path_symbol -> zipfile = zipinfo;
                    classpath.Next() = path_symbol;
                }
            }
        }

#ifdef WIN32_FILE_SYSTEM
        delete [] full_directory_name;
        delete [] input_name;
#endif

        delete [] path_name;
    }
}

void Control::ProcessSourcePath()
{
    if (option.sourcepath)
    {
        // The longest possible path name we can encounter.
        int max_path_name_length = strlen(option.sourcepath) + 1;
        wchar_t* path_name = new wchar_t[max_path_name_length + 1];

        wchar_t* input_name = NULL;
#ifdef WIN32_FILE_SYSTEM
        char* full_directory_name = NULL;
#endif

        for (char* path = option.sourcepath, *path_tail = &path[strlen(path)];
             path < path_tail; path++)
        {
#ifdef WIN32_FILE_SYSTEM
            delete [] full_directory_name;
            delete [] input_name;
#endif
            char* head;
            for (head = path; path < path_tail && *path != PathSeparator(); path++)
                ;
            *path = U_NULL; // If a separator was encountered, replace it by \0 to terminate the string.
            int input_name_length = path - head;

            int path_name_length = input_name_length;
            for (int i = 0; i < path_name_length; i++)
                path_name[i] = head[i];
            path_name[path_name_length] = U_NULL;

#ifdef UNIX_FILE_SYSTEM

            input_name = path_name;

#elif defined(WIN32_FILE_SYSTEM)

            input_name = NULL;
            full_directory_name = NULL;
            char disk = (input_name_length >= 2 && Case::IsAsciiAlpha(head[0]) && head[1] == U_COLON ? head[0] : 0);

            //
            // Look for the directory. If it is found, update input_name and head.
            //
            option.SaveCurrentDirectoryOnDisk(disk);
            if (SetCurrentDirectory(head))
            {
                char tmp[1];
                DWORD directory_length = GetCurrentDirectory(0, tmp); // first, get the right size
                full_directory_name = new char[directory_length + 1];  // allocate the directory
                DWORD length = GetCurrentDirectory(directory_length, full_directory_name);
                if (length <= directory_length)
                {
                    for (char* ptr = full_directory_name; *ptr; ptr++)
                        *ptr = (*ptr != U_BACKSLASH ? *ptr : (char) U_SLASH); // turn '\' to '/'.

                    input_name_length = length;
                    input_name = new wchar_t[input_name_length + 1];
                    for (int k = 0; k < input_name_length; k++)
                        input_name[k] = full_directory_name[k];
                    input_name[input_name_length] = U_NULL;
                    head = full_directory_name;
                }
            }

            //
            // Default input_name, in case we do not succeed in finding the directory
            //
            if (! input_name)
            {
                input_name = new wchar_t[input_name_length + 1];
                for (int j = 0; j < input_name_length; j++)
                    input_name[j] = path_name[j];
                input_name[input_name_length] = U_NULL;
            }

            // reset the current directory on disk
            option.ResetCurrentDirectoryOnDisk(disk);
            // reset the real current directory...
            option.SetMainCurrentDirectory();
#endif // WIN32_FILE_SYSTEM

            //
            //
            //
            if (input_name_length > 0)
            {
                NameSymbol* name_symbol = FindOrInsertName(path_name, path_name_length);

                //
                // If a directory is specified more than once, ignore the duplicates.
                //
                if (classpath_table.FindPathSymbol(name_symbol))
                {
                    if (name_symbol == dot_name_symbol)
                    {
                        dot_classpath_index = classpath.Length(); // next index
                        classpath.Next() = classpath[0]; // share "." directory
                        unnamed_package -> directory.Next() =
                            classpath[0] -> RootDirectory();
                    }
                    continue;
                }

                //
                // Check whether or not the path points to a system directory.
                // If not, assume it's a zip file.
                //
                if (SystemIsDirectory(head))
                {
                    // This is the sourcepath, so pass true
                    DirectorySymbol* dot_directory =
                        ProcessSubdirectories(input_name, input_name_length,
                                              true);
                    unnamed_package -> directory.Next() = dot_directory;
                    classpath.Next() =
                        classpath_table.InsertPathSymbol(name_symbol,
                                                         dot_directory);
                }
                else
                {
                    // We don't process zip files as source directories
                    wchar_t* name = new wchar_t[input_name_length + 1];
                    for (int i = 0; i < input_name_length; i++)
                        name[i] = input_name[i];
                    name[input_name_length] = U_NULL;
                    bad_dirnames.Next() = name;
                }
            }
        }

#ifdef WIN32_FILE_SYSTEM
        delete [] full_directory_name;
        delete [] input_name;
#endif

        delete [] path_name;
    }
}

TypeSymbol* Control::GetPrimitiveType(const char* name, char signature)
{
    NameSymbol* name_symbol = FindOrInsertSystemName(name);
    TypeSymbol* type = unnamed_package -> InsertSystemTypeSymbol(name_symbol);
    char sig[2] = { signature, U_NU };
    type -> SetSignature(Utf8_pool.FindOrInsert(sig, 1));
    type -> outermost_type = type;
    type -> SetOwner(unnamed_package);
    type -> SetACC_PUBLIC();
    type -> MarkPrimitive();
    return type;
}


void Control::ProcessSystemInformation()
{
    //
    // Add entry for system package
    //
    lang_package = ProcessPackage(US_java_SL_lang);

    //
    // Create an entry for each primitive type. Note that the type void is
    // treated as a primitive. We do not set up any subtyping relationships,
    // as that would violate the assumptions made elsewhere.
    //
    void_type = GetPrimitiveType("void", U_V);
    boolean_type = GetPrimitiveType("boolean", U_Z);
    byte_type = GetPrimitiveType("byte", U_B);
    char_type = GetPrimitiveType("char", U_C);
    short_type = GetPrimitiveType("short", U_S);
    int_type = GetPrimitiveType("int", U_I);
    long_type = GetPrimitiveType("long", U_J);
    float_type = GetPrimitiveType("float", U_F);
    double_type = GetPrimitiveType("double", U_D);
}


//
// Find the given system type.
//
TypeSymbol* Control::ProcessSystemType(PackageSymbol* package,
                                       const char* name)
{
    NameSymbol* name_symbol = FindOrInsertSystemName(name);
    TypeSymbol* type = package -> FindTypeSymbol(name_symbol);

    if (! type)
    {
        Control& control = *this;
        FileSymbol* file_symbol = GetFile(control, package, name_symbol);
        type = system_semantic -> ReadType(file_symbol, package,
                                           name_symbol, 0);
    }
    else if (type -> SourcePending())
        ProcessHeaders(type -> file_symbol);
    return type;
}


//
// Find the given system method.
//
MethodSymbol* Control::ProcessSystemMethod(TypeSymbol* type,
                                           const char* name,
                                           const char* descriptor)
{
    NameSymbol* name_symbol = FindOrInsertSystemName(name);
    MethodSymbol* method = NULL;
    if (! type -> Bad())
    {
        for (method = type -> FindMethodSymbol(name_symbol);
             method; method = method -> next_method)
        {
            if (! strcmp(descriptor, method -> SignatureString()))
                break;
        }
    }
    if (! method)
    {
        if (! type -> Bad())
        {
            system_semantic ->
                ReportSemError(SemanticError::NON_STANDARD_LIBRARY_TYPE,
                               BAD_TOKEN, type -> ContainingPackageName(),
                               type -> ExternalName());
            return NULL;
        }
        method = type -> InsertMethodSymbol(name_symbol);
        method -> SetType(no_type);
        method -> SetContainingType(type);
        method -> SetSignature(FindOrInsertSystemName(descriptor) ->
                               Utf8_literal);
    }
    return method;
}


//
// Find the given system field.
//
VariableSymbol* Control::ProcessSystemField(TypeSymbol* type,
                                            const char* name,
                                            const char* descriptor)
{
    NameSymbol* name_symbol = FindOrInsertSystemName(name);
    VariableSymbol* field = NULL;
    if (! type -> Bad())
    {
        field = type -> FindVariableSymbol(name_symbol);
        if (! field -> IsTyped())
            field -> ProcessVariableSignature(system_semantic, BAD_TOKEN);
        field -> MarkInitialized();
    }
    if (! field)
    {
        if (! type -> Bad())
        {
            system_semantic ->
                ReportSemError(SemanticError::NON_STANDARD_LIBRARY_TYPE,
                               BAD_TOKEN, type -> ContainingPackageName(),
                               type -> ExternalName());
            return NULL;
        }
        field = type -> InsertVariableSymbol(name_symbol);
        field -> SetType(no_type);
        field -> SetOwner(type);
        field -> MarkInitialized();
        Utf8LiteralValue* utf8 =
            FindOrInsertSystemName(descriptor) -> Utf8_literal;
        field -> SetSignatureString(utf8 -> value, utf8 -> length);
    }
    return field;
}


DirectorySymbol* Control::GetOutputDirectory(FileSymbol* file_symbol)
{
    DirectorySymbol* directory_symbol;

    // A FileSymbol for a .class file has a NULL semantic.
    if (file_symbol -> semantic == NULL ||
        (file_symbol -> semantic -> control).option.directory == NULL) {
        directory_symbol = file_symbol -> directory_symbol;
    }
    else
    {
        Control& control = file_symbol -> semantic -> control;
        char* directory_prefix = control.option.directory;
        int directory_prefix_length = strlen(directory_prefix);
        int utf8_name_length =
            file_symbol -> package -> PackageNameLength() * 3;
        // +1 for slash
        int estimated_length = directory_prefix_length + utf8_name_length + 1;

        char* directory_name = new char[estimated_length + 1];

        strcpy(directory_name, directory_prefix);

        if (file_symbol -> package != control.unnamed_package)
        {
            // If there was a package declaration, then...
            if (directory_prefix[directory_prefix_length - 1] != U_SLASH)
                strcat(directory_name, StringConstant::U8S_SL);
            char* utf8_name = new char[utf8_name_length + 1];
            ConvertUnicodeToUtf8(file_symbol -> package -> PackageName(),
                                 utf8_name);
            strcat(directory_name, utf8_name);
            delete [] utf8_name;

            if (! SystemIsDirectory(directory_name))
            {
                // The directory does not yet exist.
                for (char* ptr = &directory_name[directory_prefix_length + 1];
                     *ptr; ptr++)
                {
                    // all the slashes in a package_name are forward slashes
                    if (*ptr == U_SLASH)
                    {
                        *ptr = U_NULL;
                        if (! SystemIsDirectory(directory_name))
                            SystemMkdir(directory_name);
                        *ptr = U_SLASH; // restore slash
                    }
                }
                SystemMkdir(directory_name);
            }
        }

        //
        // Read the directory and insert the directory symbol. Note that since
        // the original length computation was an estimate, we compute the real
        // length here.
        //
        int length = strlen(directory_name);
        wchar_t* name = new wchar_t[length + 1];
        for (int i = 0; i < length; i++)
            name[i] = directory_name[i];
        name[length] = U_NULL;

        // This is the output directory, so unless it's added to the
        // classpath, it won't matter whether it's a sourcedir or not.
        directory_symbol = control.ProcessSubdirectories(name, length, false);

        delete [] name;
        delete [] directory_name;
    }
    return directory_symbol;
}


FileSymbol* Control::GetJavaFile(PackageSymbol* package,
                                 const NameSymbol* name_symbol)
{
    FileSymbol* file_symbol = NULL;

    //
    //
    //
    int length = name_symbol -> Utf8NameLength() +
        FileSymbol::java_suffix_length;
    char* full_filename = new char[length + 1]; // +1 for \0
    strcpy(full_filename, name_symbol -> Utf8Name());
    strcat(full_filename, FileSymbol::java_suffix);

    DirectoryEntry* entry = NULL;
    DirectorySymbol* directory_symbol = NULL;
    for (unsigned k = 0; k < package -> directory.Length(); k++)
    {
        directory_symbol = package -> directory[k];
        if ((entry = directory_symbol -> FindEntry(full_filename, length)))
            break;
    }

    if (entry)
    {
        PathSymbol* path_symbol = directory_symbol -> PathSym();

        file_symbol = directory_symbol -> FindFileSymbol(name_symbol);
        if (! ((file_symbol && file_symbol -> IsJava()) ||
               path_symbol -> IsZip()))
        {
            file_symbol = directory_symbol -> InsertFileSymbol(name_symbol);

            file_symbol -> directory_symbol = directory_symbol;
            file_symbol -> SetJava();
            file_symbol -> mtime = entry -> Mtime();
        }
    }

    delete [] full_filename;
    return file_symbol;
}


FileSymbol* Control::GetFile(Control& control, PackageSymbol* package,
                             const NameSymbol* name_symbol)
{
    return control.option.old_classpath_search_order
        ? GetFileFirst(control, package, name_symbol)
        : GetFileBoth(control, package, name_symbol);
}

FileSymbol* Control::GetFileBoth(Control& control, PackageSymbol* package,
                                 const NameSymbol* name_symbol)
{
    FileSymbol* java_file_symbol = NULL;
    FileSymbol* class_file_symbol = NULL;

    //
    // calculate a length that is large enough...
    //
    int class_length = name_symbol -> Utf8NameLength() +
        FileSymbol::class_suffix_length;
    int java_length = name_symbol -> Utf8NameLength() +
        FileSymbol::java_suffix_length;

    char* class_name = new char[class_length + 1]; // +1 for \0
    strcpy(class_name, name_symbol -> Utf8Name());
    strcat(class_name, FileSymbol::class_suffix);

    char* java_name = new char[java_length + 1]; // +1 for \0
    strcpy(java_name, name_symbol -> Utf8Name());
    strcat(java_name, FileSymbol::java_suffix);

    for (unsigned k = 0; k < package -> directory.Length(); k++)
    {
        DirectorySymbol* directory_symbol = package -> directory[k];
        bool foundBothEntries = false;
        FileSymbol* file_symbol =
            directory_symbol -> FindFileSymbol(name_symbol);
        if (! file_symbol)
        {
            PathSymbol* path_symbol = directory_symbol -> PathSym();
            if (! path_symbol -> IsZip())
            {
                DirectoryEntry* java_entry =
                    directory_symbol -> FindEntry(java_name, java_length),
                *class_entry = (((! control.option.depend ||
                                  java_entry == NULL) &&
                                 (! directory_symbol -> IsSourceDirectory()))
                                ? directory_symbol -> FindEntry(class_name,
                                                                class_length)
                                : (DirectoryEntry*) NULL);

                if (java_entry || class_entry)
                {
                    file_symbol =
                        directory_symbol -> InsertFileSymbol(name_symbol);
                    file_symbol -> directory_symbol = directory_symbol;

                    if (java_entry &&
                        (! class_entry ||
                         class_entry -> Mtime() < java_entry -> Mtime()))
                    {
                        file_symbol -> SetJava();
                        file_symbol -> mtime = java_entry -> Mtime();
                    }
                    else
                    {
                        if (java_entry)
                            file_symbol -> SetClass();
                        else file_symbol -> SetClassOnly();
                        file_symbol -> mtime = class_entry -> Mtime();
                    }
                }

                // Flag case where both .java and class found in same path.
                if (java_entry && class_entry)
                    foundBothEntries = true;
            }
        }

        if (file_symbol)
        {
            // If no .java file seen yet, note this one.
            if (file_symbol -> IsJava() && ! java_file_symbol)
                java_file_symbol = file_symbol;
            // If no .class file seen yet, note this one.
            else if (! class_file_symbol)
                class_file_symbol = file_symbol;


            if (foundBothEntries == true ||
                (java_file_symbol && class_file_symbol))
            {
                // Both .java and .class seen, so no point in continuing the
                // search.
                break;
            }
        }
    }

    delete [] java_name;
    delete [] class_name;

    //
    // If both .java and .class seen, do a mod time check to decide which one
    // to deliver. Otherwise just return whichever kind we found, or NULL.
    //
    if (java_file_symbol &&
        (! class_file_symbol ||
         class_file_symbol -> mtime < java_file_symbol -> mtime))
    {
        return java_file_symbol;
    }
    return class_file_symbol;
}

FileSymbol* Control::GetFileFirst(Control& control, PackageSymbol* package,
                                  const NameSymbol* name_symbol)
{
    FileSymbol* file_symbol = NULL;

    //
    // calculate a length that is large enough...
    //
    int class_length = name_symbol -> Utf8NameLength() +
        FileSymbol::class_suffix_length;
    int java_length = name_symbol -> Utf8NameLength() +
        FileSymbol::java_suffix_length;

    char* class_name = new char[class_length + 1]; // +1 for \0
    strcpy(class_name, name_symbol -> Utf8Name());
    strcat(class_name, FileSymbol::class_suffix);

    char* java_name = new char[java_length + 1]; // +1 for \0
    strcpy(java_name, name_symbol -> Utf8Name());
    strcat(java_name, FileSymbol::java_suffix);

    for (unsigned k = 0; k < package -> directory.Length(); k++)
    {
        DirectorySymbol* directory_symbol = package -> directory[k];
        file_symbol = directory_symbol -> FindFileSymbol(name_symbol);
        if (file_symbol)
             break;

        PathSymbol* path_symbol = directory_symbol -> PathSym();
        if (! path_symbol -> IsZip())
        {
            DirectoryEntry* java_entry =
                directory_symbol -> FindEntry(java_name, java_length);
            DirectoryEntry* class_entry = ((! control.option.depend ||
                                            (java_entry == NULL))
                                           ? directory_symbol -> FindEntry(class_name,
                                                                           class_length)
                                           : (DirectoryEntry*) NULL);

            if (java_entry || class_entry)
            {
                file_symbol =
                    directory_symbol -> InsertFileSymbol(name_symbol);
                file_symbol -> directory_symbol = directory_symbol;

                if (java_entry &&
                    (! class_entry ||
                     class_entry -> Mtime() < java_entry -> Mtime()))
                {
                    file_symbol -> SetJava();
                    file_symbol -> mtime = java_entry -> Mtime();
                }
                else
                {
                    if (java_entry)
                         file_symbol -> SetClass();
                    else file_symbol -> SetClassOnly();
                    file_symbol -> mtime = class_entry -> Mtime();
                }
                break;
            }
        }
    }

    delete [] java_name;
    delete [] class_name;
    return file_symbol;
}


#ifdef HAVE_JIKES_NAMESPACE
} // Close namespace Jikes block
#endif

