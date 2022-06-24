#include "zipper.h"
#include "defs.h"
#include "tools.h"
#include "CDirEntry.h"

#include <fstream>
#include <stdexcept>

namespace zipper {

enum class zipper_error {
    NO_ERROR = 0,
    GENERIC_ERROR = 1,
    ERROR_OPENING_MEMORY = 2,
    ERROR_OPENING_FILE = 3,
    ERROR_CREATING_MEMORY = 4,
    ERROR_CREATING_FILE = 5,
    ERROR_ADDING = 6
};
struct zipper_error_category : std::error_category
{
    std::string custom_message {};
    const char* name() const noexcept override
    {
        return "zipper";
    };
    std::string message(int ev) const override
    {
        if (!custom_message.empty())
        {
            return custom_message;
        }
        switch (static_cast<zipper_error>(ev))
        {
        case zipper_error::NO_ERROR:
            return "There was no error";
        case zipper_error::GENERIC_ERROR:
            return "There was an error";
        case zipper_error::ERROR_OPENING_MEMORY:
            return "Error opening zip memory";
        case zipper_error::ERROR_OPENING_FILE:
            return "Error opening zip file";
        case zipper_error::ERROR_CREATING_MEMORY:
            return "Error creating zip in memory";
        case zipper_error::ERROR_CREATING_FILE:
            return "Error creating zip in file";
        case zipper_error::ERROR_ADDING:
            return "Error adding file to zip";
        default:
            return "This error is not handled";
        }
    };
    zipper_error_category() {};
    zipper_error_category(const std::string& message)
        : custom_message(message) {};
};


std::error_code make_error_code(zipper_error e)
{
    return { static_cast<int>(e), zipper_error_category() };
}

std::error_code make_error_code(zipper_error e, const std::string& message)
{
    return { static_cast<int>(e), zipper_error_category(message) };
}

struct Zipper::Impl
{
    Zipper& m_outer;
    zipFile m_zf;
    ourmemory_t m_zipmem;
    zlib_filefunc_def m_filefunc;

    Impl(Zipper& outer)
        : m_outer(outer), m_zipmem(), m_filefunc()
    {
        m_zf = NULL;
        m_zipmem.base = NULL;
        //m_filefunc = { 0 };
    }

    ~Impl()
    {
        close();
    }

    bool initFile(const std::string& filename)
    {
#ifdef USEWIN32IOAPI
        zlib_filefunc64_def ffunc = { 0 };
#endif

        int mode = 0;
        int flags = Zipper::Append;

        /* open the zip file for output */
        if (checkFileExists(filename))
        {
            mode = (flags & Zipper::Overwrite) ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;
        }
        else
        {
            mode = APPEND_STATUS_CREATE;
        }

#ifdef USEWIN32IOAPI
        fill_win32_filefunc64A(&ffunc);
        m_zf = zipOpen2_64(filename.c_str(), mode, NULL, &ffunc);
#else
        m_zf = zipOpen64(filename.c_str(), mode);
#endif

        return NULL != m_zf;
    }

