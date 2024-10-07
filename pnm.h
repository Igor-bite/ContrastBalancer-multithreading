//
// Created by Igor Kluzhev on 25.09.2024.
//

#ifndef TESTPROJECT_PNM_H
#define TESTPROJECT_PNM_H

#pragma once

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

class PNMPicture {
public:
    PNMPicture();
    explicit PNMPicture(string fileName);

    void read(string fileName);
    void read(ifstream& inputFile);

    void write(const string& fileName) const;
    void write(ofstream& outputFile) const;

    void modify(float coeff) noexcept;
    void modifyParallel(float coeff, int threads_count) noexcept;

    int format;
    int width, height;
    int colors;
    vector<uchar> data;
    bool is2ILPSupported = false;
    bool is3ILPSupported = false;
    bool is4ILPSupported = false;

private:
    void analyzeData(vector<size_t> &elements) const noexcept;
    void analyzeDataParallel(vector<size_t> &elements, int threads_count) const noexcept;

    void determineMinMax(size_t ignoreCount, const vector<size_t> &elements, uchar &min_v,
                         uchar &max_v) const noexcept;
    void determineMinMaxParallel(size_t ignoreCount, const vector<size_t> &elements, uchar &min_v,
                         uchar &max_v, int threads_count) const noexcept;
};


#endif //TESTPROJECT_PNM_H
