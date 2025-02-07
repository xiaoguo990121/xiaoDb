cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/")
include(ReadVersion)
get_xiaodb_version(xiaodb_VERSION)
project(xiaodb
VERSION ${xiaodb_VERSION}
DESCRIPTION "An embeddable persistent key-value store for fast storage"
)

if(NOT CMAKE_BUILD_TYPE)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        set(default_build_type "Debug")
    else()
        set(default_build_type "RelWithDebInfo")
    endif()
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING
    "Default BUILD_TYPE is ${default_build_type}" FORCE)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

option(WITH_JEMALLOC "build with JeMalloc" OFF)
option(WITH_LIBURING "build with liburing" ON)
option(WITH_SNAPPY "build with SNAPPY" OFF)
option(WITH_LZ4 "build with lz4" OFF)
option(WITH_ZLIB "build with zlib" OFF)
option(WITH_ZSTD "build with zstd" OFF)
option(WITH_WINDOWS_UTF8_FILENAMES "use UTF8 as characterset for opening files, regardles of the system code page" OFF)
if (WITH_WINDOWS_UTF8_FILENAMES)
  add_definitions(-DXIAODB_WINDOWS_UTF8_FILENAMES)
endif()
option(XIAODB_BUILD_SHARED "Build shared versions of the XiaoDB libraries" ON)

if( NOT DEFINED CMAKE_CXX_STANDARD )
  set(CMAKE_CXX_STANDARD 20)
endif()

include(CMakeDependentOption)

if(MSVC)
  option(WITH_GFLAGS "build with GFlags" OFF)
  option(WITH_XPRESS "build with windows built in compression" OFF)
  option(XIAODB_SKIP_THIRDPARTY "skip thirdparty.inc" OFF)

  if(NOT XIAODB_SKIP_THIRDPARTY)
    include(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty.inc)
  endif()
else()
  if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD" AND NOT CMAKE_SYSTEM_NAME MATCHES "kFreeBSD")
    # FreeBSD has jemalloc as default malloc
    # but it does not have all the jemalloc files in include/...
    set(WITH_JEMALLOC ON)
  else()
    if(WITH_JEMALLOC)
      find_package(JeMalloc REQUIRED)
      add_definitions(-DXIAODB_JEMALLOC -DJEMALLOC_NO_DEMANGLE)
      list(APPEND THIRDPARTY_LIBS JeMalloc::JeMalloc)
    endif()
  endif()

  if(MINGW)
    option(WITH_GFLAGS "build with GFlags" OFF)
  else()
    option(WITH_GFLAGS "build with GFlags" ON)
  endif()
  set(GFLAGS_LIB)
  if(WITH_GFLAGS)
    # Config with namespace available since gflags 2.2.2
    option(GFLAGS_USE_TARGET_NAMESPACE "Use gflags import target with namespace." ON)
    find_package(gflags CONFIG)
    if(gflags_FOUND)
      if(TARGET ${GFLAGS_TARGET})
        # Config with GFLAGS_TARGET available since gflags 2.2.0
        set(GFLAGS_LIB ${GFLAGS_TARGET})
      else()
        # Config with GFLAGS_LIBRARIES available since gflags 2.1.0
        set(GFLAGS_LIB ${gflags_LIBRARIES})
      endif()
    else()
      find_package(gflags REQUIRED)
      set(GFLAGS_LIB gflags::gflags)
    endif()
    include_directories(${GFLAGS_INCLUDE_DIR})
    list(APPEND THIRDPARTY_LIBS ${GFLAGS_LIB})
    add_definitions(-DGFLAGS=1)
  endif()

  if(WITH_SNAPPY)
    find_package(Snappy CONFIG)
    if(NOT Snappy_FOUND)
      find_package(Snappy REQUIRED)
    endif()
    add_definitions(-DSNAPPY)
    list(APPEND THIRDPARTY_LIBS Snappy::snappy)
  endif()

  if(WITH_ZLIB)
    find_package(ZLIB REQUIRED)
    add_definitions(-DZLIB)
    list(APPEND THIRDPARTY_LIBS ZLIB::ZLIB)
  endif()

  option(WITH_BZ2 "build with bzip2" OFF)
  if(WITH_BZ2)
    find_package(BZip2 REQUIRED)
    add_definitions(-DBZIP2)
    if(BZIP2_INCLUDE_DIRS)
      include_directories(${BZIP2_INCLUDE_DIRS})
    else()
      include_directories(${BZIP2_INCLUDE_DIR})
    endif()
    list(APPEND THIRDPARTY_LIBS ${BZIP2_LIBRARIES})
  endif()

  if(WITH_LZ4)
    find_package(lz4 REQUIRED)
    add_definitions(-DLZ4)
    list(APPEND THIRDPARTY_LIBS lz4::lz4)
  endif()

  if(WITH_ZSTD)
    find_package(zstd REQUIRED)
    add_definitions(-DZSTD)
    include_directories(${ZSTD_INCLUDE_DIRS})
    list(APPEND THIRDPARTY_LIBS zstd::zstd)
  endif()
