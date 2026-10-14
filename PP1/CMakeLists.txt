cmake_minimum_required(VERSION 3.22)
message(STATUS "Root level CMake")
set(project "PP1")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(${project} LANGUAGES CXX)
# cmake_policy(SET CMP0079 NEW)
# NOTE: May or may not need above line

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
message("Runtime dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

add_library(common src/common/PDU.cpp)
target_include_directories(common PUBLIC src/common)
# add_library(hello src/hello.cpp)
# target_include_directories(hello PUBLIC src)

add_executable(PP1_UDPFileReceiver 
   src/Receiver.cpp
   src/PP1_UDPFileReceiver.cpp
)
target_link_libraries(PP1_UDPFileReceiver PRIVATE common)
# target_link_libraries(PP1_UDPFileReceiver PRIVATE hello)

add_executable(PP1_UDPFileSender src/Sender.cpp)
target_link_libraries(PP1_UDPFileSender PRIVATE common)
# target_link_libraries(PP1_UDPFileSender PRIVATE hello)
