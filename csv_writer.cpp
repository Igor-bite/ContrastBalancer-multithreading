//
// Created by Igor Kluzhev on 09.10.2024.
//

#include "csv_writer.h"
#include <string>

CSVWriter::CSVWriter(string fileName) {
    file.open(fileName);
    file << "FILE;THREADS;KIND;SCHEDULE_KIND;CHUNK_SIZE;TIME" << endl;
}

void CSVWriter::write(
    string inputFileName,
    int threadsCount,
    bool isCppOff,
    bool isOmp,
    string scheduleModifier,
    string scheduleKind,
    int chunkSize,
    double time
) {
    file << inputFileName << ";" << threadsCount << ";" << (isOmp ? "OMP" : "CPP") << ";" << (isCppOff ? "no-cpp" : scheduleKind) << ";" << (chunkSize == 0 ? to_string(-1) : to_string(chunkSize)) << ";" <<  time << endl;
}