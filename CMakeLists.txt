CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(rpc_framework_root)

# color
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColourReset "${Esc}[m")
  set(ColourBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

set(MY_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(MY_LIB_DIR ${MY_ROOT}/lib)

set(BASE_INCLUDE ${MY_ROOT}/src)
set(LIBCO_INCLUDE ${MY_ROOT}/third_party/libco)
set(ASIO_INCLUDE ${MY_ROOT}/third_party/asio/asio/include)
set(PLUGIN_INCLUDE ${MY_ROOT}/rpc_service_plugin)

set(PROTOBUF_ROOT_DIR ${MY_ROOT}/third_party/protobuf)
set(PROTOBUF_INCLUDE ${PROTOBUF_ROOT_DIR}/src)

set(PROTOBUF_LIB_DIR ${PROTOBUF_INCLUDE}/.libs/)

set(PROTOC_TOOL ${PROTOBUF_INCLUDE}/protoc)

set(BINARY_PATH ${MY_ROOT}/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -fPIC -std=c++17")

add_subdirectory(rpc_service_plugin)
add_subdirectory(src)
