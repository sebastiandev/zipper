#include "zipper.h"

extern "C"
{
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <errno.h>
	#include <fcntl.h>

    #if (defined(_WIN32)) || (defined(_WIN64))
	# include <direct.h>
	# include <io.h>
	#else
	# include <unistd.h>
	# include <utime.h>
	#endif

	#include "zip.h"
	#include "unzip.h"
	#include "ioapi_mem.h"
	#define CASESENSITIVITY (0)
	#define WRITEBUFFERSIZE (8192)
	#define MAXFILENAME (256)

	#if (defined(_WIN32)) || (defined(_WIN64))
	#define USEWIN32IOAPI
	#include "iowin32.h"
	#endif
}

#if (defined(_WIN64)) && (!defined(__APPLE__))
        #ifndef __USE_FILE_OFFSET64
                #define __USE_FILE_OFFSET64
        #endif
        #ifndef __USE_LARGEFILE64
                #define __USE_LARGEFILE64
        #endif
        #ifndef _LARGEFILE64_SOURCE
                #define _LARGEFILE64_SOURCE
        #endif
        #ifndef _FILE_OFFSET_BIT
                #define _FILE_OFFSET_BIT 64
        #endif
#endif


/* calculate the CRC32 of a file,
   because to encrypt a file, we need known the CRC32 of the file before */

void getFileCrc(std::istream& input_stream, std::vector<char>& buff, unsigned long& result_crc)
{
    unsigned long calculate_crc=0;
    unsigned long size_read = 0;
    unsigned long total_read = 0;

    do {   
        input_stream.read(buff.data(), buff.size());
	    size_read = input_stream.gcount();

        if (size_read>0)
            calculate_crc = crc32(calculate_crc, (const unsigned char*)buff.data(), size_read);
		
        total_read += size_read;

    } while (size_read>0);

    input_stream.seekg(0);
    result_crc=calculate_crc;
}

bool isLargeFile(std::istream& input_stream)
{
    ZPOS64_T pos = 0;
    input_stream.seekg(0, std::ios::end);
    pos = input_stream.tellg();
	input_stream.seekg(0);
	
    return pos >= 0xffffffff;
}

namespace zipper {

	struct Zipper::Impl
	{
		Zipper& m_outer;
		zipFile m_zf;
		ourmemory_t m_zipmem = ourmemory_t();

		Impl(Zipper& outer) : m_outer(outer)
		{
			m_zf = NULL;
		}

		bool initFile(const std::string& filename)
		{
			int fileExists = 0;// chequear si existe el archivo!
			int mode = 0;
			int flags = Zipper::Overwrite;

			/* open the zip file for output */
			if (fileExists)
				mode = (flags & Zipper::Overwrite) ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;
			else
				mode = APPEND_STATUS_CREATE;

			m_zf = zipOpen64(filename.c_str(), mode);

			return m_zf != NULL;
		}

		bool initMemory()
		{
			zlib_filefunc64_def filefunc = { 0 };
			m_zipmem.grow = 1;

			//fill_memory_filefunc (&filefunc, &m_zipmem);
			fill_win32_filefunc64(&filefunc);

			m_zf = zipOpen3_64("__notused__", APPEND_STATUS_CREATE, 0, 0, &filefunc);

			return m_zf != NULL;
		}

		bool add(std::istream& input_stream, const std::string& nameInZip, const std::string& password, int flags)
		{
			if (!m_zf) return false;

			int compressLevel = 0;
			int zip64 = 0;
			int size_buf = WRITEBUFFERSIZE;
			int err = ZIP_OK;
			unsigned long crcFile = 0;

			zip_fileinfo zi;
			int size_read;

			std::vector<char> buff;
			buff.resize(size_buf);

			if (nameInZip.empty())
				return false;

			if (flags & Zipper::Faster) compressLevel = 1;
			if (flags & Zipper::Better) compressLevel = 9;

			//err = getFileCrc(input_stream, buff, crcFile);
			zip64 = (int)isLargeFile(input_stream);
			if (password.empty())
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
			else
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

			if (err != ZIP_OK)
				//printf("error in opening %s in zipfile\n", contentPath.c_str() );
				std::cerr << "Error!" << std::endl;
			else
			{
				do {
					err = ZIP_OK;
					input_stream.read(buff.data(), buff.size());
					size_read = input_stream.gcount();
					if (size_read < buff.size() && !input_stream.eof() && !input_stream.good())
						err = ZIP_ERRNO;

					if (size_read > 0)
					{
						err = zipWriteInFileInZip(this->m_zf, buff.data(), size_read);
						if (err < 0)
							printf("error in writing %s in the zipfile\n", nameInZip.c_str());
					}
				} while ((err == ZIP_OK) && (size_read>0));
			}

			if (err == ZIP_OK)
				err = zipCloseFileInZip(this->m_zf);

			return err == ZIP_OK;
		}

		void close()
		{
			free(m_zipmem.base);

			if (m_zf)
				zipClose(m_zf, NULL);
		}
	};

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////

	Zipper::Zipper(const std::string& zipname)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(std::ostringstream())
		, m_usingMemoryVector(false)
		, m_usingStream(false)
		, m_zipname(zipname)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initFile(zipname))
			throw std::exception("Error creating zip in file!");
	}

	Zipper::Zipper(const std::string& zipname, const std::string& password)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(std::ostringstream())
		, m_usingMemoryVector(false)
		, m_usingStream(false)
		, m_zipname(zipname)
		, m_password(password)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initFile(zipname))
			throw std::exception("Error creating zip in file!");
	}

	Zipper::Zipper(std::ostream& buffer)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(buffer)
		, m_usingMemoryVector(false)
		, m_usingStream(true)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initMemory())
			throw std::exception("Error creating zip in memory!");
	}

	Zipper::Zipper(std::vector<unsigned char>& buffer)
		: m_vecbuffer(buffer)
		, m_obuffer(std::ostringstream())
		, m_usingMemoryVector(true)
		, m_usingStream(false)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initMemory())
			throw std::exception("Error creating zip in memory!");
	}

	Zipper::~Zipper(void)
	{
		//m_impl->close();
	}

	bool Zipper::add(std::istream& source, const std::string& nameInZip, zipFlags flags)
	{
		return m_impl->add(source, nameInZip, "", flags);
	}

	void Zipper::close()
	{
		if (m_usingMemoryVector)
			m_vecbuffer.assign(m_impl->m_zipmem.base, m_impl->m_zipmem.base + m_impl->m_zipmem.size);
		else if (m_usingStream)
			m_obuffer.write(m_impl->m_zipmem.base, m_impl->m_zipmem.size);

		m_impl->close();
	}

}