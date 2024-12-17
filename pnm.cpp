//
// Created by Igor Kluzhev on 25.09.2024.
//

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include "pnm.h"
#include "time_monitor.h"
#include <cmath>
#include <stdio.h>
#include <stdexcept>
#ifndef __APPLE__
#include <algoriphm>
#endif

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
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

void PNMPicture::modify(const float coeff) noexcept {
    auto tm = TimeMonitor(false);
    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements;
    uchar min_v = 255;
    uchar max_v = 0;
    tm.start();
    analyzeData(elements);
    auto analyze_elapsed = tm.stop();
//    for (int i = 0; i < elements.size(); i++) {
//        cout << elements[i] << ", ";
//    }
//    cout << endl;
    tm.start();
    determineMinMax(ignoreCount, elements, min_v, max_v);
    auto determine_min_max_elapsed = tm.stop();

    // если уже растянуто - не делаем ничего
    // или если например 1 цвет - не делаем ничего
    if ((min_v == 0 && max_v == 255) || min_v >= max_v) {
        return;
    }

    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    uchar* d = data.data();
    tm.start();
    for (size_t i = 0; i < data_size; i++) {
        int scaledValue1 = scale * d[i] - scaledMinV;
        d[i] = max(0, min(scaledValue1, 255));
    }
    auto scale_elapsed = tm.stop();

//    cout << "analyze = " << analyze_elapsed << endl;
//    cout << "determine_min_max_elapsed = " << determine_min_max_elapsed << endl;
//    cout << "scale_elapsed = " << scale_elapsed << endl;
}

