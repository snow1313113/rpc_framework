cmake_minimum_required (VERSION 2.6)

project(rpc_service_plugin)

file(GLOB_RECURSE SOURCES *.cpp *.cc)

set(rpc_proto_file ${PROJECT_SOURCE_DIR}/rpc_options.proto)
get_filename_component(proto_file_prefix ${rpc_proto_file} NAME_WE)
set(rpc_file_cc ${PROJECT_SOURCE_DIR}/${proto_file_prefix}.pb.cc)
set(rpc_file_h ${PROJECT_SOURCE_DIR}/${proto_file_prefix}.pb.h)
add_custom_command(
    OUTPUT ${rpc_file_cc} ${rpc_file_h}
    COMMAND ${PROTOC_TOOL} --cpp_out=${PROJECT_SOURCE_DIR} --proto_path=${PROJECT_SOURCE_DIR}:${PROTOBUF_DIR} ${rpc_proto_file}
    DEPENDS ${rpc_proto_file}
    COMMENT "gen files of ${rpc_proto_file}"
    )
list(APPEND SOURCES ${rpc_file_cc})

include_directories(${PROTOBUF_INCLUDE} ${PROJECT_SOURCE_DIR})

link_directories(${PROTOBUF_LIB_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} 
    libprotoc.a
    libprotobuf.a
    pthread
    )

set(EXECUTABLE_OUTPUT_PATH ${BINARY_PATH})
