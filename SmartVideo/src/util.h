#ifndef SMARTVIDEO_UTIL_H
#define SMARTVIDEO_UTIL_H

//C
#include <stdio.h>

//C++
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

// lightweight Json library
#include "JSonUtil.h"

namespace SmartVideo
{
    /// Reads all lines from the given file and stores them in the returned vector.
    inline std::string ReadText(std::string fname)
    {
        std::ifstream t(fname);
        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    /// Reads all lines from the given file and stores them in the returned vector.
    inline std::vector<std::string> ReadLines(std::string fname)
    {
        std::ifstream infile(fname);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(infile, line))
        {
            lines.push_back(line);
        }

        return lines;
    }

    /// Gets the size of the given file
    inline std::ifstream::pos_type GetFileSize(const char* filename)
    {
        std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
        in.seekg(0, std::ifstream::end);
        return in.tellg(); 
    }
}

#endif // SMARTVIDEO_UTIL_H