#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <locale>
#include <cstring>
#include "csxparser.h"

void usage() {
    std::cerr << "Usage: csxtool (import file.csx scenario1.txt [scenario2.txt scenario3.txt ...]|export file.csx)\n";
}

int main(int argc, const char **argv) {
    if (argc < 3) {
        usage();
        return 1;
    }

    std::string command(argv[1]);
    std::string csxFilename(argv[2]);
    std::string filenamePrefix = csxFilename.substr(0, csxFilename.find_last_of('.'));

    if (command == "export") {
        if (argc != 3) {
            usage();
            return 1;
        }
        std::string utfFilename = filenamePrefix + ".txt";

        std::cout << "Exporting " << csxFilename << " to " << utfFilename << "..." << std::endl;

        CSXParser csx;
        try {
            csx.exportCsx(csxFilename, utfFilename);
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        std::cout << "Export successful!" << std::endl;
        return 0;
    } else if (command == "import") {
        if (argc < 4) {
            usage();
            return 1;
        }
        std::string origCsxFilename = filenamePrefix + "_original.csx";
        std::deque<std::string> utfFilenames;
        for (int i = 3; i < argc; ++i) {
            utfFilenames.push_back(argv[i]);
        }

        std::cout << "Importing ";
        for (size_t i = 0; i < utfFilenames.size(); ++i) {
            if (i != 0) {
                std::cout << ", ";
            }
            std::cout << utfFilenames[i];
        }
        std::cout << " and " << origCsxFilename << " to " << csxFilename << "..." << std::endl;

        CSXParser csx;
        try {
            csx.importCsx(origCsxFilename, csxFilename, utfFilenames);
        } catch (std::runtime_error &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        std::cout << "Import successful!" << std::endl;
        return 0;
    } else {
        usage();
        return 1;
    }

    return 0;
}
