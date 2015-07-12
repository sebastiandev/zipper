#include "unzipper.h"
#include "defs.h"
#include "tools.h"

#include <functional>

namespace zipper {

struct Unzipper::Impl
{
	Unzipper& m_outer;
	zipFile m_zf;
	ourmemory_t m_zipmem = ourmemory_t();
	zlib_filefunc_def m_filefunc;

    private:

		// recibe un lambda como parametro https://en.wikipedia.org/wiki/C%2B%2B11#Polymorphic_wrappers_for_function_objects
		void iterFiles(std::function<void()> callback)
		{
			int err = unzGoToFirstFile(m_zf);
			if (err == UNZ_OK)
			{
				do
				{
					callback();
					err = unzGoToNextFile(m_zf);
				} while (err == UNZ_OK);

				if (err != UNZ_END_OF_LIST_OF_FILE && err != UNZ_OK)
					return;
			}
		}

		std::string getCurrentFilename()
		{
			char filename_inzip[256] = { 0 };
			unz_file_info64 file_info = { 0 };

			int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
			if (err == UNZ_OK)
				return std::string(filename_inzip);
			else
				return std::string();
		}

		bool extractCurrentFile()
		{
			unz_file_info64 file_info = { 0 };
			uInt size_buf = WRITEBUFFERSIZE;
			int err = UNZ_OK;
			char filename_inzip[256] = { 0 };

			err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
			if (err != UNZ_OK)
				return false;

			/* If zip entry is a directory then create it on disk */
			makedir((const char*)filename_inzip);

			std::vector<char> buff;
			buff.resize(size_buf);

			//err = unzOpenCurrentFilePassword(m_zf, password);
			//if (err != UNZ_OK)
			//printf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);

			/* Determine if the file should be overwritten or not and ask the user if needed */
			//if ((err == UNZ_OK) && (check_file_exists(filename_inzip)))
			//{
			//}

			/* Create the file on disk so we can unzip to it */
			std::ofstream output_file(filename_inzip);

			if (output_file.good())
			{
				err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
				if (err != UNZ_OK)
					printf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);

				do
				{
					err = unzReadCurrentFile(m_zf, buff.data(), buff.size());
					if (err < 0)
					{
						printf("error %d with zipfile in unzReadCurrentFile\n", err);
						break;
					}

					if (err == 0)
						break;

					output_file.write(buff.data(), err);
					if (!output_file.good())
					{
						printf("error %d in writing extracted file\n", errno);
						err = UNZ_ERRNO;
						break;
					}
				} while (err > 0);

				output_file.flush();
				output_file.close();

				err = unzCloseCurrentFile(m_zf);
				if (err != UNZ_OK)
					printf("error %d with zipfile in unzCloseCurrentFile\n", err);

				/* Set the time of the file that has been unzipped */
				change_file_date((const char*)filename_inzip, file_info.dosDate, file_info.tmu_date);
			}
		}

	public:

		Impl(Unzipper& outer) : m_outer(outer)
		{
			m_zf = NULL;
			m_filefunc = { 0 };
		}

		~Impl()
		{
		}

		void close()
		{
			if (m_zf)
				unzClose(m_zf);
		}

		bool initFile(const std::string& filename)
		{
			m_zf = unzOpen64(filename.c_str());
			return m_zf != NULL;
		}

		bool initWithStream(std::istream& stream)
		{
			stream.seekg(0, std::ios::end);
			auto size = stream.tellg();
			stream.seekg(0);

			if (size > 0)
			{
				m_zipmem.base = new char[size];
				stream.read(m_zipmem.base, size);
			}

			fill_memory_filefunc(&m_filefunc, &m_zipmem);

			return initMemory(m_filefunc);
		}

		bool initWithVector(std::vector<unsigned char>& buffer)
		{
			if (!buffer.empty())
			{
				m_zipmem.base = (char*)buffer.data();
				m_zipmem.size = buffer.size();
			}

			fill_memory_filefunc(&m_filefunc, &m_zipmem);

			return initMemory(m_filefunc);
		}

		bool initMemory(zlib_filefunc_def& filefunc)
		{
			m_zf = unzOpen2("__notused__", &filefunc);
			return m_zf != NULL;
		}


		std::vector<std::string> files()
		{
			std::vector<std::string> filelist;
			iterFiles([this, &filelist](){ filelist.push_back(this->getCurrentFilename());});

			return filelist;
		}

		bool extractAll()
		{
			iterFiles([this](){ this->extractCurrentFile(); });
			return true;
		}

		bool extractFile(const std::string& filename)
		{
			if (unzLocateFile(m_zf, filename.c_str(), NULL) != UNZ_OK)
			{
				printf("file %s not found in the zipfile\n", filename);
				return false;
			}

			return extractCurrentFile();
		}
};

Unzipper::Unzipper(std::istream& buffer)
	: m_ibuffer(buffer)
	, m_vecbuffer(std::vector<unsigned char>())
	, m_usingMemoryVector(false)
	, m_usingStream(true)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initWithStream(m_ibuffer))
		throw std::exception("Error loading zip in memory!");
	m_open = true;
}

Unzipper::Unzipper(std::vector<unsigned char>& buffer)
	: m_ibuffer(std::stringstream())
	, m_vecbuffer(buffer)
	, m_usingMemoryVector(true)
	, m_usingStream(false)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initWithVector(m_vecbuffer))
		throw std::exception("Error loading zip in memory!");

	m_open = true;
}

Unzipper::Unzipper(const std::string& zipname)
	: m_ibuffer(std::stringstream())
	, m_vecbuffer(std::vector<unsigned char>())
	, m_zipname(zipname)
	, m_usingMemoryVector(false)
	, m_usingStream(false)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initFile(zipname))
		throw std::exception("Error loading zip file!");

	m_open = true;
}

Unzipper::Unzipper(const std::string& zipname, const std::string& password)
	: m_ibuffer(std::stringstream())
	, m_vecbuffer(std::vector<unsigned char>())
	, m_zipname(zipname)
	, m_password(password)
	, m_usingMemoryVector(false)
	, m_usingStream(false)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initFile(zipname))
		throw std::exception("Error loading zip file!");

	m_open = true;
}

Unzipper::~Unzipper(void)
{
	close();
}

std::vector<std::string> Unzipper::files()
{
	return m_impl->files();
}

bool Unzipper::extractFile(const std::string& filename)
{
	return m_impl->extractFile(filename);
}

bool Unzipper::extract()
{
	return m_impl->extractAll();
}

void Unzipper::close()
{
	if (m_open)
	{
		m_impl->close();
		m_open = false;
	}
}

}