#ifndef TIMESTAMP_HPP
#  define TIMESTAMP_HPP

#  if defined(USE_WINDOWS)
#    include <Windows.h>
#  elif __linux__
#    include <sys/stat.h>
#  endif

#  include <string>

namespace zipper {

// *****************************************************************************
//! \brief Creates a timestap either from file or just current time If it fails
//! to read the timestamp of a file, it set the time stamp to current time
//!
//! \warning It uses std::time to get current time, which is not standardized to
//! be 1970-01-01....  However, it works on Windows and Unix
//! https://stackoverflow.com/questions/6012663/get-unix-timestamp-with-c With
//! C++20 this will be standardized
// *****************************************************************************
struct Timestamp
{
    Timestamp();
    Timestamp(const std::string& filepath);

    tm timestamp;
};

} // namespace

#endif // TIMESTAMP_HPP
