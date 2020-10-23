/*

    Use the current way of playing audio here.
    See: soundplayback.cpp@master

    Would be nice to change that though, the problem is that miniaudio can't create sinks (as far as I know)
    However we can use miniaudio to play the audio on that sink - this would remove the depenency on mpg123 (?)

*/
#ifdef __linux___
#include <exception>
#include <iostream>
#include <stdio.h>
#include <string>
#include <memory>
#include <vector>
#include <array>

namespace Soundux
{
    namespace Playback
    {
        namespace internal
        {
            inline std::string getOutput(const std::string &command)
            {
                std::array<char, 128> buffer;
                std::string result;
                std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
                if (!pipe)
                {
                    throw std::exception("popen failed");
                    return "";
                }
                while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
                {
                    result += buffer.data();
                }
                return result;
            }

            inline void createSink();
            inline void deleteSink();
            inline void getDefaultCaptureDevice();
            inline std::vector<std::string> getSources();
        } // namespace internal
    }     // namespace Playback
} // namespace Soundux
#endif