cmake_minimum_required (VERSION 2.6)

project(rpc_framework)

include_directories(${LIBCO_INCLUDE})

file(GLOB_RECURSE CC_SOURCE *.cpp)

add_library(${PROJECT_NAME} ${CC_SOURCE})

set(LIBRARY_OUTPUT_PATH ${MY_LIB_DIR})
