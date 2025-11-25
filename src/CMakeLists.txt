set(JOPA_SOURCES
    ast.cpp
    ast_serializer.cpp
    body.cpp
    bytecode.cpp
    case.cpp
    class.cpp
    code.cpp
    control.cpp
    decl.cpp
    definite.cpp
    depend.cpp
    diagnose.cpp
    double.cpp
    dump.cpp
    error.cpp
    expr.cpp
    incrmnt.cpp
    init.cpp
    jast.cpp
    jast2.cpp
    jast_semantic.cpp
    grammar/javaact.cpp
    jikes.cpp
    jikesapi.cpp
    long.cpp
    lookup.cpp
    lpginput.cpp
    modifier.cpp
    op.cpp
    option.cpp
    paramtype.cpp
    parser.cpp
    parser_facade.cpp
    platform.cpp
    scanner.cpp
    segment.cpp
    set.cpp
    stream.cpp
    symbol.cpp
    system.cpp
    tab.cpp
    typeparam.cpp
    unparse.cpp
    zip.cpp
)

include(CheckIncludeFileCXX)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)
include(FetchContent)

# Fetch nlohmann_json for AST serialization
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

function(require_header header var)
    check_include_file_cxx(${header} ${var})
    if(NOT ${var})
        message(FATAL_ERROR "Required header ${header} not found")
    endif()
endfunction()

require_header(assert.h HAVE_ASSERT_H)
require_header(ctype.h HAVE_CTYPE_H)
require_header(errno.h HAVE_ERRNO_H)
require_header(float.h HAVE_FLOAT_H)
require_header(inttypes.h HAVE_INTTYPES_H)
require_header(limits.h HAVE_LIMITS_H)
require_header(locale.h HAVE_LOCALE_H)
require_header(math.h HAVE_MATH_H)
require_header(memory.h HAVE_MEMORY_H)
require_header(stdint.h HAVE_STDINT_H)
require_header(stdio.h HAVE_STDIO_H)
require_header(stdlib.h HAVE_STDLIB_H)
require_header(string.h HAVE_STRING_H)
require_header(strings.h HAVE_STRINGS_H)
require_header(sys/stat.h HAVE_SYS_STAT_H)
require_header(sys/types.h HAVE_SYS_TYPES_H)
require_header(time.h HAVE_TIME_H)
require_header(wchar.h HAVE_WCHAR_H)

if(NOT WIN32)
    require_header(dirent.h HAVE_DIRENT_H)
    require_header(unistd.h HAVE_UNISTD_H)
    set(UNIX_FILE_SYSTEM 1)
    set(HAVE_GLIBC_MKDIR 1)
    set(PATH_SEPARATOR ":")
else()
    check_include_file_cxx(windows.h HAVE_WINDOWS_H)
    check_include_file_cxx(direct.h HAVE_DIRECT_H)
    set(HAVE_WIN32_MKDIR 1)
    set(HAVE_PATHNAME_STYLE_DOS 1)
    set(WIN32_FILE_SYSTEM 1)
    set(PATH_SEPARATOR ";")
    if(MSVC)
        set(HAVE_VCPP_SET_NEW_HANDLER 1)
    endif()
endif()

set(STDC_HEADERS 1)

if(WIN32)
    check_symbol_exists(mkdir "direct.h" HAVE_MKDIR)
else()
    check_symbol_exists(mkdir "sys/stat.h;unistd.h" HAVE_MKDIR)
endif()
check_symbol_exists(wcslen "wchar.h" HAVE_WCSLEN)
check_symbol_exists(wcscpy "wchar.h" HAVE_WCSCPY)
check_symbol_exists(wcsncpy "wchar.h" HAVE_WCSNCPY)
check_symbol_exists(wcscat "wchar.h" HAVE_WCSCAT)
check_symbol_exists(wcscmp "wchar.h" HAVE_WCSCMP)
check_symbol_exists(wcsncmp "wchar.h" HAVE_WCSNCMP)
check_symbol_exists(cygwin_win32_to_posix_path_list "sys/cygwin.h" HAVE_CYGWIN_WIN32_TO_POSIX_PATH_LIST)

check_type_size("wchar_t" SIZEOF_WCHAR_T LANGUAGE CXX)
if(NOT SIZEOF_WCHAR_T)
    message(FATAL_ERROR "Unable to determine sizeof(wchar_t)")
