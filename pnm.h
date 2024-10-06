//
// Created by Igor Kluzhev on 25.09.2024.
//

#ifndef TESTPROJECT_PNM_H
#define TESTPROJECT_PNM_H

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

typedef unsigned char uchar;

class FileIOException : public exception {
public:
    const char* what() const _NOEXCEPT override {
        return "Error while trying to read file or write to file";
    }
};

class UnsupportedFormatException : public exception {
public:
    const char* what() const _NOEXCEPT override {
        return "Unsupported format of PNM file";
    }
};

class ExecutionException : public exception {
public:
    const char* what() const _NOEXCEPT override {
        return "Failed to execute the command";
    }
};

class PNMPicture {
public:
    PNMPicture();
    explicit PNMPicture(string fileName);

    void read(string fileName);
    void read(ifstream& inputFile);

    void write(const string& fileName) const;
    void write(ofstream& outputFile) const;

    void printInfo() const noexcept;
    void modify(float coeff, bool isDebug, bool isParallel) noexcept;

    int format;
    int width, height;
    int colors;
    vector<uchar> data;

private:
    void analyzeData(vector<size_t> &elements, bool isParallel) const noexcept;

    void determineMinMax(bool isDebug, bool isParallel, size_t ignoreCount, const vector<size_t> &elements, uchar &min_v,
                         uchar &max_v) const noexcept;
};


#endif //TESTPROJECT_PNM_H
