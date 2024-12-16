cmake_minimum_required(VERSION 3.20)
project(TestProject)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenCL REQUIRED)

add_executable(ContrastBalancer main.cpp
        pnm.cpp
        pnm.h
        args_parser.cpp
        args_parser.h
        time_monitor.cpp
        time_monitor.h
        csv_writer.cpp
        csv_writer.h
)

target_link_libraries(ContrastBalancer OpenCL::OpenCL)
