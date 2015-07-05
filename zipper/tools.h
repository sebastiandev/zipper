#pragma once

#include "defs.h"

#include <vector>
#include <istream>

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc);
bool isLargeFile(std::istream& input_stream);
void change_file_date(const char *filename, uLong dosdate, tm_unz tmu_date);
bool check_file_exists(const char* filename);
bool makedir(const char *newdir);




