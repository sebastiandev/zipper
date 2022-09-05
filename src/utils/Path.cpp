// -----------------------------------------------------------------------------
// Copyright (C) 2010 - 2014 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., University of Heidelberg, and The University
// of Manchester.
// All rights reserved.
//
// Copyright (C) 2008 - 2009 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., EML Research, gGmbH, University of Heidelberg,
// and The University of Manchester.
// All rights reserved.
//
// Copyright (C) 2005 - 2007 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc. and EML Research, gGmbH.
// All rights reserved.
// -----------------------------------------------------------------------------

#include "utils/OS.hpp"
#include "utils/Path.hpp"
#include <fstream>
#include <iterator>
#include <sstream>

#if defined(USE_WINDOWS)
#    include "utils/dirent.h"
#    include "utils/dirent.c"
#else
#    include <sys/types.h>
#    include <dirent.h>
#    include <unistd.h>
#endif /* WINDOWS */

using namespace zipper;

const std::string Path::Separator(DIRECTORY_SEPARATOR);

// -----------------------------------------------------------------------------
std::string Path::currentPath()
{
    char buffer[1024u];
    return (getcwd(buffer, sizeof(buffer)) ? std::string(buffer) : std::string(""));
}

// -----------------------------------------------------------------------------
bool Path::isFile(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(USE_WINDOWS)
    return ((st.st_mode & S_IFREG) == S_IFREG);
#else
    return S_ISREG(st.st_mode);
#endif
}

// -----------------------------------------------------------------------------
bool Path::isDir(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(USE_WINDOWS)
    return ((st.st_mode & S_IFDIR) == S_IFDIR);
#else
    return S_ISDIR(st.st_mode);
#endif
}

// -----------------------------------------------------------------------------
bool Path::exist(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(USE_WINDOWS)
    return ((st.st_mode & S_IFREG) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR);
#else
    return (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode));
#endif
}

// -----------------------------------------------------------------------------
bool Path::isReadable(const std::string& path)
{
    return (access(path.c_str(), 0x4) == 0);
}

// -----------------------------------------------------------------------------
bool Path::isWritable(const std::string& path)
{
    return (access(path.c_str(), 0x2) == 0);
}

// -----------------------------------------------------------------------------
std::string Path::baseName(const std::string& path)
{
    std::string::size_type start = path.find_last_of(Separator);

#if defined(USE_WINDOWS) // WIN32 also understands '/' as the separator.
    if (start == std::string::npos)
    {
        start = path.find_last_of("/");
    }
#endif

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    std::string::size_type end = path.find_last_of(".");

    if (end == std::string::npos || end < start)
    {
        end = path.length();
    }

    return path.substr(start, end - start);
}

// -----------------------------------------------------------------------------
std::string Path::fileName(const std::string& path)
{
    std::string::size_type start = path.find_last_of(Separator);

#if defined(USE_WINDOWS) // WIN32 also understands '/' as the separator.
    if (start == std::string::npos)
    {
        start = path.find_last_of("/");
    }
#endif

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    return path.substr(start);
}

// -----------------------------------------------------------------------------
// WIN32 also understands '/' as the separator.
std::string Path::dirName(const std::string& path)
{
    if (path.empty())
        return path;

#if defined(USE_WINDOWS)
    std::string::size_type end = path.find_last_of(Separator + "/");
#else
    std::string::size_type end = path.find_last_of(Separator);
#endif

    if (end == path.length() - 1)
    {
#if defined(USE_WINDOWS)
        end = path.find_last_of(Separator + "/", end - 1);
#else
        end = path.find_last_of(Separator, end - 1);
#endif
    }

    if (end == std::string::npos)
        return {};

    return path.substr(0, end);
}

// -----------------------------------------------------------------------------
// Note: WIN32 also understands '/' as the separator.
std::string Path::suffix(const std::string& path)
{
    std::string::size_type start = path.find_last_of(Separator);

#if defined(USE_WINDOWS)
    if (start == std::string::npos)
    {
        start = path.find_last_of("/");
    }
#endif

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    std::string::size_type end = path.find_last_of(".");

    if (end == std::string::npos || end < start)
        return {};

    return path.substr(end);
}

