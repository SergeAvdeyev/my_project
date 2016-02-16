project(lapb_so)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)
cmake_minimum_required(VERSION 2.8)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_CURRENT_SOURCE_DIR}/../export)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "../../../bin/")
add_library(lapb SHARED ../src/lapb_iface.c ../src/lapb_in.c ../src/lapb_out.c ../src/lapb_queue.c ../src/lapb_subr.c ../src/lapb_timer.c)

