# Grammar generation
add_subdirectory(grammar)

set(JOPA_SOURCES
    ast.cpp
    body.cpp
    case.cpp
    code.cpp
    control.cpp
    decl.cpp
    definite.cpp
    depend.cpp
    double.cpp
    dump.cpp
    error.cpp
    expr_lookup.cpp
    expr_names.cpp
    expr_primary.cpp
    expr_ops.cpp
    incrmnt.cpp
    init.cpp
    jikes.cpp
    jikesapi.cpp
    long.cpp
    lookup.cpp
    modifier.cpp
    op.cpp
    option.cpp
    paramtype.cpp
    platform.cpp
    set.cpp
    symbol.cpp
    system.cpp
    tab.cpp
    typeparam.cpp
    unparse.cpp
    zip.cpp

    # Parser
    parser/diagnose.cpp
    parser/lpginput.cpp
    parser/parser.cpp
    parser/scanner.cpp
    parser/stream.cpp

    # Code generation (bytecode split for maintainability)
    codegen/bytecode_init.cpp
    codegen/bytecode_stmt.cpp
    codegen/bytecode_expr.cpp
    codegen/bytecode_ops.cpp
    codegen/class.cpp
    codegen/segment.cpp

    # Generated grammar
    grammar/javaact.cpp
)

include(CheckIncludeFileCXX)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

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

require_header(dirent.h HAVE_DIRENT_H)
require_header(unistd.h HAVE_UNISTD_H)
set(UNIX_FILE_SYSTEM 1)
set(HAVE_GLIBC_MKDIR 1)
set(PATH_SEPARATOR ":")

set(STDC_HEADERS 1)

check_symbol_exists(mkdir "sys/stat.h;unistd.h" HAVE_MKDIR)
check_symbol_exists(wcslen "wchar.h" HAVE_WCSLEN)
check_symbol_exists(wcscpy "wchar.h" HAVE_WCSCPY)
check_symbol_exists(wcsncpy "wchar.h" HAVE_WCSNCPY)
check_symbol_exists(wcscat "wchar.h" HAVE_WCSCAT)
check_symbol_exists(wcscmp "wchar.h" HAVE_WCSCMP)
check_symbol_exists(wcsncmp "wchar.h" HAVE_WCSNCMP)

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
    if(NOT JOPA_ENABLE_LEAK_SANITIZER)
        set(JOPA_DISABLE_LEAK_SANITIZER 1)
    endif()
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/Config.h.in"
    "${CMAKE_BINARY_DIR}/generated/config.h"
    @ONLY
)

add_executable(jopa ${JOPA_SOURCES})
target_include_directories(jopa PRIVATE
    "${CMAKE_BINARY_DIR}/src"  # For generated grammar/ headers
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser"
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen"
    "${CMAKE_BINARY_DIR}/generated"
)
target_link_libraries(jopa PRIVATE ${JOPA_LIBS})

# Depend on grammar generation if available
if(TARGET generate_parser)
    add_dependencies(jopa generate_parser)
endif()

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
