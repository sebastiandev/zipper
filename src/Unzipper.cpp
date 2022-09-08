#include "Zipper/Unzipper.hpp"
#include "external/minizip/zip.h"
#include "external/minizip/unzip.h"
#include "external/minizip/ioapi_mem.h"
#include "utils/Path.hpp"

#include <functional>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <utime.h>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#  define ZIPPER_WRITE_BUFFER_SIZE (8192)
#endif

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

    bool locateEntry(const std::string& name)
    {
        return UNZ_OK == unzLocateFile(m_zf, name.c_str(), nullptr);
    }

    bool currentEntryInfo(ZipEntry &entry)
    {
        unz_file_info64 file_info;
        char filename_inzip[256] = { 0 };

        int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip,
                                          sizeof(filename_inzip), nullptr, 0,
                                          nullptr, 0);
        if (UNZ_OK != err)
            throw std::runtime_error("Error, couln't get the current entry info");

        entry = ZipEntry(std::string(filename_inzip), file_info.compressed_size,
                         file_info.uncompressed_size, file_info.tmu_date.tm_year,
                         file_info.tmu_date.tm_mon, file_info.tmu_date.tm_mday,
                         file_info.tmu_date.tm_hour, file_info.tmu_date.tm_min,
                         file_info.tmu_date.tm_sec, file_info.dosDate);
        return true;
    }

#if 0
    // lambda as a parameter https://en.wikipedia.org/wiki/C%2B%2B11#Polymorphic_wrappers_for_function_objects
    void iterEntries(std::function<void(ZipEntry&)> callback)
    {
        int err = unzGoToFirstFile(m_zf);
        if (UNZ_OK == err)
        {
            do
            {
                ZipEntry entryinfo;
                if (currentEntryInfo(entryinfo) && entryinfo.valid())
                {
                    callback(entryinfo);
                    err = unzGoToNextFile(m_zf);
                }
                else
                {
                    err = UNZ_ERRNO;
                }
            } while (UNZ_OK == err);

            if (UNZ_END_OF_LIST_OF_FILE != err && UNZ_OK != err)
                return;
        }
    }