void PNMPicture::analyzeData(vector<size_t> & elements) const noexcept {
    elements.resize(256, 0);

    const uchar* d = data.data();
    for (size_t i = 0; i < data_size; i++) {
        elements[d[i]] += 1;
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

string getPlatformInfo(cl_platform_id id, cl_platform_info info) {
    size_t size;
    cl_int error1 = clGetPlatformInfo(id, info, 0, NULL, &size);
    if (error1 != 0) {
        return "ERROR: " + to_string(error1);
    }
    vector<char> data(size);
    cl_int error2 = clGetPlatformInfo(id, info, size, data.data(), NULL);
    if (error2 != 0) {
        return "ERROR: " + to_string(error2);
    }
    string name(data.begin(), data.end() - 1);
    return name;
}

string getDeviceInfo(cl_device_id id, cl_device_info info) {
    size_t size;
    cl_int error1 = clGetDeviceInfo(id, info, 0, NULL, &size);
    if (error1 != 0) {
        return "ERROR: " + to_string(error1);
    }
    vector<char> data(size);
    cl_int error2 = clGetDeviceInfo(id, info, size, data.data(), NULL);
    if (error2 != 0) {
        return "ERROR: " + to_string(error2);
    }
    string name(data.begin(), data.end() - 1);
    return name;
}

vector<cl_device_id> getAllDevices(cl_platform_id platform, cl_device_type device_type) {
    cl_uint num_devices;
    clGetDeviceIDs(platform, device_type, 0, NULL, &num_devices);
    vector<cl_device_id> devices(num_devices);
    clGetDeviceIDs(platform, device_type, num_devices, devices.data(), NULL);
    return devices;
}

cl_device_id getSelectedDevice(int device_index, string device_type) {
    vector<cl_device_id> all_devices = {};
    vector<cl_device_id> filtered_devices;

    cl_uint num;
    clGetPlatformIDs(0, NULL, &num);

    vector<cl_platform_id> platforms(num);
    clGetPlatformIDs(num, platforms.data(), NULL);

    vector<cl_device_type> cur_device_types = {CL_DEVICE_TYPE_ALL };
    if (device_type == "all") {
        cur_device_types = { CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_GPU };
    } else if (device_type.find("gpu") != string::npos) {
        cur_device_types = { CL_DEVICE_TYPE_GPU };
    } else if (device_type == "cpu") {
        cur_device_types = { CL_DEVICE_TYPE_CPU };
    }

    for (cl_platform_id platform : platforms) {
        vector<cl_device_id> devices;
        for (cl_device_type type : cur_device_types) {
            devices = getAllDevices(platform, type);
            all_devices.insert(all_devices.end(), devices.begin(), devices.end());
        }

        if (device_type == "dgpu" || device_type == "igpu") {
            copy_if(all_devices.begin(), all_devices.end(), back_inserter(filtered_devices), [device_type](cl_device_id device){
                cl_bool isIntegrated;
                clGetDeviceInfo(device, CL_DEVICE_HOST_UNIFIED_MEMORY, sizeof(cl_bool), &isIntegrated, NULL);
                if (device_type == "dgpu") {
                    return isIntegrated == 0;
                } else {
                    return isIntegrated == 1;
                }
            });
        } else {
            filtered_devices = all_devices;
        }
    }
    if (device_index < filtered_devices.size()) {
        return filtered_devices[device_index];
    }
    throw runtime_error("No device with specified characteristics");
}

cl_program getOpenCLProgram(cl_context context) {
    FILE *fp;
    #ifdef __APPLE__
    const char fileName[] = "../kernel.cl";
    #else
    const char fileName[] = "kernel.cl";
    #endif

    size_t source_size;
    char *source_str;
    int i;

    try {
        fp = fopen(fileName, "r");
        if (!fp) {
            fprintf(stderr, "Failed to load kernel.\n");
            exit(1);
        }
        int MAX_SOURCE_SIZE = 100000;
        source_str = (char *)malloc(MAX_SOURCE_SIZE);
        source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
        fclose(fp);
    }
    catch (int a) {
        printf("%d", a);
    }

    cl_int error;
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &error);
    if (error != 0) {
        fprintf(stderr, "Failed to create program.\n");
        exit(error);
    }
    return program;
}

cl_uint getDeviceInfoUInt(cl_device_id id, cl_device_info info) {
    cl_uint data;
    clGetDeviceInfo(id, info, sizeof(cl_uint), &data, NULL);
    return data;
}

cl_ulong getDeviceInfoULong(cl_device_id id, cl_device_info info) {
    cl_ulong data;
    clGetDeviceInfo(id, info, sizeof(cl_ulong), &data, NULL);
    return data;
}

size_t getDeviceInfoSizeT(cl_device_id id, cl_device_info info) {
    size_t data;
    clGetDeviceInfo(id, info, sizeof(size_t), &data, NULL);
    return data;
}

vector<size_t> getDeviceInfoSizeTArray(cl_device_id id, cl_device_info info, unsigned int size) {
    vector<size_t> data(size);
    clGetDeviceInfo(id, info, sizeof(size_t) * size, data.data(), NULL);
    return data;
}

void printBuildLog(cl_device_id selectedDevice, cl_program program) {
    size_t log_size;
    clGetProgramBuildInfo(program, selectedDevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    vector<char> build_log_data(log_size);
    clGetProgramBuildInfo(program, selectedDevice, CL_PROGRAM_BUILD_LOG, log_size, build_log_data.data(), NULL);
    string build_log(build_log_data.begin(), build_log_data.end() - 1);

    fprintf(stderr, "Error building open cl program\n\nBuild log:\n%s\n", build_log.c_str());
}

void PNMPicture::modifyOpenCL(const float coeff, const int device_index, const string device_type) noexcept {
    auto tm = TimeMonitor(false);

    cl_device_id device;
    try {
        device = getSelectedDevice(device_index, device_type);
//        cout << "Device = " << getDeviceInfo(device, CL_DEVICE_NAME) << endl;
    } catch(runtime_error e) {
        fprintf(stderr, "%s\n", e.what());
        device = getSelectedDevice(0, "all");
//        cout << "Device = " << getDeviceInfo(device, CL_DEVICE_NAME) << endl;
    }

    auto compute_units_count = getDeviceInfoUInt(device, CL_DEVICE_MAX_COMPUTE_UNITS);
//    cout << "CL_DEVICE_MAX_COMPUTE_UNITS = " << compute_units_count << endl;
    auto dims = getDeviceInfoUInt(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
//    cout << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS = " << dims << endl;
    auto wi_sizes = getDeviceInfoSizeTArray(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, dims);
//    cout << "CL_DEVICE_MAX_WORK_ITEM_SIZES = " << wi_sizes[0] << endl;
    auto max_group_size = getDeviceInfoSizeT(device, CL_DEVICE_MAX_WORK_GROUP_SIZE);
//    cout << "CL_DEVICE_MAX_WORK_GROUP_SIZE = " << max_group_size << endl;
    auto max_mem = getDeviceInfoULong(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
//    cout << "CL_DEVICE_MAX_MEM_ALLOC_SIZE = " << max_mem << endl;
    auto max_parallel = compute_units_count * max_group_size;
    max_parallel_computing = max_parallel / 4;
//    cout << "MAX = " << compute_units_count * max_group_size << endl;

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, NULL);

    size_t device_data_size = data_size * sizeof(cl_uchar);
    cl_mem device_data = clCreateBuffer(context, CL_MEM_READ_WRITE, device_data_size, NULL, NULL);
    clEnqueueWriteBuffer(queue, device_data, CL_FALSE, 0, device_data_size, data.data(), 0, NULL, NULL);

    cl_program program = getOpenCLProgram(context);
    cl_int build_error = clBuildProgram(program, 0, NULL, "", NULL, NULL);

    if (build_error != 0) {
        printBuildLog(device, program);
        exit(1);
    }

    if (data_size == 1) {
        return;
    }

    size_t ignoreCount = data_size * coeff;
    vector<size_t> elements(256, 0);
    uchar min_v = 255;
    uchar max_v = 0;

    double analyze_elapsed = analyzeDataOpenCL(elements, device_data, program, device, context, queue);
//    for (int i = 0; i < elements.size(); i++) {
//        cout << elements[i] << ", ";
//    }
//    cout << endl;
    tm.start();
    determineMinMax(ignoreCount, elements, min_v, max_v);
    auto determine_min_max_elapsed = tm.stop();

    // если уже растянуто - не делаем ничего
    // или если например 1 цвет - не делаем ничего
    if ((min_v == 0 && max_v == 255) || min_v >= max_v) {
        printf("Time: %g\n", analyze_elapsed + determine_min_max_elapsed);
        return;
    }
    float const scale = 255 / float(max_v - min_v);
    float scaledMinV = scale * float(min_v);

    double scale_elapsed = scaleImageData(device_data, device, program, context, queue, scale, scaledMinV);

    clFinish(queue);
    clReleaseContext(context);

//    cout << "analyze = " << analyze_elapsed << endl;
//    cout << "determine_min_max_elapsed = " << determine_min_max_elapsed << endl;
//    cout << "scale_elapsed = " << scale_elapsed << endl;

    printf("Time: %g\n", scale_elapsed + analyze_elapsed + determine_min_max_elapsed);
}

double PNMPicture::scaleImageData(
    cl_mem device_data,
    cl_device_id device,
    cl_program program,
    cl_context context,
    cl_command_queue queue,
    cl_float scale,
    cl_float scaledMinV
) noexcept {
    cl_int kernel_creation_error;
    cl_kernel kernel = clCreateKernel(program, "modify", &kernel_creation_error);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &device_data);
    clSetKernelArg(kernel, 1, sizeof(cl_float), &scale);
    clSetKernelArg(kernel, 2, sizeof(cl_float), &scaledMinV);

    cl_event event;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &data_size, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    size_t device_data_size = data_size * sizeof(cl_uchar);
    clEnqueueReadBuffer(queue, device_data, CL_TRUE, 0, device_data_size, data.data(), 0, NULL, NULL);

    cl_ulong time_start;
    cl_ulong time_end;

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    double open_cl_ms_elapsed = (time_end - time_start) / 1000000.0;
    return open_cl_ms_elapsed;
}

void checkError(cl_int error) {
    if (error != 0) {
        fprintf(stderr, "Error: %d", error);
    }
}

double PNMPicture::analyzeDataOpenCL(
    vector<size_t> &elements,
    cl_mem device_data,
    cl_program program,
    cl_device_id device,
    cl_context context,
    cl_command_queue queue
) const noexcept {
    int chunk_size = data_size / max_parallel_computing;
//    cout << "CHUNK_SIZE = " << chunk_size << endl;

    cl_int kernel_creation_error;
    cl_kernel kernel = clCreateKernel(program, "makeGist", &kernel_creation_error);

    size_t gist_size = 256 * max_parallel_computing;
    vector<size_t> gist(gist_size, 0);
    size_t gist_data_size = gist_size * sizeof(size_t);
    cl_mem device_gist = clCreateBuffer(context, CL_MEM_READ_WRITE, gist_data_size, NULL, NULL);
    checkError(clEnqueueWriteBuffer(queue, device_gist, CL_FALSE, 0, gist_data_size, gist.data(), 0, NULL, NULL));

    checkError(clSetKernelArg(kernel, 0, sizeof(cl_mem), &device_data));
    checkError(clSetKernelArg(kernel, 1, sizeof(cl_mem), &device_gist));
    checkError(clSetKernelArg(kernel, 2, sizeof(cl_int), &chunk_size));

    cl_event event;
    const size_t work_size = max_parallel_computing;
    cl_int kernel_error = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &work_size, NULL, 0, NULL, &event);
    if (kernel_error != 0) {
        fprintf(stderr, "Kernel error %d\n", kernel_error);
    }
    clWaitForEvents(1, &event);
    clEnqueueReadBuffer(queue, device_gist, CL_TRUE, 0, gist_data_size, gist.data(), 0, NULL, NULL);

    for (size_t i = 0; i < 256; i++) {
        for (size_t j = 0; j < work_size; j++) {
            elements[i] += gist[i + j * 256];
        }
    }

    cl_ulong time_start;
    cl_ulong time_end;

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    double open_cl_ms_elapsed = (time_end - time_start) / 1000000.0;
    return open_cl_ms_elapsed;
}















