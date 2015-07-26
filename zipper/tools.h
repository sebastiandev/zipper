#pragma once

#include "defs.h"

#include <vector>
#include <istream>

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc);
bool isLargeFile(std::istream& input_stream);
void changeFileDate(const std::string& filename, uLong dosdate, tm_unz tmu_date);
bool checkFileExists(const std::string& filename);
bool makedir(const std::string& newdir);
void removeFolder(const std::string& foldername);
std::string parentDirectory(const std::string& filepath);
std::string currentPath();
bool isDirectory(const std::string& path);
std::vector<std::string> filesFromDirectory(const std::string& path);
std::string fileNameFromPath(const std::string& path);



