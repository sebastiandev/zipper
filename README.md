![Zipper](https://github.com/sebastiandev/zipper/blob/master/logo.png)

C++ wrapper around minizip compression library

**Zipper**'s goal is to bring the power and simplicity of minizip to a more object oriented/c++ user friendly library.
It was born out of the necessity of a compression library that would be reliable, simple and flexible.
By flexibility I mean supporting all kinds of inputs and outputs, but specifically been able to compress into memory instead of been restricted to file compression only, and using data from memory instead of just files as well.

### Features

- [x] Create zip in memory.
- [x] Allow files, vector and generic streams as input to zip.
- [x] File mappings for replacing strategies (overwrite if exists or use alternative name from mapping).
- [x] Password protected zip (EAS).
- [x] Multi platform.


### :warning: Security Notice
Zipper currently follows an unmaintaind and vulnerable version of the minizip library.
It is vulnerable to ZipSlip attack and mitigations should be put in place by Zipper's users.

> References:
>
> [ZipSlip](https://snyk.io/research/zip-slip-vulnerability),
> [CVE-2014-9485](https://cve.mitre.org/cgi-bin/cvename.cgi?name=2014-9485)

### Getting Started

In order to use and compile zipper you need to have [zlib](http://www.zlib.net) source files.
**Zipper** depends on minizip as well but since it is used as a submodule, you get it when cloning
the repo and it gets compiled with the project.

*Note*: For windows users, zlib is expected to be found at ZLIBROOT. Soon Zipper
will follow the new generation of zlib named [zlib-ng](https://github.com/zlib-ng/zlib-ng) and will be
compiled directly with this project.

#### Download dependencies

```shell
sudo apt-get install zlib1g-dev  # for ubuntu

sudo dnf install zlib-devel  # for fedora
sudo dnf install gcc-c++  # for fedora
```

#### Compiling

The preferred way is to create a folder for the compilation output to avoid polluting the root folder

```shell
git clone --recursive https://github.com/sebastiandev/zipper.git  # to get zipper and minizip submodule
cd zipper
mkdir build
cd build
cmake ../
make
```

#### Installing

Following the previous section `Compiling`, still from the `build` folder, type:

```shell
sudo make install
sudo ldconfig
```

You will see a message like:

```shell
Install the project...
-- Install configuration: "Release"
-- Installing: /usr/local/lib/libZipper.so.1.0.1
-- Up-to-date: /usr/local/lib/libZipper.so.1
-- Up-to-date: /usr/local/lib/libZipper.so
-- Installing: /usr/local/lib/libZipper.a
-- Installing: /usr/local/lib/libZipper-static.a
-- Installing: /usr/local/bin/Zipper-test
-- Installing: /usr/local/share/pkgconfig/zipper.pc
-- Installing: /usr/local/bin/Zipper-test
-- Installing: /usr/local/include/zipper/crypt.h
-- Installing: /usr/local/include/zipper/ioapi.h
-- Installing: /usr/local/include/zipper/ioapi_buf.h
-- Installing: /usr/local/include/zipper/ioapi_mem.h
-- Installing: /usr/local/include/zipper/iowin32.h
-- Installing: /usr/local/include/zipper/unzip.h
-- Installing: /usr/local/include/zipper/zip.h
-- Installing: /usr/local/include/zipper/CDirEntry.h
-- Installing: /usr/local/include/zipper/defs.h
-- Installing: /usr/local/include/zipper/tools.h
-- Installing: /usr/local/include/zipper/unzipper.h
-- Installing: /usr/local/include/zipper/zipper.h
-- Installing: /usr/local/lib/cmake/zipperConfig.cmake
-- Installing: /usr/local/lib/cmake/zipperTargets.cmake
-- Installing: /usr/local/lib/cmake/zipperTargets-release.cmake
```

### Usage

There are two classes available Zipper and Unzipper. They behave in the same manner regarding constructors and storage parameters. (for a complete example take a look at the [zip file tests](test/file_zip_test.cpp) and [zip memory tests](test/memory_zip_test.cpp) using the awesome BDD's from Catch library )

#### Zipping

- Header:

```c++
#include <zipper/zipper.h>
using namespace zipper;
```

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
zipper.add("myFolder");
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

- Adding a password. You can protect your file by adding a password as a `std::string` as second parameter to any of `Zipper` constructors. For example:

```c++
Zipper zipper("ziptest.zip", "mypassword");
```

- Appending files inside the archive.

By default the constructor `Zipper` for zip file uses an implicit flag `Zipper::Overwrite` you can
change it by `Zipper::Append`. Note: in previous versions of Zipper the `Zipper::Append` flag was the one by default but now replaced by `Zipper::Overwrite`.

##### Unzipping

- Header:

```c++
#include <zipper/unzipper.h>
using namespace zipper;
```

- Getting all entries in zip:

```c++
Unzipper unzipper("zipfile.zip");
std::vector<ZipEntry> entries = unzipper.entries();
unzipper.close();
```

- Extracting all entries from zip:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extract();
unzipper.close();
```

- Extracting all entries from zip using alternative names for existing files on disk:

```c++
std::map<std::string, std::string> alternativeNames = { {"Test1", "alternative_name_test1"} };
Unzipper unzipper("zipfile.zip");
unzipper.extract(".", alternativeNames);
unzipper.close();
```

- Extracting a single entry from zip:

```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractEntry("entry name");
unzipper.close();
```

- Extracting a single entry from zip to memory:

```c++
std::vector<unsigned char> unzipped_entry;
Unzipper unzipper("zipfile.zip");
unzipper.extractEntryToMemory("entry name", unzipped_entry);
unzipper.close();
```

- Extracting from a vector:

```c++

std::vector<unsigned char> zip_vect; // Populated with Zipper zipper(zip_vect);

Unzipper unzipper(zip_vect);
unzipper.extractEntry("Test1")
```

**Note:** Methods `extract`, `extractEntry`, `extractEntryToMemory` return a boolean indicating the success (`true`) or the failure (`false`).

- Extraction using a password. You can pass your password as a `std::string` as second parameter to any of constructors:

```c++
Zipper unzipper("ziptest.zip", "mypassword");
```

##### Linking Zipper to your project

In your project add the needed headers in your c++ files:

```c++
#include <zipper/unzipper.h>
#include <zipper/zipper.h>
```

There are several ways to link your project against Zipper:

- Straight forward: `g++ -W -Wall -I/usr/local/include main.cpp -o prog -L/usr/local/lib/ -lZipper -lz`. Note: you may have to adapt `/usr/local` to your installation directory (see the previous section `Installing`). You can also adapt and export your environment variable `LD_LIBRARY_PATH` (via you .bashrc for example).

- Pkg-config is a better alternative to the previous command:

```shell
g++ -W -Wall main.cpp -o prog `pkg-config zipper --cflags --libs`
```

Indeed pkg-config knows for you where to find libraries and, by default, it will choose the shared library. In the case it is not present then the static library will be chosen. You can force choosing the static library with `pkg-config libZipper --static --libs`

- Makefile: set `LDFLAGS` to `pkg-config zipper --libs` and set `CPPFLAGS` to `pkg-config zipper --cflags`

- CMake:  Simply place zipper in your project hierarchy, and then use `add_subdirectory(zipper)` or whatever you called the zipper folder. Then link it with `Zipper`/`staticZipper`

```cmake
Project(projZipper)

add_subdirectory(zipper)

add_executable(projZipper main.cpp)
target_link_libraries(
    projZipper
    PUBLIC
    staticZipper
)
```

### For developpers

##### Git branches

- The `developement` is here for your pull requests. This branch contains the latest fixes but can have regressions.
- The `master` is for maintenaning the stablest version.

Because, Zipper uses submodules, when your are commuting of branch, do not forget to synchronize your submodules:

``` shell
git submodule sync
git submodule update
```

##### Coding style

Before submitting a pull request, you can indent the code with the following command:

```shell
cd zipper
clang-format -i *.cpp *.h
```
