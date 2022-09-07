#ifndef ZIPPER_UNZIPPER_HPP
#  define ZIPPER_UNZIPPER_HPP

#include <vector>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <memory>
#include <map>

namespace zipper {

class ZipEntry;

// *****************************************************************************
//! \brief Zip archive extractor/decompressor.
// *****************************************************************************
class Unzipper
{
public:

    // -------------------------------------------------------------------------
    //! \brief Regular zip decompressor (from zip archive file).
    //!
    //! \param[in] zipname: the path of the zip file.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(const std::string& zipname,
             const std::string& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::iostream).
    //!
    //! \param[in,out] buffer: the stream in which zipped entries are stored.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(std::istream& buffer,
             const std::string& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief In-memory zip decompressor (from std::vector).
    //!
    //! \param[in,out] buffer: the vector in which zipped entries are stored.
    //! \param[in] password: the password used by the Zipper class (set empty
    //!   if no password is needed).
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    Unzipper(std::vector<unsigned char>& buffer,
             const std::string& password = std::string());

    // -------------------------------------------------------------------------
    //! \brief Call release() and close().
    // -------------------------------------------------------------------------
    ~Unzipper();

    // -------------------------------------------------------------------------
    //! \brief Return entries of the zip archive.
    // -------------------------------------------------------------------------
    std::vector<ZipEntry> entries();

    // -------------------------------------------------------------------------
    //! \brief Extract the whole zip archive using alternative destination names
    //! for existing files on the disk.
    //!
    //! \param[in] destination: the full path of the file to be created that
    //!   will hold uncompressed data. If no destination is given extract in the
    //!   same folder than the zip file.
    //! \param[in] alternativeNames: dictionary of alternative names for
    //!   existing files on disk (dictionary key: zip entry name, dictionary
    //!   data: newly desired path name on the disk).
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extract(const std::string& destination,
                 const std::map<std::string, std::string>& alternativeNames,
                 bool replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract the whole archive to the desired disk destination. If no
    //!   destination is given extract in the same folder than the zip file.
    //!
    //! \param[in] destination: the full path on the disk of the file to be
    //!   created that will hold uncompressed data. If no destination is given
    //!   extract in the same folder than the zip file.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extract(const std::string& destination, bool replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract the whole archive to the same folder than the zip file.
    //!
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extract(bool replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from the archive.
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[in] destination: the full path on the disk of the file to be
    //!   created that will hold uncompressed data. If no destination is given
    //!   extract in the same folder than the zip file.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extractEntry(const std::string& name, const std::string& destination,
                      bool replace = false);

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from the archive in the same folder than
    //! the zip file.
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[in] replace if set false (set by default) throw an exeception
    //!   when a file already exist.
    //!
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extractEntry(const std::string& name, bool replace = false)
    {
       return extractEntry(name, std::string(), replace);
    }

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (stream).
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[out] stream: the stream that will hold the extracted entry.
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extractEntryToStream(const std::string& name, std::ostream& stream);

    // -------------------------------------------------------------------------
    //! \brief Extract a single entry from zip to memory (vector).
    //!
    //! \param[in] name: the entry path inside the zip archive.
    //! \param[out] vec: the vector that will hold the extracted entry.
    //! \return true on success, else return false.
    //! \throw std::runtime_error if something odd happened.
    // -------------------------------------------------------------------------
    bool extractEntryToMemory(const std::string& name,
                              std::vector<unsigned char>& vec);

    // -------------------------------------------------------------------------
    //! \brief Relese memory. Called by the destructor.
    // -------------------------------------------------------------------------
    void close();

private:

    //! \brief Relese memory
    void release();

private:

    std::istream& m_ibuffer;
    std::vector<unsigned char>& m_vecbuffer;
    std::string m_zipname;
    std::string m_password;
    bool m_usingMemoryVector;
    bool m_usingStream;
    bool m_open;

    struct Impl;
    Impl* m_impl;
};

// *************************************************************************
//! \brief
// *************************************************************************
class ZipEntry
{
public:

    ZipEntry() = default;
    ZipEntry(const std::string& name_,
             unsigned long long int compressed_size,
             unsigned long long int uncompressed_size,
             unsigned int year,
             unsigned int month,
             unsigned int day,
             unsigned int hour,
             unsigned int minute,
             unsigned int second,
             unsigned long dosdate_)
        : name(name_), compressedSize(compressed_size),
          uncompressedSize(uncompressed_size), dosdate(dosdate_)
    {
        // timestamp YYYY-MM-DD HH:MM:SS
        std::stringstream str;
        str << year << "-" << month << "-" << day << " "
            << hour << ":" << minute << ":" << second;
        timestamp = str.str();

        unixdate.tm_year = year;
        unixdate.tm_mon = month;
        unixdate.tm_mday = day;
        unixdate.tm_hour = hour;
        unixdate.tm_min = minute;
        unixdate.tm_sec = second;
    }

    ZipEntry(ZipEntry const& other)
        : ZipEntry(other.name,
                   other.compressedSize,
                   other.uncompressedSize,
                   other.unixdate.tm_year,
                   other.unixdate.tm_mon,
                   other.unixdate.tm_mday,
                   other.unixdate.tm_hour,
                   other.unixdate.tm_min,
                   other.unixdate.tm_sec,
                   other.dosdate)
    {}

    ZipEntry& operator=(ZipEntry const& other)
    {
        this->~ZipEntry(); // destroy
        new (this) ZipEntry(other); // copy construct in place
        return *this;
    }

    ZipEntry& operator=(ZipEntry && other)
    {
        this->~ZipEntry(); // destroy
        new (this) ZipEntry(other); // copy construct in place
        return *this;
    }

    inline bool valid() const { return !name.empty(); }

public:

    typedef struct
    {
        unsigned int tm_sec;
        unsigned int tm_min;
        unsigned int tm_hour;
        unsigned int tm_mday;
        unsigned int tm_mon;
        unsigned int tm_year;
    } tm_s;

    std::string name, timestamp;
    unsigned long long int compressedSize;
    unsigned long long int uncompressedSize;
    unsigned long dosdate;
    tm_s unixdate;
};

} // namespace zipper

#endif // ZIPPER_UNZIPPER_HPP
