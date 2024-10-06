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

//#define DEBUG
#define PARALLEL
//#define MANY

int pseudoMain(int argc, char* argv[]) {
    bool isDebug = false;
#ifdef DEBUG
    isDebug = true;
#endif

    bool isParallel = false;
#ifdef PARALLEL
    isParallel = true;
#endif

    map<string, string> argsMap = {};
    parseArguments(argsMap, argc, argv);

    if (isDebug) {
        for (auto arg: argsMap) {
            cout << arg.first << " - " << arg.second << endl;
        }
    }

    if (argsMap[constants::helpFlag] == args_parser_constants::trueFlagValue) {
        printHelp();
        return 0;
    }

    if (argc < 7 || argc > 8) {
        cerr << "Incorrect count of arguments" << endl;
        return 1;
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
    if (isDebug) {
        picture.printInfo();
        cout << "coef = " << coeff << endl;
    }

    auto timeMonitor = TimeMonitor("Main", true);
    timeMonitor.start();
    picture.modify(coeff, isDebug, isParallel);
    timeMonitor.stop();

    string newPictureFilename = argsMap[constants::outputFileParam];
    picture.write(newPictureFilename);
    return 0;
}

int main(int argc, char* argv[]) {

    int count = 1;
#ifdef MANY
    count = 10;
#endif
    for (auto i = 0; i < count; i++) {
        pseudoMain(argc, argv);
    }

    return 0;
}