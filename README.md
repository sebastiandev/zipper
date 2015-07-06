# Zipper
C++ wrapper around minizip compression library using the latest C++11.

**WARNING: This is a work in progress and shouldn't be used yet until stability is reached**

Zipper's goal is to bring the power and simplicity of minizip to a more object oriented/c++ user friendly library.
It was born out of the necesity of a compression library that would be reliable, simple and flexible. By flexibility I mean supporting all kinds of inputs and outputs but specifically been able to compress into memory instead of been restricted to file compression only, and using data from memory instead of just files as well.

#### Features:
- [x] Create zip in memory
- [x] Allow files, vector and generic streams as input to zip
- [ ] Password protected zip
- [ ] File mappings for replacing strategies (if file exists overwrite or use alternative name provided in mapping)


#### Configuration
Zipper depends on minizip and zlib. Minizip is used as a submodule, thus it is compiled with the solution. Zlib is expected to be found at ZLIBROOT. 
```
ZLIBROOT = c:\Projects\zlib-1.2.8\

Ex: c:\Projects\zlib-1.2.8\
                      |_ include\
                      |_ lib\
```
Compilation produces zlib.lib

So far its been tested and focused on Windows using Visual Studio 2013


#### Usage:

There are two classes available Zipper and Unzipper. They behave in the same manner regarding constructors and storage parameters.

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
- TODO
