#include <string>
#include "pnm.h"
#include "args_parser.h"
#include "time_monitor.h"
#include <map>
#include <omp.h>

using namespace std;

namespace constants {
    static string inputFileParam = "--input";
    static string outputFileParam = "--output";
    static string helpFlag = "--help";
    static string coefParam = "--coef";
    static string ompOff = "--no-omp";
    static string ompThreads = "--omp-threads";
    static string ompThreadsDefaultValue = "default";
}

void printHelp() {
    string output = "========= Help page =========\n";
    output.append(constants::helpFlag + " - shows this help page\n");
    output.append(constants::inputFileParam + " [fname] - input filename with pnm/ppm format\n");
    output.append(constants::outputFileParam + " [fname] - output file for modified image\n");
    output.append(constants::coefParam + " [coef] - coefficient for ignoring not important colors\n");
    output.append(constants::ompOff + " | " + constants::ompThreads + " [num_threads | default] - multithreading options\n\n");
    printf("%s", output.c_str());
}

int pseudoMain(int argc, char* argv[]) {
    map<string, string> argsMap = {};
    parseArguments(argsMap, argc, argv);

    if (argsMap[constants::helpFlag] == args_parser_constants::trueFlagValue && argc == 2) {
        printHelp();
        return 0;
    }

    if (argc < 8 || argc > 9) {
        fprintf(stderr, "Incorrect number of arguments, see help with --help");
        return 1;
    }

    bool isOmpOff = argsMap[constants::ompOff] == args_parser_constants::trueFlagValue;
    int threads_count;
    if (!isOmpOff) {
        string ompThreads = argsMap[constants::ompThreads];
        if (ompThreads == constants::ompThreadsDefaultValue) {
            threads_count = omp_get_max_threads();
        } else {
            threads_count = stoi(ompThreads);
        }
    } else {
        threads_count = 1;
    }

    string inputFileName = argsMap[constants::inputFileParam];
    string outputFilename = argsMap[constants::outputFileParam];

    PNMPicture picture;
    try {
        picture.read(inputFileName);
    } catch (exception& e) {
        fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    float coeff = stof(argsMap[constants::coefParam]);

    if (coeff < 0 || coeff >= 0.5) {
        fprintf(stderr, "Error: coeff must be in range [0, 0.5)\n");
        return 1;
    }

    auto timeMonitor = TimeMonitor(threads_count, true);
    timeMonitor.start();
    if (isOmpOff) {
        picture.modify(coeff);
    } else {
        picture.modifyParallel(coeff, threads_count);
    }
    timeMonitor.stop();

    try {
        picture.write(outputFilename);
    } catch (exception& e) {
        fprintf(stderr, "%s", e.what());
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    pseudoMain(argc, argv);
    return 0;
}