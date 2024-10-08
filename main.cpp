#include <string>
#include "pnm.h"
#include "args_parser.h"
#include "time_monitor.h"
#include <map>
#include <sys/stat.h>
#include <omp.h>

using namespace std;
namespace fs = filesystem;

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
    printf(output.c_str());
}

inline bool isFileExists(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

int pseudoMain(int argc, char* argv[]) {
    map<string, string> argsMap = {};
    parseArguments(argsMap, argc, argv);

    if (argsMap[constants::helpFlag] == args_parser_constants::trueFlagValue) {
        printHelp();
        return 0;
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

    if (!isFileExists(inputFileName)) {
        fprintf(stderr, "No file with name %s\n", inputFileName.c_str());
        return 1;
    }

    PNMPicture picture;
    try {
        picture.read(inputFileName);
    } catch (FileIOException&) {
        fprintf(stderr, "Error while trying to read file %s\n", inputFileName.c_str());
        return 1;
    } catch (UnsupportedFormatException&) {
        fprintf(stderr, "Error: unsupported format\n");
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
    picture.write(outputFilename);
    return 0;
}

int main(int argc, char* argv[]) {
    pseudoMain(argc, argv);
    return 0;
}