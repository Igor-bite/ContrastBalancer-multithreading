//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "args_parser.h"

void parseArguments(map<string, string>& argsMap, int argc, char* argv[]) {
    argsMap[args_parser_constants::programPath] = argv[0];

    size_t i = 1;
    while (i < argc) {
        string curArg = string(argv[i]);
        bool isCurParamFlag;
        string nextArg;
        if (i + 1 < argc) {
            nextArg = string(argv[i + 1]);
            isCurParamFlag = nextArg.starts_with("-");
        } else {
            isCurParamFlag = true;
        }

        if (isCurParamFlag) {
            argsMap[curArg] = args_parser_constants::trueFlagValue;
        } else {
            argsMap[curArg] = nextArg;
            i += 1;
        }
        i += 1;
    }
}