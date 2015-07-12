#include "catch.hpp"

#include "zipper\zipper.h"
#include "zipper\unzipper.h"
#include "zipper\tools.h"

#include <vector>
#include <fstream>
#include <ostream>
#include <string>

SCENARIO("zip vector feed with different inputs", "[zip]")
{
	GIVEN("A Zip outputed to a vector")
	{
		std::vector<unsigned char> zipvec;
		zipper::Zipper zipper(zipvec);

		WHEN("a file containing 'test file compression' is added and named 'test1'")
		{
			std::ofstream test1("test1.txt");
			test1 << "test file compression";
			test1.flush();
			test1.close();

			std::ifstream test1stream("test1.txt");

			zipper.add(test1stream, "test1.txt");

			test1stream.close();
			zipper.close();

			std::remove("test1.txt");
			
			zipper::Unzipper unzipper(zipvec);

			THEN("the zip vector has one entry named 'test1.txt'")
			{
				REQUIRE(unzipper.files().size() == 1);
				REQUIRE(unzipper.files().front() == "test1.txt");

				AND_THEN("extracting the test1.txt entry creates a file named 'test1.txt' with the text 'test file compression'")
				{
					unzipper.extractFile("test1.txt");
					// due to sections forking or creating different stacks we need to make sure the local instance is closed to
					// prevent mixing the closing when both instances are freed at the end of the scope
					unzipper.close();

					REQUIRE(check_file_exists("test1.txt"));

					std::ifstream testfile("test1.txt");
					REQUIRE(testfile.good());

					std::string test((std::istreambuf_iterator<char>(testfile)), std::istreambuf_iterator<char>());
					testfile.close();
					REQUIRE(test == "test file compression");

					AND_WHEN("another file containing 'other data to compression test' and named 'test2.dat' is added inside a folder 'TestFolder'")
					{
						std::ofstream test2("test2.dat");
						test2 << "other data to compression test";
						test2.flush();
						test2.close();

						std::ifstream test2stream("test2.dat");

						zipper.open();
						zipper.add(test2stream, "TestFolder\\test2.dat");
						zipper.close();

						test2stream.close();
						std::remove("test2.dat");

						zipper::Unzipper unzipper(zipvec);

						THEN("the zip vector has two entries named 'test1.txt' and 'TestFolder\\test2.dat'")
						{
							REQUIRE(unzipper.files().size() == 2);
							REQUIRE(unzipper.files().front() == "test1.txt");
							REQUIRE(unzipper.files()[1] == "TestFolder\\test2.dat");

							AND_THEN("extracting the test2.dat entry creates a folder 'TestFolder' with a file named 'test2.dat' with the text 'other data to compression test'")
							{
								unzipper.extract();
								unzipper.close();

								REQUIRE(check_file_exists("TestFolder\\test2.dat"));

								std::ifstream testfile("TestFolder\\test2.dat");
								REQUIRE(testfile.good());

								std::string test((std::istreambuf_iterator<char>(testfile)), std::istreambuf_iterator<char>());
								testfile.close();
								REQUIRE(test == "other data to compression test");
							}
						}

						std::remove("test1.txt");
						std::remove("TestFolder\\test2.dat");
						std::remove("TestFolder");
					}
				}
			}

			zipvec.clear();
		}

		WHEN("a stringstream containing 'test string data compression' is added and named 'strdata'")
		{
			std::stringstream strdata;
			strdata << "test string data compression";

			zipper.add(strdata, "strdata");
			zipper.close();

			zipper::Unzipper unzipper(zipvec);

			THEN("the zip vector has one entry named 'strdata'")
			{
				REQUIRE(unzipper.files().size() == 1);
				REQUIRE(unzipper.files().front() == "strdata");

				AND_THEN("extracting the strdata entry creates a file named 'strdata' with the txt 'test string data compression'")
				{
					unzipper.extract();
					unzipper.close();

					REQUIRE(check_file_exists("strdata"));

					std::ifstream testfile("strdata");
					REQUIRE(testfile.good());

					std::string test((std::istreambuf_iterator<char>(testfile)), std::istreambuf_iterator<char>());
					testfile.close();
					REQUIRE(test == "test string data compression");
				}
			}

			std::remove("strdata");
			zipvec.clear();
		}
	}
}