endif()

option(WITH_MD_LIBRARY "build with MD" ON)
if(WIN32 AND MSVC)
  if(WITH_MD_LIBRARY)
    set(RUNTIME_LIBRARY "MD")
  else()
    set(RUNTIME_LIBRARY "MT")
  endif()
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /nologo /EHsc /GS /Gd /GR /GF /fp:precise /Zc:wchar_t /Zc:forScope /errorReport:queue")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FC /d2Zi+ /W4 /wd4127 /wd4996 /wd4100 /wd4324")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wextra -Wall -pthread")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-compare -Wshadow -Wno-unused-parameter -Wno-unused-variable -Woverloaded-virtual -Wnon-virtual-dtor -Wno-missing-field-initializers -Wno-strict-aliasing -Wno-invalid-offsetof")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")
  endif()
  if(MINGW)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
    add_definitions(-D_POSIX_C_SOURCE=1)
  endif()
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-momit-leaf-frame-pointer" HAVE_OMIT_LEAF_FRAME_POINTER)
    if(HAVE_OMIT_LEAF_FRAME_POINTER)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -momit-leaf-frame-pointer")
    endif()
  endif()
endif()

include(CheckCCompilerFlag)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")
  CHECK_C_COMPILER_FLAG("-mcpu=power9" HAS_POWER9)
  if(HAS_POWER9)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=power9 -mtune=power9")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=power9 -mtune=power9")
  else()
    CHECK_C_COMPILER_FLAG("-mcpu=power8" HAS_POWER8)
    if(HAS_POWER8)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=power8 -mtune=power8")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=power8 -mtune=power8")
    endif(HAS_POWER8)
  endif(HAS_POWER9)
  CHECK_C_COMPILER_FLAG("-maltivec" HAS_ALTIVEC)
  if(HAS_ALTIVEC)
    message(STATUS " HAS_ALTIVEC yes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maltivec")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maltivec")
  endif(HAS_ALTIVEC)
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|AARCH64")
        CHECK_C_COMPILER_FLAG("-march=armv8-a+crc+crypto" HAS_ARMV8_CRC)
  if(HAS_ARMV8_CRC)
    message(STATUS " HAS_ARMV8_CRC yes")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc+crypto -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc+crypto -Wno-unused-function")
  endif(HAS_ARMV8_CRC)
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|AARCH64")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "s390x")
  CHECK_C_COMPILER_FLAG("-march=native" HAS_S390X_MARCH_NATIVE)
  if(HAS_S390X_MARCH_NATIVE)
    message(STATUS " HAS_S390X_MARCH_NATIVE yes")
  endif(HAS_S390X_MARCH_NATIVE)
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "s390x")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")
  CHECK_C_COMPILER_FLAG("-march=loongarch64" HAS_LOONGARCH64)
  if(HAS_LOONGARCH64)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=loongarch64 -mtune=loongarch64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=loongarch64 -mtune=loongarch64")
  endif(HAS_LOONGARCH64)
endif(CMAKE_SYSTEM_PROCESSOR MATCHES "loongarch64")

set(PORTABLE 0 CACHE STRING "Minimum CPU arch to support, or 0 = current CPU, 1 = baseline CPU")
if(PORTABLE MATCHES "1|ON|YES|TRUE|Y")
  # Usually nothing to do; compiler default is typically the most general
  if(NOT MSVC)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^s390x")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=z196")
    endif()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^loongarch64")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=loongarch64")
    endif()
  endif()
