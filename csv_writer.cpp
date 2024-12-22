//
// Created by Igor Kluzhev on 09.10.2024.
//

#include "csv_writer.h"
#include <string>

CSVWriter::CSVWriter(string fileName) {
    file.open(fileName);
    file << "FILE;PART;WORK_ITEMS_COUNT;CHUNK_SIZE;TIME" << endl;
}

void CSVWriter::write(
        string inputFileName,
        int partOfAllWorkItems,
        int workItemsCount,
        int chunkSize,
        double time
        ) {
    if (file.is_open())
        file << inputFileName << ";" << partOfAllWorkItems << ";" << workItemsCount << ";" << chunkSize << ";" << time << "\n";
}