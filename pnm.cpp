//
// Created by Igor Kluzhev on 25.09.2024.
//

#include "pnm.h"
#include <omp.h>
#include <cmath>
#include "time_monitor.h"

PNMPicture::PNMPicture() = default;
PNMPicture::PNMPicture(string filename) {
    read(filename);
}

void PNMPicture::read(string fileName) {
    ifstream inputFile(fileName, ios::binary);
    if (!inputFile.is_open())
        throw FileIOException();
    read(inputFile);
    inputFile.close();
}

void PNMPicture::read(ifstream& inputFile) {
    char P;
    inputFile >> P;
    if (P != 'P')
        throw UnsupportedFormatException();

    inputFile >> format;
    short colorsCount;
    if (format == 5) {
        colorsCount = 1;
    } else if (format == 6) {
        colorsCount = 3;
    } else {
        throw UnsupportedFormatException();
    }
    inputFile >> width >> height;
    inputFile >> colors;

    if (width % 4 == 0 || height % 4 == 0 || (width % 2 == 0 && height % 2 == 0)) {
        is4ILPSupported = true;
    } else if (colorsCount == 3 || height % 3 == 0 || width % 3 == 0) {
        is3ILPSupported = true;
    } else if (height % 2 == 0 || width % 2 == 0) {
        is2ILPSupported = true;
    }

    inputFile.get();

    size_t dataSize = width * height * colorsCount;
    data.resize(dataSize);
    inputFile.read((char*) &data[0], dataSize);

    if (inputFile.fail())
        throw FileIOException();
}

void PNMPicture::write(const string& fileName) const {
    ofstream outputFile(fileName, ios::binary);
    if (!outputFile.is_open())
        throw FileIOException();
    write(outputFile);
    outputFile.close();
}

void PNMPicture::write(ofstream& outputFile) const {
    outputFile << "P" << format << '\n';
    outputFile << width << ' ' << height << '\n';
    outputFile << colors << '\n';
    short colorsCount;
    if (format == 5) {
        colorsCount = 1;
    } else if (format == 6) {
        colorsCount = 3;
    } else {
        throw UnsupportedFormatException();
    }
    outputFile.write((char*) &data[0], width * height * colorsCount);
}

// 1) изначально при итерировании собираем кол-ва по каждому цвету
// 2) при итерировании по цветам суммируем кол-во для тёмных и светлых
// 3) при достижении нужного кол-ва - идём дальше
// и сохраняем первый попавшийся индекс с ненулевым значением как минимальный/максимальный цвет
// 4) вычисляем min/max
// 5) пробегаемся ещё раз и меняем значения
// методы: omp + simd + ilp
// TODO: если один цвет - ничего не делать


void PNMPicture::modify(float coeff) noexcept {
    size_t size = data.size();

    if (size == 1) {
        return;
    }

    size_t ignoreCount = size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeData(elements);

    determineMinMax(ignoreCount, elements, min_v, max_v);

    if (min_v == 0 && max_v == 255) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    if (is2ILPSupported) {
        for (size_t i = 0; i < size; i += 2) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
        }
    } else if (is3ILPSupported) {
        for (size_t i = 0; i < size; i += 3) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
            int scaledValue3 = int(scale * float(data[i+2]) - scaledMinV);
            data[i+2] = max(0, min(scaledValue3, 255));
        }
    } else if (is4ILPSupported) {
        for (size_t i = 0; i < size; i += 4) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
            int scaledValue3 = int(scale * float(data[i+2]) - scaledMinV);
            data[i+2] = max(0, min(scaledValue3, 255));
            int scaledValue4 = int(scale * float(data[i+3]) - scaledMinV);
            data[i+3] = max(0, min(scaledValue4, 255));
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
        }
    }
}

