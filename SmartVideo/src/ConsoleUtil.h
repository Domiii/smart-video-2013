#ifndef UTIL_CONSOLEUTIL_H
#define UTIL_CONSOLEUTIL_H

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

namespace Util
{
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
        void UpdateProgress(int nNewValue, std::string statusString = "")
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
            
                std::cout << '\r' << progressString << " " << std::setw(8) << frameNumberString << " (" << std::setprecision(3) << std::showpoint << (100 * progress) << "%)   ";
                std::cout << std::left << std::setw(80) << statusString;
                std::cout.flush();
            }

            // update value
            nValue = nNewValue;

            // TODO: update speed
            /*std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(lastUpdateTime - now).count();*/
            
        }
    };
}

#endif // UTIL_CONSOLEUTIL_H