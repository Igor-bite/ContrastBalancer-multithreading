#include <string>
#include "pnm.h"
#include "args_parser.h"
#include "time_monitor.h"
#include <map>
#include <omp.h>
#include <iostream>
#include "csv_writer.h"
#include <cstdlib>

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

auto csvWriter = CSVWriter("test_results.csv");

int main2(
        string inputFileName,
        string outputFileName,
        float coeff,
        bool isOmpOff,
        int threadsCount,
        const char *schedule,
        string scheduleModifier,
        string scheduleKind,
        int chunkSize
) {
    if (schedule != "") {
        bool flag;
        setenv("OMP_SCHEDULE", schedule, flag);
    }

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

    auto timeMonitor = TimeMonitor(threadsCount, true);
    timeMonitor.start();
    if (isOmpOff) {
        picture.modify(coeff);
    } else {
        picture.modifyParallel(coeff, threadsCount);
    }
    double elapsedTime = timeMonitor.stop();

    csvWriter.write(inputFileName, threadsCount, isOmpOff, scheduleModifier, scheduleKind, chunkSize, elapsedTime);

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
    float coeff = stof(argsMap[constants::coefParam]);

    return main2(inputFileName, outputFilename, coeff, isOmpOff, threads_count, "dynamic", "", "", 0);
}

int main(int argc, char* argv[]) {
    return pseudoMain(argc, argv);

    vector<string> files = { "4.ppm" };
    vector<string> scheduleModifiers = {
        "monotonic",
        "nonmonotonic"
    };
    vector<string> scheduleKinds = {
        "static",
        "dynamic",
        "guided",
        "auto"
    };
    int maxChunkSize = 10;
    string outputFile = "out.ppm";
    int threadsMaxCount = omp_get_max_threads() * 2;
    float coeff = 1 / 256;
    vector<bool> isOmpOff = { true, false };

    /*
    modifier is one of monotonic or nonmonotonic;
    kind is one of static, dynamic, guided, or auto;
    chunk is an optional positive integer that specifies the chunk size.

    setenv OMP_SCHEDULE "guided,4"
    setenv OMP_SCHEDULE "dynamic"
    setenv OMP_SCHEDULE "nonmonotonic:dynamic,4"
     */

    for (auto file : files) {
        for (auto sk : scheduleKinds) {
            for (int chunkSize = 0; chunkSize <= maxChunkSize; ++chunkSize) {
                string scheduleValue;
                if (chunkSize == 0) {
                    scheduleValue = sk;
                } else {
                    scheduleValue = sk + "," + to_string(chunkSize);
                }
                for (auto ompOffFlag: isOmpOff) {
                    if (ompOffFlag) {
                        main2(file, outputFile, coeff, ompOffFlag, 1, scheduleValue.c_str(), "", sk, chunkSize);
                    } else {
                        for (int threadCount = 1; threadCount <= threadsMaxCount; ++threadCount) {
                            main2(file, outputFile, coeff, ompOffFlag, threadCount, scheduleValue.c_str(), "", sk, chunkSize);
                        }
                    }
                }
            }
        }
    }
}