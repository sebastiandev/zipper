#include "unzipper.h"
#include "defs.h"
#include "tools.h"

#include <functional>

namespace zipper {

struct Unzipper::Impl
{
	private:

		Unzipper& m_outer;
		zipFile m_zf;
		ourmemory_t m_zipmem = ourmemory_t();

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
			std::fstream output_file(filename_inzip);

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
		}

		bool initFile(const std::string& filename)
		{
			m_zf = unzOpen64(filename.c_str());
			return m_zf != NULL;
		}

		bool initMemory()
		{
			m_zipmem.grow = 1;

			zlib_filefunc64_def ffunc;
			fill_win32_filefunc64A(&ffunc);

			m_zf = unzOpen2_64("__notused__", &ffunc);

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

Unzipper::Unzipper(std::ostream& buffer)
	: m_obuffer(buffer)
	, m_vecbuffer(std::vector<unsigned char>())
	, m_impl(new Impl(*this))
{
	if (!m_impl->initMemory())
		throw std::exception("Error loading zip in memory!");
}

Unzipper::Unzipper(std::vector<unsigned char>& buffer)
	: m_obuffer(std::ostringstream())
	, m_vecbuffer(buffer)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initMemory())
		throw std::exception("Error loading zip in memory!");
}

Unzipper::Unzipper(const std::string& zipname)
	: m_obuffer(std::ostringstream())
	, m_vecbuffer(std::vector<unsigned char>())
	, m_zipname(zipname)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initFile(zipname))
		throw std::exception("Error loading zip file!");
}

Unzipper::Unzipper(const std::string& zipname, const std::string& password)
	: m_obuffer(std::ostringstream())
	, m_vecbuffer(std::vector<unsigned char>())
	, m_zipname(zipname)
	, m_password(password)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initFile(zipname))
		throw std::exception("Error loading zip file!");
}

Unzipper::~Unzipper(void)
{
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

}