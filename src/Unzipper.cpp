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
#  define ZIPPER_WRITE_BUFFER_SIZE (8192u)
#endif

namespace zipper {

enum class unzipper_error
{
    NO_ERROR = 0,
    //! Error when accessing to a info entry
    NO_ENTRY,
    //! Error inside libraries
    INTERNAL_ERROR,
    //! Zip slip vulnerability
    //! https://snyk.io/research/zip-slip-vulnerability
    SECURITY_ERROR,
};

// *************************************************************************
//! \brief std::error_code instead of throw() or errno.
// *************************************************************************
struct UnzipperErrorCategory : std::error_category
{
    virtual const char* name() const noexcept override
    {
        return "unzipper";
    }

    virtual std::string message(int ev) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }

        switch (static_cast<unzipper_error>(ev))
        {
        case unzipper_error::NO_ERROR:
            return "There was no error";
        case unzipper_error::NO_ENTRY:
            return "Error, couldn't get the current entry info";
        case unzipper_error::INTERNAL_ERROR:
            return "Internal error";
        case unzipper_error::SECURITY_ERROR:
            return "ZipSlip security";
        default:
            return "Unkown Error";
        }
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static UnzipperErrorCategory theUnzipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(unzipper_error e)
{
    return { static_cast<int>(e), theUnzipperErrorCategory };
}

// -----------------------------------------------------------------------------
static std::error_code make_error_code(unzipper_error e, std::string const& message)
{
    std::cerr << message << std::endl;
    theUnzipperErrorCategory.custom_message = message;
    return { static_cast<int>(e), theUnzipperErrorCategory };
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Unzipper::Impl
{
    Unzipper& m_outer;
    zipFile m_zf;
    ourmemory_t m_zipmem;
    zlib_filefunc_def m_filefunc;
    std::error_code& m_error_code;

private:

    // -------------------------------------------------------------------------
    bool initMemory(zlib_filefunc_def& filefunc)
    {
        m_zf = unzOpen2("__notused__", &filefunc);
        if (m_zf == nullptr)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           strerror(errno));
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool locateEntry(std::string const& name)
    {
        bool res = (UNZ_OK == unzLocateFile(m_zf, name.c_str(), nullptr));
        if (!res)
        {
            std::stringstream str;
            str << "Invalid info entry '" << name << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool failIfInvalidEntry(ZipEntry const& entryinfo)
    {
        if (!entryinfo.valid())
        {
            std::stringstream str;
            str << "Invalid info entry '" << entryinfo.name << "'";
            m_error_code = make_error_code(
                unzipper_error::NO_ENTRY, str.str());
            return false;
        }
        return true;
    }

    // -------------------------------------------------------------------------
    bool currentEntryInfo(ZipEntry &entry)
    {
        unz_file_info64 file_info;
        char filename_inzip[1024] = { 0 };

        int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip,
                                          sizeof(filename_inzip), nullptr, 0,
                                          nullptr, 0);
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Invalid info entry '" << entry.name << "'";
            m_error_code = make_error_code(unzipper_error::NO_ENTRY, str.str());
            return false;
        }

        entry = ZipEntry(std::string(filename_inzip), file_info.compressed_size,
                         file_info.uncompressed_size, file_info.tmu_date.tm_year,
                         file_info.tmu_date.tm_mon, file_info.tmu_date.tm_mday,
                         file_info.tmu_date.tm_hour, file_info.tmu_date.tm_min,
                         file_info.tmu_date.tm_sec, file_info.dosDate);
        return true;
    }

    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToFile(ZipEntry& entryinfo,
                                   std::string const& fileName,
                                   bool const replace)
    {
        int err = UNZ_OK;

        if (!failIfInvalidEntry(entryinfo))
            return false;

        if (!entryinfo.uncompressedSize)
        {
            if (!Path::createDir(fileName))
            {
                std::stringstream str;
                str << "Error cannot create folder '" << fileName << "'";
                m_error_code = make_error_code(
                    unzipper_error::INTERNAL_ERROR, str.str());
                err = UNZ_ERRNO;
            }
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
                    str << "Error opening internal file '" << entryinfo.name
                        << "' in zip";

                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, str.str());
                    return false;
                }
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToStream(ZipEntry& entryinfo, std::ostream& stream)
    {
        int err = UNZ_OK;

        if (!failIfInvalidEntry(entryinfo))
            return false;

        err = extractToStream(stream, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error opening internal file '" << entryinfo.name
                    << "' in zip";

                m_error_code = make_error_code(
                    unzipper_error::INTERNAL_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    bool extractCurrentEntryToMemory(ZipEntry& entryinfo,
                                     std::vector<unsigned char>& outvec)
    {
        int err = UNZ_OK;

        if (!failIfInvalidEntry(entryinfo))
            return false;

        err = extractToMemory(outvec, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error opening internal file '"
                    << entryinfo.name << "' in zip";

                m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    // -------------------------------------------------------------------------
    void changeFileDate(std::string const& filename, uLong dosdate, tm_unz tmu_date)
    {
#if defined(USE_WINDOWS)
        (void) tmu_date;
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
#else // !USE_WINDOWS
        (void) dosdate;
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
#endif // USE_WINDOWS
    }

    // -------------------------------------------------------------------------
    int extractToFile(std::string const& filename, ZipEntry& info, bool const replace)
    {

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

                m_error_code = make_error_code(unzipper_error::SECURITY_ERROR,
                                               str.str());
                return UNZ_ERRNO;
            }
            if (!Path::createDir(folder))
            {
                std::stringstream str;
                str << "Error: cannot create the folder '"
                    << folder << "'";

                m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                               str.str());
                return UNZ_ERRNO;
            }
        }

        /* Avoid to replace the file. Prevent Zip Slip attack (See ticket #33) */
        if (!replace && Path::exist(filename))
        {
            std::stringstream str;
            str << "Security Error: '" << filename
                << "' already exists and will be replaced";

            m_error_code = make_error_code(unzipper_error::SECURITY_ERROR,
                                           str.str());
            return UNZ_ERRNO;
        }

        std::string canon = Path::canonicalPath(filename);
        if ((canon.size() >= 2u) && (canon[0] == '.') && (canon[1] == '.'))
        {
            std::stringstream str;
            str << "Security error: entry '" << filename
                << "' would be outside your target directory";

            m_error_code = make_error_code(unzipper_error::SECURITY_ERROR,
                                           str.str());
            return UNZ_ERRNO;
        }

        /* Create the file on disk so we can unzip to it */
        std::ofstream output_file(filename.c_str(), std::ofstream::binary);
        if (output_file.good())
        {
            int err = extractToStream(output_file, info);

            output_file.close();

            /* Set the time of the file that has been unzipped */
            tm_unz timeaux;
            memcpy(&timeaux, &info.unixdate, sizeof(timeaux));

            changeFileDate(filename, info.dosdate, timeaux);
            return err;
        }
        else
        {
            m_error_code = make_error_code(
                unzipper_error::INTERNAL_ERROR, strerror(errno));
            output_file.close();
            return UNZ_ERRNO;
        }
    }

    // -------------------------------------------------------------------------
    int extractToStream(std::ostream& stream, ZipEntry& /*info*/)
    {
        int err;

        err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK == err)
        {
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
                    m_error_code = make_error_code(
                        unzipper_error::INTERNAL_ERROR, strerror(errno));
                    stream.flush();
                    return UNZ_ERRNO;
                }
            } while (err > 0);
            stream.flush();
        }

        if (err != UNZ_OK)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR);
        }
        return err;
    }

    // -------------------------------------------------------------------------
    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info)
    {
        int err;

        err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK == err)
        {
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
        }

        if (err != UNZ_OK)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR);
        }

        return err;
    }

