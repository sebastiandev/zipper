#include "tools.h"
#include "defs.h"
#include <algorithm>
#include <filesystem>
#include <iterator>

#include <cstdio>
#include <iostream>

#if defined(USE_WINDOWS)
#    include "tps/dirent.h"
#    include "tps/dirent.c"
#else
#    include <sys/types.h>
#    include <dirent.h>
#    include <unistd.h>
#endif /* WINDOWS */

namespace zipper {

// -----------------------------------------------------------------------------
// Calculate the CRC32 of a file because to encrypt a file, we need known the
// CRC32 of the file before.
void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
    unsigned long calculate_crc = 0;
    unsigned long size_read = 0;
    unsigned long total_read = 0;

    do
    {
        input_stream.read(buff.data(), buff.size());
        size_read = (unsigned long)input_stream.gcount();

        if (size_read > 0)
        {
            calculate_crc = crc32(calculate_crc, (const unsigned char*)buff.data(), size_read);
        }

        total_read += size_read;

    } while (size_read > 0);

    input_stream.clear();
    input_stream.seekg(0, std::ios_base::beg);
    result_crc = calculate_crc;
}

// -----------------------------------------------------------------------------
bool isLargeFile(std::istream& input_stream)
{
    ZPOS64_T pos = 0;
    input_stream.seekg(0, std::ios::end);
    pos = input_stream.tellg();
    input_stream.seekg(0);

    return pos >= 0xffffffff;
}

bool makedir(std::filesystem::path newdir)
{
    newdir = newdir.lexically_normal();

    if (newdir.empty())
    {
        return false;
    }

    return std::filesystem::create_directories(newdir);
}

// -----------------------------------------------------------------------------
std::vector<std::filesystem::path> filesFromDirectory(const std::filesystem::path& path)
{
    std::vector<std::filesystem::path> files;

    for (auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
	if(!entry.is_directory())
	{
	    files.emplace_back(entry.path());
	}
    }

    return files;
}

} // namespace zipper