elseif(PORTABLE MATCHES "0|OFF|NO|FALSE|N")
  if(MSVC)
    # NOTE: No auto-detection of current CPU, but instead assume some useful
    # level of optimization is supported
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
  else()
    # Require instruction set from current CPU (with some legacy or opt-out
    # exceptions)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^s390x" AND NOT HAS_S390X_MARCH_NATIVE)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=z196")
    elseif(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(powerpc|ppc)64" AND NOT HAS_ARMV8_CRC)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
  endif()
else()
  # Name of a CPU arch spec or feature set to require
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:${PORTABLE}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${PORTABLE}")
  endif()
endif()

include(CheckCXXSourceCompiles)
set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
if(NOT MSVC)
  set(CMAKE_REQUIRED_FLAGS "-msse4.2 -mpclmul")
endif()

# Check if -latomic is required or not
if (NOT MSVC)
  set(CMAKE_REQUIRED_FLAGS "--std=c++17")
  CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
std::atomic<uint64_t> x(0);
int main() {
  uint64_t i = x.load(std::memory_order_relaxed);
  bool b = x.is_lock_free();
  return 0;
}
" BUILTIN_ATOMIC)
  if (NOT BUILTIN_ATOMIC)
    #TODO: Check if -latomic exists
    list(APPEND THIRDPARTY_LIBS atomic)
  endif()
endif()

if (WITH_LIBURING)
  find_package(uring)
  if (uring_FOUND)
    add_definitions(-DROCKSDB_IOURING_PRESENT)
    list(APPEND THIRDPARTY_LIBS uring::uring)
  endif()
endif()

# Reset the required flags
set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

option(WITH_IOSTATS_CONTEXT "Enable IO stats context" ON)
if (NOT WITH_IOSTATS_CONTEXT)
  add_definitions(-DNIOSTATS_CONTEXT)
endif()

option(WITH_PERF_CONTEXT "Enable perf context" ON)
if (NOT WITH_PERF_CONTEXT)
  add_definitions(-DNPERF_CONTEXT)
endif()

option(FAIL_ON_WARNINGS "Treat compile warnings as errors" ON)
if(FAIL_ON_WARNINGS)
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
  else() # assume GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif()
endif()

option(WITH_ASAN "build with ASAN" OFF)
if(WITH_ASAN)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  if(WITH_JEMALLOC)
    message(FATAL "ASAN does not work well with JeMalloc")
  endif()
endif()

option(WITH_TSAN "build with TSAN" OFF)
if(WITH_TSAN)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread -Wl,-pie")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread -fPIC")
  if(WITH_JEMALLOC)
    message(FATAL "TSAN does not work well with JeMalloc")
  endif()
endif()

option(WITH_UBSAN "build with UBSAN" OFF)
if(WITH_UBSAN)
  add_definitions(-DROCKSDB_UBSAN_RUN)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
  if(WITH_JEMALLOC)
    message(FATAL "UBSAN does not work well with JeMalloc")
  endif()
endif()

option(WITH_NUMA "build with NUMA policy support" OFF)
if(WITH_NUMA)
  find_package(NUMA REQUIRED)
  add_definitions(-DNUMA)
  include_directories(${NUMA_INCLUDE_DIRS})
  list(APPEND THIRDPARTY_LIBS NUMA::NUMA)
endif()

option(WITH_TBB "build with Threading Building Blocks (TBB)" OFF)
if(WITH_TBB)
  find_package(TBB REQUIRED)
  add_definitions(-DTBB)
  list(APPEND THIRDPARTY_LIBS TBB::TBB)
endif()

# Stall notifications eat some performance from inserts
option(DISABLE_STALL_NOTIF "Build with stall notifications" OFF)
if(DISABLE_STALL_NOTIF)
  add_definitions(-DROCKSDB_DISABLE_STALL_NOTIFICATION)
endif()

option(WITH_DYNAMIC_EXTENSION "build with dynamic extension support" OFF)
if(NOT WITH_DYNAMIC_EXTENSION)
  add_definitions(-DROCKSDB_NO_DYNAMIC_EXTENSION)
endif()

option(ASSERT_STATUS_CHECKED "build with assert status checked" OFF)
if (ASSERT_STATUS_CHECKED)
  message(STATUS "Build with assert status checked")
  add_definitions(-DROCKSDB_ASSERT_STATUS_CHECKED)
endif()


# RTTI is by default AUTO which enables it in debug and disables it in release.
set(USE_RTTI AUTO CACHE STRING "Enable RTTI in builds")
set_property(CACHE USE_RTTI PROPERTY STRINGS AUTO ON OFF)
if(USE_RTTI STREQUAL "AUTO")
  message(STATUS "Enabling RTTI in Debug builds only (default)")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
  if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GR-")
  else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-rtti")
  endif()