public:

    // -------------------------------------------------------------------------
    Impl(Unzipper& outer, std::error_code& error_code)
        : m_outer(outer), m_zipmem(), m_filefunc(),
          m_error_code(error_code)
    {
        m_zipmem.base = nullptr;
        m_zf = nullptr;
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    bool initFile(std::string const& filename)
    {
#if defined(USE_WINDOWS)
        zlib_filefunc64_def ffunc;
        fill_win32_filefunc64A(&ffunc);
        m_zf = unzOpen2_64(filename.c_str(), &ffunc);
#else
        m_zf = unzOpen64(filename.c_str());
#endif
        return m_zf != nullptr;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::istream& stream)
    {
        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        stream.seekg(0);

        if (s < 0)
        {
            m_error_code = make_error_code(unzipper_error::INTERNAL_ERROR,
                                           strerror(errno));
            return false;
        }
        else
        {
            size_t size = static_cast<size_t>(s);
            m_zipmem.base = new char[size];
            m_zipmem.size = static_cast<uLong>(size);
            stream.read(m_zipmem.base, std::streamsize(size));
            if (!stream.good())
            {
                m_error_code = make_error_code(
                    unzipper_error::INTERNAL_ERROR, strerror(errno));
                return false;
            }
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);
        return initMemory(m_filefunc);
    }

    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries()
    {
        std::vector<ZipEntry> entrylist;
        getEntries(entrylist);
        return entrylist;
    }

    // -------------------------------------------------------------------------
    bool extractAll(std::string const& destination, const std::map<std::string,
                    std::string>& alternativeNames, bool const replace)
    {
        bool res = true;

        std::vector<ZipEntry> entries;
        getEntries(entries);
        std::vector<ZipEntry>::iterator it = entries.begin();
        for (; it != entries.end(); ++it)
        {
            if (!locateEntry(it->name))
            {
                res = false;
                continue;
            }

            std::string alternativeName;
            if (!destination.empty())
                alternativeName = destination + Path::Separator;

            auto const& alt = alternativeNames.find(it->name);
            if (alt != alternativeNames.end())
                alternativeName += alt->second;
            else
                alternativeName += it->name;

            if (!extractCurrentEntryToFile(*it, alternativeName, replace))
            {
                res = false;
                continue;
            }
        };

        return res;
    }

    // -------------------------------------------------------------------------
    bool extractEntry(std::string const& name, std::string const& destination,
                      bool const replace)
    {
        ZipEntry entry;
        std::string outputFile = destination.empty()
                                 ? name
                                 : destination + Path::Separator + name;
        std::string canonOutputFile = Path::canonicalPath(outputFile);

        return locateEntry(name) && currentEntryInfo(entry) &&
                extractCurrentEntryToFile(entry, canonOutputFile, replace);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToStream(std::string const& name, std::ostream& stream)
    {
        ZipEntry entry;

        return locateEntry(name) && currentEntryInfo(entry) &&
                extractCurrentEntryToStream(entry, stream);
    }

    // -------------------------------------------------------------------------
    bool extractEntryToMemory(std::string const& name, std::vector<unsigned char>& vec)
    {
        ZipEntry entry;

        return locateEntry(name) && currentEntryInfo(entry) &&
                extractCurrentEntryToMemory(entry, vec);
    }
};

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::istream& zippedBuffer, std::string const& password)
    : m_ibuffer(zippedBuffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(true)
    , m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithStream(m_ibuffer))
    {
        release();
        throw std::runtime_error(m_impl->m_error_code.message());
    }
    m_open = true;
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::vector<unsigned char>& zippedBuffer, std::string const& password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(zippedBuffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this, m_error_code))
{
    //if (zippedBuffer.empty())
    //{
    //    release();
    //    throw std::runtime_error("std::vector shall not be empty");
    //}
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    else
    {
        m_open = true;
    }
}

