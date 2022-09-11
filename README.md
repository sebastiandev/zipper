![Zipper](doc/logo.png)

C++ wrapper around minizip compression library. You are reading the work in progress version 2 of this library.

[Zipper](https://github.com/sebastiandev/zipper/tree/v2.x.y)'s goal is to bring the power and simplicity of minizip to a more object oriented/c++ user friendly library.
It was born out of the necessity of a compression library that would be reliable, simple and flexible.
By flexibility I mean supporting all kinds of inputs and outputs, but specifically been able to compress into memory instead of been restricted to file compression only, and using data from memory instead of just files as well.

### Features

- [x] Create zip in memory.
- [x] Allow files, vector and generic streams as input to zip.
- [x] File mappings for replacing strategies (overwrite if exists or use alternative name from mapping).
- [x] Password protected zip (EAS).
- [x] Multi platform.

### TODO

- [x] Protection against [Zip Slip](https://github.com/sebastiandev/zipper/issues/33) has been added in this v2.x.y branch but
not fully tested by unit tests.
- [x] Write non-regression tests and improve the GitHub workflow (CI).
- [] Use C++17 for using filesystem. This branch needs to be merged with the dev-modernize branch.

### :warning: Security Notice

- Zipper is no longer actively maintained.
- Zipper currently follows an outdated (and probably vulnerable) version of the minizip library.
- While some fixes have been added this lib may be still vulnerable to ZipSlip attack and mitigations
should be put in place by Zipper's users.

### Getting Started

#### Compiling

Compilation is made with the following steps:

```shell
git clone https://github.com/sebastiandev/zipper.git -b "v2.x.y" --recursive
cd zipper
make download-external-libs
make compile-external-libs
make -j`nproc --all`
# make -j`sysctl -n hw.logicalcpu` for MacOS X
```

Notes:
- `make download-external-libs` will git clone zlib and moinizip to the `external` folder. This replaces git submodules.
- `make compile-external-libs` will compile zlib and moinizip but not install them.
- For developpers: these two commands has to be compiled once.

#### Installing

Following the previous section `Compiling`, type:

```shell
sudo make install
```

You will see a message like:

```shell
*** Installing: doc => /usr/share/Zipper/2.0.0/doc
*** Installing: libs => /usr/lib
*** Installing: pkg-config => /usr/lib/pkgconfig
*** Installing: headers => /usr/include/Zipper-2.0.0
*** Installing: Zipper => /usr/include/Zipper-2.0.0
```

### Usage

There are two classes available Zipper and Unzipper. They behave in the same manner regarding constructors and storage parameters. (for a complete example take a look at the [zip file tests](test/file_zip_test.cpp) and [zip memory tests](test/memory_zip_test.cpp) using the awesome BDD's from Catch library )

#### Zipping

- Header:

```c++
#include <Zipper/Zipper.hpp>
using namespace zipper;
```

- Adding a password. You can protect your file by adding a password as a `std::string` as second parameter to any of `Zipper` constructors. Do not forget to call close else the zip will not well formed (`Unzipper` will fail opening it for example).

For example:

```c++
Zipper zipper("ziptest.zip", "mypassword");
...
zipper.close();
```

Throw `std::run_time` in case of failure.

- No password. Do not forget to call close else the zip will not well formed (`Unzipper` will fail opening it for example).

```c++
Zipper zipper("ziptest.zip");
...
zipper.close();
```

Throw `std::run_time` in case of failure.

- Appending files inside the archive.

By default the constructor `Zipper` for zip file uses an implicit flag `Zipper::Overwrite` you can change it by `Zipper::Append`.

- Creating a zip file with 2 files:

```c++
std::ifstream input1("some file");
std::ifstream input2("some file");

Zipper zipper("ziptest.zip");
zipper.add(input1, "Test1");
zipper.add(input2, "Test1");

zipper.close();
```

- Adding a file by name and an entire folder to a zip:

```c++
Zipper zipper("ziptest.zip");
zipper.add("somefile.txt");
zipper.add("myFolder/");
zipper.close();
```

- Creating a zip file using the awesome streams from boost that lets us use a vector as a stream:

```c++
#include <boost\interprocess\streams\vectorstream.hpp>
...

boost::interprocess::basic_vectorstream<std::vector<char>> input_data(some_vector);

Zipper zipper("ziptest.zip");
zipper.add(input_data, "Test1");
zipper.close();
```

- Creating a zip in memory stream with files:

```c++
#include <boost\interprocess\streams\vectorstream.hpp>
...

boost::interprocess::basic_vectorstream<std::vector<char>> zip_in_memory;
std::ifstream input1("some file");

Zipper zipper(zip_in_memory);
zipper.add(input1, "Test1");
zipper.close();
```

- Creating a zip in a vector with files:

```c++
std::vector<unsigned char> zip_vect;
std::ifstream input1("some file");

Zipper zipper(zip_vect);
zipper.add(input1, "Test1");
zipper.close();
```

- Changing the compression factor: By default the `add` method uses an implicit flag `Zipper::Better`
which compress the best but can takes some time to perform the compression. You can change this flag by `Zipper::Store` (no compression) or `Zipper::Faster` (for a light compression) or `Zipper::Medium` (for a compromise).

```c++
zipper.add(input1, "Test1", Zipper::Medium);
```

##### Unzipping

- Header:

```c++
#include <Zipper/Unzipper.hpp>
using namespace zipper;
```

- Extraction using a password: pass your password as a `std::string` as second parameter to any of constructors:

```c++
Zipper unzipper("ziptest.zip", "mypassword");
...
zipper.close();
```

Throw `std::run_time` in case of failure.

- Extraction not using a password.

```c++
Zipper unzipper("ziptest.zip");
...
zipper.close();
```

Throw `std::run_time` in case of failure.

- Getting all entries in the zip:

```c++
Unzipper unzipper("zipfile.zip");
std::vector<ZipEntry> entries = unzipper.entries();
for (auto& it: unzipper.entries())
{
    std::cout << it.name << ": "
              << it.timestamp
              << std::endl;
}
unzipper.close();
```

- Extracting all entries from the zip:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractAll(); // Fail if a file exists (false argument is implicit)
unzipper.extractAll(true);  // Replace existing files
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting all entries from zip to desired destination:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractAll("/the/destination/path");        // Fail if a file exists (false argument is implicit)
unzipper.extractAll("/the/destination/path", true);  // Replace existing files
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting all entries from zip using alternative names for existing files on disk:

```c++
std::map<std::string, std::string> alternativeNames = { {"Test1", "alternative_name_test1"} };
Unzipper unzipper("zipfile.zip");
unzipper.extractAll(".", alternativeNames);
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting a single entry from zip:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractEntry("entry name");
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting a single entry from zip to destination:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractEntry("entry name", "/the/destination/path"); // Fail if a file exists (false argument is implicit)
unzipper.extractEntry("entry name", "/the/destination/path", true); // Replace existing file
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting a single entry from zip to memory:

```c++
std::vector<unsigned char> unzipped_entry;
Unzipper unzipper("zipfile.zip");
unzipper.extractEntryToMemory("entry name", unzipped_entry);
unzipper.close();
```

Return `true` in case of success or `false` in case of failure.
In case of failure, use `unzipper.error();` to get the `std::error_code`.

- Extracting from a vector:

```c++

std::vector<unsigned char> zip_vect; // Populated with Zipper zipper(zip_vect);

Unzipper unzipper(zip_vect);
unzipper.extractEntry("Test1");
```

##### Linking Zipper to your project

- In your project add the needed headers in your c++ files:

```c++
#include <Zipper/Unzipper.hpp>
#include <Zipper/Zipper.hpp>
```

- To compile your project against Zipper use pkg-config:

```shell
g++ -W -Wall --std=c++11 main.cpp -o prog `pkg-config zipper --cflags --libs`
```

- For Makefile:
  - set `LDFLAGS` to `pkg-config zipper --libs`
  - set `CPPFLAGS` to `pkg-config zipper --cflags`

- For CMake:
```
include(FindPkgConfig)
find_package(zipper)
```

### For developpers

##### Unit Tests and code coverage

```shell
make coverage
```

##### Coding style

Before submitting a pull request, you can indent the code with the following command:

```shell
cd zipper
clang-format -i *.[ch]pp
```
