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

// -----------------------------------------------------------------------------
static std::string readFileContent(const char* file)
{
    std::ifstream ifs(file);
    std::string str((std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>());
    return str.c_str();
}

// -----------------------------------------------------------------------------
static bool zipEntry(Zipper& zipper, const char* filepath, const char* content,
                     const char* entrypath)
{
    std::ofstream ofs(filepath);
    ofs << content;
    ofs.flush();
    ofs.close();

    std::ifstream ifs(filepath);
    bool res = zipper.add(ifs, entrypath);
    ifs.close();

    Path::remove(filepath);

    return res;
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipperNominalOpenings)
{
    // Clean up.
    Path::remove("ziptest.zip");
    ASSERT_EQ(Path::exist("ziptest.zip"), false);

    // Create a fake file (not zip file, just the extension).
    std::ofstream outfile("ziptest.zip");
    outfile.close();

    // Fake file exists ?
    ASSERT_EQ(Path::exist("ziptest.zip"), true);
    ASSERT_EQ(Path::isFile("ziptest.zip"), true);

    // Create a zip. The fake file is replaced.
    Zipper zipper1("ziptest.zip", Zipper::openFlags::Overwrite);
    ASSERT_EQ(zipEntry(zipper1, "test1.txt", "test1 file compression",
                       "test1.txt"), true);
    zipper1.close();

    // Check the fake file has been replaced.
    zipper::Unzipper unzipper1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries1 = unzipper1.entries();
    unzipper1.close();
    ASSERT_EQ(entries1.size(), 1u);
    ASSERT_STREQ(entries1[0].name.c_str(), "test1.txt");

    // Replace the zip
    Zipper zipper2("ziptest.zip", Zipper::openFlags::Overwrite);
    zipper2.close();

    // Check the fake file has been replaced: "test1.txt" does not exist.
    ASSERT_EQ(Path::exist("ziptest.zip"), true);
    ASSERT_EQ(Path::isFile("ziptest.zip"), true);
    zipper::Unzipper unzipper2("ziptest.zip");
    std::vector<zipper::ZipEntry> entries2 = unzipper2.entries();
    unzipper2.close();
    ASSERT_EQ(entries2.size(), 0u);

    // Create a zip file with "test1.txt" file
    Zipper zipper3("ziptest.zip", Zipper::openFlags::Overwrite);
    ASSERT_EQ(zipEntry(zipper3, "test1.txt", "test1 file compression",
                       "test1.txt"), true);
    zipper3.close();

    // Replace the zip file with "test2.txt" file
    Zipper zipper4("ziptest.zip", Zipper::openFlags::Overwrite);
    ASSERT_EQ(zipEntry(zipper4, "test2.txt", "test2 file compression",
                       "test2.txt"), true);
    zipper4.close();

    // Check if test2.txt has replaced test1.txt
    zipper::Unzipper unzipper4("ziptest.zip");
    std::vector<zipper::ZipEntry> entries4 = unzipper4.entries();
    unzipper4.close();
    ASSERT_EQ(entries4.size(), 1u);
    ASSERT_STREQ(entries4[0].name.c_str(), "test2.txt");

    // Append zip file
    Zipper zipper5("ziptest.zip", Zipper::openFlags::Append);
    ASSERT_EQ(zipEntry(zipper5, "test1.txt", "test1 file compression",
                       "test1.txt"), true);
    zipper5.close();

    // Check if test2.txt and test1.txt exist
    zipper::Unzipper unzipper5("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5 = unzipper5.entries();
    unzipper5.close();
    ASSERT_EQ(entries5.size(), 2u);
    ASSERT_STREQ(entries5[0].name.c_str(), "test2.txt");
    ASSERT_STREQ(entries5[1].name.c_str(), "test1.txt");

    // Reopen and append zip entries
    ASSERT_EQ(zipper5.open(), true); // Default behavior: append
    ASSERT_EQ(zipEntry(zipper5, "test3.txt", "test3 file compression",
                       "test3.txt"), true);
    zipper5.close();

    zipper::Unzipper unzipper5_1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5_1 = unzipper5_1.entries();
    unzipper5_1.close();
    ASSERT_EQ(entries5_1.size(), 3u);
    ASSERT_STREQ(entries5_1[0].name.c_str(), "test2.txt");
    ASSERT_STREQ(entries5_1[1].name.c_str(), "test1.txt");
    ASSERT_STREQ(entries5_1[2].name.c_str(), "test3.txt");

    // Reopen and erase zip entries
    ASSERT_EQ(zipper5.open(Zipper::openFlags::Overwrite), true);
    ASSERT_EQ(zipEntry(zipper5, "test3.txt", "test3 file compression",
                       "test3.txt"), true);
    zipper5.close();

    zipper::Unzipper unzipper5_2("ziptest.zip");
    std::vector<zipper::ZipEntry> entries5_2 = unzipper5_2.entries();
    unzipper5_2.close();
    ASSERT_EQ(entries5_2.size(), 1u);
    ASSERT_STREQ(entries5_2[0].name.c_str(), "test3.txt");

    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipperPathologicalOpenings)
{
    // Opening a folder
    ASSERT_EQ(Path::exist("/usr/bin"), true);
    ASSERT_EQ(Path::isDir("/usr/bin"), true);
    try
    {
        Zipper zipper("/usr/bin");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Is a directory");
    }

    // Permission denied
    ASSERT_EQ(Path::exist("/usr/bin/ziptest.zip"), false);
    try
    {
        Zipper zipper("/usr/bin/ziptest.zip");
    }
    catch (std::runtime_error const& e)
    {
#if defined(__APPLE__)
        ASSERT_STREQ(e.what(), "Operation not permitted");
#else
        ASSERT_STREQ(e.what(), "Permission denied");
#endif
    }
}

// -----------------------------------------------------------------------------
TEST(FileUnzipTests, UnzipperPathologicalOpenings)
{
    // Opening a non existing file
    ASSERT_EQ(Path::exist("doesnotexist"), false);
    try
    {
        Unzipper unzipper("doesnotexist");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Does not exist");
    }

    // Opening a non zip file
    ASSERT_EQ(Path::exist("./build/Zipper-UnitTest"), true);
    ASSERT_EQ(Path::isFile("./build/Zipper-UnitTest"), true);
    try
    {
        Unzipper unzipper("./build/Zipper-UnitTest");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }

    // Opening a non zip file
    ASSERT_EQ(Path::exist("/usr/bin/make"), true);
    ASSERT_EQ(Path::isFile("/usr/bin/make"), true);
    try
    {
        Unzipper unzipper("/usr/bin/make");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }

    // Opening a folder
    ASSERT_EQ(Path::exist("/usr/bin"), true);
    ASSERT_EQ(Path::isDir("/usr/bin"), true);
    try
    {
        Unzipper unzipper("/usr/bin");
    }
    catch (std::runtime_error const& e)
    {
        ASSERT_STREQ(e.what(), "Not a zip file");
    }

    // TODO Real zip + permission denied
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipfileFeedWithDifferentInputs1)
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
    Path::remove("test1.txt");

    zipper.close();

    // Check if the zip file has one entry named 'test1.txt'
    zipper::Unzipper unzipper1("ziptest.zip");
    std::vector<zipper::ZipEntry> entries = unzipper1.entries();
    ASSERT_EQ(entries.size(), 1u);
    ASSERT_STREQ(entries[0].name.c_str(), "test1.txt");

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
    Path::remove("test2.dat");
    zipper.close();

    // Check the zip has two entries named 'test1.txt' and 'TestFolder/test2.dat'
    zipper::Unzipper unzipper2("ziptest.zip");
    ASSERT_EQ(unzipper2.entries().size(), 2u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(unzipper2.entries()[1].name.c_str(), "TestFolder/test2.dat");

    // Failed extracting since test1.txt is already present.
    try
    {
        ASSERT_EQ(unzipper2.extractAll(), false);
    }
    catch (std::runtime_error const& /*e*/)
    {}

    // Extract the zip. Check the test2.dat entry creates a folder 'TestFolder'
    // with a file named 'test2.dat' with the text 'other data to compression
    // test'
    ASSERT_EQ(unzipper2.extractAll(true), true); // replace the "test1.txt"
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
    Path::createDir(Path::currentPath() + "/TestFiles/subfolder");
    std::ofstream test4("TestFiles/test1.txt");
    test4 << "test file compression";
    test4.flush();
    test4.close();

    std::ofstream test5("TestFiles/test2.pdf");
    test5 << "test file compression";
    test5.flush();
    test5.close();

    std::ofstream test6("TestFiles/subfolder/test-sub.txt");
    test6 << "test file compression";
    test6.flush();
    test6.close();

    zipper.open();
    ASSERT_EQ(zipper.add("TestFiles"), true);
    zipper.close();

    zipper::Unzipper unzipper3("ziptest.zip");
    ASSERT_EQ(unzipper3.entries().size(), 5u);

    // And then extracting to a new folder 'NewDestination' creates the file
    // structure from zip in the new destination folder
    Path::createDir(Path::currentPath() + "/NewDestination");

    ASSERT_EQ(unzipper3.extractAll(Path::currentPath() + "/NewDestination"), true);

    std::vector<std::string> files = Path::filesFromDir(
        Path::currentPath() + "/NewDestination", true);

    ASSERT_EQ(Path::exist("NewDestination/TestFiles/test1.txt"), true);
    ASSERT_EQ(Path::exist("NewDestination/TestFiles/test2.pdf"), true);
    ASSERT_EQ(Path::exist("NewDestination/TestFiles/subfolder/test-sub.txt"), true);

    unzipper3.close();

    // Clean up
    Path::remove("TestFolder");
    Path::remove("TestFiles");
    Path::remove("NewDestination");
    Path::remove("test1.txt");
    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(FileZipTests, ZipfileFeedWithDifferentInputs2)
{
    if (Path::exist("ziptest.zip"))
        Path::remove("ziptest.zip");
    Zipper zipper("ziptest.zip");

    // Add a stringstream named 'strdata' containing 'test string data
    // compression' is added.
    std::stringstream strdata;
    strdata << "test string data compression";

    ASSERT_EQ(zipper.add(strdata, "strdata"), true);
    zipper.close();

    // Check the zip file has one entry named 'strdata'
    zipper::Unzipper unzipper("ziptest.zip");
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "strdata");

    // Extracting the strdata entry creates a file named 'strdata' with the text
    // 'test string data compression'"
    ASSERT_EQ(unzipper.extractAll(), true);
    ASSERT_EQ(Path::exist("strdata"), true);
    ASSERT_EQ(Path::isFile("strdata"), true);

    std::ifstream testfile3("strdata");
    ASSERT_EQ(testfile3.good(), true);

    std::string test7((std::istreambuf_iterator<char>(testfile3)),
                      std::istreambuf_iterator<char>());
    testfile3.close();
    ASSERT_STREQ(test7.c_str(), "test string data compression");

    // Extracting with an alternative name 'alternative_strdata.dat' crates a
    // file with that name instead of the one inside de zip".
    std::map<std::string, std::string> alt_names;
    alt_names["strdata"] = "alternative_strdata.dat";

    ASSERT_EQ(unzipper.extractAll("", alt_names), true);
    ASSERT_EQ(Path::exist("alternative_strdata.dat"), true);

    std::ifstream testfile4("alternative_strdata.dat");
    ASSERT_EQ(testfile4.good(), true);

    std::string test8((std::istreambuf_iterator<char>(testfile4)),
                      std::istreambuf_iterator<char>());
    testfile4.close();
    ASSERT_STREQ(test8.c_str(), "test string data compression");

    // Trying to extract a file 'fake.dat' that doesn't exists, returns false
    ASSERT_EQ(unzipper.extractEntry("fake.dat"), false);

    unzipper.close();
    Path::remove("strdata");
    Path::remove("alternative_strdata.dat");
    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, ZipVectorFeedWithDifferentInputs1)
{
    // A Zip outputed to a vector
    std::vector<unsigned char> zipvec;
    zipper::Zipper zipper(zipvec);

    // A file containing 'test file compression' is added and named 'test1'
    std::ofstream test1("test1.txt");
    test1 << "test file compression";
    test1.flush();
    test1.close();

    std::ifstream test1stream("test1.txt");
    zipper.add(test1stream, "test1.txt");
    test1stream.close();
    zipper.close();
    Path::remove("test1.txt");

    zipper::Unzipper unzipper(zipvec);

    // Check if the zip vector has one entry named 'test1.txt'
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "test1.txt");

    // Extracting the test1.txt entry creates a file named 'test1.txt' with the
    // text 'test file compression'
    ASSERT_EQ(unzipper.extractEntry("test1.txt"), true);
    // due to sections forking or creating different stacks we need to make sure the local instance is closed to
    // prevent mixing the closing when both instances are freed at the end of the scope
    unzipper.close();

    ASSERT_EQ(Path::exist("test1.txt"), true);

    std::ifstream testfile("test1.txt");
    ASSERT_EQ(testfile.good(), true);

    std::string test((std::istreambuf_iterator<char>(testfile)),
                     std::istreambuf_iterator<char>());
    testfile.close();
    ASSERT_STREQ(test.c_str(), "test file compression");

    // Another file containing 'other data to compression test' and named
    // 'test2.dat' is added inside a folder 'TestFolder'
    std::ofstream test2("test2.dat");
    test2 << "other data to compression test";
    test2.flush();
    test2.close();

    std::ifstream test2stream("test2.dat");

    zipper.open();
    zipper.add(test2stream, "TestFolder/test2.dat");
    zipper.close();

    test2stream.close();
    Path::remove("test2.dat");

    zipper::Unzipper unzipper2(zipvec);

    // The zip vector has two entries named 'test1.txt' and
    // 'TestFolder/test2.dat'
    ASSERT_EQ(unzipper2.entries().size(), 2u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "test1.txt");
    ASSERT_STREQ(unzipper2.entries()[1].name.c_str(), "TestFolder/test2.dat");

    // Failed extracting since test1.txt is already present.
    try
    {
        ASSERT_EQ(unzipper2.extractAll(), false);
    }
    catch (std::runtime_error const& /*e*/)
    {}

    // Extracting the test2.dat entry creates a folder 'TestFolder' with a file
    // named 'test2.dat' with the text 'other data to compression test'
    ASSERT_EQ(unzipper2.extractAll(true), true);
    ASSERT_EQ(Path::exist("TestFolder/test2.dat"), true);

    std::ifstream testfile2("TestFolder/test2.dat");
    ASSERT_EQ(testfile2.good(), true);

    std::string test3((std::istreambuf_iterator<char>(testfile2)),
                      std::istreambuf_iterator<char>());
    testfile2.close();
    ASSERT_STREQ(test3.c_str(), "other data to compression test");

    // Extracting the test2.dat entry to memory fills a vector with 'other data
    // to compression test'
    std::vector<unsigned char> resvec;
    unzipper2.extractEntryToMemory("TestFolder/test2.dat", resvec);
    unzipper2.close();

    std::string test4(resvec.begin(), resvec.end());

    ASSERT_STREQ(test4.c_str(), "other data to compression test");
    unzipper2.close();

    Path::remove("test1.txt");
    Path::remove("TestFolder");

    zipvec.clear();
    Path::remove("TestFolder");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, ZipVectorFeedWithDifferentInputs2)
{
    // A Zip outputed to a vector
    std::vector<unsigned char> zipvec;
    zipper::Zipper zipper(zipvec);

    // A stringstream containing 'test string data compression' is added and
    // named 'strdata'
    std::stringstream strdata;
    strdata << "test string data compression";

    zipper.add(strdata, "strdata");
    zipper.close();

    // The zip vector has one entry named 'strdata'
    zipper::Unzipper unzipper(zipvec);
    ASSERT_EQ(unzipper.entries().size(), 1u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "strdata");

    // Extracting the strdata entry creates a file named 'strdata' with the txt
    // 'test string data compression'
    ASSERT_EQ(unzipper.extractAll(true), true);
    ASSERT_EQ(Path::exist("strdata"), true);

    std::ifstream testfile3("strdata");
    ASSERT_EQ(testfile3.good(), true);

    std::string test5((std::istreambuf_iterator<char>(testfile3)),
                      std::istreambuf_iterator<char>());
    testfile3.close();
    ASSERT_STREQ(test5.c_str(), "test string data compression");

    // Extracting the strdata entry to memory, fills a vector with the txt 'test
    // string data compression'
    std::vector<unsigned char> resvec1;
    ASSERT_EQ(unzipper.extractEntryToMemory("strdata", resvec1), true);
    unzipper.close();

    std::string test6(resvec1.begin(), resvec1.end());
    ASSERT_STREQ(test6.c_str(), "test string data compression");

    Path::remove("strdata");
    zipvec.clear();

    // A file containing 'test file compression' is added and named 'test1' in
    // subdirectory 'subdirectory'
    bool fileWasCreated = Path::createDir("subdirectory");
    ASSERT_EQ(fileWasCreated, true);

    std::ofstream test7("./subdirectory/test1.txt");
    test7 << "test file compression";
    test7.flush();
    test7.close();

    auto flags = Zipper::zipFlags::Better | Zipper::zipFlags::SaveHierarchy;
    zipper.add("./subdirectory/test1.txt", static_cast<Zipper::zipFlags>(flags));

    zipper.close();

    Path::remove("./subdirectory/test1.txt");

#if 0
    // A Zip outputed to a vector;
    zipper::Unzipper unzipper2(zipvec);

    // The zip vector has entry named './subdirectory/test1.txt'
    ASSERT_EQ(unzipper2.entries().size(), 1u);
    ASSERT_STREQ(unzipper2.entries()[0].name.c_str(), "./subdirectory/test1.txt");

    // Extracting the test1.txt entry creates a file named 'test1.txt' with the
    // text 'test file compression'
    ASSERT_EQ(unzipper2.extractEntry("./subdirectory/test1.txt"), true);
    // due to sections forking or creating different stacks we need to make sure
    // the local instance is closed to prevent mixing the closing when both
    // instances are freed at the end of the scope
    unzipper2.close();

    ASSERT_EQ(Path::exist("./subdirectory/test1.txt"), true);
    std::ifstream testfile1("./subdirectory/test1.txt");
    ASSERT_EQ(testfile1.good(), true);

    std::string test8((std::istreambuf_iterator<char>(testfile1)),
                      std::istreambuf_iterator<char>());
    testfile1.close();
    ASSERT_STREQ(test8.c_str(), "test file compression");

    Path::remove("subdirectory");
    zipvec.clear();
#endif

    Path::remove("TestFolder");
    Path::remove("subdirectory");
}

// -----------------------------------------------------------------------------
TEST(MemoryZipTests, DummyVectorTest)
{
    std::vector<unsigned char> zipvec;
    try
    {
        zipper::Unzipper unzipper(zipvec);
        FAIL() << "An exception shall have thrown";
    }
    catch (std::runtime_error const& /*e*/)
    {}
}

// -----------------------------------------------------------------------------
TEST(ZipTests, PasswordTest)
{
    std::ofstream test1("test1.txt");
    test1 << "test file1 compression";
    test1.flush();
    test1.close();

    std::ofstream test2("test2.txt");
    test2 << "test file2 compression";
    test2.flush();
    test2.close();

    std::ifstream input1("test1.txt");
    std::ifstream input2("test2.txt");

    Zipper zipper("ziptest.zip", "123456");
    ASSERT_EQ(zipper.add(input1, "Test1"), true);
    ASSERT_EQ(zipper.add(input2, "Test2"), true);
    Path::remove("test1.txt");
    Path::remove("test2.txt");
    zipper.close();

    zipper::Unzipper unzipper("ziptest.zip", "123456");
    ASSERT_EQ(unzipper.entries().size(), 2u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "Test1");
    ASSERT_STREQ(unzipper.entries()[1].name.c_str(), "Test2");

    ASSERT_EQ(unzipper.extractAll(), true);
    ASSERT_EQ(Path::exist("Test1"), true);
    ASSERT_EQ(Path::isFile("Test1"), true);
    ASSERT_EQ(Path::exist("Test2"), true);
    ASSERT_EQ(Path::isFile("Test2"), true);

    std::ifstream testfile1("Test1");
    ASSERT_EQ(testfile1.good(), true);
    std::string test3((std::istreambuf_iterator<char>(testfile1)),
                      std::istreambuf_iterator<char>());
    testfile1.close();
    ASSERT_STREQ(test3.c_str(), "test file1 compression");

    std::ifstream testfile2("Test2");
    ASSERT_EQ(testfile2.good(), true);
    std::string test4((std::istreambuf_iterator<char>(testfile2)),
                      std::istreambuf_iterator<char>());
    testfile2.close();
    ASSERT_STREQ(test4.c_str(), "test file2 compression");

    Path::remove("ziptest.zip");
    Path::remove("Test1");
    Path::remove("Test2");
}

// -----------------------------------------------------------------------------
TEST(ZipTests, UnzipDummyTarball)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");

    // Test with the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder/"), true); // With the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
        Path::remove("ziptest.zip");
    }

    // Test without the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder"), true); // Without the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
    }

    Path::remove("ziptest.zip");
    Path::remove("data");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/21
