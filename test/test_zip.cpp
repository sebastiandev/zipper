#include "zipper\zipper.h"
#include "zipper\unzipper.h"

#include <vector>
#include <fstream>
#include <ostream>
#include <string>

int main(int argc, char** argv)
{
	std::ofstream test_file("test1.txt");
	test_file << "Test file for zipper zip" << std::endl;
	test_file.flush();
	test_file.close();

	std::ofstream test_file2("test2.txt");
	test_file2 << "Test file 2 for zipper zip" << std::endl;
	test_file2.flush();
	test_file2.close();

	std::ifstream input_test_file("test1.txt");
	std::ifstream input_test_file2("test2.txt");

	zipper::Zipper zipper("ziptest.zip");

	zipper.add(input_test_file, "Test1.txt");
	zipper.add(input_test_file2, "Folder\\Test2.txt");

	zipper.close();

	zipper::Unzipper unzipper("ziptest.zip");

	auto files = unzipper.files();
	unzipper.extractFile("Test1.txt");
	unzipper.extract();

	/*
	input_test_file.seekg(0);
	input_test_file2.seekg(0);

	std::string test1((std::istreambuf_iterator<char>(input_test_file)),
					   std::istreambuf_iterator<char>());

	input_test_file.close();	

	if (test1 != "Test file for zipper zip\n")
	{
		std::cout << "zipping test1.txt failed! :(" << std::endl;
		return 0;
	}

	std::string test2((std::istreambuf_iterator<char>(input_test_file2)),
		std::istreambuf_iterator<char>());

	input_test_file2.close();

	if (test2 != "Test file 2 for zipper zip\n")
	{
		std::cout << "zipping test2.txt failed! :(" << std::endl;
		return 0;
	}
	*/
	std::cout << "All good! keep coding..." << std::endl;

	return 0;
}