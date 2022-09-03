#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  include "gmock/gmock.h"
#  include "gtest/gtest.h"
# pragma GCC diagnostic pop

#include <fstream>
#include <chrono>
#include "utils/Path.hpp"

#define protected public
#define private public
//namespace TestZip {
#include "Zipper/Zipper.hpp"
#include "Zipper/Unzipper.hpp"
//}
#undef protected
#undef private

using namespace zipper;

TEST(FileZipTests, ZipfileFeedWithDifferentInputs)
{
    // Clean up
    if (Path::exist("ziptest.zip"))
        Path::remove("ziptest.zip");

    // Zip a file named 'test1' containing 'test file compression'
    Zipper zipper("ziptest.zip");
    std::ofstream test1("test1.txt");
    test1 << "test file compression";
    test1.flush();
    test1.close();

    std::ifstream test1stream("test1.txt");
    ASSERT_EQ(zipper.add(test1stream, "test1.txt"), true);
    test1stream.close();
    std::remove("test1.txt");

    zipper.close();

    // Check if the zip file has one entry named 'test1.txt'
    zipper::Unzipper unzipper1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries = unzipper1.entries();
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_STREQ(entries.front().name.c_str(), "test1.txt");

    // And then extracting the test1.txt entry creates a file named 'test1.txt'
    // with the text 'test file compression'
    ASSERT_EQ(unzipper1.extractEntry("test1.txt"), true);
    // due to sections forking or creating different stacks we need to make sure
    // the local instance is closed to prevent mixing the closing when both
    // instances are freed at the end of the scope
    unzipper1.close();

    // Check the content of test1.txt
    ASSERT_EQ(Path::exist("test1.txt"), true);
    ASSERT_EQ(Path::isFile("test1.txt"), true);
    std::ifstream testfile1("test1.txt");
    ASSERT_EQ(testfile1.good(), true);
    std::string test((std::istreambuf_iterator<char>(testfile1)),
                     std::istreambuf_iterator<char>());
    testfile1.close();
    ASSERT_STREQ(test.c_str(), "test file compression");


    // Zip a second file named 'test2.dat' containing 'other data to compression
    // test' inside a folder 'TestFolder'
    std::ofstream test2("test2.dat");
    test2 << "other data to compression test";
    test2.flush();
    test2.close();

    zipper.open();
    std::ifstream test2stream("test2.dat");
    ASSERT_EQ(zipper.add(test2stream, "TestFolder/test2.dat"), true);
    test2stream.close();
    std::remove("test2.dat");
    zipper.close();

    // Check the zip has two entries named 'test1.txt' and 'TestFolder/test2.dat'
    zipper::Unzipper unzipper2("ziptest.zip");
    ASSERT_EQ(unzipper2.entries().size(), 2u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(unzipper2.entries()[1].name.c_str(), "TestFolder/test2.dat");

#if 0
    // Extract the zip. Check the test2.dat entry creates a folder 'TestFolder'
    // with a file named 'test2.dat' with the text 'other data to compression
    // test'
    ASSERT_EQ(unzipper2.extract(true), true); // replace the "test1.txt"
    unzipper2.close();

    ASSERT_EQ(Path::exist("TestFolder/test2.dat"), true);
    std::ifstream testfile2("TestFolder/test2.dat");
    ASSERT_EQ(testfile2.good(), true);
    std::string test3((std::istreambuf_iterator<char>(testfile2)),
                      std::istreambuf_iterator<char>());
    testfile2.close();
    ASSERT_STREQ(test3.c_str(), "other data to compression test");

    // And when adding a folder to the zip, creates one entry for each file
    // inside the folder with the name in zip as 'Folder/...'
    Path::createDir(currentPath() + "/TestFiles/subfolder");
    std::ofstream test("TestFiles/test1.txt");
    test << "test file compression";
    test.flush();
    test.close();

    std::ofstream test1("TestFiles/test2.pdf");
    test1 << "test file compression";
    test1.flush();
    test1.close();

    std::ofstream test2("TestFiles/subfolder/test-sub.txt");
    test2 << "test file compression";
    test2.flush();
    test2.close();

    zipper.open();
    zipper.add("TestFiles");
    zipper.close();

    zipper::Unzipper unzipper3("ziptest.zip");
    ASSERT_EQ(unzipper3.entries().size(), 5u);

    // And then extracting to a new folder 'NewDestination' creates the file structure from zip in the new destination folder
    makedir(currentPath() + "/NewDestination");

    unzipper3.extract(currentPath() + "/NewDestination");

    std::vector<std::string> files = zipper::filesFromDirectory(currentPath() + "/NewDestination");

    ASSERT_EQ(Path::exist("NewDestination/TestFiles/test1.txt"), true);
    ASSERT_EQ(Path::exist("NewDestination/TestFiles/test2.pdf"), true);
    ASSERT_EQ(Path::exist("NewDestination/TestFiles/subfolder/test-sub.txt"), true);

    unzipper3.close();

    // Clean up
    removeFolder("TestFolder");
    removeFolder("TestFiles");
    removeFolder("NewDestination");
    std::remove("test1.txt");
    std::remove("ziptest.zip");

#endif
#if 0

        WHEN("a stringstream containing 'test string data compression' is added and named 'strdata'")
        {
            std::stringstream strdata;
            strdata << "test string data compression";

            zipper.add(strdata, "strdata");
            zipper.close();

            zipper::Unzipper unzipper("ziptest.zip");

            THEN("the zip file has one entry named 'strdata'")
            {
                ASSERT_EQ(unzipper.entries().size() == 1);
                ASSERT_EQ(unzipper.entries().front().name == "strdata");

                AND_THEN("extracting the strdata entry creates a file named 'strdata' with the txt 'test string data compression'")
                {
                    unzipper.extract();

                    ASSERT_EQ(checkFileExists("strdata"));

                    std::ifstream testfile("strdata");
                    ASSERT_EQ(testfile.good());

                    std::string test((std::istreambuf_iterator<char>(testfile)), std::istreambuf_iterator<char>());
                    testfile.close();
                    ASSERT_EQ(test == "test string data compression");

                    AND_THEN("extracting with an alternative name 'alternative_strdata.dat' crates a file with that name instead of the one inside de zip")
                    {
                        std::map<std::string, std::string> alt_names;
                        alt_names["strdata"] = "alternative_strdata.dat";

                        unzipper.extract("", alt_names);

                        ASSERT_EQ(checkFileExists("alternative_strdata.dat"));

                        std::ifstream testfile2("alternative_strdata.dat");
                        ASSERT_EQ(testfile2.good());

                        std::string test2((std::istreambuf_iterator<char>(testfile2)), std::istreambuf_iterator<char>());
                        testfile2.close();
                        ASSERT_EQ(test2 == "test string data compression");

                        AND_THEN("trying to extract a file 'fake.dat' that doesn't exists, returns false")
                        {
                            ASSERT_EQ(false == unzipper.extractEntry("fake.dat"));
                        }
                    }

                    unzipper.close();
                }
            }

            std::remove("strdata");
            std::remove("alternative_strdata.dat");
            std::remove("ziptest.zip");
        }
    }

#endif
}
