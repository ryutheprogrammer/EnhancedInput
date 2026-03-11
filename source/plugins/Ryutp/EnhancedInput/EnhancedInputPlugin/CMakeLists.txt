cmake_minimum_required(VERSION 3.30)
project(EnhancedInput_plugin LANGUAGES CXX)

set(vendor_name Ryutp)
set(plugin_name EnhancedInput)

set(target ${PROJECT_NAME})

set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include)
set(OUTPUT_PLUGIN_DIR ${UNIGINE_BIN_DIR}/plugins/${vendor_name}/${plugin_name})

set(binary_name ${vendor_name}${target})
string(APPEND binary_name $<$<BOOL:${UNIGINE_DOUBLE}>:_double>)
string(APPEND binary_name _x64)

file(GLOB_RECURSE SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
add_library(${target} MODULE ${SRCS})

if(UNIX)
    set_target_properties(${target} PROPERTIES
	C_VISIBILITY_PRESET hidden
	CXX_VISIBILITY_PRESET hidden
    )
endif()

target_link_libraries(${target} PRIVATE
    Unigine::Engine
)

target_include_directories(${target}
PRIVATE
${UNIGINE_INCLUDE_DIR}
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_compile_definitions(${target}
PRIVATE
$<$<BOOL:${UNIX}>:_LINUX>
)

set_target_properties(${target}
PROPERTIES
OUTPUT_NAME ${binary_name}$<$<CONFIG:Debug>:d>
LIBRARY_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PLUGIN_DIR}
LIBRARY_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PLUGIN_DIR}
RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_PLUGIN_DIR}
RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_PLUGIN_DIR}
C_VISIBILITY_PRESET hidden
CXX_VISIBILITY_PRESET hidden
)
