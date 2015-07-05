#pragma once

#include "defs.h"

#include <vector>
#include <istream>

/* calculate the CRC32 of a file,
because to encrypt a file, we need known the CRC32 of the file before */

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
	unsigned long calculate_crc = 0;
	unsigned long size_read = 0;
	unsigned long total_read = 0;

	do {
		input_stream.read(buff.data(), buff.size());
		size_read = input_stream.gcount();

		if (size_read>0)
			calculate_crc = crc32(calculate_crc, (const unsigned char*)buff.data(), size_read);

		total_read += size_read;

	} while (size_read>0);

	input_stream.seekg(0);
	result_crc = calculate_crc;
}

bool isLargeFile(std::istream& input_stream)
{
	ZPOS64_T pos = 0;
	input_stream.seekg(0, std::ios::end);
	pos = input_stream.tellg();
	input_stream.seekg(0);

	return pos >= 0xffffffff;
}

void change_file_date(const char *filename, uLong dosdate, tm_unz tmu_date)
{
#ifdef _WIN32
	HANDLE hFile;
	FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

	hFile = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
		DosDateTimeToFileTime((WORD)(dosdate >> 16), (WORD)dosdate, &ftLocal);
		LocalFileTimeToFileTime(&ftLocal, &ftm);
		SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
		CloseHandle(hFile);
	}
#else
#if defined unix || defined __APPLE__
	struct utimbuf ut;
	struct tm newdate;

	newdate.tm_sec = tmu_date.tm_sec;
	newdate.tm_min = tmu_date.tm_min;
	newdate.tm_hour = tmu_date.tm_hour;
	newdate.tm_mday = tmu_date.tm_mday;
	newdate.tm_mon = tmu_date.tm_mon;
	if (tmu_date.tm_year > 1900)
		newdate.tm_year = tmu_date.tm_year - 1900;
	else
		newdate.tm_year = tmu_date.tm_year;
	newdate.tm_isdst = -1;

	ut.actime = ut.modtime = mktime(&newdate);
	utime(filename, &ut);
#endif
#endif
}


bool check_file_exists(const char* filename)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
	return std::tr2::sys::exists(filename);
#else
	struct stat buffer;
	return (stat(filename, &buffer) == 0);
#endif
}

bool makedir(const char *newdir)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
	return std::tr2::sys::create_directories(newdir);
#else
	char *buffer = NULL;
	char *p = NULL;
	int len = (int)strlen(newdir);

	if (len <= 0)
		return true;

	buffer = (char*)malloc(len + 1);
	if (buffer == NULL)
	{
		printf("Error allocating memory\n");
		return false;
	}

	strcpy(buffer, newdir);

	if (buffer[len - 1] == '/')
		buffer[len - 1] = 0;

	if (MKDIR(buffer) == 0)
	{
		free(buffer);
		return false;
	}

	p = buffer + 1;
	while (1)
	{
		char hold;
		while (*p && *p != '\\' && *p != '/')
			p++;
		hold = *p;
		*p = 0;

		if ((MKDIR(buffer) == -1) && (errno == ENOENT))
		{
			printf("couldn't create directory %s (%d)\n", buffer, errno);
			free(buffer);
			return true;
		}

		if (hold == 0)
			break;

		*p++ = hold;
	}

	free(buffer);
	return false;
#endif
}
