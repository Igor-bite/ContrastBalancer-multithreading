#include <iostream>
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
    static string programPath = args_parser_constants::programPath;
    static string inputFileParam = "--input";
    static string outputFileParam = "--output";
    static string helpFlag = "--help";
    static string coefParam = "--coef";
    static string ompOff = "--no-omp";
    static string ompThreads = "--omp-threads";
    static string ompThreadsDefaultValue = "default";
};

void printHelp() {
    cout
         <<  "========= Help page =========" << endl
         <<  "--help - shows this help page" << endl
         <<  "--input [fname] - input filename with pnm/ppm format" << endl
         <<  "--output [fname] - output file for modified image" << endl
         <<  "--coef [coef] - coefficient for ignoring not important colors" << endl
         <<  "--no-omp | --omp-threads [num_threads | default] - multithreading options"
         << endl << endl;
}

inline bool isFileExists(const std::string& name) {
    struct stat buffer;
    return (stat (name.c_str(), &buffer) == 0);
}

int pseudoMain(int argc, char* argv[]) {
    bool isDebug = false;

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

    if (!isFileExists(inputFileName)) {
        cerr << "No file with name " << inputFileName << endl;
        return 1;
    }

    PNMPicture picture;
    try {
        picture.read(inputFileName);
    } catch (FileIOException&) {
        cerr << "Error while trying to read file " << inputFileName << endl;
        return 1;
    } catch (UnsupportedFormatException&) {
        cerr << "Error: unsupported format" << endl;
        return 1;
    }

    float coeff = stof(argsMap[constants::coefParam]);
    auto timeMonitor = TimeMonitor("main", true);
    timeMonitor.start();
    picture.modify(coeff, isDebug, !isOmpOff, threads_count);
    timeMonitor.stop();

    string newPictureFilename = argsMap[constants::outputFileParam];
    picture.write(newPictureFilename);
    return 0;
}

int main(int argc, char* argv[]) {
    pseudoMain(argc, argv);
    return 0;
}