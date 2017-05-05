![Zipper](https://github.com/sebastiandev/zipper/blob/master/logo.png)

C++ wrapper around minizip compression library

**Zipper**'s goal is to bring the power and simplicity of minizip to a more object oriented/c++ user friendly library.
It was born out of the necessity of a compression library that would be reliable, simple and flexible. 
By flexibility I mean supporting all kinds of inputs and outputs, but specifically been able to compress into memory instead of been restricted to file compression only, and using data from memory instead of just files as well.

### Features:
- [x] Create zip in memory
- [x] Allow files, vector and generic streams as input to zip
- [x] File mappings for replacing strategies (overwrite if exists or use alternative name from mapping)
- [x] Password protected zip
- [x] Multi platform


### Getting Started

In order to use and compile zipper you need to have [zlib](http://www.zlib.net) source files.
**Zipper** depends on minizip as well but since it is used as a submodule, you get it when cloning
the repo and it gets compiled with the project.

*Note*: For windows users, zlib is expected to be found at ZLIBROOT.

#### Download dependencies

```shell
sudo apt-get install zlib-dev  # for ubuntu

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

### Usage:

There are two classes available Zipper and Unzipper. They behave in the same manner regarding constructors and storage parameters. (for a complete example take a look at the [tests](https://github.com/sebastiandev/zipper/blob/develop/test/file_zip_test.cpp ) using the awesome BDD's from Catch library )

##### Zipping

- Creating a zip file with 2 files:
```c++
using namespace zipper;

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

std::vector<char> zip_vect;
std::ifstream input1("some file");

Zipper zipper(zip_vect);
zipper.add(input1, "Test1");
zipper.close();
```

##### Unzipping
- Getting all entries in zip
```c++
Unzipper unzipper("zipfile.zip");
std::vector<ZipEntry> entries = unzipper.entries();
unzipper.close();
```

- Extracting all entries from zip
```c++
Unzipper unzipper("zipfile.zip");
unzipper.extract();
unzipper.close();
```

- Extracting all entries from zip using alternative names for existing files on disk
```c++
std::map<std::string, std::string> alternativeNames = { {"Test1", "alternative_name_test1"} };
Unzipper unzipper("zipfile.zip");
unzipper.extract(".", alternativeNames);
unzipper.close();
```

- Extracting a single entry from zip
```c++
Unzipper unzipper("zipfile.zip");
unzipper.extractEntry("entry name");
unzipper.close();
```

- Extracting a single entry from zip to memory
```c++
std::vector<unsigned char> unzipped_entry;
Unzipper unzipper("zipfile.zip");
unzipper.extractEntryToMemory("entry name", unzipped_entry);
unzipper.close();
```
