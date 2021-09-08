#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  include "gmock/gmock.h"
#  include "gtest/gtest.h"
# pragma GCC diagnostic pop

#include "../zipper/CDirEntry.h"

using namespace zipper;

TEST(TestDir, exist)
{
   ASSERT_EQ(CDirEntry::exist(PWD), true);
   ASSERT_EQ(CDirEntry::exist(PWD"/main.cpp"), true);
   ASSERT_EQ(CDirEntry::exist(""), false);
   ASSERT_EQ(CDirEntry::exist("fsdfqfz"), false);
}

TEST(TestDir, isDir)
{
   ASSERT_EQ(CDirEntry::isDir(PWD), true);
   ASSERT_EQ(CDirEntry::isDir(PWD"/main.cpp"), false);
   ASSERT_EQ(CDirEntry::isDir(""), false);
   ASSERT_EQ(CDirEntry::isDir("sdsdqsd"), false);
}

TEST(TestDir, isFile)
{
   ASSERT_EQ(CDirEntry::isFile(PWD), false);
   ASSERT_EQ(CDirEntry::isFile(PWD"/main.cpp"), true);
   ASSERT_EQ(CDirEntry::isFile(""), false);
   ASSERT_EQ(CDirEntry::isFile("aazaza"), false);
}

TEST(TestDir, isReadable)
{
   ASSERT_EQ(CDirEntry::isReadable(PWD), true);
   ASSERT_EQ(CDirEntry::isReadable(PWD"/main.cpp"), true);
   ASSERT_EQ(CDirEntry::isReadable(""), false);
   ASSERT_EQ(CDirEntry::isReadable("qdqdqsdqdq"), false);
   ASSERT_EQ(CDirEntry::isReadable("/usr/bin"), true);
   ASSERT_EQ(CDirEntry::isReadable("/usr/bin"), true);
}

TEST(TestDir, isWritable)
{
   ASSERT_EQ(CDirEntry::isWritable(PWD), true);
   ASSERT_EQ(CDirEntry::isWritable(PWD"/main.cpp"), true);
   ASSERT_EQ(CDirEntry::isWritable(""), false);
   ASSERT_EQ(CDirEntry::isWritable("qdqdqsdqdq"), false);
   ASSERT_EQ(CDirEntry::isWritable("/usr/bin"), false);
}

