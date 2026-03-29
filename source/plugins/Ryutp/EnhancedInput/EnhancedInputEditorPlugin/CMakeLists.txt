##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 3.30)
project(EnhancedInput_editorplugin LANGUAGES CXX)

set(vendor_name Ryutp)
set(plugin_name EnhancedInput)

set(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTORCC TRUE)

set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include)
set(OUTPUT_PLUGIN_DIR ${UNIGINE_BIN_DIR}/plugins/${vendor_name}/${plugin_name})

##==============================================================================
## Variables.
##==============================================================================
set(target ${PROJECT_NAME})

set(binary_name ${vendor_name}${target})
string(APPEND binary_name $<$<BOOL:${UNIGINE_DOUBLE}>:_double>)
string(APPEND binary_name _x64)

##==============================================================================
## Dependencies.
##==============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake/modules)
find_package(Engine REQUIRED MODULE QUIET)
find_package(Editor REQUIRED MODULE QUIET)

if(UNIX)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
endif()

# UNIGINE_QTROOT is Unigine related global variable that points to the effective
# Qt's distributive.
set(Qt6_ROOT "$ENV{UNIGINE_QTROOT}")
if (Qt6_ROOT AND EXISTS "${Qt6_ROOT}")
        list(APPEND CMAKE_PREFIX_PATH ${Qt6_ROOT})
endif()

# The following construct is required as Editor's Debug build
# uses Release binaries of Qt6 framework
set(temp_map_imported ${CMAKE_MAP_IMPORTED_CONFIG_DEBUG})
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELEASE)

# Find additional required packages
find_package(Qt6 6.5.3 CONFIG EXACT COMPONENTS Core Gui Widgets)

# Apply imported_location for qt executable
get_target_property(MOC_EXECUTABLE Qt6::moc IMPORTED_LOCATION)
set_target_properties(Qt6::moc PROPERTIES IMPORTED_LOCATION_RELEASE ${MOC_EXECUTABLE})
get_target_property(RCC_EXECUTABLE Qt6::rcc IMPORTED_LOCATION)
set_target_properties(Qt6::rcc PROPERTIES IMPORTED_LOCATION_RELEASE ${RCC_EXECUTABLE})
get_target_property(UIC_EXECUTABLE Qt6::uic IMPORTED_LOCATION)
set_target_properties(Qt6::uic PROPERTIES IMPORTED_LOCATION_RELEASE ${UIC_EXECUTABLE})
get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
set_target_properties(Qt6::qmake PROPERTIES IMPORTED_LOCATION_RELEASE ${QMAKE_EXECUTABLE})

# Restore the actual value
set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG ${temp_map_imported})
unset(temp_map_imported)

if (NOT Qt6_FOUND)
        message(FATAL_ERROR "The required Qt version is not found on your computer, "
	"please follow this link to see the instructions on resolving this issue. "
	"https://developer.unigine.com/en/docs/latest/editor2/extensions/environment")
endif()
##==============================================================================
## Target.
##==============================================================================
file(GLOB_RECURSE COMMON_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../EnhancedInput/*.h" "${CMAKE_CURRENT_LIST_DIR}/../EnhancedInput/*.cpp")
file(GLOB_RECURSE EDITOR_PLUGIN_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.h" "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
# file(GLOB_RECURSE PLUGIN_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../EnhancedInputPlugin/*.h" "${CMAKE_CURRENT_LIST_DIR}/../EnhancedInputPlugin/*.cpp")

add_library(${target} SHARED ${EDITOR_PLUGIN_SOURCES} ${COMMON_SOURCES} ${PLUGIN_SOURCES})

target_include_directories(${target} SYSTEM
        PRIVATE
	${UNIGINE_INCLUDE_DIR}
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${target}
        PRIVATE
	Unigine::Engine
	Unigine::Editor
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
)

##==============================================================================
## Target compiling options and properties.
##==============================================================================
target_compile_definitions(${target}
        PRIVATE
	$<$<BOOL:${UNIX}>:_LINUX>
	QT_NO_DEBUG
)

message(STATUS "EnhancedInput editor plugin output dir: ${OUTPUT_PLUGIN_DIR}")
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
