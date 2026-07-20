cmake_minimum_required(VERSION 3.20)

#
# SamplePlugin - a minimal Vectorworks 2026 SDK plug-in template.
#
# The build produces a loadable macOS bundle named "SamplePlugin.vwlibrary"
# that adds a single menu command. Running the command shows an alert dialog.
# Rename the identifiers below (target, output name, bundle id, menu category)
# when you use this template for a real plug-in.
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
project(SamplePlugin LANGUAGES CXX OBJCXX)

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

# --- Build stamp -------------------------------------------------------------
# Short identifier of this exact build (git commit in CI, "local" otherwise).
# Stamped into each bundle's Info.plist and shown in the menu command's alert so
# an installed build can be identified and compared by the updater scripts.
if(NOT DEFINED VW_BUILD_VERSION)
	set(VW_BUILD_VERSION "local" CACHE STRING "Short build identifier (git commit)")
endif()

set(VW_BUILDVWR "${VW_SDK_DIR}/SDKLib/ToolsMac/BuildVWR/BuildVWR")

if(NOT EXISTS "${VW_BUILDVWR}")
	message(WARNING
		"BuildVWR tool not found at '${VW_BUILDVWR}'. The plug-ins will build, "
		"but their menu commands will have no display name. Ensure SDKLib/ToolsMac "
		"is present in VW_SDK_DIR.")
endif()

set(VW_COMMON_SOURCES
	src/ModuleMain.cpp
	src/Extensions/ExtMenu.cpp
)

# add_vw_plugin(<target> <output-name> <bundle-id> <channel> [DEV])
#
# Defines one loadable "<output-name>.vwlibrary" bundle. The same sources build
# both the stable and dev plug-ins; passing DEV compiles them with VW_DEV_BUILD
# so the plug-in takes on its dev identity (see src/BuildConfig.h). Each plug-in
# packages its own "<output-name>.vwr" resource, whose base name must match the
# identifier returned by DefaultPluginVWRIdentifier().
function(add_vw_plugin target output_name bundle_id channel)
	cmake_parse_arguments(ARG "DEV" "" "" ${ARGN})

	add_library(${target} MODULE ${VW_COMMON_SOURCES})
	# Build the sources as Objective-C++ (see note on project() above). Set per
	# target so each object is compiled with that target's own definitions.
	set_source_files_properties(${VW_COMMON_SOURCES}
		TARGET_DIRECTORY ${target} PROPERTIES LANGUAGE OBJCXX)

	# Fill in a per-target Info.plist from the shared template.
	set(VW_BUNDLE_NAME "${output_name}")
	set(VW_BUNDLE_ID   "${bundle_id}")
	set(VW_BUILD_CHANNEL "${channel}")
	set(plist "${CMAKE_CURRENT_BINARY_DIR}/${target}-Info.plist")
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/src/Module-Info.plist.in"
		"${plist}" @ONLY)

	set_target_properties(${target} PROPERTIES
		OBJCXX_STANDARD 20
		OBJCXX_STANDARD_REQUIRED ON
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED ON
		OUTPUT_NAME "${output_name}"
		BUNDLE TRUE
		BUNDLE_EXTENSION "vwlibrary"
		MACOSX_BUNDLE_INFO_PLIST "${plist}"
	)

	target_include_directories(${target} PRIVATE
		"${VW_SDK_INCLUDE}"
		"${VW_SDK_INCLUDE}/Kernel"
		"${VW_SDK_INCLUDE}/OnlyMac"
		"${VW_SDK_INCLUDE}/Interfaces"
		"${VW_SDK_INCLUDE}/VWMM"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
	)

	# RELEASE_BLD: link against the Release SDK. VW_BUILD_VERSION: build stamp.
	target_compile_definitions(${target} PRIVATE
		RELEASE_BLD=1
		"VW_BUILD_VERSION=\"${VW_BUILD_VERSION}\""
	)
	if(ARG_DEV)
		target_compile_definitions(${target} PRIVATE VW_DEV_BUILD=1)
	endif()

	target_compile_options(${target} PRIVATE
		-stdlib=libc++
		-Wno-deprecated-declarations
	)

	# Link the static SDK library plus the frameworks it depends on. VWMM is a
	# framework shipped inside the SDK; the others are system frameworks.
	target_link_libraries(${target} PRIVATE
		"${VW_SDK_LIBMAC}/libVWSDK.a"
		"-F${VW_SDK_LIBMAC}"
		"-framework VWMM"
		"-framework Carbon"
		"-framework Cocoa"
	)

	# Package "<output-name>.vwr" into the bundle. The SDK's BuildVWR tool reads
	# resources/<output-name>.vwr and, in Release mode, writes a no-compression
	# zip to Contents/Resources/<output-name>.vwr inside the bundle.
	set(vwr_src "${CMAKE_CURRENT_SOURCE_DIR}/resources/${output_name}.vwr")
	if(EXISTS "${VW_BUILDVWR}")
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E make_directory
				"$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources"
			COMMAND "${CMAKE_COMMAND}" -E env
				"VWR_INPUT=${vwr_src}"
				"VWR_OUTPUT=$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/${output_name}.vwr"
				"VWR_MODE=Release"
				"${VW_BUILDVWR}"
			VERBATIM
			COMMENT "Packaging ${output_name}.vwr resource into the bundle")
	endif()
endfunction()

# Stable plug-in (from main) and dev plug-in (from feature/PR branches). Both
# can be installed and loaded at the same time.
add_vw_plugin(SamplePlugin    "SamplePlugin"    "com.example.vectorworks.SamplePlugin"    "stable")
add_vw_plugin(SamplePluginDev "SamplePluginDev" "com.example.vectorworks.SamplePluginDev" "dev" DEV)