void PNMPicture::modifyParallel(float coeff, int threads_count) noexcept {
    size_t size = data.size();

    if (size == 1) {
        return;
    }

    size_t ignoreCount = size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeDataParallel(elements, threads_count);
    determineMinMaxParallel(ignoreCount, elements, min_v, max_v, threads_count);

    if (min_v == 0 && max_v == 255) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    if (is2ILPSupported) {
#pragma omp parallel for schedule(guided) num_threads(threads_count)
        for (size_t i = 0; i < size; i += 2) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
        }
    } else if (is3ILPSupported) {
#pragma omp parallel for schedule(guided) num_threads(threads_count)
        for (size_t i = 0; i < size; i += 3) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
            int scaledValue3 = int(scale * float(data[i+2]) - scaledMinV);
            data[i+2] = max(0, min(scaledValue3, 255));
        }
    } else if (is4ILPSupported) {
#pragma omp parallel for schedule(guided) num_threads(threads_count)
        for (size_t i = 0; i < size; i += 4) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
            int scaledValue2 = int(scale * float(data[i+1]) - scaledMinV);
            data[i+1] = max(0, min(scaledValue2, 255));
            int scaledValue3 = int(scale * float(data[i+2]) - scaledMinV);
            data[i+2] = max(0, min(scaledValue3, 255));
            int scaledValue4 = int(scale * float(data[i+3]) - scaledMinV);
            data[i+3] = max(0, min(scaledValue4, 255));
        }
    } else {
#pragma omp parallel for schedule(guided) num_threads(threads_count)
        for (size_t i = 0; i < size; i++) {
            int scaledValue1 = int(scale * float(data[i]) - scaledMinV);
            data[i] = max(0, min(scaledValue1, 255));
        }
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

    for (size_t i = 0; i < 128; i++) {
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
}

void PNMPicture::determineMinMaxParallel(
        size_t ignoreCount,
        const vector<size_t> &elements,
        uchar &min_v,
        uchar &max_v,
        int threads_count
) const noexcept {

    int darkCount = 0;
    bool isDarkComplete = false;

    int brightCount = 0;
    bool isBrightComplete = false;

#pragma omp parallel firstprivate(darkCount, isDarkComplete, brightCount, isBrightComplete) shared(min_v, max_v) num_threads(max(threads_count, 2))
    {
        auto threadNum = omp_get_thread_num();

        for (size_t i = 0; i < 128; i++) {
            bool isDarkHandler = threadNum == 0;

            if (isDarkHandler || threads_count == 1) {
                if (!isDarkComplete) {
                    size_t darkIndex = i;
                    int element = 0;
                    for (auto j = 0; j < threads_count; ++j) {
                        element += elements[darkIndex + j * 256];
                    }
                    if (darkCount < ignoreCount) {
                        darkCount += element;
                    }

                    if (darkCount >= ignoreCount && element != 0) {
                        isDarkComplete = true;
                        min_v = darkIndex;
                    }
                } else {
                    break;
                }
            }
            if (!isDarkHandler || threads_count == 1) {
                if (!isBrightComplete) {
                    size_t brightIndex = 255 - i;
                    int element = 0;
                    for (auto j = 0; j < threads_count; ++j) {
                        element += elements[brightIndex + j * 256];
                    }
                    if (brightCount < ignoreCount) {
                        brightCount += element;
                    }

                    if (brightCount >= ignoreCount && element != 0) {
                        isBrightComplete = true;
                        max_v = brightIndex;
                    }
                } else {
                    break;
                }
            }
        }
    }
}

void PNMPicture::analyzeData(vector<size_t> & elements) const noexcept {
    elements.resize(256, 0);

    if (is4ILPSupported) {
        for (size_t i = 0; i < data.size(); i += 4) {
            elements[data[i]] += 1;
            elements[data[i+1]] += 1;
            elements[data[i+2]] += 1;
            elements[data[i+3]] += 1;
        }
    } else if (is3ILPSupported) {
        for (size_t i = 0; i < data.size(); i += 3) {
            elements[data[i]] += 1;
            elements[data[i+1]] += 1;
            elements[data[i+2]] += 1;
        }
    } else if (is2ILPSupported) {
        for (size_t i = 0; i < data.size(); i += 2) {
            elements[data[i]] += 1;
            elements[data[i+1]] += 1;
        }
    } else {
        for (size_t i = 0; i < data.size(); i++) {
            elements[data[i]] += 1;
        }
    }
}

void PNMPicture::analyzeDataParallel(
    vector<size_t> &elements,
    int threads_count
) const noexcept {
    elements.resize(256 * threads_count, 0);

    if (is4ILPSupported) {
#pragma omp parallel default(shared) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i += 4) {
                elements[data[i] + 256 * thread] += 1;
                elements[data[i+1] + 256 * thread] += 1;
                elements[data[i+2] + 256 * thread] += 1;
                elements[data[i+3] + 256 * thread] += 1;
            }
        }
    } else if (is3ILPSupported) {
        int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
        for (size_t i = 0; i < data.size(); i += 3) {
            elements[data[i] + 256 * thread] += 1;
            elements[data[i+1] + 256 * thread] += 1;
            elements[data[i+2] + 256 * thread] += 1;
        }
    } else if (is2ILPSupported) {
        int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
        for (size_t i = 0; i < data.size(); i += 2) {
            elements[data[i] + 256 * thread] += 1;
            elements[data[i+1] + 256 * thread] += 1;
        }
    } else {
        int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
        for (size_t i = 0; i < data.size(); i++) {
            elements[data[i] + 256 * thread] += 1;
        }
    }

    if (is2ILPSupported) {
#pragma omp parallel default(shared) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i += 2) {
                auto index1 = data[i] + 256 * thread;
                auto index2 = data[i + 1] + 256 * thread;
                elements[index1] += 1;
                elements[index2] += 1;
            }
        }
    } else if (is3ILPSupported) {
#pragma omp parallel default(shared) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i += 3) {
                auto index1 = data[i] + 256 * thread;
                auto index2 = data[i + 1] + 256 * thread;
                auto index3 = data[i + 2] + 256 * thread;
                elements[index1] += 1;
                elements[index2] += 1;
                elements[index3] += 1;
            }
        }
    } else if (is4ILPSupported) {
#pragma omp parallel default(shared) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i += 4) {
                auto index1 = data[i] + 256 * thread;
                auto index2 = data[i + 1] + 256 * thread;
                auto index3 = data[i + 2] + 256 * thread;
                auto index4 = data[i + 3] + 256 * thread;
                elements[index1] += 1;
                elements[index2] += 1;
                elements[index3] += 1;
                elements[index4] += 1;
            }
        }
    } else {
#pragma omp parallel default(shared) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i++) {
                auto index1 = data[i] + 256 * thread;
                elements[index1] += 1;
            }
        }
    }
}