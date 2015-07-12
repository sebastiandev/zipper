#include "zipper.h"
#include "defs.h"
#include "tools.h"

namespace zipper {

	struct Zipper::Impl
	{
		Zipper& m_outer;
		zipFile m_zf;
		ourmemory_t m_zipmem = ourmemory_t();
		zlib_filefunc_def m_filefunc;

		Impl(Zipper& outer) : m_outer(outer)
		{
			m_zf = NULL;
			m_filefunc = { 0 };
		}

		bool initFile(const std::string& filename)
		{
			#ifdef USEWIN32IOAPI
				zlib_filefunc64_def ffunc = { 0 };
			#endif

			int mode = 0;
			int flags = Zipper::Append;

			/* open the zip file for output */
			if (check_file_exists(filename.c_str()))
				mode = (flags & Zipper::Overwrite) ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP;
			else
				mode = APPEND_STATUS_CREATE;

			#ifdef USEWIN32IOAPI
				fill_win32_filefunc64A(&ffunc);
				m_zf = zipOpen2_64(filename.c_str(), mode, NULL, &ffunc);
			#else
				m_zf = zipOpen64(filename.c_str(), mode);
			#endif

			return m_zf != NULL;
		}

		bool initWithStream(std::iostream& stream)
		{
			m_zipmem.grow = 1;

			stream.seekg(0, std::ios::end);
			auto size = stream.tellg();
			stream.seekg(0);

			if (size > 0)
			{
				m_zipmem.base = new char[size];
				stream.read(m_zipmem.base, size);
			}

			fill_memory_filefunc(&m_filefunc, &m_zipmem);
			
			return initMemory(size > 0 ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
		}

		bool initWithVector(std::vector<unsigned char>& buffer)
		{
			m_zipmem.grow = 1;

			if (!buffer.empty())
			{
				m_zipmem.base = new char[buffer.size()];
				memcpy(m_zipmem.base, (char*)buffer.data(), buffer.size());
				m_zipmem.size = buffer.size();
			}

			fill_memory_filefunc(&m_filefunc, &m_zipmem);

			return initMemory(buffer.empty() ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, m_filefunc);
		}

		bool initMemory(int mode, zlib_filefunc_def& filefunc)
		{
			m_zf = zipOpen3("__notused__", mode, 0, 0, &filefunc);
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
			if (m_zf)
				zipClose(m_zf, NULL);

			if (m_zipmem.base && m_zipmem.limit > 0)
			{
				if (m_outer.m_usingMemoryVector)
				{
					m_outer.m_vecbuffer.resize(m_zipmem.limit);
					m_outer.m_vecbuffer.assign(m_zipmem.base, m_zipmem.base + m_zipmem.limit);
				}

				else if (m_outer.m_usingStream)
					m_outer.m_obuffer.write(m_zipmem.base, m_zipmem.limit);
			}

			free(m_zipmem.base);
		}
	};

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////

	Zipper::Zipper(const std::string& zipname)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(std::stringstream())
		, m_usingMemoryVector(false)
		, m_usingStream(false)
		, m_zipname(zipname)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initFile(zipname))
			throw std::exception("Error creating zip in file!");

		m_open = true;
	}

	Zipper::Zipper(const std::string& zipname, const std::string& password)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(std::stringstream())
		, m_usingMemoryVector(false)
		, m_usingStream(false)
		, m_zipname(zipname)
		, m_password(password)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initFile(zipname))
			throw std::exception("Error creating zip in file!");

		m_open = true;
	}

	Zipper::Zipper(std::iostream& buffer)
		: m_vecbuffer(std::vector<unsigned char>())
		, m_obuffer(buffer)
		, m_usingMemoryVector(false)
		, m_usingStream(true)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initWithStream(m_obuffer))
			throw std::exception("Error creating zip in memory!");

		m_open = true;
	}

	Zipper::Zipper(std::vector<unsigned char>& buffer)
		: m_vecbuffer(buffer)
		, m_obuffer(std::stringstream())
		, m_usingMemoryVector(true)
		, m_usingStream(false)
		, m_impl(new Impl(*this))
	{
		if (!m_impl->initWithVector(m_vecbuffer))
			throw std::exception("Error creating zip in memory!");

		m_open = true;
	}

	Zipper::~Zipper(void)
	{
		close();
	}

	bool Zipper::add(std::istream& source, const std::string& nameInZip, zipFlags flags)
	{
		return m_impl->add(source, nameInZip, "", flags);
	}

	void Zipper::open()
	{
		if (!m_open)
		{
			if (m_usingMemoryVector)
			{
				if (!m_impl->initWithVector(m_vecbuffer))
					throw std::exception("Error opening zip memory!");
			}
			else if (m_usingStream)
			{
				if (!m_impl->initWithStream(m_obuffer))
					throw std::exception("Error opening zip memory!");
			}
			else
			{
				if (!m_impl->initFile(m_zipname))
					throw std::exception("Error opening zip file!");
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
}