    bool initWithStream(std::iostream& stream)
    {
        m_zipmem.grow = 1;

        stream.seekg(0, std::ios::end);
        std::streampos s = stream.tellg();
        if (s < 0)
        {
            return false;
        }
        size_t size = static_cast<size_t>(s);
        stream.seekg(0);

        if (size > 0)
        {
            if (m_zipmem.base != NULL)
            {
                free(m_zipmem.base);
            }
            m_zipmem.base = reinterpret_cast<char*>(malloc(size * sizeof(char)));
            stream.read(m_zipmem.base, std::streamsize(size));
        }

        fill_memory_filefunc(&m_filefunc, &m_zipmem);

        return initMemory(size > 0 ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
    }

    bool initWithVector(std::vector<unsigned char>& buffer)
    {
        m_zipmem.grow = 1;

        if (!buffer.empty())
        {
            if (m_zipmem.base != NULL)
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

    bool initMemory(int mode, zlib_filefunc_def& filefunc)
    {
        m_zf = zipOpen3("__notused__", mode, 0, 0, &filefunc);
        return m_zf != NULL;
    }

    bool add(std::istream& input_stream, const std::tm& timestamp,
             const std::string& nameInZip, const std::string& password, int flags, std::error_code& ec)
    {
        if (!m_zf)
            return false;

        int compressLevel = 0;
        bool zip64;
        size_t size_buf = WRITEBUFFERSIZE;
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
            return false;

        if (flags & Zipper::Faster)
            compressLevel = 1;
        if (flags & Zipper::Better)
            compressLevel = 9;

        zip64 = isLargeFile(input_stream);
        if (password.empty())
        {
            err = zipOpenNewFileInZip64(m_zf,
                                        nameInZip.c_str(),
                                        &zi,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        NULL /* comment*/,
                                        (compressLevel != 0) ? Z_DEFLATED : 0,
                                        compressLevel,
                                        zip64);
        }
        else
        {
            getFileCrc(input_stream, buff, crcFile);
            err = zipOpenNewFileInZip3_64(m_zf,
                                          nameInZip.c_str(),
                                          &zi,
                                          NULL,
                                          0,
                                          NULL,
                                          0,
                                          NULL /* comment*/,
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
                    err = zipWriteInFileInZip(this->m_zf, buff.data(), static_cast<unsigned int>(size_read));
                }
            } while ((err == ZIP_OK) && (size_read > 0));
        }
        else
        {
            ec = make_error_code(zipper_error::ERROR_ADDING, "Error adding '" + nameInZip + "' to zip");
            return false;
        }

        if (ZIP_OK == err)
        {
            err = zipCloseFileInZip(this->m_zf);
        }

        return ZIP_OK == err;
    }

    bool add(std::istream& input_stream, const std::tm& timestamp,
             const std::string& nameInZip, const std::string& password, int flags)
    {
        std::error_code ec = make_error_code(zipper_error::NO_ERROR);
        bool ret = add(input_stream, timestamp, nameInZip, password, flags, ec);
        if (ec.value() != 0)
        {
            throw EXCEPTION_CLASS(ec.message().c_str());
        }
        return ret;
    }

    void close()
    {
        if (m_zf != NULL)
        {
            zipClose(m_zf, NULL);
            m_zf = NULL;
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

        if (m_zipmem.base != NULL)
        {
            free(m_zipmem.base);
            m_zipmem.base = NULL;
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Zipper::Zipper(std::error_code& ec, const std::string& zipname, const std::string& password)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
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
        ec = make_error_code(zipper_error::ERROR_CREATING_FILE);
        return;
    }
    m_open = true;
}

Zipper::Zipper(const std::string& zipname, const std::string& password)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
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
        auto ec = make_error_code(zipper_error::ERROR_CREATING_FILE);
        throw EXCEPTION_CLASS(ec.message().c_str());
    }
    m_open = true;
}

Zipper::Zipper(std::error_code& ec, std::iostream& buffer, const std::string& password)
    : m_obuffer(buffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(true)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithStream(m_obuffer))
    {
        release();
        ec = make_error_code(zipper_error::ERROR_CREATING_MEMORY);
        return;
    }
    m_open = true;
}

Zipper::Zipper(std::iostream& buffer, const std::string& password)
    : m_obuffer(buffer)
    , m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
    , m_password(password)
    , m_usingMemoryVector(false)
    , m_usingStream(true)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithStream(m_obuffer))
    {
        release();
        auto ec = make_error_code(zipper_error::ERROR_CREATING_MEMORY);
        throw EXCEPTION_CLASS(ec.message().c_str());
    }
    m_open = true;
}

Zipper::Zipper(std::error_code& ec, std::vector<unsigned char>& buffer, const std::string& password)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(buffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        ec = make_error_code(zipper_error::ERROR_CREATING_MEMORY);
    }
    m_open = true;
}

Zipper::Zipper(std::vector<unsigned char>& buffer, const std::string& password)
    : m_obuffer(*(new std::stringstream())) //not used but using local variable throws exception
    , m_vecbuffer(buffer)
    , m_password(password)
    , m_usingMemoryVector(true)
    , m_usingStream(false)
    , m_impl(new Impl(*this))
{
    if (!m_impl->initWithVector(m_vecbuffer))
    {
        release();
        auto ec = make_error_code(zipper_error::ERROR_CREATING_MEMORY);
        throw EXCEPTION_CLASS(ec.message().c_str());
    }
    m_open = true;
}

