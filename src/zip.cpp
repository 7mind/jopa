#include "zipfile.h"
#include "control.h"
#include "symbol.h"
#include <cstring>
#include <cstdio>


namespace Jopa { // Open namespace Jopa block
//************************************************************
//
// The ZipFile methods follow
//
//************************************************************

ZipFile::ZipFile(FileSymbol *file_symbol) : buffer(NULL)
{
    Zip *zip = file_symbol -> Zipfile();

    assert(zip -> IsValid());

    // Build the full path within the ZIP: directory/name.class or directory/name.java
    DirectorySymbol *dir = file_symbol -> directory_symbol;
    int path_length = 0;

    // Calculate total path length
    for (DirectorySymbol *d = dir; d && d -> Identity() != d -> owner -> Identity(); d = (DirectorySymbol*) d -> owner)
    {
        path_length += d -> Utf8NameLength() + 1; // +1 for '/'
    }
    path_length += file_symbol -> Utf8NameLength();
    path_length += (file_symbol -> IsJava() ? FileSymbol::java_suffix_length : FileSymbol::class_suffix_length);

    char *zip_path = new char[path_length + 1];
    zip_path[0] = '\0';

    // Build path from root to file
    DirectorySymbol **dir_stack = new DirectorySymbol*[100]; // reasonable max depth
    int depth = 0;
    for (DirectorySymbol *d = dir; d && d -> Identity() != d -> owner -> Identity(); d = (DirectorySymbol*) d -> owner)
    {
        dir_stack[depth++] = d;
    }

    for (int i = depth - 1; i >= 0; i--)
    {
        const char* dir_name = dir_stack[i] -> Utf8Name();
        // Skip the root "." directory
        if (strcmp(dir_name, ".") != 0)
        {
            strcat(zip_path, dir_name);
            strcat(zip_path, "/");
        }
    }

    strcat(zip_path, file_symbol -> Utf8Name());
    strcat(zip_path, file_symbol -> IsJava() ? FileSymbol::java_suffix : FileSymbol::class_suffix);

    delete [] dir_stack;

    // Open and read the file from the ZIP
    // Try without "./" prefix first, then with it for compatibility
    zip_file_t *zf = zip_fopen(zip->zip_archive, zip_path, 0);

    if (!zf && zip_path[0] != '.' && zip_path[0] != '/')
    {
        // Try with "./" prefix
        char *alt_path = new char[path_length + 3];
        strcpy(alt_path, "./");
        strcat(alt_path, zip_path);
        zf = zip_fopen(zip->zip_archive, alt_path, 0);
        delete [] alt_path;
    }

    if (zf)
    {
        buffer = new char[file_symbol->uncompressed_size];
        zip_int64_t bytes_read = zip_fread(zf, buffer, file_symbol->uncompressed_size);

        if (bytes_read != (zip_int64_t)file_symbol->uncompressed_size)
        {
            delete [] buffer;
            buffer = NULL;
        }

        zip_fclose(zf);
    }

    delete [] zip_path;
}

ZipFile::~ZipFile()
{
    delete [] buffer;
}


//********************************************************************
//
// The Zip methods follow:
//
//********************************************************************

DirectorySymbol *Zip::ProcessSubdirectoryEntries(DirectorySymbol *directory_symbol, const char *name, int name_length)
{
    wchar_t *directory_name = new wchar_t[name_length];

    for (int start = 0, end; start < name_length; start = end + 1)
    {
        end = start;
        for (int i = 0; end < name_length && name[end] != U_SLASH; i++, end++)
             directory_name[i] = name[end];
        NameSymbol *name_symbol = control.FindOrInsertName(directory_name, end - start);
        DirectorySymbol *subdirectory_symbol = directory_symbol -> FindDirectorySymbol(name_symbol);
        if (! subdirectory_symbol)
            subdirectory_symbol = directory_symbol -> InsertDirectorySymbol(name_symbol, false);
        directory_symbol = subdirectory_symbol;
    }

    delete [] directory_name;

    return directory_symbol;
}


NameSymbol *Zip::ProcessFilename(const char *name, int name_length)
{
    wchar_t *input_filename = new wchar_t[name_length];
    for (int i = 0; i < name_length; i++)
        input_filename[i] = name[i];
    NameSymbol *name_symbol = control.FindOrInsertName(input_filename, name_length);

    delete [] input_filename;

    return name_symbol;
}


void Zip::ProcessDirectoryEntry(zip_int64_t index)
{
    struct zip_stat st;
    zip_stat_init(&st);

    if (zip_stat_index(zip_archive, index, 0, &st) != 0)
        return;

    const char *name = st.name;
    int file_name_length = strlen(name);
    u4 uncompressed_size = st.size;
    u4 date_time = st.mtime; // Use modification time from stat

    //
    // Note that we need to process all subdirectory entries
    // that appear in the zip file, and not just the ones that
    // contain java and class files. Recall that in java the
    // dot notation is used in specifying a package. Therefore,
    // in processing a qualified-name that represents a package,
    // we need to recognize each name as a subpackage. E.g.,
    // when processing "java.lang", we need to recognize "java"
    // as a package before looking for "lang"...

    // start at the "." directory.
    DirectorySymbol *directory_symbol = root_directory;
    // -1 to remove last '/'
    if (name[file_name_length - 1] == U_SLASH)
        ProcessSubdirectoryEntries(directory_symbol,
                                   name,
                                   file_name_length - 1);
    else
    {
        bool java_file = (static_cast<unsigned>(file_name_length) >= FileSymbol::java_suffix_length &&
                          FileSymbol::IsJavaSuffix(const_cast<char*>(&name[file_name_length - FileSymbol::java_suffix_length]))),
             class_file = (static_cast<unsigned>(file_name_length) >= FileSymbol::class_suffix_length &&
                           FileSymbol::IsClassSuffix(const_cast<char*>(&name[file_name_length - FileSymbol::class_suffix_length])));

        if (java_file || class_file)
        {
            int name_length = file_name_length - (java_file ? FileSymbol::java_suffix_length : FileSymbol::class_suffix_length);
            int i;
            for (i = name_length - 1; i >= 0 && name[i] != U_SLASH; i--)
                ;
            if (i > 0) // directory specified?
                directory_symbol = ProcessSubdirectoryEntries(directory_symbol,
                                                              name, i);
            NameSymbol *name_symbol = ProcessFilename(&name[i + 1],
                                                      name_length - (i + 1));

            //
            // Search for a file of that name in the directory.
            // If one is not found, then insert ... Otherwise,
            // either a class file of that name was previously
            // processed and now we found a java file with the
            // same name or vice-versa... In that case keep
            // (or replace with) the file with the most recent
            // date stamp.
            //
            FileSymbol *file_symbol = directory_symbol ->
                FindFileSymbol(name_symbol);
            if (! file_symbol)
            {
                file_symbol = directory_symbol -> InsertFileSymbol(name_symbol);

                file_symbol -> directory_symbol = directory_symbol;
                if (java_file)
                     file_symbol -> SetJava();
                else file_symbol -> SetClassOnly();

                file_symbol -> uncompressed_size = uncompressed_size;
                file_symbol -> date_time = date_time;
            }
            else if (file_symbol -> date_time < date_time)
            {
                if (java_file)
                     file_symbol -> SetJava();
                else file_symbol -> SetClass();

                file_symbol -> uncompressed_size = uncompressed_size;
                file_symbol -> date_time = date_time;
            }
        }
    }
}


Zip::Zip(Control &control_, char *zipfile_name) : control(control_),
                                                  root_directory(NULL),
                                                  zip_archive(NULL),
                                                  zip_filename(NULL)
{
    int err = 0;
    zip_archive = zip_open(zipfile_name, ZIP_RDONLY, &err);

    if (zip_archive)
    {
        // Store filename for later use by ZipFile
        int len = strlen(zipfile_name);
        zip_filename = new char[len + 1];
        strcpy(zip_filename, zipfile_name);
    }

    ReadDirectory();
}


Zip::~Zip()
{
    if (zip_archive)
        zip_close(zip_archive);

    delete [] zip_filename;
    delete root_directory;
}


//
// Upon successful termination of this function, IsValid() should yield true.
//
void Zip::ReadDirectory()
{
    // Not a sourcepath (since we don't read java files from zip files)
    root_directory = new DirectorySymbol(control.dot_name_symbol, NULL, false);

    if (IsValid())
    {
        zip_int64_t num_entries = zip_get_num_entries(zip_archive, 0);

        for (zip_int64_t i = 0; i < num_entries; i++)
        {
            ProcessDirectoryEntry(i);
        }
    }
}


} // Close namespace Jopa block
