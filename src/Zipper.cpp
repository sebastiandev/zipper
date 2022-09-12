#include "Zipper/Zipper.hpp"
#include "external/minizip/zip.h"
#include "external/minizip/ioapi_mem.h"
#include "utils/Path.hpp"
#include "utils/Timestamp.hpp"

#include <fstream>
#include <stdexcept>

#ifndef ZIPPER_WRITE_BUFFER_SIZE
#  define ZIPPER_WRITE_BUFFER_SIZE (8192u)
#endif

namespace zipper {

enum class zipper_error
{
    NO_ERROR = 0,
    OPENING_ERROR,
    INTERNAL_ERROR,
    NO_ENTRY,
    SECURITY_ERROR
};

// *************************************************************************
//! \brief std::error_code instead of throw() or errno.
// *************************************************************************
struct ZipperErrorCategory : std::error_category
{
    virtual const char* name() const noexcept override
    {
        return "zipper";
    }

    virtual std::string message(int ev) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }

        switch (static_cast<zipper_error>(ev))
        {
        case zipper_error::NO_ERROR:
            return "There was no error";
        case zipper_error::OPENING_ERROR:
            return "Opening error";
        case zipper_error::INTERNAL_ERROR:
            return "Internal error";
        case zipper_error::NO_ENTRY:
            return "Error, couldn't get the current entry info";
        case zipper_error::SECURITY_ERROR:
            return "ZipSlip security";
        default:
            return "Unkown Error";
        }
    }

    std::string custom_message;
};

// -----------------------------------------------------------------------------
static ZipperErrorCategory theZipperErrorCategory;

// -----------------------------------------------------------------------------
static std::error_code make_error_code(zipper_error e)
{
    return { static_cast<int>(e), theZipperErrorCategory };
}

// -----------------------------------------------------------------------------
static std::error_code make_error_code(zipper_error e, std::string const& message)
{
    std::cerr << message << std::endl;
    theZipperErrorCategory.custom_message = message;
    return { static_cast<int>(e), theZipperErrorCategory };
}

// -----------------------------------------------------------------------------
// Calculate the CRC32 of a file because to encrypt a file, we need known the
// CRC32 of the file before.
static void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
    unsigned long calculate_crc = 0;
    unsigned int size_read = 0;
    unsigned long total_read = 0;

    do
    {
        input_stream.read(buff.data(), std::streamsize(buff.size()));
        size_read = static_cast<unsigned int>(input_stream.gcount());

        if (size_read > 0)
            calculate_crc = crc32(calculate_crc, reinterpret_cast<const unsigned char*>(buff.data()), size_read);

        total_read += static_cast<unsigned long>(size_read);

    } while (size_read > 0);

    input_stream.clear();
    input_stream.seekg(0, std::ios_base::beg);
    result_crc = calculate_crc;
}

// *************************************************************************
//! \brief PIMPL implementation
// *************************************************************************
struct Zipper::Impl
{
    Zipper& m_outer;
    zipFile m_zf;
    ourmemory_t m_zipmem;
    zlib_filefunc_def m_filefunc;
    std::error_code& m_error_code;

    // -------------------------------------------------------------------------
    Impl(Zipper& outer, std::error_code& error_code)
        : m_outer(outer), m_zipmem(), m_filefunc(),
          m_error_code(error_code)
    {
        m_zf = nullptr;
        m_zipmem.base = nullptr;
        //m_filefunc = { 0 };
    }

    // -------------------------------------------------------------------------
    ~Impl()
    {
        close();
    }

