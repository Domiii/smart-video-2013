#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

//C
#include <cstdio>

//C++
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>


// lightweight Json library
#include "JSonUtil.h"

namespace Util
{
    typedef unsigned int uint32;

    /// Trim from start.
    static inline std::string &ltrim(std::string &s) 
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
    }

    /// Trim from end.
    static inline std::string &rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
    }

    /// Trim from both ends.
    /// See: http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
    static inline std::string &trim(std::string &s) 
    {
        return ltrim(rtrim(s));
    }

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
            line = trim(line);
            if (line.size() > 0)
                lines.push_back(line);
        }

        return lines;
    }

    /// Write the given vector into a text file, each line containing one value.
    template<typename T>
    inline void WriteLines(std::string fname, std::vector<T> values)
    {
        std::ofstream file(fname);
        for (auto value : values)
        {
            file << value << "\n";
        }
    }

    /// Gets the size of the given file
    inline std::ifstream::pos_type GetFileSize(const char* filename)
    {
        std::ifstream in(filename, std::ifstream::in | std::ifstream::binary);
        in.seekg(0, std::ifstream::end);
        return in.tellg(); 
    }
    
}

#endif // UTIL_UTIL_H