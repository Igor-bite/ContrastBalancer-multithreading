//
// Created by Igor Kluzhev on 25.09.2024.
//

#ifndef TESTPROJECT_PNM_H
#define TESTPROJECT_PNM_H

#pragma once

#include <string>
#include <vector>

using namespace std;

typedef unsigned char uchar;

class PNMPicture {
public:
    PNMPicture();
    explicit PNMPicture(const string& fileName);
    ~PNMPicture();

    void read(const string& fileName);
    void read();

    void write(const string& fileName) ;
    void write();

    void modify(float coeff) noexcept;
    void modifyParallel(float coeff, int threads_count) noexcept;

    int format;
    int width, height;
    int colors;
    short channelsCount;
    FILE *fin;
    FILE *fout;
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