Zipper::~Zipper()
{
    close();
    release();
}

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
    delete m_impl;
}

bool Zipper::add(std::istream& source, const std::tm& timestamp, const std::string& nameInZip, std::error_code& ec, zipFlags flags)
{
    return m_impl->add(source, timestamp, nameInZip, m_password, flags, ec);
}

bool Zipper::add(std::istream& source, const std::tm& timestamp, const std::string& nameInZip, zipFlags flags)
{
    return m_impl->add(source, timestamp, nameInZip, m_password, flags);
}

bool Zipper::add(std::istream& source, const std::string& nameInZip, std::error_code& ec, zipFlags flags)
{
    return m_impl->add(source, {}, nameInZip, m_password, flags, ec);
}

bool Zipper::add(std::istream& source, const std::string& nameInZip, zipFlags flags)
{
    return m_impl->add(source, {}, nameInZip, m_password, flags);
}

bool Zipper::add(const std::string& fileOrFolderPath, std::error_code& ec, zipFlags flags)
{
    if (isDirectory(fileOrFolderPath))
    {
        std::string folderName = fileNameFromPath(fileOrFolderPath);
        std::vector<std::string> files = filesFromDirectory(fileOrFolderPath);
        std::vector<std::string>::iterator it = files.begin();
        for (; it != files.end(); ++it)
        {
            std::ifstream input(it->c_str(), std::ios::binary);
            std::string nameInZip = it->substr(it->rfind(folderName + CDirEntry::Separator), it->size());
            add(input, nameInZip, ec, flags);
            if (ec.value() != 0)
            {
                return false;
            }
            input.close();
        }
    }
    else
    {
        std::ifstream input(fileOrFolderPath.c_str(), std::ios::binary);
        std::string fullFileName;

        if (flags & Zipper::SaveHierarchy)
        {
            fullFileName = fileOrFolderPath;
        }
        else
        {
            fullFileName = fileNameFromPath(fileOrFolderPath);
        }

        add(input, fullFileName, ec, flags);
        if (ec.value() != 0)
        {
            return false;
        }

        input.close();
    }

    return true;
}

bool Zipper::add(const std::string& fileOrFolderPath, zipFlags flags)
{
    auto ec = make_error_code(zipper_error::NO_ERROR);
    auto ret = add(fileOrFolderPath, ec, flags);
    if (ec.value() != 0)
    {
        throw EXCEPTION_CLASS(ec.message().c_str());
    }
    return ret;
}


void Zipper::open(std::error_code& ec)
{
    if (!m_open)
    {
        if (m_usingMemoryVector)
        {
            if (!m_impl->initWithVector(m_vecbuffer))
            {
                ec = make_error_code(zipper_error::ERROR_OPENING_MEMORY);
                return;
            }
        }
        else if (m_usingStream)
        {
            if (!m_impl->initWithStream(m_obuffer))
            {
                ec = make_error_code(zipper_error::ERROR_OPENING_MEMORY);
                return;
            }
        }
        else
        {
            if (!m_impl->initFile(m_zipname))
            {
                ec = make_error_code(zipper_error::ERROR_OPENING_FILE);
                return;
            }
        }

        m_open = true;
    }
}

void Zipper::open()
{
    if (!m_open)
    {
        if (m_usingMemoryVector)
        {
            if (!m_impl->initWithVector(m_vecbuffer))
            {
                auto ec = make_error_code(zipper_error::ERROR_OPENING_MEMORY);
                throw EXCEPTION_CLASS(ec.message().c_str());
            }
        }
        else if (m_usingStream)
        {
            if (!m_impl->initWithStream(m_obuffer))
            {
                auto ec = make_error_code(zipper_error::ERROR_OPENING_MEMORY);
                throw EXCEPTION_CLASS(ec.message().c_str());
            }
        }
        else
        {
            if (!m_impl->initFile(m_zipname))
            {
                auto ec = make_error_code(zipper_error::ERROR_OPENING_FILE);
                throw EXCEPTION_CLASS(ec.message().c_str());
            }
        }

        m_open = true;
    }
}

void Zipper::close()
{
    if (m_open)
    {
        m_impl->close();
        m_open = false;
    }
}

} // namespace zipper