elseif(USE_RTTI)
  message(STATUS "Enabling RTTI in all builds")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DROCKSDB_USE_RTTI")
else()
  if(MSVC)
    message(STATUS "Disabling RTTI in Release builds. Always on in Debug.")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DROCKSDB_USE_RTTI")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GR-")
  else()
    message(STATUS "Disabling RTTI in all builds")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-rtti")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-rtti")
  endif()
endif()

# Used to run CI build and tests so we can run faster
option(OPTDBG "Build optimized debug build with MSVC" OFF)
option(WITH_RUNTIME_DEBUG "build with debug version of runtime library" ON)
if(MSVC)
  if(OPTDBG)
    message(STATUS "Debug optimization is enabled")
    set(CMAKE_CXX_FLAGS_DEBUG "/Oxt")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /RTC1")

    # Minimal Build is deprecated after MSVC 2015
    if( MSVC_VERSION GREATER 1900 )
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm-")
    else()
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
    endif()

  endif()
  if(WITH_RUNTIME_DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /${RUNTIME_LIBRARY}d")
  else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /${RUNTIME_LIBRARY}")
  endif()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Oxt /Zp8 /Gm- /Gy /${RUNTIME_LIBRARY}")

  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEBUG")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-builtin-memcmp")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Cygwin")
  add_definitions(-fno-builtin-memcmp -DCYGWIN)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  add_definitions(-DOS_MACOSX)
