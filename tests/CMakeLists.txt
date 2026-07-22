# Unit tests for the plug-in's SDK-independent helpers (src/UpdaterParse.h).
#
# These deliberately do NOT link the Vectorworks SDK, so they build and run on
# any toolchain — including a plain Linux runner — which is what makes the
# coverage job cheap. Enable with -DVW_BUILD_TESTS=ON (see the top-level
# CMakeLists.txt); VW_ENABLE_COVERAGE adds gcov instrumentation.

add_executable(UpdaterParseTests
	UpdaterParseTests.cpp
)

target_compile_features(UpdaterParseTests PRIVATE cxx_std_20)

# The tests include "UpdaterParse.h" and the local "TestFramework.h".
target_include_directories(UpdaterParseTests PRIVATE
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}"
)

# Coverage instrumentation (gcov/llvm-cov compatible). Applied only to the test
# target so the numbers reflect the code under test, not the harness build.
if(VW_ENABLE_COVERAGE)
	if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
		message(FATAL_ERROR "VW_ENABLE_COVERAGE requires GCC or Clang.")
	endif()
	target_compile_options(UpdaterParseTests PRIVATE
		-O0 -g --coverage -fprofile-arcs -ftest-coverage)
	target_link_options(UpdaterParseTests PRIVATE --coverage)
endif()

add_test(NAME UpdaterParseTests COMMAND UpdaterParseTests)
