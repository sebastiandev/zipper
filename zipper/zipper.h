#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <ctime>

namespace zipper {

class Zipper
{
public:
    // Minizip options/params:
    //              -o                -a             -0            -1             -9             -j
    enum zipFlags { Overwrite = 0x01,
                    Append = 0x02,
                    Store = 0x04,
                    Faster = 0x08,
                    Better = 0x10,
                    NoPaths = 0x20,
                    SaveHierarchy = 0x40 };

    Zipper(std::iostream& buffer, const std::string& password = std::string());
    Zipper(std::vector<unsigned char>& buffer, const std::string& password = std::string());
    Zipper(const std::string& zipname, const std::string& password = std::string());

    ~Zipper();

    bool add(std::istream& source, const std::tm& timestamp, const std::string& nameInZip, zipFlags flags = Better);
    bool add(std::istream& source, const std::string& nameInZip, zipFlags flags = Better);
    bool add(const std::string& fileOrFolderPath, zipFlags flags = Better);

    void open();
    void close();

private:

    void release();

private:

    std::iostream& m_obuffer;
    std::vector<unsigned char>& m_vecbuffer;
    std::string m_zipname;
    std::string m_password;
    bool m_usingMemoryVector;
    bool m_usingStream;
    bool m_open;

    struct Impl;
    Impl* m_impl;
};

} // namespace zipper
