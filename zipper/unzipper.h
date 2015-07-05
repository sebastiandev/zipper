#pragma once

#include <vector>
#include <istream>
#include <string>
#include <memory>

namespace zipper {

	class Unzipper
	{
	public:
		Unzipper(std::ostream& buffer);
		Unzipper(std::vector<unsigned char>& buffer);
		Unzipper(const std::string& zipname);
		Unzipper(const std::string& zipname, const std::string& password);

		~Unzipper(void);

		void close();

	private:
		std::string m_password;
		std::string m_zipname;
		std::ostream& m_obuffer;
		std::vector<unsigned char>& m_vecbuffer;
		bool m_usingMemoryVector;
		bool m_usingStream;

		struct Impl;
		std::shared_ptr<Impl> m_impl;
	};

}