    // -------------------------------------------------------------------------
    bool initFile(const std::string& filename, Zipper::openFlags flags)
    {
#if defined(USE_WINDOWS)
        zlib_filefunc64_def ffunc = { 0 };
#endif

        int mode = 0;

        /* open the zip file for output */
        if (Path::exist(filename))
        {
            if (!Path::isFile(filename))
            {
                m_error_code = make_error_code(zipper_error::OPENING_ERROR,
                                               "Is a directory");
                return false;
            }
            if (flags == Zipper::openFlags::Overwrite)
            {
                Path::remove(filename);
                mode = APPEND_STATUS_CREATE;
            }
            else
            {
                mode = APPEND_STATUS_ADDINZIP;
            }
        }
        else
        {
            mode = APPEND_STATUS_CREATE;
        }

#if defined(USE_WINDOWS)
        fill_win32_filefunc64A(&ffunc);
        m_zf = zipOpen2_64(filename.c_str(), mode, nullptr, &ffunc);
#else
        m_zf = zipOpen64(filename.c_str(), mode);
#endif

        if (m_zf != nullptr)
            return true;
        m_error_code = make_error_code(zipper_error::OPENING_ERROR,
                                       strerror(errno));
        return false;
    }

    // -------------------------------------------------------------------------
    bool initWithStream(std::iostream& stream)
    {
        m_zipmem.grow = 1;

        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        if (s < 0)
        {
            m_error_code = make_error_code(zipper_error::INTERNAL_ERROR);
            return false;
        }
        size_t size = static_cast<size_t>(s);
        stream.seekg(0);

        if (size > 0)
        {
            if (m_zipmem.base != nullptr)
            {
                free(m_zipmem.base);
            }
            m_zipmem.base = reinterpret_cast<char*>(malloc(size * sizeof(char)));
            stream.read(m_zipmem.base, std::streamsize(size));
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(size > 0 ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
    }

    // -------------------------------------------------------------------------
    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        m_zipmem.grow = 1;

        if (!buffer.empty())
        {
            if (m_zipmem.base != nullptr)
            {
                free(m_zipmem.base);
            }
            m_zipmem.base = reinterpret_cast<char*>(malloc(buffer.size() * sizeof(char)));
            memcpy(m_zipmem.base, reinterpret_cast<char*>(buffer.data()), buffer.size());
            m_zipmem.size = static_cast<uLong>(buffer.size());
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(buffer.empty() ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
    }

    // -------------------------------------------------------------------------
    bool initMemory(int mode, zlib_filefunc_def& filefunc)
    {
        m_zf = zipOpen3("__notused__", mode, 0, 0, &filefunc);
        if (m_zf != nullptr)
            return true;
        m_error_code = make_error_code(zipper_error::INTERNAL_ERROR);
        return false;
    }

    // -------------------------------------------------------------------------
    bool add(std::istream& input_stream, const std::tm& timestamp,
             const std::string& nameInZip, const std::string& password, int flags)
    {
        if (!m_zf)
        {
            m_error_code = make_error_code(zipper_error::INTERNAL_ERROR);
            return false;
        }

        int compressLevel = 5; // Zipper::zipFlags::Medium
        bool zip64;
        size_t size_buf = ZIPPER_WRITE_BUFFER_SIZE;
        int err = ZIP_OK;
        unsigned long crcFile = 0;

        zip_fileinfo zi;
        zi.dosDate = 0; // if dos_date == 0, tmz_date is used
        zi.internal_fa = 0; // internal file attributes
        zi.external_fa = 0; // external file attributes
        zi.tmz_date.tm_sec = uInt(timestamp.tm_sec);
        zi.tmz_date.tm_min = uInt(timestamp.tm_min);
        zi.tmz_date.tm_hour = uInt(timestamp.tm_hour);
        zi.tmz_date.tm_mday = uInt(timestamp.tm_mday);
        zi.tmz_date.tm_mon = uInt(timestamp.tm_mon);
        zi.tmz_date.tm_year = uInt(timestamp.tm_year);

        size_t size_read;

        std::vector<char> buff;
        buff.resize(size_buf);

        if (nameInZip.empty())
        {
            m_error_code = make_error_code(zipper_error::NO_ENTRY);
            return false;
        }

        std::string canonNameInZip = Path::canonicalPath(nameInZip);

        /* Prevent Zip Slip attack (See ticket #33) */
        if (canonNameInZip.find_first_of("..") == 0u)
        {
            std::stringstream str;
            str << "Security error: forbidden insertion of "
                << nameInZip << " (canonic: " << canonNameInZip
                << ") to prevent possible Zip Slip attack";
            m_error_code = make_error_code(zipper_error::SECURITY_ERROR, str.str());
            return false;
        }

        flags = flags & ~int(Zipper::zipFlags::SaveHierarchy);
        if (flags == Zipper::zipFlags::Store)
            compressLevel = 0;
        else if (flags == Zipper::zipFlags::Faster)
            compressLevel = 1;
        else if (flags == Zipper::zipFlags::Better)
            compressLevel = 9;
        else
        {
            std::stringstream str;
            str << "Unknown compression level: " << flags;
            m_error_code = make_error_code(zipper_error::INTERNAL_ERROR, str.str());
            return false;
        }

        zip64 = Path::isLargeFile(input_stream);
        if (password.empty())
        {
            err = zipOpenNewFileInZip64(
                m_zf,
                canonNameInZip.c_str(),
                &zi,
                nullptr,
                0,
                nullptr,
                0,
                nullptr /* comment*/,
                (compressLevel != 0) ? Z_DEFLATED : 0,
                compressLevel,
                zip64);
        }
        else
        {
            getFileCrc(input_stream, buff, crcFile);
            err = zipOpenNewFileInZip3_64(
                m_zf,
                canonNameInZip.c_str(),
                &zi,
                nullptr,
                0,
                nullptr,
                0,
                nullptr /* comment*/,
                (compressLevel != 0) ? Z_DEFLATED : 0,
                compressLevel,
                0,
                /* -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, */
                -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                password.c_str(),
                crcFile,
                zip64);
        }

        if (ZIP_OK == err)
        {
            do
            {
                err = ZIP_OK;
                input_stream.read(buff.data(), std::streamsize(buff.size()));
                size_read = static_cast<size_t>(input_stream.gcount());
                if (size_read < buff.size() && !input_stream.eof() && !input_stream.good())
                {
                    err = ZIP_ERRNO;
                }

                if (size_read > 0)
                {
                    err = zipWriteInFileInZip(this->m_zf, buff.data(),
                                              static_cast<unsigned int>(size_read));
                }
            } while ((err == ZIP_OK) && (size_read > 0));
        }
        else
        {
            std::stringstream str;
            str << "Error when adding " << nameInZip << " to zip";
            m_error_code = make_error_code(zipper_error::INTERNAL_ERROR, str.str());
            return false;
        }

        if (ZIP_OK == err)
        {
            err = zipCloseFileInZip(this->m_zf);
            if (ZIP_OK != err)
            {
                std::stringstream str;
                str << "Error when closing zip";
                m_error_code = make_error_code(zipper_error::INTERNAL_ERROR, str.str());
            }
        }

        return ZIP_OK == err;
    }

    // -------------------------------------------------------------------------
    void close()
    {
        if (m_zf != nullptr)
        {
            zipClose(m_zf, nullptr);
            m_zf = nullptr;
        }

        if (m_zipmem.base && m_zipmem.limit > 0)
        {
            if (m_outer.m_usingMemoryVector)
            {
                m_outer.m_vecbuffer.resize(m_zipmem.limit);
                m_outer.m_vecbuffer.assign(m_zipmem.base, m_zipmem.base + m_zipmem.limit);
            }
            else if (m_outer.m_usingStream)
            {
                m_outer.m_obuffer.write(m_zipmem.base, std::streamsize(m_zipmem.limit));
            }
        }

        if (m_zipmem.base != nullptr)
        {
            free(m_zipmem.base);
            m_zipmem.base = nullptr;
        }
    }
};

// -------------------------------------------------------------------------
Zipper::Zipper(const std::string& zipname, const std::string& password, Zipper::openFlags flags)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_zipname(zipname)
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(false)
    , m_impl(new Impl(*this, m_error_code))
{
    if (m_impl->initFile(zipname, flags))
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

// -------------------------------------------------------------------------
Zipper::Zipper(std::iostream& buffer, const std::string& password)
    : m_obuffer(buffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(true)
    , m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithStream(m_obuffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    m_open = true;
}

// -------------------------------------------------------------------------
Zipper::Zipper(std::vector<unsigned char>& buffer, const std::string& password)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(buffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this, m_error_code))
{
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        std::runtime_error exception(m_impl->m_error_code.message());
        release();
        throw exception;
    }
    m_open = true;
}

// -------------------------------------------------------------------------
Zipper::~Zipper()
{
    close();
    release();
}

// -------------------------------------------------------------------------
void Zipper::close()
{
    if (m_open && (m_impl != nullptr))
    {
        m_impl->close();
    }
    m_open = false;
}

// -------------------------------------------------------------------------
void Zipper::release()
{
    if (!m_usingMemoryVector)
    {
        delete &m_vecbuffer;
    }
    if (!m_usingStream)
    {
        delete &m_obuffer;
    }
    if (m_impl != nullptr)
    {
        delete m_impl;
    }
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& source, const std::tm& timestamp, const std::string& nameInZip, zipFlags flags)
{
    return m_impl->add(source, timestamp, nameInZip, m_password, flags);
}

// -------------------------------------------------------------------------
bool Zipper::add(std::istream& source, const std::string& nameInZip, zipFlags flags)
{
    Timestamp time;
    return m_impl->add(source, time.timestamp, nameInZip, m_password, flags);
}

// -------------------------------------------------------------------------
bool Zipper::add(const std::string& fileOrFolderPath, Zipper::zipFlags flags)
{
    bool res = true;

    if (Path::isDir(fileOrFolderPath))
    {
        // Do not use dirName()
        // https://github.com/sebastiandev/zipper/issues/21
        char c = fileOrFolderPath.back();
        bool end_by_slash = (c == '/') || (c == '\\');
        std::string folderName(end_by_slash ? std::string(fileOrFolderPath.begin(), --fileOrFolderPath.end())
                               : fileOrFolderPath);

        std::vector<std::string> files = Path::filesFromDir(folderName, true);
        for (auto& it: files)
        {
            Timestamp time(it);
            std::ifstream input(it.c_str(), std::ios::binary);
            std::string nameInZip = it.substr(it.rfind(folderName + Path::Separator), it.size());
            res &= add(input, time.timestamp, nameInZip, flags);
            input.close();
        }
    }
    else
    {
        Timestamp time(fileOrFolderPath);
        std::ifstream input(fileOrFolderPath.c_str(), std::ios::binary);
        std::string fullFileName;

        if (flags & Zipper::SaveHierarchy)
        {
            fullFileName = fileOrFolderPath;
        }
        else
        {
            fullFileName = Path::fileName(fileOrFolderPath);
        }

        res &= add(input, time.timestamp, fullFileName, flags);

        input.close();
    }

    return res;
}

// -------------------------------------------------------------------------
bool Zipper::open(Zipper::openFlags flags)
{
    if (m_impl == nullptr)
    {
        m_error_code = make_error_code(
            zipper_error::INTERNAL_ERROR, "Malloc error");
        return false;
    }

    if (m_open)
        return true;

    if (m_usingMemoryVector)
    {
        if (!m_impl->initWithVector(m_vecbuffer))
            return false;
    }
    else if (m_usingStream)
    {
        if (!m_impl->initWithStream(m_obuffer))
            return false;
    }
    else
    {
        if (!m_impl->initFile(m_zipname, flags))
            return false;
    }

    m_open = true;
    return true;
}

// -----------------------------------------------------------------------------
std::error_code const& Zipper::error() const
{
    return m_error_code;
}

} // namespace zipper
