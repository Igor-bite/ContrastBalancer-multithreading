//
// Created by Igor Kluzhev on 09.10.2024.
//

#include "csv_writer.h"
#include <string>

CSVWriter::CSVWriter(string fileName) {
    file.open(fileName);
    file << "FILE;THREADS;SCHEDULE_KIND;CHUNK_SIZE;TIME" << endl;
}

void CSVWriter::write(
    string inputFileName,
    int threadsCount,
    bool isOmpOff,
    string scheduleModifier,
    string scheduleKind,
    int chunkSize,
    double time
) {
    file << inputFileName << ";" << threadsCount << ";" << (isOmpOff ? "no-omp" : scheduleKind) << ";" << (chunkSize == 0 ? to_string(-1) : to_string(chunkSize)) << ";" <<  time << endl;
}