endif()

check_type_size("wint_t" SIZEOF_WINT_T LANGUAGE CXX)
if(SIZEOF_WINT_T)
    set(HAVE_WINT_T 1)
endif()

check_type_size("int32_t" SIZEOF_INT32_T LANGUAGE CXX)
check_type_size("int" SIZEOF_INT LANGUAGE CXX)
if(SIZEOF_INT32_T EQUAL SIZEOF_INT)
    set(TYPE_INT32_T_IS_INT 1)
endif()

check_type_size("uint64_t" SIZEOF_UINT64_T LANGUAGE CXX)
if(SIZEOF_UINT64_T)
    set(HAVE_64BIT_TYPES 1)
endif()

test_big_endian(IS_BIG_ENDIAN)
if(IS_BIG_ENDIAN)
    set(WORDS_BIGENDIAN 1)
endif()

# Always enable Java 5 support and namespaces
set(ENABLE_SOURCE_15 1)
set(HAVE_JOPA_NAMESPACE 1)

if(JOPA_ENABLE_NATIVE_FP)
    set(HAVE_IEEE754 1)
endif()

if(JOPA_ENABLE_DEBUG)
    set(JOPA_DEBUG 1)
endif()

set(HAVE_STD 1)
set(HAVE_NAMESPACES 1)
set(HAVE_MEMBER_CONSTANTS 1)
set(HAVE_CONST_CAST 1)
set(HAVE_STATIC_CAST 1)
set(HAVE_DYNAMIC_CAST 1)
set(HAVE_REINTERPRET_CAST 1)
set(HAVE_RTTI 1)
set(HAVE_BOOL 1)
set(HAVE_EXPLICIT 1)
set(HAVE_OSTREAM_CONST_UNSIGNED_CHAR_PTR 1)

set(JOPA_STAT_S_IFDIR S_IFDIR)

unset(HAVE_ENCODING)
unset(JOPA_ICONV_ENCODING)
set(JOPA_LIBS)

# Find libzip
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)

list(APPEND JOPA_LIBS PkgConfig::LIBZIP)

if(JOPA_ENABLE_ENCODING)
    find_package(ICU COMPONENTS uc QUIET)
    if(ICU_UC_FOUND)
        set(HAVE_LIBICU_UC 1)
        set(HAVE_ENCODING 1)
        list(APPEND JOPA_LIBS ICU::uc)
    endif()

    find_package(Iconv QUIET)
    if(Iconv_FOUND)
        set(HAVE_ICONV_H 1)
        set(HAVE_ENCODING 1)
        set(JOPA_ICONV_ENCODING "wchar_t")
        set(HAVE_ERROR_CALL_ICONV_CONST 1)
        list(APPEND JOPA_LIBS Iconv::Iconv)
    endif()

    if(NOT HAVE_ENCODING)
        message(FATAL_ERROR "JOPA_ENABLE_ENCODING is ON but neither iconv nor ICU (uc) were found.")
    endif()
endif()

if(JOPA_ENABLE_CPPTRACE)
    find_package(cpptrace REQUIRED)
    list(APPEND JOPA_LIBS cpptrace::cpptrace)
    set(JOPA_HAS_CPPTRACE 1)
endif()

set(JOPA_SANITIZER_FLAGS)
if(JOPA_ENABLE_SANITIZERS)
    list(APPEND JOPA_SANITIZER_FLAGS
        -fsanitize=address
        -fsanitize=undefined
        -fsanitize=float-divide-by-zero
        -fsanitize=nullability
        -fsanitize=implicit-conversion
        -fno-sanitize-recover=all
        -fno-omit-frame-pointer
    )
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/Config.h.in"
    "${CMAKE_BINARY_DIR}/generated/config.h"
    @ONLY
)

add_executable(jopa ${JOPA_SOURCES})
target_include_directories(jopa PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/generated"
)
target_link_libraries(jopa PRIVATE ${JOPA_LIBS} nlohmann_json::nlohmann_json)

# Enable strict warnings
target_compile_options(jopa PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    ${JOPA_SANITIZER_FLAGS}
)

if(JOPA_ENABLE_SANITIZERS)
    target_link_options(jopa PRIVATE ${JOPA_SANITIZER_FLAGS})
endif()

install(TARGETS jopa RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
