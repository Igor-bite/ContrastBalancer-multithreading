#include <string>
#include <map>
#include "pnm.h"
#include "args_parser.h"
using namespace std;

namespace constants {
    static string inputFileParam = "--input";
    static string outputFileParam = "--output";
    static string helpFlag = "--help";
    static string coefParam = "--coef";
    static string deviceIndex = "device_index";
}

void printHelp() {
    string output = "========= Help page =========\n";
    output.append(constants::helpFlag + " - shows this help page\n");
    output.append(constants::inputFileParam + " [fname] - input filename with pnm/ppm format\n");
    output.append(constants::outputFileParam + " [fname] - output file for modified image\n");
    output.append(constants::coefParam + " [coef] - coefficient for ignoring not important colors\n");
    output.append(constants::deviceIndex + " [device_index] - index of selected CUDA device\n\n");
    printf("%s", output.c_str());
}

int executeContrasting(
        string inputFileName,
        string outputFileName,
        float coeff,
        int deviceIndex
) {
    PNMPicture picture;
    try {
        picture.read(inputFileName);
    } catch (exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    if (coeff < 0 || coeff >= 0.5) {
        fprintf(stderr, "Error: coeff must be in range [0, 0.5)\n");
        return 1;
    }

    picture.modifyParallelCUDA(coeff, deviceIndex);

    try {
        picture.write(outputFileName);
    } catch (exception& e) {
        fprintf(stderr, "%s", e.what());
        return 1;
    }
    return 0;
}

int pseudoMain(int argc, char* argv[]) {
    map<string, string> argsMap = {};
    parseArguments(argsMap, argc, argv);

    if (argsMap[constants::helpFlag] == args_parser_constants::trueFlagValue && argc == 2) {
        printHelp();
        return 0;
    }

    if (argc < 7 || argc > 9) {
        fprintf(stderr, "Incorrect number of arguments, see help with --help");
        return 1;
    }

    string inputFileName = argsMap[constants::inputFileParam];
    string outputFilename = argsMap[constants::outputFileParam];
    int deviceIndex = stoi(argsMap[constants::deviceIndex]);
    float coeff = stof(argsMap[constants::coefParam]);

    return executeContrasting(inputFileName, outputFilename, coeff, deviceIndex);
}

int main(int argc, char* argv[]) {
//    return pseudoMain(argc, argv);

    fprintf(stdout, "Starting\n");
    auto result = executeContrasting("in.ppm", "out.ppm", 0.00390625, 0);
    fprintf(stdout, "Ending\n");
    return result;
}