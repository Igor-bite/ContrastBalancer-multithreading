cmake_minimum_required(VERSION 3.29)
project(TestProject)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_executable(ContrastBalancer main.cpp
        pnm.cpp
        pnm.h
        args_parser.cpp
        args_parser.h
        time_monitor.cpp
        time_monitor.h
        csv_writer.cpp
        csv_writer.h
        pnm.cu
        pnm.hip
)