// -----------------------------------------------------------------------------
bool Path::createDir(const std::string& dir, const std::string& parent)
{
    std::string Dir;

    if (!parent.empty())
    {
        Dir = parent + Separator;
    }

    Dir += dir;

    // Check whether the directory already exists and is writable.
    if (isDir(Dir) && isWritable(Dir))
        return true;

    // Check whether the parent directory exists and is writable.
    if (!parent.empty() && (!isDir(parent) || !isWritable(parent)))
        return false;

    Dir = normalize(Dir);

    // ensure we have parent
    std::string actualParent = dirName(Dir);

    if (!actualParent.empty() && (!exist(actualParent)))
    {
        createDir(actualParent);
    }

#if defined(USE_WINDOWS)
    return (mkdir(Dir.c_str()) == 0);
#else
    return (mkdir(Dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
#endif
}

// -----------------------------------------------------------------------------
void Path::removeDir(const std::string& foldername)
{
    if (!Path::remove(foldername))
    {
        std::vector<std::string> files = Path::filesFromDir(foldername);
        std::vector<std::string>::iterator it = files.begin();
        for (; it != files.end(); ++it)
        {
            if (Path::isDir(*it) && *it != foldername)
            {
                Path::removeDir(*it);
            }
            else
            {
                std::remove(it->c_str());
            }
        }

        Path::remove(foldername);
    }
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::filesFromDir(const std::string& path)
{
    std::vector<std::string> files;
    DIR* dir;
    struct dirent* entry;

    dir = opendir(path.c_str());

    if (dir == nullptr)
        return files;

    for (entry = readdir(dir); entry != nullptr; entry = readdir(dir))
    {
        std::string filename(entry->d_name);

        if (filename == "." || filename == "..") continue;

        if (Path::isDir(path + Path::Separator + filename))
        {
            std::vector<std::string> moreFiles =
                    Path::filesFromDir(path + Path::Separator + filename);
            std::copy(moreFiles.begin(), moreFiles.end(),
                      std::back_inserter(files));
            continue;
        }

        files.push_back(path + Path::Separator + filename);
    }

    closedir(dir);

    return files;
}

// -----------------------------------------------------------------------------
std::string Path::createTmpName(const std::string& dir, const std::string& suffix)
{
    std::string RandomName;

    do
    {
        RandomName = dir + Separator;
        int Char;

        for (size_t i = 0; i < 8u; i++)
        {
            Char = static_cast<int>((rand() / double(RAND_MAX)) * 35.0);

            if (Char < 10)
            {
                RandomName += char('0' + Char);
            }
            else
            {
                RandomName += char('a' - 10 + Char);
            }
        }

        RandomName += suffix;
    } while (exist(RandomName));

    return RandomName;
}

// -----------------------------------------------------------------------------
bool Path::move(const std::string& from, const std::string& to)
{
    if (!isFile(from))
        return false;

    std::string To = to;

    // Check whether To is a directory and append the
    // filename of from
    if (isDir(To))
        To += Separator + fileName(from);

    if (isDir(To))
        return false;

#if defined(USE_WINDOWS)

    // The target must not exist under WIN32 for rename to succeed.
    if (exist(To) && !remove(To))
        return false;

#endif // WIN32

    bool success = (::rename(from.c_str(), To.c_str()) == 0);

    if (!success)
    {
        {
            std::ifstream in(from.c_str());
            std::ofstream out(To.c_str());

            out << in.rdbuf();

            success = out.good();
        }

        remove(from);
    }

    return success;
}

// -----------------------------------------------------------------------------
bool Path::remove(const std::string& path)
{
    if (isDir(path))
        return (rmdir(path.c_str()) == 0);

    else if (isFile(path))
#if defined(USE_WINDOWS)
        return (::remove(path.c_str()) == 0);
#else
    return (::remove(path.c_str()) == 0);
#endif

    return false;
}

// -----------------------------------------------------------------------------
bool Path::removeFiles(const std::string& pattern,
                       const std::string& path)
{
    bool success = true;
    std::vector<std::string> PatternList;

    PatternList = compilePattern(pattern);

#if defined(USE_WINDOWS)

    // We want the same pattern matching behaviour for all platforms.
    // Therefore, we do not use the MS provided one and list all files instead.
    std::string FilePattern = path + "\\*";

    // Open directory stream and try read info about first entry
    struct _finddata_t Entry;
    intptr_t hList = _findfirst(FilePattern.c_str(), &Entry);

    if (hList == -1)
        return success;

    do
    {
        std::string Utf8 = Entry.name;

        if (match(Utf8, PatternList))
        {
            if (Entry.attrib | _A_NORMAL)
            {
                if (::remove((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (rmdir((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    } while (_findnext(hList, &Entry) == 0);

    _findclose(hList);

#else //! USE_WINDOWS

    DIR* pDir = opendir(path.c_str());

    if (!pDir) return false;

    struct dirent* pEntry;

    while ((pEntry = readdir(pDir)) != nullptr)
    {
        std::string Utf8 = pEntry->d_name;

        if (match(Utf8, PatternList))
        {
            if (isDir(Utf8))
            {
                if (::rmdir((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (::remove((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    }

    closedir(pDir);

#endif // USE_WINDOWS

    return success;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::compilePattern(const std::string& pattern)
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    std::vector<std::string> PatternList;

    while (pos != std::string::npos)
    {
        start = pos;
        pos = pattern.find_first_of("*?", pos);

        end = (std::min)(pos, pattern.length());

        if (start != end)
        {
            PatternList.push_back(pattern.substr(start, end - start));
        }
        else
        {
            PatternList.push_back(pattern.substr(start, 1));
            pos++;
        }
    };

    return PatternList;
}

// -----------------------------------------------------------------------------
bool Path::match(const std::string& name,
                 const std::vector<std::string>& patternList)
{
    std::vector<std::string>::const_iterator it = patternList.begin();
    std::vector<std::string>::const_iterator end = patternList.end();
    std::string::size_type at = 0;
    std::string::size_type after = 0;

    bool Match = true;

    while (it != end && Match)
    {
        Match = matchInternal(name, *it++, at, after);
    }

    return Match;
}

// -----------------------------------------------------------------------------
bool Path::isRelativePath(const std::string& path)
{
#if defined(USE_WINDOWS)

    std::string Path = normalize(path);

    if (Path.length() < 2)
        return true;

    if (Path[1] == ':')
        return false;

    if (Path[0] == '/' && Path[1] == '/')
        return false;

    return true;

#else //! USE_WINDOWS

    return (path.length() < 1 || path[0] != '/');

#endif // USE_WINDOWS
}

// -----------------------------------------------------------------------------
bool Path::makePathRelative(std::string& absolutePath, const std::string& relativeTo)
{
    if (isRelativePath(absolutePath) || isRelativePath(relativeTo))
        return false; // Nothing can be done.

    std::string RelativeTo = normalize(relativeTo);

    if (isFile(RelativeTo))
        RelativeTo = dirName(RelativeTo);

    if (!isDir(RelativeTo))
        return false;

    absolutePath = normalize(absolutePath);

    size_t i, imax = (std::min)(absolutePath.length(), RelativeTo.length());

    for (i = 0; i < imax; i++)
    {
        if (absolutePath[i] != RelativeTo[i])
            break;
    }

    // We need to retract to the beginning of the current directory.
    if (i != imax)
    {
        i = absolutePath.find_last_of('/', i) + 1;
    }

#if defined(USE_WINDOWS)

    if (i == 0)
    {
        return false; // A different drive letter we cannot do anything
    }

#endif // USE_WINDOWS

    RelativeTo = RelativeTo.substr(i);

    std::string relativePath;

    while (RelativeTo != "")
    {
        relativePath += "../";
        RelativeTo = dirName(RelativeTo);
    }

    if (relativePath != "")
    {
        absolutePath = relativePath + absolutePath.substr(i);
    }
    else
    {
        absolutePath = absolutePath.substr(i + 1);
    }

    return true;
}

// -----------------------------------------------------------------------------
bool Path::makePathAbsolute(std::string& relativePath, const std::string& absoluteTo)
{
    if (!isRelativePath(relativePath) || isRelativePath(absoluteTo))
        return false; // Nothing can be done.

    std::string AbsoluteTo = normalize(absoluteTo);

    if (isFile(AbsoluteTo))
        AbsoluteTo = dirName(AbsoluteTo);

    if (!isDir(AbsoluteTo))
        return false;

    relativePath = normalize(relativePath);

    while (!relativePath.compare(0, 3, "../"))
    {
        AbsoluteTo = dirName(AbsoluteTo);
        relativePath = relativePath.substr(3);
    }

    relativePath = AbsoluteTo + "/" + relativePath;

    return true;
}

// -----------------------------------------------------------------------------
bool Path::matchInternal(const std::string& name,
                         const std::string pattern,
                         std::string::size_type& at,
                         std::string::size_type& after)
{
    bool Match = true;

    switch (pattern[0])
    {
    case '*':
        if (at != std::string::npos)
        {
            after = at;
            at = std::string::npos;
        }
        break;

    case '?':
        if (at != std::string::npos)
        {
            ++at;
            Match = (name.length() >= at);
        }
        else
        {
            ++after;
            Match = (name.length() >= after);
        }
        break;

    default:
        if (at != std::string::npos)
        {
            Match = (name.compare(at, pattern.length(), pattern) == 0);
            at += pattern.length();
        }
        else
        {
            at = name.find(pattern, after);
            Match = (at != std::string::npos);
            at += pattern.length();
        }
        break;
    }

    return Match;
}

// -----------------------------------------------------------------------------
std::string Path::normalize(const std::string& path)
{
    std::string Normalized = path;

#if defined(USE_WINDOWS)
    // converts all '\' to '/' (only on WIN32)
    size_t i, imax;

    for (i = 0, imax = Normalized.length(); i < imax; i++)
    {
        if (Normalized[i] == '\\')
            Normalized[i] = '/';
    }

#endif

    // Remove leading './'
    while (!Normalized.compare(0, 2, "./"))
    {
        Normalized = Normalized.substr(2);
    }

    // Collapse '//' to '/'
    std::string::size_type pos = 1;

    while (true)
    {
        pos = Normalized.find("//", pos);
        if (pos == std::string::npos)
            break;

        Normalized.erase(pos, 1);
    }

    // Collapse '/./' to '/'
    pos = 0;

    while (true)
    {
        pos = Normalized.find("/./", pos);
        if (pos == std::string::npos)
            break;

        Normalized.erase(pos, 2);
    }

    // Collapse '[^/]+/../' to '/'
    std::string::size_type start = Normalized.length();

    while (true)
    {
        pos = Normalized.rfind("/../", start);
        if (pos == std::string::npos)
            break;

        start = Normalized.rfind('/', pos - 1);
        if (start == std::string::npos)
            break;

        if (!Normalized.compare(start, 4, "/../"))
            continue;

        Normalized.erase(start, pos - start + 3);
        start = Normalized.length();
    }

    return Normalized;
}

std::string Path::canonicalPath(const std::string& path)
{
    if (path.empty())
        return {};

    std::vector<std::string> segments;

    // If the path starts with a / we must preserve it
    bool starts_with_slash = path.front() == DIRECTORY_SEPARATOR_CHAR;
    // If the path does not end with a / we need to remove the
    // extra / added by the join process
    bool ends_with_slash = path.back() == DIRECTORY_SEPARATOR_CHAR;

    size_t current;
    size_t next;

    do {
        current = size_t(next + 1);
        // Handle both Unix and Windows style separators
        next = path.find_first_of("/\\", current);

        if (next == std::string::npos)
            return path;

        std::string segment(path.substr(current, next - current));
        size_t size = segment.length();

        // skip empty (keedp initial)
        if (size == 0 && segments.size() > 0)
        {
            continue;
        }

        // skip . (keep initial)
        if (segment == "." && segments.size() > 0)
        {
            continue;
        }

        // remove ..
        if (segment == ".." && segments.size() > 0)
        {
            if (segments.back().empty())
            {
                // ignore if .. follows initial /
                continue;
            }
            if (segments.back() != "..")
            {
                segments.pop_back();
                continue;
            }
        }

        segments.push_back(segment);
    } while (next != std::string::npos);

    // Join the vector as a single string, every element is separated by a
    // '/'. This process adds an extra / at the end that might need to be
    // removed
    std::stringstream clean_path;
    std::copy(segments.begin(), segments.end(),
              std::ostream_iterator<std::string>(clean_path,
                                                 DIRECTORY_SEPARATOR));
    std::string new_path = clean_path.str();

    if (starts_with_slash && new_path.empty())
    {
        new_path = DIRECTORY_SEPARATOR;
    }

    if (!ends_with_slash && new_path.length() > 1)
    {
        new_path.pop_back();
    }

    return new_path;
}

// -----------------------------------------------------------------------------
bool Path::isLargeFile(std::istream& input_stream)
{
    std::streampos pos = 0;
    input_stream.seekg(0, std::ios::end);
    pos = input_stream.tellg();
    input_stream.seekg(0);

    return pos >= 0xffffffff;
}
