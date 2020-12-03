#include "unzipper.h"
#include "defs.h"
#include "tools.h"

#include <filesystem>
#include <functional>
#include <exception>
#include <fstream>
#include <stdexcept>

namespace zipper {

struct Unzipper::Impl
{
    Unzipper& m_outer;
    zipFile m_zf;
    ourmemory_t m_zipmem;
    zlib_filefunc_def m_filefunc;

private:
    bool initMemory(zlib_filefunc_def& filefunc)
    {
        m_zf = unzOpen2("__notused__", &filefunc);
        return m_zf != nullptr;
    }

    [[nodiscard]] bool locateEntry(const std::string& name) const
    {
        return UNZ_OK == unzLocateFile(m_zf, name.c_str(), nullptr);
    }

    [[nodiscard]] ZipEntry currentEntryInfo() const
    {
        unz_file_info64 file_info = { 0 };
        char filename_inzip[256] = { 0 };

        int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
        if (UNZ_OK != err)
        {
            throw std::runtime_error("Error, couln't get the current entry info");
        }

        return ZipEntry(std::string(filename_inzip), file_info.compressed_size, file_info.uncompressed_size,
                        file_info.tmu_date.tm_year, file_info.tmu_date.tm_mon, file_info.tmu_date.tm_mday,
                        file_info.tmu_date.tm_hour, file_info.tmu_date.tm_min, file_info.tmu_date.tm_sec, file_info.dosDate);
    }

    void getEntries(std::vector<ZipEntry>& entries)
    {
        int err = unzGoToFirstFile(m_zf);
        if (UNZ_OK == err)
        {
            do
            {
                ZipEntry entryinfo = currentEntryInfo();

                if (entryinfo.valid())
                {
                    entries.push_back(entryinfo);
                    err = unzGoToNextFile(m_zf);
                }
                else
                {
                    err = UNZ_ERRNO;
                }

            } while (UNZ_OK == err);

            if (UNZ_END_OF_LIST_OF_FILE != err && UNZ_OK != err)
            {
                return;
            }
        }
    }


public:

    bool extractCurrentEntryToFile(ZipEntry& entryinfo, const std::string& fileName)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
        {
            return false;
        }

        err = extractToFile(fileName, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " openinginternal file '"
                    << entryinfo.name << "' in zip";

                throw std::runtime_error(str.str());
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToStream(ZipEntry& entryinfo, std::ostream& stream)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
        {
            return false;
        }

        err = extractToStream(stream, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " opening internal file '"
                    << entryinfo.name << "' in zip";

                throw std::runtime_error(str.str());
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToMemory(ZipEntry& entryinfo, std::vector<unsigned char>& outvec)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
        {
            return false;
        }

        err = extractToMemory(outvec, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " opening internal file '"
                    << entryinfo.name << "' in zip";

                throw std::runtime_error(str.str());
            }
        }

        return UNZ_OK == err;
    }

    static void changeFileDate(const std::string& filename, uLong dosdate, tm_unz tmu_date)
    {
#if defined(USE_WINDOWS)
        HANDLE hFile;
        FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

        hFile = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
            DosDateTimeToFileTime((WORD)(dosdate >> 16), (WORD)dosdate, &ftLocal);
            LocalFileTimeToFileTime(&ftLocal, &ftm);
            SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
            CloseHandle(hFile);
        }
#else
#    if defined unix || defined __APPLE__
        struct utimbuf ut;
        struct tm newdate;

        newdate.tm_sec = tmu_date.tm_sec;
        newdate.tm_min = tmu_date.tm_min;
        newdate.tm_hour = tmu_date.tm_hour;
        newdate.tm_mday = tmu_date.tm_mday;
        newdate.tm_mon = tmu_date.tm_mon;
        if (tmu_date.tm_year > 1900)
        {
            newdate.tm_year = tmu_date.tm_year - 1900;
        }
        else
        {
            newdate.tm_year = tmu_date.tm_year;
        }
        newdate.tm_isdst = -1;

        ut.actime = ut.modtime = mktime(&newdate);
        utime(filename.c_str(), &ut);
#    endif
#endif
    }


    int extractToFile(const std::filesystem::path& filename, ZipEntry& info)
    {
        int err = UNZ_ERRNO;

        /* If zip entry is a directory then create it on disk */
        makedir(filename.parent_path());

        /* Create the file on disk so we can unzip to it */
        std::ofstream output_file(filename.c_str(), std::ofstream::binary);

        if (output_file.good())
        {
            if (UNZ_OK == extractToStream(output_file, info))
            {
                err = UNZ_OK;
            }

            output_file.close();

            /* Set the time of the file that has been unzipped */
            tm_unz timeaux;
            memcpy(&timeaux, &info.unixdate, sizeof(timeaux));

            changeFileDate(filename.string(), info.dosdate, timeaux);
        }
        else
        {
            output_file.close();
        }

        return err;
    }

    int extractToStream(std::ostream& stream, ZipEntry& info)
    {
        size_t err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            throw std::runtime_error(str.str());
        }

