#pragma once

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#if (defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER)) && !defined(CYGWIN) && !defined(__MINGW32__)
#    define USE_WINDOWS 1
#endif

#if defined(USE_WINDOWS)
#    define DIRECTORY_SEPARATOR "\\"
#else
#    define DIRECTORY_SEPARATOR "/"
#endif

#if defined(USE_WINDOWS)
#    include <direct.h>
#    include <io.h>
typedef struct _stat STAT;
#    define stat _stat
#    define S_IFREG _S_IFREG
#    define S_IFDIR _S_IFDIR
#    define access _access
#    define mkdir _mkdir
#    define rmdir _rmdir
#else
#    include <dirent.h>
#    include <unistd.h>
#    include <utime.h>
typedef struct stat STAT;
#endif

#include <zip.h>
#include <unzip.h>
#include <ioapi_mem.h>
#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#if defined(USE_WINDOWS)
#    define USEWIN32IOAPI
#    include "iowin32.h"
#endif
}

#if defined(USE_WINDOWS)
#    include <filesystem>
#endif

#define EXCEPTION_CLASS std::runtime_error

#if defined(_WIN64) && (!defined(__APPLE__))
#    ifndef __USE_FILE_OFFSET64
#        define __USE_FILE_OFFSET64
#    endif
#    ifndef __USE_LARGEFILE64
#        define __USE_LARGEFILE64
#    endif
#    ifndef _LARGEFILE64_SOURCE
#        define _LARGEFILE64_SOURCE
#    endif
#    ifndef _FILE_OFFSET_BIT
#        define _FILE_OFFSET_BIT 64
#    endif
#endif

#if defined(USE_WINDOWS) || defined(__MINGW32__)
#    define MKDIR(d) _mkdir(d)
#    define CHDIR(d) _chdir(d)
#else
#    define MKDIR(d) mkdir(d, 0775)
#    define CHDIR(d) chdir(d)
#endif
