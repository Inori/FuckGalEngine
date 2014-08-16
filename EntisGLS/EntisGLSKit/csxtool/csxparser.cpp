#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include "csxparser.h"
#include "txtparser.h"

static const char entisFT[] = { 'E', 'n', 't', 'i', 's', 0x1a, 0x00, 0x00 };
static const char newLine[] = { '\r', 0x00, '\n', 0x00 };

void CSXParser::exportCsx(std::string origCsxFilename, std::string utfFilename) {
    std::cout << "Parsing CSX..." << std::endl;
    parseCsx(origCsxFilename);
    std::cout << "Extracting scenario..." << std::endl;
    image.extractStrings();
    std::cout << "Scenario has " << image.messages.size() << " messages and " << image.choices.size() << " choices." << std::endl;
    std::cout << "Writing TXT file..." << std::endl;
    TxtParser txt;
    txt.messages.swap(image.messages);
    txt.choices.swap(image.choices);
    txt.writeTxt(utfFilename);
}

void CSXParser::importCsx(std::string origCsxFilename, std::string tlCsxFilename, std::deque<std::string> utfFilenames) {
    std::cout << "Parsing CSX..." << std::endl;
    parseCsx(origCsxFilename);
    std::cout << "Extracting scenario..." << std::endl;
    image.extractStrings();
    std::cout << "Scenario has " << image.messages.size() << " messages and " << image.choices.size() << " choices." << std::endl;
    std::cout << "Parsing TXT files..." << std::endl;
    TxtParser txt;
    TxtParser::Stats totalStats;
    for (size_t i = 0; i < utfFilenames.size(); ++i) {
        TxtParser::Stats fileStats = txt.parseTxt(utfFilenames[i]);
        totalStats += fileStats;
        std::cout << "File " << utfFilenames[i] << ": " << fileStats << std::endl;
    }
    std::cout << "Total: " << totalStats << std::endl;
    std::cout << "Updating scenario..." << std::endl;
    image.substituteStrings(txt.messages, txt.choices);
    std::cout << "Updating short jump offsets..." << std::endl;
    image.updateShortHops();
    std::cout << "Updating long jump offsets..." << std::endl;
    function.updateOffsets(image.positionsComputer());
    std::cout << "Updating header..." << std::endl;
    updateHeader();
    std::cout << "Writing CSX file..." << std::endl;
    writeCsx(tlCsxFilename);
}

void CSXParser::parseCsx(std::string csxFilename) {
    std::ifstream fin(csxFilename.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!fin) {
        throw std::runtime_error("Couldn't open file " + csxFilename + ".");
    }

    // Header reading and check
    fin >> header;

    if (!fin) {
        throw std::runtime_error("Corrupted header data.");
    }

    if (memcmp(header.fileType, entisFT, sizeof(entisFT)) != 0) {
        throw std::runtime_error("Wrong file type.");
    }

    if (header.zero != 0) {
        throw std::runtime_error("Corrupted header data.");
    }

    if (strncmp(header.imageType, "Cotopha Image file", 18) != 0) {
        throw std::runtime_error("Wrong image type.");
    }

    // Reading of sections raw data and size checks
    fin >> image >> function >> global >> data >> conststr >> linkinf;

    if (!fin || header.size != 16 * 6 + image.size + function.size + global.size + data.size + conststr.size + linkinf.size || fin.peek() >= 0) {
        throw std::runtime_error("Wrong content size.");
    }

    fin.close();
}

void CSXParser::updateHeader() {
    header.size = 16 * 6 + image.size + function.size + global.size + data.size + conststr.size + linkinf.size;
}

void CSXParser::writeCsx(std::string csxFilename) const {
    std::ofstream fout(csxFilename.c_str(), std::ios_base::out | std::ios_base::binary);
    if (!fout) {
        throw std::runtime_error("Couldn't open file " + csxFilename + ".");
    }

    fout << header << image << function << global << data << conststr << linkinf;

    fout.close();
}
