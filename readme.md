Super Duck (aka I dont give a duck!)
====================================

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Little tool to upload, download and delete files from an S3 bucket. The difference is that it stores the whole tree and can filter by name, so
you only _pay_ for uploads, download and deletion of files, as there is no need to pay for listing files and directories. 
Also it can export a list of the whole tree or sub-tree as a CSV or an Excel file (XLS).
The name is just an internal-joke about a program with similar functionality and name ;-).

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows.

## External dependencies
The following libraries are required:
* [Qt opensource framework](http://www.qt.io/).
* [Amazon Web Services SDK for C++](https://aws.amazon.com/sdk-for-cpp/).
* [curl library](https://curl.haxx.se/libcurl/). Only needed if using Mingw64 compiler, as AWS rely on it.
* [xlslib library](http://xlslib.sourceforge.net/).

# Install
The only current option is build from source as binaries are not provided.

# Screenshots
No screenshots.

# Repository information

**Version**: 1.0.0

**Status**: finished

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |  11           | 520         | 338              | 2131  |
| C/C++ Header                 |  10           | 204         | 611              |  354  |
| CMake                        |   1           | 18          |   8              |   72  |
| **Total**                    | **22**        | **743**     | **957**          | **2557** |
