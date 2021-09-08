rm -fr *.gcda *.gcno rapport.info doc unit-tests

g++ -W -Wall --std=c++14 --coverage -UUSE_WINDOWS -I.. -DPWD=\"`pwd`\" main.cpp TestDir.cpp TestZip.cpp -o unit-tests `pkg-config --cflags --libs gtest gmock zipper`

./unit-tests

# Code coverage
COVERAGE_RAPPORT=rapport.info
COVERAGE_DIR=doc
TARGET=logexporter
lcov --quiet --directory .. -c -o $COVERAGE_RAPPORT
lcov --quiet --remove $COVERAGE_RAPPORT '/usr*' 'external/*' 'tests/*' -o $COVERAGE_RAPPORT
genhtml --quiet -o $COVERAGE_DIR -t "$TARGET" $COVERAGE_RAPPORT
xdg-open $COVERAGE_DIR/index.html >/dev/null
