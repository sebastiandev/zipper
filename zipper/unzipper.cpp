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

		bool initMemory(zlib_filefunc_def& filefunc)
		{
			m_zf = unzOpen2("__notused__", &filefunc);
			return m_zf != NULL;
		}

		bool locateEntry(const std::string& name)
		{
			return UNZ_OK == unzLocateFile(m_zf, name.c_str(), NULL);
		}

		ZipEntry currentEntryInfo()
		{
			unz_file_info64 file_info = { 0 };
			char filename_inzip[256] = { 0 };

			int err = unzGetCurrentFileInfo64(m_zf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
			if (UNZ_OK != err)
				throw std::exception("Error, couln't get the current entry info");

			return ZipEntry(std::string(filename_inzip), file_info.compressed_size, file_info.uncompressed_size,
							file_info.tmu_date.tm_year, file_info.tmu_date.tm_mon, file_info.tmu_date.tm_mday,
							file_info.tmu_date.tm_hour, file_info.tmu_date.tm_min, file_info.tmu_date.tm_sec, file_info.dosDate);
		}

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

	public:
		bool extractCurrentEntry(ZipEntry& entryinfo, std::function <int(ZipEntry&)> extractStrategy)
		{
			int err = UNZ_OK;

			if (!entryinfo.valid())
				return false;

			err = extractStrategy(entryinfo);
			if (UNZ_OK == err)
			{
				err = unzCloseCurrentFile(m_zf);
				if (UNZ_OK != err)
					throw std::exception(("Error " + std::to_string(err) + " closing internal file '" + entryinfo.name +
										  "' in zip").c_str());
			}

			return UNZ_OK == err;
		}

		int extractToFile(const std::string& filename, ZipEntry& info)
		{
			int err = UNZ_ERRNO;

			/* If zip entry is a directory then create it on disk */
			makedir(parentDirectory(filename));

			/* Create the file on disk so we can unzip to it */
			std::ofstream output_file(filename);

			if (output_file.good())
			{
				if (extractToStream(output_file, info))
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

		int extractToStream(std::ostream& stream, ZipEntry& info)
		{
			size_t err = UNZ_ERRNO;

			err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
			if (UNZ_OK != err)
				throw std::exception(("Error " + std::to_string(err) + " opening internal file '" + info.name +
				                      "' in zip").c_str());

			std::vector<char> buffer;
			buffer.resize(WRITEBUFFERSIZE);

			do
			{
				err = unzReadCurrentFile(m_zf, buffer.data(), (unsigned int)buffer.size());
				if (err < 0 || err == 0)
					break;

				stream.write(buffer.data(), err);
				if (!stream.good())
				{
					err = UNZ_ERRNO;
					break;
				}

			} while (err > 0);

			stream.flush();

			return (int)err;
		}

		int extractToMemory(std::vector<unsigned char>& outvec, ZipEntry& info)
		{
			size_t err = UNZ_ERRNO;

			err = unzOpenCurrentFilePassword(m_zf, m_outer.m_password.c_str());
			if (UNZ_OK != err)
				throw std::exception(("Error " + std::to_string(err) + " opening internal file '" + info.name +
                   					  "' in zip").c_str());

			std::vector<unsigned char> buffer;
			buffer.resize(WRITEBUFFERSIZE);

			outvec.reserve(info.uncompressedSize);

			do
			{
				err = unzReadCurrentFile(m_zf, buffer.data(), (unsigned int)buffer.size());
				if (err < 0 || err == 0)
					break;

				outvec.insert(outvec.end(), buffer.data(), buffer.data() + err);

			} while (err > 0);

			return (int)err;
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
				m_zipmem.size = (uLong)buffer.size();
			}

			fill_memory_filefunc(&m_filefunc, &m_zipmem);

			return initMemory(m_filefunc);
		}

		std::vector<ZipEntry> entries()
		{
			std::vector<ZipEntry> entrylist;
			iterEntries([this, &entrylist](ZipEntry& entryinfo){ entrylist.push_back(entryinfo); });

			return entrylist;
		}

		bool extractAll(const std::string& destination, const std::map<std::string, std::string>& alternativeNames)
		{
			iterEntries(
				[this, &destination, &alternativeNames](ZipEntry& entryinfo)
				{ 
					std::string alternativeName = destination.empty() ? "" : destination + "\\";

					if (alternativeNames.find(entryinfo.name) != alternativeNames.end())
						alternativeName += alternativeNames.at(entryinfo.name);
					else
						alternativeName += entryinfo.name;

					std::function<int(ZipEntry&)> func = std::bind(&zipper::Unzipper::Impl::extractToFile, this, alternativeName, std::placeholders::_1);
					this->extractCurrentEntry(entryinfo, func);
				}
			);

			return true;
		}

		bool extractEntry(const std::string& name, const std::string& destination)
		{
			auto outputfile = destination.empty() ? name : destination + "\\" + name;
			std::function<int(ZipEntry&)> func = std::bind(&zipper::Unzipper::Impl::extractToFile, this, outputfile, std::placeholders::_1);

			return locateEntry(name) ? extractCurrentEntry(currentEntryInfo(), func) : false;
		}

		bool extractEntryToStream(const std::string& name, std::ostream& stream)
		{
			std::function<int(ZipEntry&)> func = std::bind(&zipper::Unzipper::Impl::extractToStream, this, std::ref(stream), std::placeholders::_1);

			return locateEntry(name) ? extractCurrentEntry(currentEntryInfo(), func) : false;
		}

		bool extractEntryToMemory(const std::string& name, std::vector<unsigned char>& vec)
		{
			std::function<int(ZipEntry&)> func = std::bind(&zipper::Unzipper::Impl::extractToMemory, this, std::ref(vec), std::placeholders::_1);

			return locateEntry(name) ? extractCurrentEntry(currentEntryInfo(), func) : false;
		}
};

Unzipper::Unzipper(std::istream& zippedBuffer)
	: m_ibuffer(zippedBuffer)
	, m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
	, m_usingMemoryVector(false)
	, m_usingStream(true)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initWithStream(m_ibuffer))
		throw std::exception("Error loading zip in memory!");
	m_open = true;
}

Unzipper::Unzipper(std::vector<unsigned char>& zippedBuffer)
	: m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
	, m_vecbuffer(zippedBuffer)
	, m_usingMemoryVector(true)
	, m_usingStream(false)
	, m_impl(new Impl(*this))
{
	if (!m_impl->initWithVector(m_vecbuffer))
		throw std::exception("Error loading zip in memory!");

	m_open = true;
}

Unzipper::Unzipper(const std::string& zipname)
	: m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
	, m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
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
	: m_ibuffer(*(new std::stringstream())) //not used but using local variable throws exception
	, m_vecbuffer(*(new std::vector<unsigned char>())) //not used but using local variable throws exception
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

void Unzipper::close()
{
	if (m_open)
	{
		m_impl->close();
		m_open = false;
	}
}

}

