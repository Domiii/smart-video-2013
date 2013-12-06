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

// POSIX
#include <dirent.h>

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
    

    /// TODO Move the below stuff to a different file

    const static float SpeedAttenuationWindow = 10.f;

    /// Progress bar displayed in a console.
    /// Requires console to support \r to go back to beginning of line.
    /// Also requires no other console output to interfere with progress bar output.
    struct ConsoleProgressBar
    {
        int nValue, nMax, nMaxDisplayLen;

        std::chrono::steady_clock::time_point lastUpdateTime;
        float nSpeed;

        ConsoleProgressBar(int nMaxDisplayLen) :
            nValue(0),
            nMaxDisplayLen(nMaxDisplayLen)
        {
            lastUpdateTime = std::chrono::steady_clock::now();
        }

        void InitProgressBar(int nMax)
        {
            nValue = -nMax;
            this->nMax = nMax;
            nSpeed = 0;

            UpdateProgress(1);
        }

        /// Sets the new current value for this progress bar. 
        /// Re-draws bar if number of bars have changed.
        void UpdateProgress(int nNewValue)
        {   
            // draw progress to console
            std::string frameNumberString = std::to_string(nNewValue) + " / " + std::to_string(nMax);
            float progress = static_cast<float>(nNewValue) / nMax;
            float lastProgress = static_cast<float>(nValue) / nMax;
            int progressLen = static_cast<int>(nMaxDisplayLen * progress + .5f);
            int lastProgressLen = static_cast<int>(nMaxDisplayLen * lastProgress + .5f);

            // progress bar moved, or we processed the last frame
            if (progressLen != lastProgressLen || nNewValue == nMax)
            {
                std::string progressString("|");
                progressString.reserve(nMaxDisplayLen + 2);
                for (int i = 1; i < nMaxDisplayLen; ++i)
                {
                    if (i <= progressLen)
                        progressString += 'i';
                    else
                        progressString += ' ';
                }
                progressString += '|';
            
                std::cout << '\r' << progressString << std::setw(15) << frameNumberString << " (" << std::setprecision(3) << (100 * progress) << "%)   ";
                std::cout.flush();
            }

            // update value
            nValue = nNewValue;

            // update speed
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(lastUpdateTime - now).count();
            
        }
    };
}

#endif // UTIL_UTIL_H