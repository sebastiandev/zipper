#include "unzipper.h"
#include "CDirEntry.h"
#include "defs.h"
#include "tools.h"

#include <functional>
#include <exception>
#include <fstream>
#include <stdexcept>


namespace zipper {

enum class unzipper_error {
    NO_ERROR = 0,
    GENERIC_ERROR = 1,
    NO_ENTRY = 2,
    ERROR_LOAD_MEMORY = 3,
    ERROR_LOAD_FILE = 4,
};
struct unzipper_error_category : std::error_category
{
    std::string custom_message {};
    const char* name() const noexcept override
    {
        return "unzipper";
    };
    std::string message(int ev) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }
        switch (static_cast<unzipper_error>(ev))
        {
        case unzipper_error::NO_ERROR:
            return "There was no error";
        case unzipper_error::GENERIC_ERROR:
            return "There was an error";
        case unzipper_error::NO_ENTRY:
            return "Error, couldn't get the current entry info";
        case unzipper_error::ERROR_LOAD_MEMORY:
            return "Error loading zip in memory";
        case unzipper_error::ERROR_LOAD_FILE:
            return "Error loading zip file";
        default:
            return "Unkown Error";
        }
    };
    unzipper_error_category() {};
    unzipper_error_category(const std::string& message)
        : custom_message(message) {};
};


std::error_code make_error_code(unzipper_error e)
{
    return { static_cast<int>(e), unzipper_error_category() };
}

std::error_code make_error_code(unzipper_error e, const std::string& message)
{
    return { static_cast<int>(e), unzipper_error_category(message) };
}

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
        return m_zf != NULL;
    }

    bool locateEntry(const std::string& name)
    {
        return UNZ_OK == unzLocateFile(m_zf, name.c_str(), NULL);
    }

    ZipEntry currentEntryInfo(std::error_code& ec)
    {
        unz_file_info64 file_info;
        char filename_inzip[256] = { 0 };

        int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        if (UNZ_OK != err)
            ec = make_error_code(unzipper_error::NO_ENTRY);

        return ZipEntry(std::string(filename_inzip), file_info.compressed_size, file_info.uncompressed_size,
                        file_info.tmu_date.tm_year, file_info.tmu_date.tm_mon, file_info.tmu_date.tm_mday,
                        file_info.tmu_date.tm_hour, file_info.tmu_date.tm_min, file_info.tmu_date.tm_sec, file_info.dosDate);
    }

    ZipEntry currentEntryInfo()
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        ZipEntry ze = currentEntryInfo(ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ze;
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
                ZipEntry entryinfo = currentEntryInfo();

                if (entryinfo.valid())
                {
                    callback(entryinfo);
                    err = unzGoToNextFile(m_zf);
                }
                else
                    err = UNZ_ERRNO;

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
                ZipEntry entryinfo = currentEntryInfo();

                if (entryinfo.valid())
                {
                    entries.push_back(entryinfo);
                    err = unzGoToNextFile(m_zf);
                }
                else
                    err = UNZ_ERRNO;

            } while (UNZ_OK == err);

            if (UNZ_END_OF_LIST_OF_FILE != err && UNZ_OK != err)
                return;
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
                throw EXCEPTION_CLASS(("Error " + std::to_string(err) + " closing internal file '" + entryinfo.name +
                                       "' in zip").c_str());
        }

        return UNZ_OK == err;
    }
