//
// Created by Igor Kluzhev on 03.10.2024.
//

#include "args_parser.h"

void parseArguments(map<string, string>& argsMap, int argc, char* argv[]) {
    argsMap[args_parser_constants::programPath] = argv[0];

    size_t i = 1;
    while (i < argc) {
        string curArg = string(argv[i]);
        string nextArg = string(argv[i + 1]);

        bool isCurParamFlag = nextArg.starts_with("-");
        if (isCurParamFlag) {
            argsMap[curArg] = args_parser_constants::trueFlagValue;
        } else {
            argsMap[curArg] = nextArg;
            i += 1;
        }
        i += 1;
    }
}
/*

    argsMap[constants::programPath] = argv[0];

    size_t i = 1;
    while (i < argc) {
        string arg = string(argv[i]);
        arg = arg.substr(2, arg.length());
        if (arg == constants::helpFlag) {
            argsMap[constants::helpFlag] = "1";
        }
        if (arg == constants::inputFileParam) {
            argsMap[constants::inputFileParam] = string(argv[i + 1]);
            i += 1;
        }
        if (arg == constants::outputFileParam) {
            argsMap[constants::outputFileParam] = string(argv[i + 1]);
            i += 1;
        }
        if (arg == constants::coefParam) {
            argsMap[constants::coefParam] = string(argv[i + 1]);
            i += 1;
        }
        if (arg == constants::ompOff) {
            argsMap[constants::ompOff] = "1";
        }
        i += 1;
    }

 */