#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **args)
{
    if (argc < 2)
    {
        std::cerr << "Too few arguments!" << std::endl;
        return 1;
    }

    if (std::filesystem::exists(args[1]))
    {
        std::filesystem::create_directory("embedded");
        std::ofstream output("embedded/webview_base.hpp");
        output << "#pragma once" << std::endl
               << "#include <map>" << std::endl
               << "#include <string>" << std::endl
               << "#include <core/resource.hpp>" << std::endl
               << "namespace Webview {" << std::endl
               << "namespace Embedded {" << std::endl
               << "inline std::map <const std::string, Resource> files;" << std::endl
               << "}" << std::endl
               << "}" << std::endl;
        output.close();
        std::vector<std::string> filesToInclude;

        std::filesystem::recursive_directory_iterator iterator(args[1]);

        for (const auto &file : iterator)
        {
            if (file.path().filename().string().at(0) == '.' || !file.is_regular_file())
                continue;

            std::cout << "Embedding: " << file.path() << std::endl;

            filesToInclude.emplace_back(file.path().filename().string() + ".hpp");
            auto fileFunc = file.path().filename().string();
            std::replace(fileFunc.begin(), fileFunc.end(), '.', '_');
            std::replace(fileFunc.begin(), fileFunc.end(), '-', '_');

            std::ofstream fileStream("embedded/" + file.path().filename().string() + ".hpp");
            fileStream << "#pragma once" << std::endl
                       << "#include \"webview_base.hpp\"" << std::endl
                       << "namespace Webview::Embedded {" << std::endl
                       << "inline unsigned char embed_file_" << fileFunc << "[] = {";

            std::ifstream fileDataStream(file.path(), std::ios::binary);
            std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(fileDataStream), {});

            for (auto it = buffer.begin(); it != buffer.end(); it++)
            {
                fileStream << "0x" << std::setw(2) << std::setfill('0') << std::hex << static_cast<unsigned int>(*it);
                if (std::distance(it, buffer.end()) > 1)
                {
                    fileStream << ",";
                }
            }
            fileStream << "};";

            fileStream << "inline auto webview_embed_file_" << fileFunc << " = []() -> bool { files.insert({\""
                       << file.path().filename().string() << "\", {" << std::dec << buffer.size() << ","
                       << "embed_file_" << fileFunc << "}});"
                       << "return true; }();" << std::endl
                       << "}";
            fileStream.close();
        }

        std::ofstream includeFile("embedded/include.hpp");
        includeFile << "#pragma once" << std::endl << "#include \"webview_base.hpp\"" << std::endl;
        for (const auto &file : filesToInclude)
        {
            includeFile << "#include \"" + file << "\"" << std::endl;
        }
        includeFile.close();
    }
    else
    {
        std::cerr << "Invalid path!" << std::endl;
    }

    return 0;
}
