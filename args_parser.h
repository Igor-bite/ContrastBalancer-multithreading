//
// Created by Igor Kluzhev on 03.10.2024.
//

#ifndef TESTPROJECT_ARGS_PARSER_H
#define TESTPROJECT_ARGS_PARSER_H

#include <string>
#include <map>

using namespace std;

namespace args_parser_constants {
    static string programPath = "programPath";
    static string trueFlagValue = "1";
};

void parseArguments(map<string, string>& argsMap, int argc, char* argv[]);

#endif //TESTPROJECT_ARGS_PARSER_H