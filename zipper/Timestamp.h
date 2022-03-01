#pragma once

#include "defs.h"
#include <ctime>
#include <chrono>
#if defined(USE_WINDOWS)
#    include <Windows.h>
#elif __linux__
#    include <sys/stat.h>
#endif
/**
 * @brief Creates a timestap either from file or just current time
 * If it fails to read the timestamp of a file, it set the time stamp to current time
 * 
 * @warning It uses std::time to get current time, which is not standardized to be 1970-01-01....
 * However, it works on Windows and Unix https://stackoverflow.com/questions/6012663/get-unix-timestamp-with-c 
 * With C++20 this will be standardized
 */
struct Timestamp
{
    tm timestamp;
    Timestamp();
    Timestamp(const std::string& filepath);
};

Timestamp::Timestamp()
{
    std::time_t now = std::time(nullptr);
    timestamp = *std::localtime(&now);
}

Timestamp::Timestamp(const std::string& filepath)
{
    //Set default
    std::time_t now = std::time(nullptr);
    timestamp = *std::localtime(&now);
#if defined(USE_WINDOWS)
    //Implementation based on Ian Boyd's https://stackoverflow.com/questions/20370920/convert-current-time-from-windows-to-unix-timestamp-in-c-or-c
    HANDLE hFile1;
    FILETIME filetime;
    hFile1 = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile1 == INVALID_HANDLE_VALUE)
    {
        return;
    }

    if (!GetFileTime(hFile1, &filetime, NULL, NULL))
    {
        CloseHandle(hFile1);
        return;
    }
    const int64_t UNIX_TIME_START = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
    const int64_t TICKS_PER_SECOND = 10000000; //a tick is 100ns

    //Copy the low and high parts of FILETIME into a LARGE_INTEGER
    //This is so we can access the full 64-bits as an Int64 without causing an alignment fault
    LARGE_INTEGER li;
    li.LowPart = filetime.dwLowDateTime;
    li.HighPart = filetime.dwHighDateTime;

    //Convert ticks since 1/1/1970 into seconds
    time_t time_s = (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;

    timestamp = *std::localtime(&time_s);
    CloseHandle(hFile1);
#elif __linux__
    struct stat buf;
    if (stat(filepath.c_str(), &buf) != 0)
    {
        return;
    }
    auto timet = static_cast<time_t>(buf.st_mtim.tv_sec);
    timestamp = *std::localtime(&timet);
#endif
}
