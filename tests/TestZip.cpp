#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  include "gmock/gmock.h"
#  include "gtest/gtest.h"
# pragma GCC diagnostic pop

#include <fstream>
#include <chrono>

#include "../zipper/CDirEntry.h"

#define protected public
#define private public
//namespace TestZip {
#include "../zipper/zipper.h"
//}
#undef protected
#undef private

using namespace zipper;

TEST(TestZip, zipping)
{
   if (CDirEntry::exist("ziptest.zip"))
      CDirEntry::remove("ziptest.zip");

   Zipper zipper("ziptest.zip");
}

