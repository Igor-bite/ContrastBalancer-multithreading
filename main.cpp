#include <string>
#include "pnm.h"
#include "args_parser.h"
#include "time_monitor.h"
#include <map>
#include <omp.h>
#include "csv_writer.h"
#include <cstdlib>
#include <thread>

using namespace std;

namespace constants {
    static string inputFileParam = "--input";
    static string outputFileParam = "--output";
    static string helpFlag = "--help";
    static string coefParam = "--coef";
    static string threadsOff = "--no-cpp-threads";
    static string cppThreads = "--cpp-threads";
    static string cppThreadsDefaultValue = "default";
}

void printHelp() {
    string output = "========= Help page =========\n";
    output.append(constants::helpFlag + " - shows this help page\n");
    output.append(constants::inputFileParam + " [fname] - input filename with pnm/ppm format\n");
    output.append(constants::outputFileParam + " [fname] - output file for modified image\n");
    output.append(constants::coefParam + " [coef] - coefficient for ignoring not important colors\n");
    output.append(constants::threadsOff + " | " + constants::cppThreads + " [num_threads | default] - multithreading options\n\n");
    printf("%s", output.c_str());
}

auto csvWriter = CSVWriter("test_results.csv");

int executeContrasting(
        string inputFileName,
        string outputFileName,
        float coeff,
        bool isThreadsOff,
        int threadsCount,
        const char *schedule,
        string scheduleModifier,
        string scheduleKind,
        int chunkSize,
        bool isOmpOn
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
    if (isThreadsOff || threadsCount == 1) {
        picture.modify(coeff);
    } else if (isOmpOn) {
        picture.modifyParallelOmp(coeff, threadsCount);
    } else {
        if (scheduleKind != "static" && scheduleKind != "dynamic") {
            printf("Unsupported schedule type. Supported: static/dynamic");
            return 100;
        }
        picture.modifyParallelCpp(coeff, threadsCount, scheduleKind, chunkSize);
    }
    double elapsedTime = timeMonitor.stop();

    csvWriter.write(inputFileName, threadsCount, isThreadsOff, isOmpOn, scheduleModifier, scheduleKind, chunkSize, elapsedTime);

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

    bool isThreadsOff = argsMap[constants::threadsOff] == args_parser_constants::trueFlagValue;
    int threads_count;
    if (!isThreadsOff) {
        string ompThreads = argsMap[constants::cppThreads];
        if (ompThreads == constants::cppThreadsDefaultValue) {
            threads_count = int(thread::hardware_concurrency());
        } else {
            threads_count = stoi(ompThreads);
        }
    } else {
        threads_count = 1;
    }

    string inputFileName = argsMap[constants::inputFileParam];
    string outputFilename = argsMap[constants::outputFileParam];
    float coeff = stof(argsMap[constants::coefParam]);

    return executeContrasting(inputFileName, outputFilename, coeff, isThreadsOff, threads_count, "dynamic", "", "dynamic", 1024 * 32, false);
}

int main(int argc, char* argv[]) {
//    return pseudoMain(argc, argv);

    string schedule_kind = "dynamic";
    int chunk_size = 1024*32;

    string schedule;
    if (chunk_size == 0) {
        schedule = schedule_kind;
    } else {
        schedule = schedule_kind + "," + to_string(chunk_size);
    }

    printf("OMP: ");
    executeContrasting("in.ppm", "out.ppm", 0.00390625, false, omp_get_max_threads(), schedule.c_str(), "", schedule_kind, chunk_size, true);
    printf("CPP: ");
    return executeContrasting("in.ppm", "out.ppm", 0.00390625, false, omp_get_max_threads(), schedule.c_str(), "", schedule_kind, chunk_size, false);

    vector<string> files = { "1.ppm" };
    vector<string> scheduleKinds = {
        "static",
        "dynamic"
    };
    vector<int> chunkSizes = {1024, 1024*2, 1024*4, 1024*8, 1024*16};
    string outputFile = "out.ppm";
    vector<int> threads = {1, 2, 5, 8, 9, 10};
    float coeff = 1 / 256;
    vector<bool> isOmpOff = { true, false };

    for (auto file : files) {
        for (auto sk : scheduleKinds) {
            for (int chunkSize : chunkSizes) {
                string scheduleValue;
                if (chunkSize == 0) {
                    scheduleValue = sk;
                } else {
                    scheduleValue = sk + "," + to_string(chunkSize);
                }
                for (auto ompOffFlag: isOmpOff) {
                    for (auto isThreadsOff: isOmpOff) {
                        if (isThreadsOff) {
                            executeContrasting(file, outputFile, coeff, isThreadsOff, 1, scheduleValue.c_str(), "", sk,
                                               -1, !ompOffFlag);
                        } else {
                            for (int threadCount : threads) {
                                executeContrasting(file, outputFile, coeff, isThreadsOff, threadCount,
                                                   scheduleValue.c_str(), "", sk, chunkSize, !ompOffFlag);
                            }
                        }
                    }
                }
            }
        }
    }
}