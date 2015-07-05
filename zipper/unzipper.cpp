#include "unzipper.h"
#include "defs.h"
#include "tools.hpp"

namespace zipper {

struct Unzipper::Impl
{
	Unzipper& m_outer;
	zipFile m_zf;
	ourmemory_t m_zipmem = ourmemory_t();

	Impl(Unzipper& outer) : m_outer(outer)
	{
		m_zf = NULL;
	}

	bool initFile(const std::string& filename)
	{
		m_zf = zipOpen64(filename.c_str());
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

		int err = unzGoToFirstFile(m_zf);
		if (err == UNZ_OK)
		{
			do
			{
				char filename_inzip[256] = { 0 };
				unz_file_info64 file_info = { 0 };

				err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
				if (err != UNZ_OK)
					break;

				filelist.push_back(std::string(filename_inzip));

				err = unzGoToNextFile(m_zf);
			} while (err == UNZ_OK);

			if (err != UNZ_END_OF_LIST_OF_FILE && err != UNZ_OK)
				return std::vector<std::string>();
		}

		return filelist;
	}

	bool extractAll()
	{
		int err = unzGoToFirstFile(m_zf);
		if (err != UNZ_OK)
			return false;

		do
		{
			err = extractCurrentFile();
			if (err != UNZ_OK)
				break;

			err = unzGoToNextFile(m_zf);
		} while (err == UNZ_OK);

		if (err != UNZ_END_OF_LIST_OF_FILE)
			return false;

		return true;
	}

	bool extractFile(std::string& filename, std::string& password = std::string())
	{
		if (unzLocateFile(m_zf, filename.c_str(), NULL) != UNZ_OK)
		{
			printf("file %s not found in the zipfile\n", filename);
			return false;
		}

		return extractCurrentFile();
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
		makedir(filename_inzip);

		std::vector<char> buff;
		buff.resize(size_buf);

		//err = unzOpenCurrentFilePassword(m_zf, password);
		//if (err != UNZ_OK)
			printf("error %d with zipfile in unzOpenCurrentFilePassword\n", err);

		/* Determine if the file should be overwritten or not and ask the user if needed */
		//if ((err == UNZ_OK) && (check_file_exists(filename_inzip)))
		//{
		//}

		/* Create the file on disk so we can unzip to it */
		std::fstream output_file(filename_inzip);

		if (output_file.good())
		{
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

			/* Set the time of the file that has been unzipped */
			change_file_date(filename_inzip, file_info.dosDate, file_info.tmu_date);
		}

		err = unzCloseCurrentFile(m_zf);
		if (err != UNZ_OK)
			printf("error %d with zipfile in unzCloseCurrentFile\n", err);
	}
};

Unzipper::Unzipper()
{
}


Unzipper::~Unzipper()
{
}

}