// Copyright (C) 2008 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., EML Research, gGmbH, University of Heidelberg,
// and The University of Manchester.
// All rights reserved.

// Copyright (C) 2001 - 2007 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc. and EML Research, gGmbH.
// All rights reserved.

#ifndef ZIPPER_UTILS_PATH_HPP
#  define ZIPPER_UTILS_PATH_HPP

#  include <string>
#  include <vector>

namespace zipper {

// *****************************************************************************
//! \brief This class provides an OS independent interface to directory entries
//! such as files and directories.
// *****************************************************************************
class Path
{
public:

    //! \brief The character used to separate directory entries.
    static const std::string Separator;

    //! \brief
    std::string currentPath();

    //! \brief Check whether the directory entry specified by 'path' is
    //! a file.
    //! \param[in] path: file path.
    //! \return bool isFile
    static bool isFile(const std::string& path);

    //! \brief Check whether the directory entry specified by 'path' is
    //! is a directory.
    //! \param[in] path: file path.
    //! \return bool isDir
    static bool isDir(const std::string& path);

    //! \brief Check whether the directory entry specified by 'path' exists.
    //! \param[in] path: file path.
    //! \return bool exist
    static bool exist(const std::string& path);

    //! \brief Check whether the directory entry specified by 'path' is
    //! is readable.
    //! \param[in] path: file path.
    //! \return bool isReadable
    static bool isReadable(const std::string& path);

    //! \brief Check whether the directory entry specified by 'path' is
    //! writable.
    //! \param[in] path: file path.
    //! \return bool isWritable
    static bool isWritable(const std::string& path);

    //! \brief Returns the base name, i.e., the directory path and the
    //! the suffix are removed from 'path'.
    //! \param[in] path: file path.
    //! \return std::string baseName
    static std::string baseName(const std::string& path);

    //! \brief Returns the file name, i.e., the directory path is removed from 'path'.
    //! \param[in] path: file path.
    //! \return std::string fileName
    static std::string fileName(const std::string& path);

    //! \brief Returns the directory path to the parent directoryu, i.e.,
    //! the file name or directory name are removed from 'path'.
    //! \param[in] path: file path.
    //! \return std::string dirName
    static std::string dirName(const std::string& path);

    //! \brief Returns the suffix, i.e., the directory path and the
    //! the base name are removed from 'path'.
    //! \param[in] path: file path.
    //! \return std::string basename
    static std::string suffix(const std::string& path);

    //! \brief Create the directory 'dir' in the parent directory 'parent'.
    //! \param[in] dir: folder path.
    //! \param[in] parent (Default: current working directory)
    //! \return bool success
    static bool createDir(const std::string& dir,
                          const std::string& parent = "");

    static void removeDir(const std::string& foldername);

    //! \brief
    static std::vector<std::string> filesFromDir(const std::string& path);

    //! \brief Create a name for a temporary directory entry. The directory entry
    //! will be located in the directory given
    //! \param[in] dir: folder path.
    //! \param[in] suffix: file extension.
    //! \return std::string tmpName
    static std::string createTmpName(const std::string& dir,
                                     const std::string& suffix);

    //! \brief Move a file from. If to is the directory the filename of from is
    //! appended.
    //! \param[in] from: source path.
    //! \param[in] to:  destination path.
    //! \return bool success
    static bool move(const std::string& from,
                     const std::string& to);

    //! \brief Removes a file or directory specified by path.
    //! \param[in] path: file path.
    //! \return bool success

    static bool remove(const std::string& path);

    //! \brief Remove files or directories matching the pattern in directory dir.
    //! \param[in] pattern
    //! \param[in] dir
    //! \return bool success
    static bool removeFiles(const std::string& pattern,
                            const std::string& dir);

    //! \brief Compiles the pattern to a patternList. Valid wildcards in the pattern are:
    //! '*' matches any number of characters and '?' matches exactly one character.
    //! \param[in] pattern
    //! \return std::vector< std::string > patternList

    static std::vector<std::string> compilePattern(const std::string& pattern);

    //! \brief Compare the name against the pattern list and returns whether the
    //! name matches. The patternList can be created from a pattern by the
    //! compilePattern method.
    //! \param[in] name
    //! \param[in] patternList
    //! \return bool match
    static bool match(const std::string& name,
                      const std::vector<std::string>& patternList);

    //! \brief Checks whether the given path is relative
    //! \param[in] path: file path.
    //! \return bool isRelative

    static bool isRelativePath(const std::string& path);

    //! \brief Makes the absolute path relative to the path given in relativeTo
    //! \param[in] absolutePath
    //! \param[in] relativeTo
    //! \return bool success
    static bool makePathRelative(std::string& absolutePath,
                                 const std::string& relativeTo);

    //! \brief Makes the relative path absolute to the path given in absoluteTo
    //! \param[in] relativePath
    //! \param[in] absoluteTo
    //! \return bool success
    static bool makePathAbsolute(std::string& relativePath,
                                 const std::string& absoluteTo);

    //! \brief This method normalizes the path, i.e.,
    //! it converts all '\' to '/' (only on WIN32)
    //! and collapses '^./' to '^', '/./' to '/', and '[^/]+/../' to '/'
    //! \param[in] path
    //! \return std::string normalizedPath
    static std::string normalize(const std::string& path);

    //! \brief This substitute ../ ie foo/../bar will return bar
    static std::string canonicalPath(const std::string& path);

    static bool isLargeFile(std::istream& input_stream);

private:

    //! \brief This private methods checks whether the active section matches the
    //! specified patter. The section is automatically advanced to allow
    //! repeated calls. On the first call 'at' must be 0. The parameters
    //! 'at' and 'after' must not be changed outside this method.
    //! \param[in] name
    //! \param[in] pattern
    //! \param[out] at
    //! \param[out] after
    //! \return bool match
    static bool matchInternal(const std::string& name,
                              const std::string pattern,
                              std::string::size_type& at,
                              std::string::size_type& after);
};

} // namespace

#endif // ZIPPER_UTILS_PATH_HPP
