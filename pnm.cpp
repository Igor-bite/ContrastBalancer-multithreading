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

void PNMPicture::modify(const float coeff) noexcept {
    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeData(elements);
    determineMinMax(ignoreCount, elements, min_v, max_v);

    // если уже растянуто - не делаем ничего
    // или если например 1 цвет - не делаем ничего
    if ((min_v == 0 && max_v == 255) || min_v >= max_v) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    uchar* d = data.data();
    for (size_t i = 0; i < data_size; i++) {
        int scaledValue1 = scale * d[i] - scaledMinV;
        d[i] = max(0, min(scaledValue1, 255));
    }
}

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

void PNMPicture::modifyParallelOmp(const float coeff, const int threads_count) noexcept {
    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeDataParallelOmp(elements, threads_count);
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

void PNMPicture::modifyParallelCpp(
    const float coeff,
    const int threads_count,
    const string schedule_kind,
    const int chunk_size
) noexcept {
    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;

    analyzeDataParallelCpp(elements, threads_count, schedule_kind, chunk_size);
    determineMinMax(ignoreCount, elements, min_v, max_v);

    // если уже растянуто - не делаем ничего
    // или если например 1 цвет - не делаем ничего
    if ((min_v == 0 && max_v == 255) || min_v >= max_v) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    uchar* d = data.data();

    auto lock = mutex();
    vector<thread> threads;
    threads.resize(threads_count);

    if (schedule_kind == "static") {
        if (chunk_size == 0) {
            for (auto thread_index = 0; thread_index < threads_count; thread_index++) {
                auto start = data_size * thread_index / threads_count;
                auto end = data_size * (thread_index + 1) / threads_count;

                threads[thread_index] = thread([start, end, scale, scaledMinV, &d]() {
                    for (size_t i = start; i < end; i++) {
                        int scaledValue = int(scale * float(d[i]) - scaledMinV);
                        d[i] = max(0, min(scaledValue, 255));
                    }
                });
            }
        } else {
            for (auto thread_index = 0; thread_index < threads_count; thread_index++) {
                auto start = thread_index  * chunk_size;
                auto end = data_size;
                auto increment = (threads_count - 1) * chunk_size + 1;

                threads[thread_index] = thread([chunk_size, start, end, increment, scale, scaledMinV, &d]() {
                    size_t i = start;
                    int cur_chunk = chunk_size;
                    while (i < end) {
                        int scaledValue = int(scale * float(d[i]) - scaledMinV);
                        d[i] = max(0, min(scaledValue, 255));

                        cur_chunk -= 1;

                        if (cur_chunk > 0) {
                            i += 1;
                        } else if (cur_chunk == 0) {
                            cur_chunk = chunk_size;
                            i += increment;
                        }
                    }
                });
            }
        }
    } else if (schedule_kind == "dynamic") {
        int cur_chunk_start = 0;
        auto data_end = data_size;
        int dynamic_chunk_size;
        if (chunk_size == 0) {
            dynamic_chunk_size = 1;
        } else {
            dynamic_chunk_size = chunk_size;
        }

        for (int thread_index = 0; thread_index < threads_count; ++thread_index) {
            threads[thread_index] = thread([&d, data_end, &lock, &cur_chunk_start, dynamic_chunk_size, scale, scaledMinV]() {

                lock.lock();
                int i = cur_chunk_start;
                cur_chunk_start += dynamic_chunk_size;
                lock.unlock();

                int end = i + dynamic_chunk_size;

                while (i < data_end) {
                    int scaledValue = int(scale * float(d[i]) - scaledMinV);
                    d[i] = max(0, min(scaledValue, 255));
                    i++;

                    if (i == end) {
                        lock.lock();
                        i = cur_chunk_start;
                        cur_chunk_start += dynamic_chunk_size;
                        lock.unlock();
                        end = i + dynamic_chunk_size;
                    }
                }
            });
        }
    } else {
        printf("Unsupported schedule type");
        return;
    }

    for (int thread_index = 0; thread_index < threads_count; ++thread_index) {
        threads[thread_index].join();
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

void PNMPicture::analyzeData(vector<size_t> & elements) const noexcept {
    elements.resize(256, 0);

    const uchar* d = data.data();
    for (size_t i = 0; i < data_size; i++) {
        elements[d[i]] += 1;
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

void PNMPicture::analyzeDataParallelOmp(
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

void PNMPicture::analyzeDataParallelCpp(
        vector<size_t> &elements,
        const int threads_count,
        const string schedule_kind,
        const int chunk_size
) const noexcept {
    elements.resize(256, 0);
    size_t* result = elements.data();

    const uchar* d = data.data();

    auto critical_section_lock = mutex();
    auto dynamic_schedule_lock = mutex();
    vector<thread> threads;
    threads.resize(threads_count);

    if (schedule_kind == "static") {
        if (chunk_size == 0) {
            for (auto thread_index = 0; thread_index < threads_count; thread_index++) {
                auto start = data_size * thread_index / threads_count;
                auto end = data_size * (thread_index + 1) / threads_count;

                threads[thread_index] = thread([start, end, &d, &critical_section_lock, &result]() {
                    size_t els[256] = {0};
                    for (size_t i = start; i < end; i++) {
                        els[d[i]] += 1;
                    }

                    critical_section_lock.lock();
                    for (auto i = 0; i < 256; i++) {
                        result[i] += els[i];
                    }
                    critical_section_lock.unlock();
                });
            }
        } else {
            for (auto thread_index = 0; thread_index < threads_count; thread_index++) {
                auto start = thread_index  * chunk_size;
                auto end = data_size;
                auto increment = (threads_count - 1) * chunk_size + 1;

                threads[thread_index] = thread([chunk_size, start, end, increment, &d, &critical_section_lock, &result]() {
                    size_t els[256] = {0};
                    size_t i = start;
                    int cur_chunk = chunk_size;
                    while (i < end) {
                        els[d[i]] += 1;

                        cur_chunk -= 1;

                        if (cur_chunk > 0) {
                            i += 1;
                        } else if (cur_chunk == 0) {
                            cur_chunk = chunk_size;
                            i += increment;
                        }
                    }

                    critical_section_lock.lock();
                    for (auto j = 0; j < 256; j++) {
                        result[j] += els[j];
                    }
                    critical_section_lock.unlock();
                });
            }
        }
    } else if (schedule_kind == "dynamic") {
        int cur_chunk_start = 0;
        auto data_end = data_size;
        int dynamic_chunk_size;
        if (chunk_size == 0) {
            dynamic_chunk_size = 1;
        } else {
            dynamic_chunk_size = chunk_size;
        }

        for (int thread_index = 0; thread_index < threads_count; ++thread_index) {
            threads[thread_index] = thread([&d, data_end, &critical_section_lock, &cur_chunk_start, dynamic_chunk_size, &dynamic_schedule_lock, &result]() {
                size_t els[256] = {0};
                dynamic_schedule_lock.lock();
                int i = cur_chunk_start;
                cur_chunk_start += dynamic_chunk_size;
                dynamic_schedule_lock.unlock();

                int end = i + dynamic_chunk_size;

                while (i < data_end) {
                    els[d[i]] += 1;
                    i++;

                    if (i == end) {
                        dynamic_schedule_lock.lock();
                        i = cur_chunk_start;
                        cur_chunk_start += dynamic_chunk_size;
                        dynamic_schedule_lock.unlock();
                        end = i + dynamic_chunk_size;
                    }
                }

                critical_section_lock.lock();
                for (auto j = 0; j < 256; j++) {
                    result[j] += els[j];
                }
                critical_section_lock.unlock();
            });
        }
    } else {
        printf("Unsupported schedule type");
        return;
    }

    for (int thread_index = 0; thread_index < threads_count; ++thread_index) {
        threads[thread_index].join();
    }
}