        std::vector<char> buffer;
        buffer.resize(WRITEBUFFERSIZE);

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(), (unsigned int)buffer.size());
            if (err < 0 || err == 0)
            {
                break;
            }

            stream.write(buffer.data(), err);
            if (!stream.good())
            {
                err = UNZ_ERRNO;
                break;
            }

        } while (err > 0);

        stream.flush();

        return (int)err;
    }

    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info)
    {
        size_t err = UNZ_ERRNO;

        err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            throw std::runtime_error(str.str());
        }

        std::vector<unsigned char> buffer;
        buffer.resize(WRITEBUFFERSIZE);

        outvec.reserve((size_t)info.uncompressedSize);

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(), (unsigned int)buffer.size());
            if (err < 0 || err == 0)
            {
                break;
            }

            outvec.insert(outvec.end(), buffer.data(), buffer.data() + err);

        } while (err > 0);

        return (int)err;
    }

    Impl(Unzipper& outer)
        : m_outer(outer), m_zipmem(), m_filefunc()
    {
        m_zipmem.base = nullptr;
        m_zf = nullptr;
    }

    ~Impl()
    {
        close();
    }

    void close()
    {
        if (m_zf != nullptr)
        {
            unzClose(m_zf);
            m_zf = nullptr;
        }

        if (m_zipmem.base != nullptr)
        {
            free(m_zipmem.base);
            m_zipmem.base = nullptr;
        }
    }

    bool initFile(const std::string& filename)
    {
#ifdef USEWIN32IOAPI
        zlib_filefunc64_def ffunc;
        fill_win32_filefunc64A(&ffunc);
        m_zf = unzOpen2_64(filename.c_str(), &ffunc);
#else
        m_zf = unzOpen64(filename.c_str());
#endif
        return m_zf != nullptr;
    }

    bool initWithStream(std::istream& stream)
    {
        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        if (s < 0)
        {
            return false;
        }
        auto size = static_cast<size_t>(s);
        stream.seekg(0);

        if (size > 0)
        {
            m_zipmem.base = new char[(size_t)size];
            m_zipmem.size = (uLong)size;
            stream.read(m_zipmem.base, size);
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(m_filefunc);
    }

    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        if (!buffer.empty())
        {
            m_zipmem.base = (char*)malloc(buffer.size() * sizeof(char));
            memcpy(m_zipmem.base, (char*)buffer.data(), buffer.size());
            m_zipmem.size = (uLong)buffer.size();
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(m_filefunc);
    }

    std::vector<ZipEntry> entries()
    {
        std::vector<ZipEntry> entrylist;
        getEntries(entrylist);
        return entrylist;
    }


    bool extractAll(const std::filesystem::path& destination, const std::map<std::string, std::string>& alternativeNames)
    {
        std::vector<ZipEntry> entries;
        getEntries(entries);

        for (auto& entrie : entries)
        {
            if (!locateEntry(entrie.name))
            {
                continue;
            }

            std::filesystem::path alternativeName = destination;

            if (alternativeNames.find(entrie.name) != alternativeNames.end())
            {
                alternativeName /= alternativeNames.at(entrie.name);
            }
            else
            {
                alternativeName /= entrie.name;
            }

            extractCurrentEntryToFile(entrie, alternativeName.string());
        };

        return true;
    }

    bool extractEntry(const std::string& name, const std::filesystem::path& destination)
    {
        std::filesystem::path outputFile = destination / name;

        if (locateEntry(name))
        {
            ZipEntry entry = currentEntryInfo();
            return extractCurrentEntryToFile(entry, outputFile.string());
        }

        return false;
    }

    bool extractEntryToStream(const std::string& name, std::ostream& stream)
    {
        if (locateEntry(name))
        {
            ZipEntry entry = currentEntryInfo();
            return extractCurrentEntryToStream(entry, stream);
        }

        return false;
    }

    bool extractEntryToMemory(const std::string& name, std::vector<unsigned char>& vec)
    {
        if (locateEntry(name))
        {
            ZipEntry entry = currentEntryInfo();
            return extractCurrentEntryToMemory(entry, vec);
        }

        return false;
    }
};

Unzipper::Unzipper(std::istream& zippedBuffer)
    : m_ibuffer(zippedBuffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_usingMemoryVector(false)
    , m_usingStream(true)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithStream(m_ibuffer))
    {
        release();
        throw std::runtime_error("Error loading zip in memory!");
    }
    m_open = true;
}

Unzipper::Unzipper(std::vector<unsigned char>& zippedBuffer)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(zippedBuffer)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        throw std::runtime_error("Error loading zip in memory!");
    }

    m_open = true;
}

Unzipper::Unzipper(const std::string& zipname)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_zipname(zipname)
    , m_usingMemoryVector(false)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initFile(zipname))
    {
        release();
        throw std::runtime_error("Error loading zip file!");
    }

    m_open = true;
}

Unzipper::Unzipper(const std::string& zipname, std::string password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_zipname(zipname)
    , m_password(std::move(password))
    , m_usingMemoryVector(false)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initFile(zipname))
    {
        release();
        throw std::runtime_error("Error loading zip file!");
    }
    m_open = true;
}

Unzipper::~Unzipper()
{
    close();
    release();
}

std::vector<ZipEntry> Unzipper::entries()
{
    return m_impl->entries();
}

bool Unzipper::extractEntry(const std::string& name, const std::filesystem::path& destination)
{
    return m_impl->extractEntry(name, destination);
}

bool Unzipper::extractEntryToStream(const std::string& name, std::ostream& stream)
{
    return m_impl->extractEntryToStream(name, stream);
}

bool Unzipper::extractEntryToMemory(const std::string& name, std::vector<unsigned char>& vec)
{
    return m_impl->extractEntryToMemory(name, vec);
}


bool Unzipper::extract(const std::filesystem::path& destination, const std::map<std::string, std::string>& alternativeNames)
{
    return m_impl->extractAll(destination, alternativeNames);
}

bool Unzipper::extract(const std::filesystem::path& destination)
{
    return m_impl->extractAll(destination, std::map<std::string, std::string>());
}

void Unzipper::release()
{
    if (!m_usingMemoryVector)
    {
        delete &m_vecbuffer;
    }
    if (!m_usingStream)
    {
        delete &m_ibuffer;
    }
    delete m_impl;
}

void Unzipper::close()
{
    if (m_open)
    {
        m_impl->close();
        m_open = false;
    }
}

} // namespace zipper