#endif

    void getEntries(std::vector<ZipEntry>& entries)
    {
        int err = unzGoToFirstFile(m_zf);
        if (UNZ_OK == err)
        {
            do
            {
                ZipEntry entryinfo;
                if (currentEntryInfo(entryinfo) && entryinfo.valid())
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
#if 0
    bool extractCurrentEntry(ZipEntry& entryinfo, int (extractStrategy)(ZipEntry&) )
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
            return false;

        err = extractStrategy(entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                throw std::runtime_error(("Error " + std::to_string(err) +
                                          " closing internal file '" +
                                          entryinfo.name +
                                          "' in zip").c_str());
            }
        }

        return UNZ_OK == err;
    }
#endif

    bool extractCurrentEntryToFile(ZipEntry& entryinfo, const std::string& fileName, bool const replace)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
            return false;

        if (!entryinfo.uncompressedSize)
        {
            if (!Path::createDir(fileName))
                err = UNZ_ERRNO;
        }
        else
        {
            err = extractToFile(fileName, entryinfo, replace);
            if (UNZ_OK == err)
            {
                err = unzCloseCurrentFile(m_zf);
                if (UNZ_OK != err)
                {
                    std::stringstream str;
                    str << "Error " << err << " openinginternal file '"
                        << entryinfo.name << "' in zip";

                    throw std::runtime_error(str.str().c_str());
                }
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToStream(ZipEntry& entryinfo, std::ostream& stream)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
            return false;

        err = extractToStream(stream, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " opening internal file '"
                    << entryinfo.name << "' in zip";

                throw std::runtime_error(str.str().c_str());
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToMemory(ZipEntry& entryinfo,
                                     std::vector<unsigned char>& outvec)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
            return false;

        err = extractToMemory(outvec, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " opening internal file '"
                    << entryinfo.name << "' in zip";

                throw std::runtime_error(str.str().c_str());
            }
        }

        return UNZ_OK == err;
    }

#if defined(USE_WINDOWS)
    void changeFileDate(const std::string& filename, uLong dosdate,
                        tm_unz /*tmu_date*/)
    {
        HANDLE hFile;
        FILETIME ftm, ftLocal, ftCreate, ftLastAcc, ftLastWrite;

        hFile = CreateFileA(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                            0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            GetFileTime(hFile, &ftCreate, &ftLastAcc, &ftLastWrite);
            DosDateTimeToFileTime((WORD)(dosdate >> 16), (WORD)dosdate, &ftLocal);
            LocalFileTimeToFileTime(&ftLocal, &ftm);
            SetFileTime(hFile, &ftm, &ftLastAcc, &ftm);
            CloseHandle(hFile);
        }
    }

#else // !USE_WINDOWS
    void changeFileDate(const std::string& filename, uLong /*dosdate*/,
                        tm_unz tmu_date)
    {
        struct utimbuf ut;
        struct tm newdate;

        newdate.tm_sec = int(tmu_date.tm_sec);
        newdate.tm_min = int(tmu_date.tm_min);
        newdate.tm_hour = int(tmu_date.tm_hour);
        newdate.tm_mday = int(tmu_date.tm_mday);
        newdate.tm_mon = int(tmu_date.tm_mon);
        if (tmu_date.tm_year > 1900u)
            newdate.tm_year = int(tmu_date.tm_year - 1900u);
        else
            newdate.tm_year = int(tmu_date.tm_year);
        newdate.tm_isdst = -1;

        ut.actime = ut.modtime = mktime(&newdate);
        utime(filename.c_str(), &ut);
    }
#endif // USE_WINDOWS

    int extractToFile(const std::string& filename, ZipEntry& info, bool const replace)
    {
        int err = UNZ_ERRNO;

        /* If zip entry is a directory then create it on disk */
        std::string folder = Path::dirName(filename);
        if (!folder.empty())
        {
            std::string canon = Path::canonicalPath(folder);
            if (canon.rfind(folder, 0) != 0)
            {
                /* Prevent Zip Slip attack (See ticket #33) */
                std::stringstream str;
                str << "Security error: entry '" << filename
                    << "' would be outside your target directory";

                throw std::runtime_error(str.str().c_str());
            }
            if (!Path::createDir(folder))
            {
                std::stringstream str;
                str << "Error: cannot create the folder '"
                    << folder << "'";

                throw std::runtime_error(str.str().c_str());
            }
        }

        /* Avoid to replace the file. Prevent Zip Slip attack (See ticket #33) */
        if (!replace && Path::exist(filename))
        {
            std::stringstream str;
            str << "Security Error: '" << filename
                << "' already exists and will be replaced";

            throw std::runtime_error(str.str().c_str());
        }

        /* Create the file on disk so we can unzip to it */
        std::ofstream output_file(filename.c_str(), std::ofstream::binary);
        if (output_file.good())
        {
            if (UNZ_OK == extractToStream(output_file, info))
                err = UNZ_OK;

            output_file.close();

            /* Set the time of the file that has been unzipped */
            tm_unz timeaux;
            memcpy(&timeaux, &info.unixdate, sizeof(timeaux));

            changeFileDate(filename, info.dosdate, timeaux);
            err = UNZ_OK;
        }
        else
        {
            output_file.close();
        }

        return err;
    }

    int extractToStream(std::ostream& stream, ZipEntry& info)
    {
        int err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            throw std::runtime_error(str.str().c_str());
        }

        std::vector<char> buffer;
        buffer.resize(ZIPPER_WRITE_BUFFER_SIZE);

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(),
                                     static_cast<unsigned int>(buffer.size()));
            if (err < 0 || err == 0)
                break;

            stream.write(buffer.data(), std::streamsize(err));
            if (!stream.good())
            {
                err = UNZ_ERRNO;
                break;
            }

        } while (err > 0);

        stream.flush();

        return err;
    }

    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info)
    {
        int err = UNZ_ERRNO;

        err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            throw std::runtime_error(str.str().c_str());
        }

        std::vector<unsigned char> buffer;
        buffer.resize(ZIPPER_WRITE_BUFFER_SIZE);

        outvec.reserve(static_cast<size_t>(info.uncompressedSize));

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(),
                                     static_cast<unsigned int>(buffer.size()));
            if (err < 0 || err == 0)
                break;

            outvec.insert(outvec.end(), buffer.data(), buffer.data() + err);

        } while (err > 0);

        return err;
    }

