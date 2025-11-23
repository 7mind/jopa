#ifndef JIKES_API_H_FLAG_
#define JIKES_API_H_FLAG_

class JikesOption
{
public:
    char* bootclasspath; // Location of the libraries
    char* extdirs;       // Location of external drop-in jars
    char* classpath;     // Location of source and user class files
    char* sourcepath;    // Location of source files only
    char* directory;     // Target directory for output
    char* encoding;      // Character encoding name

    // Each of these fields is a boolean value
    // 0 if false, non-zero if true
    int nowrite;         // Don't generate output, useful with verbose
    int deprecation;     // Warn about deprecated code
    int optimize;        // Enable optimizations
    int verbose;         // Verbosely track compilation progress
    int depend;          // Require full dependency check
    int old_classpath_search_order; // Use older classpath search order
    int help;            // Display a usage help message
    int version;         // Display a version message

    enum DebugLevel
    {
        NONE = 0,
        SOURCE = 1,
        LINES = 2,
        VARS = 4
    };

    enum ReleaseLevel
    {
        UNKNOWN,
        SDK1_1,
        SDK1_2,
        SDK1_3,
        SDK1_4,
        SDK1_4_2,
        SDK1_5,
        SDK1_6,
        SDK1_7
    };

    // This field can be 0 through 7 to represent all debug level combinations.
    int g;               // Annotate class files with debugging information

    //
    // The JDK release number of the syntax rules to obey (for example,
    // assert was added in 1.4), as well as the VM level to target.
    //
    ReleaseLevel source;
    ReleaseLevel target;

    enum ToleranceLevel
    {
        NO_WARNINGS = 0,
        CAUTIONS_ARE_ERRORS = 1,
        WARNINGS_ARE_ERRORS = 2,
        DEFAULT = 4
    };

    ToleranceLevel tolerance;

    virtual ~JikesOption();

protected:
    JikesOption();
};

class JikesError
{
public:
    enum JikesErrorSeverity
    {
        JIKES_ERROR,
        JIKES_CAUTION,
        JIKES_WARNING
    };

    virtual JikesErrorSeverity getSeverity() = 0;

    /**
     * The filename where the error occurred. The caller should not delete
     * the return value.
     */
    virtual const char* getFileName() = 0;

    virtual int getLeftLineNo() = 0;
    virtual int getLeftColumnNo() = 0;
    virtual int getRightLineNo() = 0;
    virtual int getRightColumnNo() = 0;

    /**
     * Returns message describing error. The caller should not delete the
     * return value.
     */
    virtual const wchar_t* getErrorMessage() = 0;

    /**
     * Returns formatted error report. The caller should not delete the
     * return value.
     */
    virtual const wchar_t* getErrorReport() = 0;

protected:
    const char* getSeverityString();
};

/**
 * API to jikes compiler.
 */
class JikesAPI
{
public:

    JikesAPI();
    virtual ~JikesAPI();

    /**
     * Returns instance of current compiler options.
     * Returned pointer can be used to modify current compiler options.
     */
    virtual JikesOption* getOptions();

    /**
     * Creates instance of compiler options,
     * corresponding to given command line parameters.
     *
     * @return list of java file names found on command line
     * Caller should not attempt to manage the memory returned
     * by this method as it can be freed during another call
     * to parseOptions() or when this object is destroyed.
     */
    virtual char** parseOptions(int argc, char** argv);

    /**
     * Compile given list of files using current compiler options.
     */
    virtual int compile(char** filenames);

    /**
     * Jikes API implements singelton pattern.
     * This is a way to get instance of it.
     */
    static JikesAPI* getInstance();

    /**
     * This method will be called for each error reported.
     */
    virtual void reportError(JikesError* error);

    /**
     * Define the virtual base class for all Readers.
     * A pointer to an object of this type is returned by JikesAPI::read()
     */
    class FileReader
    {
    public:
        virtual ~FileReader() {}

        // If the file is unreadable an object should still be created but
        // GetBuffer() should return NULL.
        virtual const char* getBuffer() = 0;
        // If the file is unreadable GetBufferSize() is undefined.
        virtual size_t getBufferSize() = 0;
    };

    /**
     * Define the virtual base class for all WriteObjects.
     * A pointer to an object of this type is returned by JikesAPI::write()
     */
    class FileWriter
    {
    public:
        FileWriter(size_t mS) : maxSize(mS) {}
        virtual ~FileWriter() {}

        size_t write(const unsigned char* data, size_t size);
        virtual int isValid() = 0;

    private:
        // Guaranteed not to be called with a combined total of more than
        // maxSize bytes during the lifespan of the object.
        virtual size_t doWrite(const unsigned char* data, size_t size) = 0;
        size_t maxSize;
    };

    virtual int stat(const char* filename, struct stat* status);

    virtual FileReader* read(const char* filename);
    virtual FileWriter* write(const char* filename, size_t bytes);

private:
    void cleanupOptions(); // Helper to delete option and parsedOptions

    JikesOption* option;
    char** parsedOptions;

    static JikesAPI* instance;
};

#endif // JIKES_API_H_FLAG_
