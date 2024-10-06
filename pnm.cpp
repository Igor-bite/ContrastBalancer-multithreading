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

void PNMPicture::printInfo() const noexcept {
    cout << "Format = P" << format << endl;
    cout << "W x H = " << width << " x " << height << endl;
    cout << "Colors = " << colors << endl;
    cout << "Data size = " << data.size() << endl;

    size_t size = data.size();
    uchar min_v = 255;
    uchar max_v = 0;
    for (size_t i = 0; i < size; i++) {
        uchar v = data[i];
        min_v = min(min_v, v);
        max_v = max(max_v, v);
    }

    cout << "Min = " << int(min_v) << endl;
    cout << "Max = " << int(max_v) << endl << endl;
}

// 1) изначально при итерировании собираем кол-ва по каждому цвету
// (возможно не надо) дополнительно считаем сумму светлых и тёмных цветов
// 2) при итерировании по цветам суммируем кол-во для тёмных и светлых
// 3) при достижении нужного кол-ва - идём дальше
// и сохраняем первый попавшийся индекс с ненулевым значением как минимальный/максимальный цвет
// 4) вычисляем min/max
// 5) пробегаемся ещё раз и меняем значения
// не забыть посчитать коэффициент умножения заранее - done
// TODO: если один цвет - ничего не делать
// omp + simd + ilp

// TODO: оптимизировать и сделать игнорирование для разных каналов раздельно
void PNMPicture::modify(float coeff, bool isDebug, bool isParallel, int threads_count) noexcept {
    size_t size = data.size();

    size_t ignoreCount = size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    auto analyzeTM = TimeMonitor("analyzeData", true);
    analyzeTM.start();
    analyzeData(elements, isParallel, threads_count);
    analyzeTM.stop();

    auto analyzeMinMax = TimeMonitor("determineMinMax", true);
    analyzeMinMax.start();
    determineMinMax(isDebug, isParallel, ignoreCount, elements, min_v, max_v, threads_count);
    analyzeMinMax.stop();

    if (min_v == 0 && max_v == 255) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    auto scalingTM = TimeMonitor("scaling", true);
    scalingTM.start();

    // TODO: проверить не выйдем ли за границы (если size не делится на 4)
#pragma omp parallel for schedule(guided) if(isParallel) num_threads(threads_count)
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

    scalingTM.stop();
}

void PNMPicture::determineMinMax(
    bool isDebug,
    bool isParallel,
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

#pragma omp parallel if(isParallel) firstprivate(darkCount, isDarkComplete, brightCount, isBrightComplete) shared(min_v, max_v) num_threads(2)
    {
        auto threadNum = omp_get_thread_num();

        for (size_t i = 0; i < 128; i++) {
            bool isDarkHandler = threadNum == 0;

            if (isDarkHandler) {
                if (!isDarkComplete) {
                    size_t darkIndex = i;
                    int element = 0;
                    if (isParallel) {
                        for (auto j = 0; j < threads_count; ++j) {
                            element += elements[darkIndex + j * 256];
                        }
                    } else {
                        element = elements[darkIndex];
                    }
                    if (darkCount < ignoreCount) {
                        darkCount += element;
                    }

                    if (darkCount >= ignoreCount && element != 0) {
                        isDarkComplete = true;
                        min_v = darkIndex;
                    }
                } else if (isParallel) {
                    break;
                }
            } else {
                if (!isBrightComplete) {
                    size_t brightIndex = 255 - i;
                    int element = 0;
                    if (isParallel) {
                        for (auto j = 0; j < threads_count; ++j) {
                            element += elements[brightIndex + j * 256];
                        }
                    } else {
                        element = elements[brightIndex];
                    }
                    if (brightCount < ignoreCount) {
                        brightCount += element;
                    }

                    if (brightCount >= ignoreCount && element != 0) {
                        isBrightComplete = true;
                        max_v = brightIndex;
                    }
                } else if (isParallel) {
                    break;
                }
            }

            if (!isParallel && isDarkComplete && isBrightComplete) {
                break;
            }
        }
    }
}

void PNMPicture::analyzeData(vector<size_t> & elements, bool isParallel, int threads_count) const noexcept {

    // TODO: ILP: надо проверять, что не выйдем в конце за границы массива
    if (isParallel) {
        elements.resize(256 * threads_count, 0);

#pragma omp parallel default(shared) if(isParallel) num_threads(threads_count)
        {
            int thread = omp_get_thread_num();
#pragma omp for schedule(guided)
            for (size_t i = 0; i < data.size(); i += 4) {
                auto index1 = data[i] + 256 * thread;
                auto index2 = data[i+1] + 256 * thread;
                auto index3 = data[i+2] + 256 * thread;
                auto index4 = data[i+3] + 256 * thread;
                elements[index1] += 1;
                elements[index2] += 1;
                elements[index3] += 1;
                elements[index4] += 1;
            }
        }
    } else {
        elements.resize(256, 0);
        for (size_t i = 0; i < data.size(); i += 4) {
            int v1 = data[i];
            int v2 = data[i+1];
            int v3 = data[i+2];
            int v4 = data[i+3];
            elements[v1] += 1;
            elements[v2] += 1;
            elements[v3] += 1;
            elements[v4] += 1;
        }
    }
}