TEST(TestDir, baseName)
{
   ASSERT_STREQ(CDirEntry::baseName("/foo/bar/file.txt").c_str(), "file");
   ASSERT_STREQ(CDirEntry::baseName("/foo/bar/file.foo.txt").c_str(), "file.foo");
   ASSERT_STREQ(CDirEntry::baseName("/foo/bar").c_str(), "bar");
   ASSERT_STREQ(CDirEntry::baseName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(CDirEntry::baseName("./foo/../bar/file.txt").c_str(), "file");
   ASSERT_STREQ(CDirEntry::baseName("./foo/../bar/../file.txt").c_str(), "file");
   ASSERT_STREQ(CDirEntry::baseName("").c_str(), "");
   //KO ASSERT_STREQ(CDirEntry::baseName("..").c_str(), "..");
   //KO ASSERT_STREQ(CDirEntry::baseName("../..").c_str(), "..");
   ASSERT_STREQ(CDirEntry::baseName("../../").c_str(), "");
   //KO ASSERT_STREQ(CDirEntry::baseName(".").c_str(), ".");
   ASSERT_STREQ(CDirEntry::baseName("/").c_str(), "");
   ASSERT_STREQ(CDirEntry::baseName("//").c_str(), "");
   ASSERT_STREQ(CDirEntry::baseName("//.").c_str(), "");
}

TEST(TestDir, fileName)
{
   ASSERT_STREQ(CDirEntry::fileName("/foo/bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(CDirEntry::fileName("/foo/bar/file.foo.txt").c_str(), "file.foo.txt");
   ASSERT_STREQ(CDirEntry::fileName("/foo/bar").c_str(), "bar");
   ASSERT_STREQ(CDirEntry::fileName("/foo/bar/").c_str(), "");
   ASSERT_STREQ(CDirEntry::fileName("./foo/../bar/file.txt").c_str(), "file.txt");
   ASSERT_STREQ(CDirEntry::fileName("./foo/../bar/../file.txt").c_str(), "file.txt");
   ASSERT_STREQ(CDirEntry::fileName("").c_str(), "");
   ASSERT_STREQ(CDirEntry::fileName("..").c_str(), "..");
   ASSERT_STREQ(CDirEntry::fileName("/").c_str(), "");
   ASSERT_STREQ(CDirEntry::fileName("//").c_str(), "");
   ASSERT_STREQ(CDirEntry::fileName("//.").c_str(), ".");
}

TEST(TestDir, dirName)
{
   ASSERT_STREQ(CDirEntry::dirName("/foo/bar/file.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(CDirEntry::dirName("/foo/bar/file.foo.txt").c_str(), "/foo/bar");
   ASSERT_STREQ(CDirEntry::dirName("/foo/bar").c_str(), "/foo");
   //KO ASSERT_STREQ(CDirEntry::dirName("/foo/bar/").c_str(), "/foo/bar");
   ASSERT_STREQ(CDirEntry::dirName("./foo/../bar/file.txt").c_str(), "./foo/../bar");
   ASSERT_STREQ(CDirEntry::dirName("./foo/../bar/../file.txt").c_str(), "./foo/../bar/..");
   ASSERT_STREQ(CDirEntry::dirName("").c_str(), "");
   ASSERT_STREQ(CDirEntry::dirName("..").c_str(), "");
   //KO ASSERT_STREQ(CDirEntry::dirName("/").c_str(), "/");
   //KO ASSERT_STREQ(CDirEntry::dirName("//").c_str(), "//");
   //KO ASSERT_STREQ(CDirEntry::dirName("//.").c_str(), "//");
}

TEST(TestDir, suffix)
{
   ASSERT_STREQ(CDirEntry::suffix("/foo/bar/file.txt").c_str(), ".txt");
   ASSERT_STREQ(CDirEntry::suffix("/foo/bar/file.foo.txt").c_str(), ".txt");
   ASSERT_STREQ(CDirEntry::suffix(".txt").c_str(), ".txt");
   ASSERT_STREQ(CDirEntry::suffix("/a/b.c/d").c_str(), "");
   ASSERT_STREQ(CDirEntry::suffix("").c_str(), "");
   ASSERT_STREQ(CDirEntry::suffix("txt").c_str(), "");
}

TEST(TestDir, createDir)
{
   ASSERT_EQ(CDirEntry::exist("/tmp"), true);
   std::remove("/tmp/foo/bar");

   ASSERT_EQ(CDirEntry::createDir("foo/bar", "/tmp"), true);
   ASSERT_EQ(CDirEntry::exist("/tmp/foo/bar"), true);
   ASSERT_EQ(CDirEntry::isDir("/tmp/foo/bar"), true);
   ASSERT_EQ(CDirEntry::isWritable("/tmp/foo/bar"), true);
   ASSERT_EQ(CDirEntry::isReadable("/tmp/foo/bar"), true);

   ASSERT_EQ(CDirEntry::createDir("foo/bar", "doesnotexist"), false);
   ASSERT_EQ(CDirEntry::exist("doesnotexist/foo/bar"), false);

   ASSERT_EQ(CDirEntry::createDir("foo", "/usr/bin"), false);
   ASSERT_EQ(CDirEntry::exist("/usr/bin/foo"), false);

   ASSERT_EQ(CDirEntry::createDir("", "/tmp"), true);
   ASSERT_EQ(CDirEntry::exist("/tmp"), true);

   ASSERT_EQ(CDirEntry::createDir("tmp", ""), true);
   ASSERT_EQ(CDirEntry::exist("tmp"), true);
   std::remove("tmp");

   ASSERT_EQ(CDirEntry::createDir("", ""), false);
}

TEST(TestDir, createTmpName)
{
   std::string dir = CDirEntry::createTmpName("/tmp", "foo");
  std::cout << dir << std::endl;
   ASSERT_EQ(CDirEntry::exist(dir), false);
   ASSERT_EQ(CDirEntry::isDir(dir), false);
   ASSERT_EQ(CDirEntry::isWritable(dir), false);
   ASSERT_EQ(CDirEntry::isReadable(dir), false);

   ASSERT_EQ(CDirEntry::createDir(dir, ""), true);
   ASSERT_EQ(CDirEntry::exist(dir), true);
   ASSERT_EQ(CDirEntry::isDir(dir), true);
   ASSERT_EQ(CDirEntry::isWritable(dir), true);
   ASSERT_EQ(CDirEntry::isReadable(dir), true);
}

TEST(TestDir, canonicalPath)
{
   ASSERT_STREQ(CDirEntry::canonicalPath("/foo/bar/file.txt").c_str(), "/foo/bar/file.txt");
   ASSERT_STREQ(CDirEntry::canonicalPath("./foo/bar/file.txt").c_str(), "./foo/bar/file.txt");
   ASSERT_STREQ(CDirEntry::canonicalPath("/foo/../bar/file.txt").c_str(), "/bar/file.txt");
   ASSERT_STREQ(CDirEntry::canonicalPath("./foo/../bar/file.txt").c_str(), "./bar/file.txt");
   ASSERT_STREQ(CDirEntry::canonicalPath("").c_str(), "");
   ASSERT_STREQ(CDirEntry::canonicalPath("..").c_str(), "..");
   ASSERT_STREQ(CDirEntry::canonicalPath("/").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("//").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("////").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("///.///").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("//.").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("/..").c_str(), "/");
   ASSERT_STREQ(CDirEntry::canonicalPath("/out").c_str(), "/out");
   ASSERT_STREQ(CDirEntry::canonicalPath("./out").c_str(), "./out");
   ASSERT_STREQ(CDirEntry::canonicalPath("./././out").c_str(), "./out");
   ASSERT_STREQ(CDirEntry::canonicalPath("./out/./bin").c_str(), "./out/bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("./out/./././bin").c_str(), "./out/bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("out/../../bin").c_str(), "../bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("../../bin").c_str(), "../../bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("../..//bin").c_str(), "../../bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("../.././bin").c_str(), "../../bin");
   ASSERT_STREQ(CDirEntry::canonicalPath("/../out/../in").c_str(), "/in");
}

TEST(TestDir, normalize)
{
   ASSERT_STREQ(CDirEntry::normalize("A//B").c_str(), "A/B");
   //KO ASSERT_STREQ(CDirEntry::normalize("A/B/").c_str(), "A/B");
   //KO ASSERT_STREQ(CDirEntry::normalize("A/B//").c_str(), "A/B");
   ASSERT_STREQ(CDirEntry::normalize("A/./B").c_str(), "A/B");
   ASSERT_STREQ(CDirEntry::normalize("A/foo/../B").c_str(), "A/B");
}