TEST(ZipTests, Issue21)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");
    std::ofstream test("data/somefolder/test.txt");
    test << "test file2 compression";
    test.flush();
    test.close();

    // Test with the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder/"), true); // With the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "data/somefolder/test.txt");
        Path::remove("ziptest.zip");
    }

    // Test without the '/'
    {
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipper.add("data/somefolder"), true); // Without the '/'
        zipper.close();
        zipper::Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "data/somefolder/test.txt");
    }

    Path::remove("ziptest.zip");
    Path::remove("data");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/33
TEST(ZipTests, Issue33_zipping)
{
    {
        Path::remove("ziptest.zip");
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipEntry(zipper, "Test1.txt", "hello", "../Test1"), false);
        ASSERT_STREQ(zipper.error().message().c_str(),
                     "Security error: forbidden insertion of ../Test1 "
                     "(canonic: ../Test1) to prevent possible Zip Slip attack");
        zipper.close();

        Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 0u);
        unzipper.close();
    }

    {
        Path::remove("ziptest.zip");
        Zipper zipper("ziptest.zip");
        ASSERT_EQ(zipEntry(zipper, "Test1.txt", "world", "foo/../Test1"), true);
        zipper.close();

        Unzipper unzipper("ziptest.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "Test1");
        unzipper.close();
    }

    Path::remove("ziptest.zip");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/33
TEST(ZipTests, Issue33_unzipping)
{
    {
        ASSERT_EQ(Path::exist("../Test1"), false);

        Unzipper unzipper("issues/issue33_1.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "../Test1");
        ASSERT_EQ(unzipper.extractEntry("../Test1"), false);
        ASSERT_STREQ(unzipper.error().message().c_str(),
                     "Security error: entry '../Test1' would be outside your "
                     "target directory");
        unzipper.close();
        ASSERT_EQ(Path::exist("../Test1"), false);
    }

    {
        Path::remove("Test1");

        Unzipper unzipper("issues/issue33_2.zip");
        ASSERT_EQ(unzipper.entries().size(), 1u);
        ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "foo/../Test1");
        ASSERT_EQ(unzipper.extractEntry("foo/../Test1"), true);
        unzipper.close();
        ASSERT_EQ(Path::exist("Test1"), true);
        ASSERT_EQ(Path::isFile("Test1"), true);
        ASSERT_STREQ(readFileContent("Test1").c_str(), "hello");

        Path::remove("Test1");
    }
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/34
TEST(ZipTests, Issue34)
{
    zipper::Unzipper unzipper("issues/issue34.zip");
    ASSERT_EQ(unzipper.entries().size(), 13u);
    ASSERT_STREQ(unzipper.entries()[0].name.c_str(), "issue34/");
    ASSERT_STREQ(unzipper.entries()[1].name.c_str(), "issue34/1/");
    ASSERT_STREQ(unzipper.entries()[2].name.c_str(), "issue34/1/.dummy");
    ASSERT_STREQ(unzipper.entries()[3].name.c_str(), "issue34/1/2/");
    ASSERT_STREQ(unzipper.entries()[4].name.c_str(), "issue34/1/2/3/");
    ASSERT_STREQ(unzipper.entries()[5].name.c_str(), "issue34/1/2/3/4/");
    ASSERT_STREQ(unzipper.entries()[6].name.c_str(), "issue34/1/2/3_1/");
    ASSERT_STREQ(unzipper.entries()[7].name.c_str(), "issue34/1/2/3_1/3.1.txt");
    ASSERT_STREQ(unzipper.entries()[8].name.c_str(), "issue34/1/2/foobar.txt");
    ASSERT_STREQ(unzipper.entries()[9].name.c_str(), "issue34/11/");
    ASSERT_STREQ(unzipper.entries()[10].name.c_str(), "issue34/11/foo/");
    ASSERT_STREQ(unzipper.entries()[11].name.c_str(), "issue34/11/foo/bar/");
    ASSERT_STREQ(unzipper.entries()[12].name.c_str(), "issue34/11/foo/bar/here.txt");

    Path::remove("/tmp/issue34");
    ASSERT_EQ(unzipper.extractAll("/tmp"), true);

    ASSERT_EQ(Path::exist("/tmp/issue34/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/.dummy"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/3/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/3/4/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/3_1/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/3_1/3.1.txt"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/1/2/foobar.txt"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/11/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/11/foo/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/11/foo/bar/"), true);
    ASSERT_EQ(Path::exist("/tmp/issue34/11/foo/bar/here.txt"), true);

    ASSERT_EQ(Path::isDir("/tmp/issue34/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/.dummy"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/2/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/2/3/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/2/3/4/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/1/2/3_1/"), true);
    ASSERT_EQ(Path::isFile("/tmp/issue34/1/2/3_1/3.1.txt"), true);
    ASSERT_EQ(Path::isFile("/tmp/issue34/1/2/foobar.txt"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/11/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/11/foo/"), true);
    ASSERT_EQ(Path::isDir("/tmp/issue34/11/foo/bar/"), true);
    //FIXME ASSERT_EQ(Path::isFile("/tmp/issue34/11/foo/bar/here.txt"), true);

    ASSERT_STREQ(readFileContent("/tmp/issue34/1/2/3_1/3.1.txt").c_str(), "3.1\n");
    ASSERT_STREQ(readFileContent("/tmp/issue34/1/2/foobar.txt").c_str(), "foobar.txt\n");
    //FIXME ASSERT_STREQ(readFileContent("/tmp/issue34/11/foo/bar/here.txt").c_str(), "");
}

// -----------------------------------------------------------------------------
// https://github.com/sebastiandev/zipper/issues/83
TEST(MemoryZipTests, Issue83)
{
    // Clean up
    Path::remove("ziptest.zip");
    Path::remove("data");

    // Create folder
    Path::createDir("data/somefolder/");
    std::ofstream test("data/somefolder/test.txt");
    test << "test file2 compression";
    test.flush();
    test.close();

    // Zip
    Zipper zipper("ziptest.zip");
    ASSERT_EQ(zipper.add("data/somefolder/"), true);
    zipper.close();

    // Unzip
    zipper::Unzipper unzipper("ziptest.zip");
    ASSERT_EQ(unzipper.extractEntry("data/somefolder/test.txt",
                                    "/does/not/exist"),
              false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/does/not/exist/data/somefolder'");

    ASSERT_EQ(unzipper.extractEntry("data/somefolder/test.txt",
                                    "/usr/bin"),
              false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/usr/bin/data/somefolder'");

    ASSERT_EQ(unzipper.extractAll("/does/not/exist/"), false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/does/not/exist/data/somefolder'");

    ASSERT_EQ(unzipper.extractAll("/usr/bin"), false);
    ASSERT_STREQ(unzipper.error().message().c_str(),
                 "Error: cannot create the folder '/usr/bin/data/somefolder'");

    Path::remove("ziptest.zip");
    Path::remove("data");
}
