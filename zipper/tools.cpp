#include "tools.h"

#if defined(USE_QT)

#  include <QtCore/QDir>
#  include <QtCore/QDirIterator>

#elif defined(USE_BOOST)

#  include <boost/filesystem.hpp>
   namespace fs = boost::filesystem;

#elif defined(_MSC_VER) && (_MSC_VER >= 1800) && (_MSC_VER < 1900)

  namespace fs = std::tr2::sys;

#elif defined(_MSC_VER) && (_MSC_VER >= 1900)

#  define USE_NEW_FS_API
   namespace fs = std::experimental::filesystem;

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

  input_stream.clear();
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
#if defined(USE_QT)
  QFile file(filename.c_str());
  return file.exists();
#elif defined(_MSC_VER) && (_MSC_VER >= 1800)
  auto file = fs::path(filename);
  return fs::exists(file);
#else
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
#endif
}

bool makedir(const std::string& newdir)
{
#if defined(USE_QT)
  QDir dir;
  return dir.mkpath(newdir.c_str());
#else
  auto path = fs::path(newdir);
#ifdef USE_BOOST
  if (newdir.empty())
    return false;
#endif
  return fs::create_directories(path);
#endif

}

void removeFolder(const std::string& foldername)
{
#if defined(USE_QT)
  QDir dir(foldername.c_str());
  if (dir.exists())
  {
    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
    {
      if (info.isDir()) {
        removeFolder(info.absoluteFilePath().toStdString());
      }
      else {
        QFile::remove(info.absoluteFilePath());
      }

    }
  }
  QDir().rmpath(foldername.c_str());
#else
  auto folder = fs::path(foldername);
  fs::remove_all(folder);
#endif
}

bool isDirectory(const std::string& path)
{
#if defined(USE_QT)
  QFileInfo info(path.c_str());
  return info.isDir();
#else
  auto file = fs::path(path);
  return fs::is_directory(file);
#endif
}

std::string parentDirectory(const std::string& filepath)
{
#if defined(USE_QT)
  QFileInfo info(filepath.c_str());
  return info.dir().absolutePath().toStdString();
#elif defined(USE_BOOST)
  return fs::path(filepath).branch_path().string();
#elif defined(USE_NEW_FS_API)
  return fs::path(filepath).parent_path().string();
#else
  return fs::path(filepath).branch_path();
#endif
}

std::string currentPath()
{
#if defined(USE_QT)
  return QDir::currentPath().toStdString();
#elif defined(USE_BOOST) || defined(USE_NEW_FS_API)
  return fs::current_path().string();
#else
  return fs::current_path<fs::path>().string();
#endif
}

std::vector<std::string> filesFromDirectory(const std::string& path)
{
  std::vector<std::string> files;
#if defined(USE_QT)
  QDirIterator it(path.c_str(), QStringList() << "*.*", QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext())
    files.push_back(it.next().toStdString());
#else

  auto folder = fs::path(path);
  auto it = fs::recursive_directory_iterator(folder);
  for (it; it != fs::recursive_directory_iterator(); ++it)
  {
    // file object contains absolute path in the case of recursive iterators
    const auto& file = it->path();
    if (!fs::is_directory(file))
#if defined(USE_BOOST) || defined(USE_NEW_FS_API)
      files.push_back(file.string());
#else
      files.push_back(file);
#endif
  }
#endif

  return files;
}

std::string fileNameFromPath(const std::string& fullPath)
{
#if defined(USE_QT)
  return QFile(fullPath.c_str()).fileName().toStdString();
#elif defined(USE_BOOST) || defined(USE_NEW_FS_API)
  return fs::path(fullPath).filename().string();
#else
  return fs::path(fullPath).filename();
#endif
}