elseif(CMAKE_SYSTEM_NAME MATCHES "iOS")
  add_definitions(-DOS_MACOSX -DIOS_CROSS_COMPILE)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_definitions(-DOS_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
  add_definitions(-DOS_SOLARIS)
elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD")
  add_definitions(-DOS_GNU_KFREEBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
  add_definitions(-DOS_FREEBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
  add_definitions(-DOS_NETBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
  add_definitions(-DOS_OPENBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "DragonFly")
  add_definitions(-DOS_DRAGONFLYBSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
  add_definitions(-DOS_ANDROID)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
  add_definitions(-DWIN32 -DOS_WIN -D_MBCS -DWIN64 -DNOMINMAX)
  if(MINGW)
    add_definitions(-D_WIN32_WINNT=_WIN32_WINNT_VISTA)
  endif()
endif()

if(NOT WIN32)
  add_definitions(-DROCKSDB_PLATFORM_POSIX -DROCKSDB_LIB_IO_POSIX)
endif()

option(WITH_FALLOCATE "build with fallocate" ON)
if(WITH_FALLOCATE)
  CHECK_CXX_SOURCE_COMPILES("
#include <fcntl.h>
#include <linux/falloc.h>
int main() {
 int fd = open(\"/dev/null\", 0);
 fallocate(fd, FALLOC_FL_KEEP_SIZE, 0, 1024);
}
" HAVE_FALLOCATE)
  if(HAVE_FALLOCATE)
    add_definitions(-DROCKSDB_FALLOCATE_PRESENT)
  endif()
endif()

CHECK_CXX_SOURCE_COMPILES("
#include <fcntl.h>
int main() {
  int fd = open(\"/dev/null\", 0);
  sync_file_range(fd, 0, 1024, SYNC_FILE_RANGE_WRITE);
}
" HAVE_SYNC_FILE_RANGE_WRITE)
if(HAVE_SYNC_FILE_RANGE_WRITE)
  add_definitions(-DROCKSDB_RANGESYNC_PRESENT)
endif()

CHECK_CXX_SOURCE_COMPILES("
#include <pthread.h>
int main() {
  (void) PTHREAD_MUTEX_ADAPTIVE_NP;
}
" HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
if(HAVE_PTHREAD_MUTEX_ADAPTIVE_NP)
  add_definitions(-DROCKSDB_PTHREAD_ADAPTIVE_MUTEX)
endif()

include(CheckCXXSymbolExists)
if(CMAKE_SYSTEM_NAME MATCHES "^FreeBSD")
  check_cxx_symbol_exists(malloc_usable_size malloc_np.h HAVE_MALLOC_USABLE_SIZE)
else()
  check_cxx_symbol_exists(malloc_usable_size malloc.h HAVE_MALLOC_USABLE_SIZE)
endif()
if(HAVE_MALLOC_USABLE_SIZE)
  add_definitions(-DROCKSDB_MALLOC_USABLE_SIZE)
endif()

check_cxx_symbol_exists(sched_getcpu sched.h HAVE_SCHED_GETCPU)
if(HAVE_SCHED_GETCPU)
  add_definitions(-DROCKSDB_SCHED_GETCPU_PRESENT)
endif()

check_cxx_symbol_exists(getauxval "sys/auxv.h" HAVE_AUXV_GETAUXVAL)
if(HAVE_AUXV_GETAUXVAL)
  add_definitions(-DROCKSDB_AUXV_GETAUXVAL_PRESENT)
endif()

check_cxx_symbol_exists(F_FULLFSYNC "fcntl.h" HAVE_FULLFSYNC)
if(HAVE_FULLFSYNC)
  add_definitions(-DHAVE_FULLFSYNC)
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

if(USE_COROUTINES)
  if(USE_FOLLY OR USE_FOLLY_LITE)
    message(FATAL_ERROR "Please specify exactly one of USE_COROUTINES,"
    " USE_FOLLY, and USE_FOLLY_LITE")
  endif()
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines -Wno-maybe-uninitialized")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-redundant-move")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-memory-model")
  add_compile_definitions(USE_COROUTINES)
  set(USE_FOLLY 1)
endif()

if(USE_FOLLY)
  if(USE_FOLLY_LITE)
    message(FATAL_ERROR "Please specify one of USE_FOLLY or USE_FOLLY_LITE")
  endif()
  if(ROCKSDB_BUILD_SHARED)
    message(FATAL_ERROR "Cannot build RocksDB shared library with folly")
  endif()
  set(ROCKSDB_BUILD_SHARED OFF)
  set(GFLAGS_SHARED FALSE)
  find_package(folly)
  # If cmake could not find the folly-config.cmake file, fall back
  # to looking in third-party/folly for folly and its dependencies
  if(NOT FOLLY_LIBRARIES)
    exec_program(python3 ${PROJECT_SOURCE_DIR}/third-party/folly ARGS
    build/fbcode_builder/getdeps.py show-inst-dir OUTPUT_VARIABLE
    FOLLY_INST_PATH)
    exec_program(ls ARGS -d ${FOLLY_INST_PATH}/../boost* OUTPUT_VARIABLE
    BOOST_INST_PATH)
    exec_program(ls ARGS -d ${FOLLY_INST_PATH}/../fmt* OUTPUT_VARIABLE
    FMT_INST_PATH)
    exec_program(ls ARGS -d ${FOLLY_INST_PATH}/../gflags* OUTPUT_VARIABLE
    GFLAGS_INST_PATH)
    set(Boost_DIR ${BOOST_INST_PATH}/lib/cmake/Boost-1.83.0)
    if(EXISTS ${FMT_INST_PATH}/lib64)
      set(fmt_DIR ${FMT_INST_PATH}/lib64/cmake/fmt)
    else()
      set(fmt_DIR ${FMT_INST_PATH}/lib/cmake/fmt)
    endif()
    set(gflags_DIR ${GFLAGS_INST_PATH}/lib/cmake/gflags)

    exec_program(sed ARGS -i 's/gflags_shared//g'
    ${FOLLY_INST_PATH}/lib/cmake/folly/folly-targets.cmake)

    include(${FOLLY_INST_PATH}/lib/cmake/folly/folly-config.cmake)
  endif()

  add_compile_definitions(USE_FOLLY FOLLY_NO_CONFIG HAVE_CXX11_ATOMIC)
  list(APPEND THIRDPARTY_LIBS Folly::folly)
  set(FOLLY_LIBS Folly::folly)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--copy-dt-needed-entries")
endif()
find_package(Threads REQUIRED)

set(SOURCES
    cache/cache.cc
)

set(XIAODB_STATIC_LIB xiaodb${ARTIFACT_SUFFIX})
set(XIAODB_SHARED_LIB xiaodb-shared${ARTIFACT_SUFFIX})

add_library(${XIAODB_STATIC_LIB} STATIC ${SOURCES})
target_include_directories(${XIAODB_STATIC_LIB} PUBLIC
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

if(XIAODB_BUILD_SHARED)
    add_library(${XIAODB_SHARED_LIB} SHARED ${SOURCES})
    target_include_directories(${XIAODB_SHARED_LIB} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
endif()