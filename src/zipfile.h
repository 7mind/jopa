#ifndef zipfile_INCLUDED
#define zipfile_INCLUDED

#include "platform.h"
#include "tuple.h"
#include <zip.h>


namespace Jopa { // Open namespace Jopa block
class Control;
class Zip;
class DirectorySymbol;
class FileSymbol;
class NameSymbol;


class ZipFile
{
public:
    ZipFile(FileSymbol *);
    ~ZipFile();

    inline char *Buffer() { return buffer; }

private:
    char *buffer;
};


class Zip
{
public:
    Zip(Control &, char *);
    ~Zip();

    bool IsValid() { return zip_archive != NULL; }

    DirectorySymbol *RootDirectory() { return root_directory; }

private:
    friend class ZipFile;

    Control &control;
    DirectorySymbol *root_directory;

    zip_t *zip_archive;
    char *zip_filename;

    void ReadDirectory();

    NameSymbol *ProcessFilename(const char *, int);
    DirectorySymbol *ProcessSubdirectoryEntries(DirectorySymbol *, const char *, int);
    void ProcessDirectoryEntry(zip_int64_t index);
};


} // Close namespace Jopa block

#endif // zipfile_INCLUDED
