#include "tools.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1800)
namespace fs = std::tr2::sys;
#else
namespace fs = std::experimental::filesystem;
#endif

/* calculate the CRC32 of a file,
because to encrypt a file, we need known the CRC32 of the file before */

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
	unsigned long calculate_crc = 0;
	unsigned long size_read = 0;
	unsigned long total_read = 0;

	do {
		input_stream.read(buff.data(), buff.size());
		size_read = (unsigned long)input_stream.gcount();

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

void changeFileDate(const std::string& filename, uLong dosdate, tm_unz tmu_date)
{
#ifdef _WIN32
	HANDLE hFile;
	FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

	hFile = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
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
	utime(filename.c_str(), &ut);
#endif
#endif
}


bool checkFileExists(const std::string& filename)
{
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
	auto file = fs::path(filename);
	return fs::exists(file);
#else
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
#endif
}

bool makedir(const std::string& newdir)
{
	auto path = fs::path(newdir);
	return fs::create_directories(path);
}

void removeFolder(const std::string& foldername)
{
	auto folder = fs::path(foldername);
	fs::remove_all(folder);
}

bool isDirectory(const std::string& path)
{
	auto file = fs::path(path);
	return fs::is_directory(file);
}

std::string parentDirectory(const std::string& filepath)
{
	return fs::path(filepath).branch_path();
}

std::string currentPath()
{
	return fs::current_path<fs::path>().string();
}

std::vector<std::string> filesFromDirectory(const std::string& path)
{
	std::vector<std::string> files;

	auto folder = fs::path(path);
	auto it = fs::recursive_directory_iterator(folder);
	for (it; it != fs::recursive_directory_iterator(); ++it)
	{
		// file object contains absolute path in the case of recursive iterators
		const auto& file = it->path();
		if (!fs::is_directory(file))
			files.push_back(file);
	}	
	
	return files;
}

std::string fileNameFromPath(const std::string& path)
{
	return fs::path(path).filename();
}