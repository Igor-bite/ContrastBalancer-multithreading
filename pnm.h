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

    void modify(const float coeff) noexcept;
    void modifyParallelCUDA(const float coeff, const int threads_count) noexcept;
    void modifyParallelOmp(const float coeff, const int threads_count) noexcept;
    void modifyParallelCpp(const float coeff, const int threads_count, const string schedule_kind, const int chunk_size) noexcept;

    int format;
    int width, height;
    int colors;
    size_t data_size;
    short channelsCount;
    FILE *fin;
    FILE *fout;
    vector<uchar> data;

private:
    void analyzeData(vector<size_t> &elements) const noexcept;
    void analyzeDataParallelCUDA(vector<size_t> &elements, const int threads_count) const noexcept;
    void analyzeDataParallelOmp(vector<size_t> &elements, const int threads_count) const noexcept;
    void analyzeDataParallelCpp(vector<size_t> &elements, const int threads_count, const string schedule_kind, const int chunk_size) const noexcept;

    void determineMinMax(size_t ignoreCount, const vector<size_t> &elements, uchar &min_v,
                         uchar &max_v) const noexcept;
};


#endif //TESTPROJECT_PNM_H
