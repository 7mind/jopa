// $Id: jikes.cpp,v 1.98 2004/03/20 04:48:17 jikesadmin Exp $
//
// This software is subject to the terms of the IBM Jikes Compiler
// License Agreement available at the following URL:
// http://ibm.com/developerworks/opensource/jikes.
// Copyright (C) 1996, 2004 IBM Corporation and others.  All Rights Reserved.
// You must accept the terms of that agreement to use this software.
//

#include "platform.h"
#include "jikesapi.h"
#include "error.h"


using namespace Jikes;
#endif

int main(int argc, char *argv[])
{
    // Here we are creating instance of default API
    JikesAPI *compiler = new JikesAPI();

    int return_code;
    char **files;

    files = compiler -> parseOptions(argc, argv);

    if (compiler -> getOptions() -> help)
    {
        printf("%s%s", StringConstant::U8S_help_header,
               StringConstant::U8S_command_format);
        printf("\n"
               "\tRegular options:\n"
               "-bootclasspath path location of system classes [default '']\n"
               "-classpath path     location of user classes and source files [default .]\n"
               "-d dir              write class files in directory dir [default .]\n"
               "-debug              no effect (ignored for compatibility)\n"
               "-depend | -Xdepend  recompile all used classes\n"
               "-deprecation        report uses of deprecated features\n"
#if defined(HAVE_ENCODING)
               "-encoding encoding  use specified encoding to read source files\n"
               "                      [default is system and locale dependent]\n"
# if defined(HAVE_LIBICU_UC)
               "                      this binary requires the ICU library\n"
# endif
#endif
               "-extdirs path       location of zip/jar files with platform extensions\n"
               "                      [default '']\n"
               "-g | -g:none | -g:{lines,vars,source}\n"
               "                      control level of debug information in class files\n"
               "                      [default lines,source]\n"
               "-J...               no effect (ignored for compatibility)\n"
               "-nowarn             javac-compatible equivalent of +Z0\n"
               "-nowrite            do not write any class files, useful with -verbose\n"
               "-O                  optimize bytecode (presently does nothing)\n"
               "-source release     interpret source by Java SDK release rules\n"
               "                      [default to max(target, 1.4)]\n"
               "-sourcepath path    location of user source files [default '']\n"
               "-target release     output bytecode for Java SDK release rules\n"
               "                      [default to source if specified, else 1.4.2]\n"
               "-verbose            list files read and written\n"
               "-Werror             javac-compatible equivalent of +Z2\n"
               "-Xstdout            redirect output listings to stdout\n"
               "-Xswitchcheck       warn about fallthrough between switch statement cases\n"
               "\tEnhanced options:\n"
               "++                  compile in incremental mode\n"
               "+a                  omit assert statements from class files\n"
               "+B                  do not invoke bytecode generator\n"
               "+D                  report errors immediately in emacs-form without buffering\n"
               "+DR=filename        generate dependence report in filename\n"
               "+E                  list errors in emacs-form\n"
               "+F                  do full dependence check except for Zip and Jar files\n"
               "+Kname=TypeKeyWord  map name to type keyword\n"
               "+M                  generate makefile dependencies\n"
               "+OLDCSO             perform original Jikes classpath order for compatibility\n"
               "+P                  pedantic compilation - issues lots of warnings\n"
               "                      some warnings can be turned on or off independently:\n");

        SemanticError::PrintNamedWarnings();

        printf("+T=n                set value of tab to n spaces, defaults to 8\n"
               "+U                  do full dependence check including Zip and Jar files\n"
               "+Z0                 do not issue warning messages\n"
               "+Z1                 treat cautions as errors\n"
               "+Z2                 treat both warnings and cautions as errors\n"
               "+Z                  equivalent to +Z1\n"
#ifdef JIKES_DEBUG
               "\tDebugging options:\n"
               "+A                  dump AST to standard output\n"
               "+c                  do not discard comments from lexer output, use with +L\n"
               "+C                  dump bytecodes to standard output\n"
               "+L                  dump lexer output (stream of tokens) to file.java.tok\n"
               "+O numbytes         call no-op op_trap() for bytecodes of the given length\n"
               "+S                  trace method stack depth to standard output\n"
               "+u                  unparse AST; produces Java code for the AST\n"
               "+ud                 unparse AST, with extra debugging information\n"
#endif
               "\tMiscellaneous options:\n"
               "-help | --help      display this message and exit\n"
               "-version | --version  display version and contact information, and exit\n");

        return_code = 0;
    }
    else if (compiler -> getOptions() -> version)
    {
        printf("%s", StringConstant::U8S_help_header);
        printf("Originally written by Philippe Charles and Dave Shields of IBM Research\n"
               "in 1996-1999, Jikes is once again maintained by Dave Shields at:\n"
               "<http://github.com/daveshields/jikes>\n"
               "Please consult this URL for more information and for reporting problems.\n");

        return_code = 0;
    }
    else if (files && files[0])
    {
        return_code = compiler -> compile(files);
    }
    else
    {
        printf("%s", StringConstant::U8S_command_format);
        printf("For more help, try -help or -version.\n");

        return_code = 2;
    }

    delete compiler;
    return return_code;
}