// -----------------------------------------------------------------------------
Unzipper::Unzipper(std::string const& zipname, std::string const& password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_zipname(zipname)
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(false)
    , m_impl(new Impl(*this, m_error_code))
{
    if (!Path::exist(zipname))
    {
        std::runtime_error exception("Does not exist");
        release();
        throw exception;
    }
    else if (!Path::isFile(zipname))
    {
        std::runtime_error exception("Not a zip file");
        release();
        throw exception;
    }
    else if (m_impl->initFile(zipname))
    {
        // success
        m_open = true;
    }
    else if (m_impl->m_error_code)
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    else
    {
        // Other error (like dummy zip). Let it dummy
        m_open = true;
    }
}

// -----------------------------------------------------------------------------
Unzipper::~Unzipper()
{
    close();
    release();
}

// -----------------------------------------------------------------------------
std::vector<ZipEntry> Unzipper::entries()
{
    return m_impl->entries();
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntry(std::string const& name,
                            std::string const& destination,
                            bool const replace)
{
    return m_impl->extractEntry(name, destination, replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntry(std::string const& name,
                            bool const replace)
{
    return m_impl->extractEntry(name, std::string(), replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntryToStream(std::string const& name,
                                    std::ostream& stream)
{
    return m_impl->extractEntryToStream(name, stream);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractEntryToMemory(std::string const& name,
                                    std::vector<unsigned char>& vec)
{
    return m_impl->extractEntryToMemory(name, vec);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(std::string const& destination,
                          const std::map<std::string, std::string>& alternativeNames,
                          bool const replace)
{
    return m_impl->extractAll(Path::canonicalPath(destination),
                              alternativeNames,
                              replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(bool replace)
{
    return m_impl->extractAll(std::string(),
                              std::map<std::string, std::string>(),
                              replace);
}

// -----------------------------------------------------------------------------
bool Unzipper::extractAll(std::string const& destination, bool const replace)
{
    return m_impl->extractAll(Path::canonicalPath(destination),
                              std::map<std::string, std::string>(),
                              replace);
}

// -----------------------------------------------------------------------------
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
    if (m_impl != nullptr)
    {
        delete m_impl;
    }
}

// -----------------------------------------------------------------------------
void Unzipper::close()
{
    if (m_open && (m_impl != nullptr))
    {
        m_impl->close();
    }
    m_open = false;
}

// -----------------------------------------------------------------------------
std::error_code const& Unzipper::error() const
{
    return m_error_code;
}

} // namespace zipper
