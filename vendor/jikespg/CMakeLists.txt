# CMakeLists.txt for jikespg (Jikes Parser Generator)
# This builds jikespg from source as part of the main build

# Enable C language for this subdirectory
enable_language(C)

set(JIKESPG_SOURCES
    src/ctabs.c
    src/globals.c
    src/lpgparse.c
    src/lpgutil.c
    src/main.c
    src/mkfirst.c
    src/mkred.c
    src/mkstates.c
    src/partset.c
    src/prntstat.c
    src/produce.c
    src/ptables.c
    src/remsp.c
    src/resolve.c
    src/spacetab.c
    src/tabutil.c
    src/timetab.c
)

add_executable(jikespg ${JIKESPG_SOURCES})

target_include_directories(jikespg PRIVATE src)

# Suppress warnings for legacy C code
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(jikespg PRIVATE -w)
endif()
