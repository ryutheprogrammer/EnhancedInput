cmake_minimum_required(VERSION 3.30)
project(EnhancedInput)

set(vendor_name Ryutp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

if(NOT DEFINED UNIGINE_DOUBLE)
	set(UNIGINE_DOUBLE 1)
endif()

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

list(APPEND CMAKE_MODULE_PATH ${UNIGINE_SDK_PATH}source/cmake/modules)

find_package(Engine REQUIRED MODULE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(UNIGINE_COMPILER_IS_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(UNIGINE_COMPILER_IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(UNIGINE_COMPILER_IS_GNU TRUE)
endif()

if(UNIGINE_COMPILER_IS_MSVC)
        add_compile_definitions(
		_CRT_SECURE_NO_DEPRECATE
	)
        add_compile_options(
		/TP
		/FS
		/fp:fast
		/bigobj
		/EHsc
	)
        add_link_options("/FIXED:NO")
elseif(UNIGINE_COMPILER_IS_GNU OR UNIGINE_COMPILER_IS_CLANG)
        add_compile_options(
		-m64
		-march=athlon64
		-msse4.2
		-ffast-math
		-fno-strict-aliasing
		$<$<NOT:$<CONFIG:Debug>>:-fomit-frame-pointer>
		$<$<CONFIG:Debug>:-Wall>
		$<$<CONFIG:Debug>:-Wno-unknown-pragmas>
		$<$<CONFIG:Debug>:-Wno-unused-parameter>
	)
endif ()

add_subdirectory(EnhancedInputPlugin)
add_subdirectory(EnhancedInputEditorPlugin)
