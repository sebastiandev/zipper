#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  include "gmock/gmock.h"
#  include "gtest/gtest.h"
# pragma GCC diagnostic pop

#include "utils/Path.hpp"

using namespace zipper;

TEST(TestDir, exist)
{
   ASSERT_EQ(Path::exist(PWD), true);
   ASSERT_EQ(Path::exist(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::exist(""), false);
   ASSERT_EQ(Path::exist("fsdfqfz"), false);
}

TEST(TestDir, isDir)
{
   ASSERT_EQ(Path::isDir(PWD), true);
   ASSERT_EQ(Path::isDir(PWD"/main.cpp"), false);
   ASSERT_EQ(Path::isDir(""), false);
   ASSERT_EQ(Path::isDir("sdsdqsd"), false);
}

TEST(TestDir, isFile)
{
   ASSERT_EQ(Path::isFile(PWD), false);
   ASSERT_EQ(Path::isFile(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isFile(""), false);
   ASSERT_EQ(Path::isFile("aazaza"), false);
}

TEST(TestDir, isReadable)
{
   ASSERT_EQ(Path::isReadable(PWD), true);
   ASSERT_EQ(Path::isReadable(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isReadable(""), false);
   ASSERT_EQ(Path::isReadable("qdqdqsdqdq"), false);
   ASSERT_EQ(Path::isReadable("/usr/bin"), true);
   ASSERT_EQ(Path::isReadable("/usr/bin"), true);
}

TEST(TestDir, isWritable)
{
   ASSERT_EQ(Path::isWritable(PWD), true);
   ASSERT_EQ(Path::isWritable(PWD"/main.cpp"), true);
   ASSERT_EQ(Path::isWritable(""), false);
   ASSERT_EQ(Path::isWritable("qdqdqsdqdq"), false);
   ASSERT_EQ(Path::isWritable("/usr/bin"), false);
}

TEST(TestDir, fileName)
{
   ASSERT_STREQ(Path::fileName("/foo/bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("/foo/bar/file.foo.txt").c_str(), "file.foo.txt");
   ASSERT_STREQ(Path::fileName("/foo/bar").c_str(), "bar");
   ASSERT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(Path::fileName("./foo/../bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("./foo/../bar/../file.txt").c_str(), "file.txt");
   ASSERT_STREQ(Path::fileName("").c_str(), "");
   ASSERT_STREQ(Path::fileName("..").c_str(), "..");
   ASSERT_STREQ(Path::fileName("/").c_str(), "");
   ASSERT_STREQ(Path::fileName("//").c_str(), "");
   ASSERT_STREQ(Path::fileName("//.").c_str(), ".");

   ASSERT_STREQ(Path::fileName("/foo/bar.txt").c_str(), "bar.txt");
   ASSERT_STREQ(Path::fileName("/foo/.bar").c_str(), ".bar");
   ASSERT_STREQ(Path::fileName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(Path::fileName("/foo/.").c_str(), ".");
   ASSERT_STREQ(Path::fileName("/foo/..").c_str(), "..");
   ASSERT_STREQ(Path::fileName(".").c_str(), ".");
   ASSERT_STREQ(Path::fileName("..").c_str(), "..");
   ASSERT_STREQ(Path::fileName("/").c_str(), "");
   ASSERT_STREQ(Path::fileName("//host").c_str(), "host");
}

TEST(TestDir, dirName)
{
   ASSERT_STREQ(Path::dirName("/foo/bar/file.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("/foo/bar/file.foo.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("/foo/bar").c_str(), "/foo");
   ASSERT_STREQ(Path::dirName("/foo/bar/").c_str(), "/foo/bar");
   ASSERT_STREQ(Path::dirName("./foo/../bar/file.txt").c_str(), "./foo/../bar");
   ASSERT_STREQ(Path::dirName("./foo/../bar/../file.txt").c_str(), "./foo/../bar/..");
   ASSERT_STREQ(Path::dirName("/var/tmp/.").c_str(), "/var/tmp");;
   ASSERT_STREQ(Path::dirName("/usr/lib").c_str(), "/usr");
   ASSERT_STREQ(Path::dirName("/usr/").c_str(), "/usr");
   ASSERT_STREQ(Path::dirName("/usr").c_str(), "/");
   ASSERT_STREQ(Path::dirName("usr").c_str(), "");
   ASSERT_STREQ(Path::dirName("/").c_str(), "/");
   ASSERT_STREQ(Path::dirName(".").c_str(), "");
   ASSERT_STREQ(Path::dirName("..").c_str(), "");
   ASSERT_STREQ(Path::dirName("//").c_str(), "//");
   ASSERT_STREQ(Path::dirName("//.").c_str(), "/");
}

TEST(TestDir, suffix)
{
   ASSERT_STREQ(Path::suffix("/foo/bar/file.txt").c_str(), ".txt");
   ASSERT_STREQ(Path::suffix("/foo/bar/file.foo.txt").c_str(), ".txt");
   ASSERT_STREQ(Path::suffix(".txt").c_str(), ".txt");
   ASSERT_STREQ(Path::suffix("/a/b.c/d").c_str(), "");
   ASSERT_STREQ(Path::suffix("").c_str(), "");
   ASSERT_STREQ(Path::suffix("txt").c_str(), "");
}

TEST(TestDir, createDir)
{
   ASSERT_EQ(Path::exist("/tmp"), true);
   std::remove("/tmp/foo/bar");

   ASSERT_EQ(Path::createDir("foo/bar", "/tmp"), true);
   ASSERT_EQ(Path::exist("/tmp/foo/bar"), true);
   ASSERT_EQ(Path::isDir("/tmp/foo/bar"), true);
   ASSERT_EQ(Path::isWritable("/tmp/foo/bar"), true);
   ASSERT_EQ(Path::isReadable("/tmp/foo/bar"), true);

   ASSERT_EQ(Path::createDir("foo/bar", "doesnotexist"), false);
   ASSERT_EQ(Path::exist("doesnotexist/foo/bar"), false);

   ASSERT_EQ(Path::createDir("foo", "/usr/bin"), false);
   ASSERT_EQ(Path::exist("/usr/bin/foo"), false);

   ASSERT_EQ(Path::createDir("", "/tmp"), true);
   ASSERT_EQ(Path::exist("/tmp"), true);

   ASSERT_EQ(Path::createDir("tmp", ""), true);
   ASSERT_EQ(Path::exist("tmp"), true);
   std::remove("tmp");

   ASSERT_EQ(Path::createDir("", ""), false);
}

TEST(TestDir, createTmpName)
{
   std::string dir = Path::createTmpName("/tmp", "foo");
  std::cout << dir << std::endl;
   ASSERT_EQ(Path::exist(dir), false);
   ASSERT_EQ(Path::isDir(dir), false);
   ASSERT_EQ(Path::isWritable(dir), false);
   ASSERT_EQ(Path::isReadable(dir), false);

   ASSERT_EQ(Path::createDir(dir, ""), true);
   ASSERT_EQ(Path::exist(dir), true);
   ASSERT_EQ(Path::isDir(dir), true);
   ASSERT_EQ(Path::isWritable(dir), true);
   ASSERT_EQ(Path::isReadable(dir), true);
}

TEST(TestDir, canonicalPath)
{
   ASSERT_STREQ(Path::canonicalPath("/foo/bar/file.txt").c_str(), "/foo/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("./foo/bar/file.txt").c_str(), "./foo/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("/foo/../bar/file.txt").c_str(), "/bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("./foo/../bar/file.txt").c_str(), "./bar/file.txt");
   ASSERT_STREQ(Path::canonicalPath("").c_str(), "");
   ASSERT_STREQ(Path::canonicalPath("..").c_str(), "..");
   ASSERT_STREQ(Path::canonicalPath("/").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("//").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("////").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("///.///").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("//.").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("/..").c_str(), "/");
   ASSERT_STREQ(Path::canonicalPath("/out").c_str(), "/out");
   ASSERT_STREQ(Path::canonicalPath("./out").c_str(), "./out");
   ASSERT_STREQ(Path::canonicalPath("./././out").c_str(), "./out");
   ASSERT_STREQ(Path::canonicalPath("./out/./bin").c_str(), "./out/bin");
   ASSERT_STREQ(Path::canonicalPath("./out/./././bin").c_str(), "./out/bin");
   ASSERT_STREQ(Path::canonicalPath("out/../../bin").c_str(), "../bin");
   ASSERT_STREQ(Path::canonicalPath("../../bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("../..//bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("../.././bin").c_str(), "../../bin");
   ASSERT_STREQ(Path::canonicalPath("/../out/../in").c_str(), "/in");
   ASSERT_STREQ(Path::canonicalPath("/../out/../in/").c_str(), "/in");
   ASSERT_STREQ(Path::canonicalPath("/does/not/exist//data/somefolder").c_str(),
                "/does/not/exist/data/somefolder");
}

TEST(TestDir, normalize)
{
   ASSERT_STREQ(Path::normalize("A//B").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/B/").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/B//").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/./B").c_str(), "A/B");
   ASSERT_STREQ(Path::normalize("A/foo/../B").c_str(), "A/B");
}
