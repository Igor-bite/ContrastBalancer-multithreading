//
// Created by Igor Kluzhev on 25.09.2024.
//

#include "pnm.h"
#include <omp.h>
#include <cmath>
#include <stdio.h>
#include "time_monitor.h"
#include <stdexcept>
#include <thread>
#include <mutex>
#if __APPLE__
#else
#ifdef USING_CUDA
#include <cuda_runtime.h>
#include <cuda.h>
#endif
#ifdef USING_HIP
#endif
#endif

using namespace std;

PNMPicture::PNMPicture() = default;
PNMPicture::PNMPicture(const string& filename) {
    read(filename);
}

PNMPicture::~PNMPicture() {
    if (fin != nullptr) {
        fclose(fin);
    }
    if (fout != nullptr) {
        fclose(fout);
    }
};

void PNMPicture::read(const string& fileName) {
    fin = fopen(fileName.c_str(), "rb");
    if (fin == nullptr) {
        throw runtime_error("Error while trying to open input file");
    }

    char p;
    char binChar;
    fscanf(fin, "%c%i%c%d %d%c%d%c", &p, &format, &binChar, &width, &height, &binChar, &colors, &binChar);

    if (p != 'P')
        throw runtime_error("Unsupported format input file");

    read();
    fclose(fin);
    fin = nullptr;
}

void PNMPicture::read() {
    if (format == 5) {
        channelsCount = 1;
    } else if (format == 6) {
        channelsCount = 3;
    } else {
        throw runtime_error("Unsupported format of PNM file");
    }
    data_size = width * height * channelsCount;
    data.resize(data_size);

    const size_t bytesRead = fread(data.data(), 1, data_size, fin);

    if (bytesRead != data_size) {
        throw runtime_error("Error while trying to read file");
    }
}

void PNMPicture::write(const string& fileName) {
    fout = fopen(fileName.c_str(), "wb");
    if (fout == nullptr) {
        throw runtime_error("Error while trying to open output file");
    }

    write();
    fclose(fout);
    fout = nullptr;
}

void PNMPicture::write() {
    fprintf(fout, "P%d\n%d %d\n%d\n", format, width, height, colors);

    size_t dataSize = width * height * channelsCount;
    const size_t writtenBytes = fwrite(data.data(), 1, dataSize, fout);

    if (writtenBytes != dataSize) {
        throw runtime_error("Error while trying to write to file");
    }
}

// 1) изначально при итерировании собираем кол-ва по каждому цвету
// 2) при итерировании по цветам суммируем кол-во для тёмных и светлых
// 3) при достижении нужного кол-ва - идём дальше
// и сохраняем первый попавшийся индекс с ненулевым значением как минимальный/максимальный цвет
// 4) вычисляем min/max
// 5) пробегаемся ещё раз и меняем значения
// доступные методы: omp + simd + ilp

void PNMPicture::modifyParallelCUDA(const float coeff, const int threads_count) noexcept {
    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeDataParallelCUDA(elements, threads_count);
    determineMinMax(ignoreCount, elements, min_v, max_v);

    // если уже растянуто - не делаем ничего
    // или если например 1 цвет - не делаем ничего
    if ((min_v == 0 && max_v == 255) || min_v >= max_v) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    uchar* d = data.data();
#pragma omp parallel for schedule(runtime) num_threads(threads_count)
    for (size_t i = 0; i < data_size; i++) {
        int scaledValue = int(scale * float(d[i]) - scaledMinV);
        d[i] = max(0, min(scaledValue, 255));
    }
}

void PNMPicture::determineMinMax(
    size_t ignoreCount,
    const vector<size_t> &elements,
    uchar &min_v,
    uchar &max_v
) const noexcept {
    int darkCount = 0;
    bool isDarkComplete = false;

    int brightCount = 0;
    bool isBrightComplete = false;

    for (size_t i = 0; i < 256; i++) {
        if (!isDarkComplete) {
            size_t darkIndex = i;
            int element = elements[darkIndex];
            if (darkCount < ignoreCount) {
                darkCount += element;
            }

            if (darkCount >= ignoreCount && element != 0) {
                isDarkComplete = true;
                min_v = darkIndex;
            }
        }
        if (!isBrightComplete) {
            size_t brightIndex = 255 - i;
            int element = elements[brightIndex];
            if (brightCount < ignoreCount) {
                brightCount += element;
            }

            if (brightCount >= ignoreCount && element != 0) {
                isBrightComplete = true;
                max_v = brightIndex;
            }
        }

        if (isDarkComplete && isBrightComplete) {
            break;
        }
    }

    for (size_t i = 0; i < 256; i++) {
        size_t brightIndex = 255 - i;
        int element = elements[brightIndex];
        if (brightCount < ignoreCount) {
            brightCount += element;
        }

        if (brightCount >= ignoreCount && element != 0) {
            max_v = brightIndex;
            break;
        }
    }
}

void PNMPicture::analyzeDataParallelCUDA(
    vector<size_t> &elements,
    const int threads_count
) const noexcept {
    elements.resize(256, 0);
    size_t* result = elements.data();

    const uchar* d = data.data();

#pragma omp parallel num_threads(threads_count)
    {
        size_t els[256] = {0};

#pragma omp for schedule(runtime)
        for (size_t i = 0; i < data_size; i++) {
            els[d[i]] += 1;
        }

#pragma omp critical
        {
            for (auto i = 0; i < 256; i++) {
                result[i] += els[i];
            }
        }
    }
}