#pragma once

#include <filesystem>
#include <vector>
#include <istream>

namespace zipper {

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc);
bool isLargeFile(std::istream& input_stream);
bool makedir(std::filesystem::path newdir);
std::vector<std::filesystem::path> filesFromDirectory(const std::filesystem::path& path);

} // namespace zipper
