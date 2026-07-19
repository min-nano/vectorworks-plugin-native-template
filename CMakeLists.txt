cmake_minimum_required(VERSION 3.20)

#
# HelloVW - a minimal Vectorworks 2026 SDK plug-in.
#
# The build produces a loadable macOS bundle named "HelloVW.vwlibrary" that
# adds a single menu command. Running the command shows an alert dialog.
#
# Requirements:
#   * macOS with the Xcode command line tools (Xcode 16.2 is the version
#     Vectorworks 2026 officially targets).
#   * The Vectorworks 2026 mac SDK, extracted somewhere on disk. Point CMake
#     at the folder that CONTAINS "SDKLib" via -DVW_SDK_DIR=/path/to/sdk
#     (or the VW_SDK_DIR environment variable).
#
# Example:
#   cmake -S . -B build -DVW_SDK_DIR=/path/to/2026-NNA-eng-mac-SDK
#   cmake --build build --config Release
#

# --- Architecture / deployment target (must be set before project()) --------
# Default to Apple Silicon. Override with e.g.
#   -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"  for a universal binary.
if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
	set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target architectures")
endif()
if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version")
endif()

# The SDK headers are compiled as Objective-C++ (some mac headers #import
# CoreFoundation), matching how the SDK's own Xcode projects build.
project(HelloVW LANGUAGES CXX OBJCXX)

if(NOT APPLE)
	message(FATAL_ERROR "This Vectorworks plug-in can only be built on macOS.")
endif()

# --- Locate the Vectorworks SDK ---------------------------------------------
if(NOT DEFINED VW_SDK_DIR AND DEFINED ENV{VW_SDK_DIR})
	set(VW_SDK_DIR "$ENV{VW_SDK_DIR}")
endif()

if(NOT DEFINED VW_SDK_DIR)
	message(FATAL_ERROR
		"VW_SDK_DIR is not set. Point it at the folder that contains 'SDKLib', e.g.\n"
		"  cmake -S . -B build -DVW_SDK_DIR=/path/to/2026-NNA-eng-mac-SDK")
endif()

set(VW_SDK_INCLUDE "${VW_SDK_DIR}/SDKLib/Include")
set(VW_SDK_LIBMAC  "${VW_SDK_DIR}/SDKLib/LibMac/Release")

if(NOT EXISTS "${VW_SDK_INCLUDE}/VectorworksSDK.h")
	message(FATAL_ERROR
		"Could not find VectorworksSDK.h under '${VW_SDK_INCLUDE}'.\n"
		"Check VW_SDK_DIR (currently '${VW_SDK_DIR}').")
endif()
if(NOT EXISTS "${VW_SDK_LIBMAC}/libVWSDK.a")
	message(FATAL_ERROR "Could not find libVWSDK.a under '${VW_SDK_LIBMAC}'.")
endif()

# --- The plug-in target ------------------------------------------------------
set(HELLOVW_SOURCES
	src/ModuleMain.cpp
	src/Extensions/ExtMenu.cpp
)

add_library(HelloVW MODULE ${HELLOVW_SOURCES})

# Build the sources as Objective-C++ (see note on project() above).
set_source_files_properties(${HELLOVW_SOURCES} PROPERTIES LANGUAGE OBJCXX)

set_target_properties(HelloVW PROPERTIES
	OBJCXX_STANDARD 20
	OBJCXX_STANDARD_REQUIRED ON
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
	OUTPUT_NAME "HelloVW"
	# Produce a proper loadable bundle named HelloVW.vwlibrary
	BUNDLE TRUE
	BUNDLE_EXTENSION "vwlibrary"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Module-Info.plist"
)

target_include_directories(HelloVW PRIVATE
	"${VW_SDK_INCLUDE}"
	"${VW_SDK_INCLUDE}/Kernel"
	"${VW_SDK_INCLUDE}/OnlyMac"
	"${VW_SDK_INCLUDE}/Interfaces"
	"${VW_SDK_INCLUDE}/VWMM"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# We link against the Release build of the SDK, which requires RELEASE_BLD.
target_compile_definitions(HelloVW PRIVATE RELEASE_BLD=1)

target_compile_options(HelloVW PRIVATE
	-stdlib=libc++
	-Wno-deprecated-declarations
)

# Link the static SDK library plus the frameworks it depends on. VWMM is a
# framework shipped inside the SDK; the others are system frameworks.
target_link_libraries(HelloVW PRIVATE
	"${VW_SDK_LIBMAC}/libVWSDK.a"
	"-F${VW_SDK_LIBMAC}"
	"-framework VWMM"
	"-framework Carbon"
	"-framework Cocoa"
)
