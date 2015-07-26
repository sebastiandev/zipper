# Zipper
C++ wrapper around minizip compression library using the latest C++11.

Zipper's goal is to bring the power and simplicity of minizip to a more object oriented/c++ user friendly library.
It was born out of the necessity of a compression library that would be reliable, simple and flexible. By flexibility I mean supporting all kinds of inputs and outputs but specifically been able to compress into memory instead of been restricted to file compression only, and using data from memory instead of just files as well.

#### Features:
- [x] Create zip in memory
- [x] Allow files, vector and generic streams as input to zip
- [x] File mappings for replacing strategies (overwrite if exists or use alternative name from mapping)
- [ ] Password protected zip


#### Configuration
Zipper depends on minizip and zlib. Minizip is used as a submodule, thus it is compiled with the solution. Zlib is expected to be found at ZLIBROOT. 
```
ZLIBROOT = c:\Projects\zlib-1.2.8\

Ex: c:\Projects\zlib-1.2.8\
                      |_ include\
                      |_ lib\
```
Compilation produces zipper.lib

So far its been tested and focused on Windows using Visual Studio 2013


#### Usage:

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
unzipper.extract(alternativeNames);
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