public:
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
        size_t size = static_cast<size_t>(s);
        stream.seekg(0);

        if (size > 0u)
        {
            m_zipmem.base = new char[size];
            m_zipmem.size = static_cast<uLong>(size);
            stream.read(m_zipmem.base, std::streamsize(size));
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(m_filefunc);
    }

    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        if (!buffer.empty())
        {
            m_zipmem.base = reinterpret_cast<char*>(malloc(buffer.size() * sizeof(char)));
            memcpy(m_zipmem.base, reinterpret_cast<char*>(buffer.data()), buffer.size());
            m_zipmem.size = static_cast<uLong>(buffer.size());
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


    bool extractAll(const std::string& destination, const std::map<std::string,
                    std::string>& alternativeNames, bool const replace)
    {
        std::vector<ZipEntry> entries;
        getEntries(entries);
        std::vector<ZipEntry>::iterator it = entries.begin();
        for (; it != entries.end(); ++it)
        {
            if (!locateEntry(it->name))
                continue;

            std::string alternativeName = destination.empty()
                                          ? "" : destination + Path::Separator;

            if (alternativeNames.find(it->name) != alternativeNames.end())
                alternativeName += alternativeNames.at(it->name);
            else
                alternativeName += it->name;

            if (!extractCurrentEntryToFile(*it, alternativeName, replace))
                return false;
        };

        return true;
    }

    bool extractEntry(const std::string& name, const std::string& destination,
                      bool const replace)
    {
        std::string outputFile = destination.empty()
                                 ? name : destination + Path::Separator + name;

        if (locateEntry(name))
        {
            ZipEntry entry;
            if (!currentEntryInfo(entry))
                return false;
            return extractCurrentEntryToFile(entry, outputFile, replace);
        }
        else
        {
            return false;
        }
    }

    bool extractEntryToStream(const std::string& name, std::ostream& stream)
    {
        if (locateEntry(name))
        {
            ZipEntry entry;
            if (!currentEntryInfo(entry))
                return false;
            return extractCurrentEntryToStream(entry, stream);
        }
        else
        {
            return false;
        }
    }

    bool extractEntryToMemory(const std::string& name, std::vector<unsigned char>& vec)
    {
        if (locateEntry(name))
        {
            ZipEntry entry;
            if (!currentEntryInfo(entry))
                return false;
            return extractCurrentEntryToMemory(entry, vec);
        }
        else
        {
            return false;
        }
    }
};

Unzipper::Unzipper(std::istream& zippedBuffer, const std::string& password)
    : m_ibuffer(zippedBuffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_password(password)
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

Unzipper::Unzipper(std::vector<unsigned char>& zippedBuffer, const std::string& password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(zippedBuffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    //if (zippedBuffer.empty())
    //{
    //    release();
    //    throw std::runtime_error("std::vector shall not be empty");
    //}
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        throw std::runtime_error("Error loading zip in memory!");
    }

    m_open = true;
}

Unzipper::Unzipper(const std::string& zipname, const std::string& password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_zipname(zipname)
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initFile(zipname))
    {
        release();
        if (!Path::exist(zipname))
        {
            throw std::runtime_error("Non existent zip file!");
        }
        else
        {
            // Other error (like dummy zip). Let it dummy
        }
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

bool Unzipper::extractEntry(const std::string& name, const std::string& destination, bool const replace)
{
    return m_impl->extractEntry(name, destination, replace);
}

bool Unzipper::extractEntryToStream(const std::string& name, std::ostream& stream)
{
    return m_impl->extractEntryToStream(name, stream);
}

bool Unzipper::extractEntryToMemory(const std::string& name, std::vector<unsigned char>& vec)
{
    return m_impl->extractEntryToMemory(name, vec);
}

bool Unzipper::extract(bool replace)
{
    return extract(std::string(), replace);
}

bool Unzipper::extract(const std::string& destination, const std::map<std::string, std::string>& alternativeNames, bool const replace)
{
    return m_impl->extractAll(destination, alternativeNames, replace);
}

bool Unzipper::extract(const std::string& destination, bool const replace)
{
    return m_impl->extractAll(destination, std::map<std::string, std::string>(), replace);
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
