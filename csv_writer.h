//
// Created by Igor Kluzhev on 09.10.2024.
//

#ifndef TESTPROJECT_CSV_WRITER_H
#define TESTPROJECT_CSV_WRITER_H

#include <string>
#include <fstream>

using namespace std;

class CSVWriter {
public:
    explicit CSVWriter(string filename);

    void write(
            string inputFileName,
            int partOfAllWorkItems,
            int workItemsCount,
            int chunkSize,
            double time
            );

private:
    string filename;
    ofstream file;
};

#endif //TESTPROJECT_CSV_WRITER_H
