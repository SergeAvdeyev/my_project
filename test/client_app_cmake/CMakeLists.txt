project(x25_client_cmake)
set(CMAKE_BUILD_TYPE Debug)
cmake_minimum_required(VERSION 2.8)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/export)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../LAPB/lib/export)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../LAPB/lib/include)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../../bin/")
add_executable(${PROJECT_NAME}  ../src/client_main.c ../src/tcp_client.c
				../src/common.c ../src/my_timer.c ../src/logger.c
				../../LAPB/lib/src/lapb_iface.c ../../LAPB/lib/src/lapb_subr.c ../../LAPB/lib/src/lapb_timer.c
				../../LAPB/lib/src/lapb_out.c  ../../LAPB/lib/src/lapb_in.c
				../../LAPB/lib/src/subr.c ../../LAPB/lib/src/queue.c
				../../lib/src/x25_iface.c ../../lib/src/x25_link.c ../../lib/src/x25_subr.c
				../../lib/src/x25_timer.c ../../lib/src/x25_in.c ../../lib/src/x25_out.c
				../../lib/src/x25_facilities.c)

