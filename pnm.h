//
// Created by Igor Kluzhev on 25.09.2024.
//

#ifndef TESTPROJECT_PNM_H
#define TESTPROJECT_PNM_H

#pragma once

#include <string>
#include <vector>
#include "csv_writer.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

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
    void modifyOpenCL(const float coeff, const int device_index, const string device_type, const int partOfAllWorkItems) noexcept;

    int format;
    int width, height;
    int colors;
    size_t data_size;
    short channelsCount;
    FILE *fin;
    FILE *fout;
    vector<uchar> data;
    int compute_units_count;
    int max_group_size;
    CSVWriter *csv;

private:
    void analyzeData(vector<size_t> &elements) const noexcept;
    double analyzeDataOpenCL(
        vector<size_t> &elements,
        cl_mem device_data,
        cl_program program,
        cl_device_id device,
        cl_context context,
        cl_command_queue queue
    ) const noexcept;
    double scaleImageData(
        cl_mem device_data,
        cl_device_id device,
        cl_program program,
        cl_context context,
        cl_command_queue queue,
        cl_float scale,
        cl_float scaledMinV
    ) noexcept;

    void determineMinMax(size_t ignoreCount, const vector<size_t> &elements, uchar &min_v, uchar &max_v) const noexcept;
};


#endif //TESTPROJECT_PNM_H