#endif

    bool extractCurrentEntryToFile(ZipEntry& entryinfo, const std::string& fileName, std::error_code& ec)
    {
        int err = UNZ_OK;

        if (!entryinfo.valid())
            return false;

        err = extractToFile(fileName, entryinfo);
        if (UNZ_OK == err)
        {
            err = unzCloseCurrentFile(m_zf);
            if (UNZ_OK != err)
            {
                std::stringstream str;
                str << "Error " << err << " openinginternal file '"
                    << entryinfo.name << "' in zip";

                ec = make_error_code(unzipper_error::GENERIC_ERROR, str.str());
                return false;
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToFile(ZipEntry& entryinfo, const std::string& fileName)
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        bool ret = extractCurrentEntryToFile(entryinfo, fileName, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

    bool extractCurrentEntryToStream(ZipEntry& entryinfo, std::ostream& stream, std::error_code& ec)
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

                ec = make_error_code(unzipper_error::GENERIC_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToStream(ZipEntry& entryinfo, std::ostream& stream)
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        bool ret = extractCurrentEntryToStream(entryinfo, stream, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

    bool extractCurrentEntryToMemory(ZipEntry& entryinfo, std::vector<unsigned char>& outvec, std::error_code& ec)
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

                ec = make_error_code(unzipper_error::GENERIC_ERROR, str.str());
            }
        }

        return UNZ_OK == err;
    }

    bool extractCurrentEntryToMemory(ZipEntry& entryinfo, std::vector<unsigned char>& outvec)
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        bool ret = extractCurrentEntryToMemory(entryinfo, outvec, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

#if defined(USE_WINDOWS)
    void changeFileDate(const std::string& filename, uLong dosdate, tm_unz /*tmu_date*/)
    {
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
    }

#elif defined unix || defined __APPLE__
    void changeFileDate(const std::string& filename, uLong /*dosdate*/, tm_unz tmu_date)
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
#endif

    int extractToFile(const std::string& filename, ZipEntry& info)
    {
        int err = UNZ_ERRNO;

        /* If zip entry is a directory then create it on disk */
        makedir(parentDirectory(filename));

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
        }
        else
            output_file.close();

        return err;
    }

    int extractToStream(std::ostream& stream, ZipEntry& info, std::error_code& ec)
    {
        int err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            ec = make_error_code(unzipper_error::GENERIC_ERROR, str.str());
            return -1;
        }

        std::vector<char> buffer;
        buffer.resize(WRITEBUFFERSIZE);

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(), static_cast<unsigned int>(buffer.size()));
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

    int extractToStream(std::ostream& stream, ZipEntry& info)
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        int ret = extractToStream(stream, info, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info, std::error_code& ec)
    {
        int err = UNZ_ERRNO;

        err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
        if (UNZ_OK != err)
        {
            std::stringstream str;
            str << "Error " << err << " opening internal file '"
                << info.name << "' in zip";

            ec = make_error_code(unzipper_error::GENERIC_ERROR, str.str());
            return -1;
        }

        std::vector<unsigned char> buffer;
        buffer.resize(WRITEBUFFERSIZE);

        outvec.reserve(static_cast<size_t>(info.uncompressedSize));

        do
        {
            err = unzReadCurrentFile(m_zf, buffer.data(), static_cast<unsigned int>(buffer.size()));
            if (err < 0 || err == 0)
                break;

            outvec.insert(outvec.end(), buffer.data(), buffer.data() + err);

        } while (err > 0);

        return err;
    }

    int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info)
    {
        std::error_code ec = make_error_code(unzipper_error::NO_ERROR);
        int ret = extractToMemory(outvec, info, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

public:
    Impl(Unzipper& outer)
        : m_outer(outer), m_zipmem(), m_filefunc()
    {
        m_zipmem.base = NULL;
        m_zf = NULL;
    }

    ~Impl()
    {
        close();
    }

    void close()
    {
        if (m_zf != NULL)
        {
            unzClose(m_zf);
            m_zf = NULL;
        }

        if (m_zipmem.base != NULL)
        {
            free(m_zipmem.base);
            m_zipmem.base = NULL;
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
        return m_zf != NULL;
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


    bool extractAll(const std::string& destination, const std::map<std::string, std::string>& alternativeNames)
    {
        std::vector<ZipEntry> entries;
        getEntries(entries);
        std::vector<ZipEntry>::iterator it = entries.begin();
        for (; it != entries.end(); ++it)
        {
            if (!locateEntry(it->name))
                continue;

            std::string alternativeName = destination.empty() ? "" : destination + CDirEntry::Separator;

            if (alternativeNames.find(it->name) != alternativeNames.end())
                alternativeName += alternativeNames.at(it->name);
            else
                alternativeName += it->name;

            if (!extractCurrentEntryToFile(*it, alternativeName))
                return false;
        };

        return true;
    }

    bool extractEntry(const std::string& name, const std::string& destination)
    {
        std::string outputFile = destination.empty() ? name : destination + CDirEntry::Separator + name;

        if (locateEntry(name))
        {
            ZipEntry entry = currentEntryInfo();
            return extractCurrentEntryToFile(entry, outputFile);
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
            ZipEntry entry = currentEntryInfo();
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
            ZipEntry entry = currentEntryInfo();
            return extractCurrentEntryToMemory(entry, vec);
        }
        else
        {
            return false;
        }
    }
};

Unzipper::Unzipper(std::error_code& ec, std::istream& zippedBuffer, const std::string& password)
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
        ec = make_error_code(unzipper_error::ERROR_LOAD_MEMORY);
        return;
    }
    m_open = true;
}

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
        throw EXCEPTION_CLASS("Error loading zip in memory!");
    }
    m_open = true;
}

Unzipper::Unzipper(std::error_code& ec, std::vector<unsigned char>& zippedBuffer, const std::string& password)
    : m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(zippedBuffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        ec = make_error_code(unzipper_error::ERROR_LOAD_MEMORY);
        return;
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
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        throw EXCEPTION_CLASS("Error loading zip in memory!");
    }

    m_open = true;
}

Unzipper::Unzipper(std::error_code& ec, const std::string& zipname, const std::string& password)
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
        ec = make_error_code(unzipper_error::ERROR_LOAD_FILE);
        return;
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
        throw EXCEPTION_CLASS("Error loading zip file!");
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

bool Unzipper::extractEntry(const std::string& name, const std::string& destination)
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


bool Unzipper::extract(const std::string& destination, const std::map<std::string, std::string>& alternativeNames)
{
    return m_impl->extractAll(destination, alternativeNames);
}

bool Unzipper::extract(const std::string& destination)
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
