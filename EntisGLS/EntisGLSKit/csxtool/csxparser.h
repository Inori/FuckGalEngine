#ifndef CSXPARSER_H
#define CSXPARSER_H

#include <string>
#include <deque>
#include "section.h"
#include "functionsection.h"
#include "imagesection.h"
#include "header.h"

class CSXParser {
public:
    void exportCsx(std::string origCsxFilename, std::string utfFilename);
    void importCsx(std::string origCsxFilename, std::string tlCsxFilename, std::deque<std::string> utfFilenames);

private:
    Header header;
    ImageSection image;
    FunctionSection function;
    Section global, data, conststr, linkinf;

    void parseCsx(std::string csxFilename);
    void updateHeader();
    void writeCsx(std::string csxFilename) const;
};

#endif // CSXPARSER_H
