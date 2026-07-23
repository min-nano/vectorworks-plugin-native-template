# Unit tests for the plug-in's SDK-independent code. Two executables:
#
#   * UpdaterParseTests — the pure parsing/quoting/path/decision helpers in
#                         src/UpdaterParse.h (header-only).
#   * UpdaterFlowTests  — the update FLOWS in src/UpdaterFlow.cpp, driven through
#                         a fake IUpdaterHost so the whole flow (branches + dialog
#                         wording) is exercised without the SDK.
#
# Neither links the Vectorworks SDK, so they build and run on any toolchain —
# including a plain Linux runner — which is what makes the coverage job cheap.
# Enable with -DVW_BUILD_TESTS=ON (see the top-level CMakeLists.txt);
# VW_ENABLE_COVERAGE adds gcov instrumentation.

# Coverage instrumentation (gcov/llvm-cov compatible), applied per test target so
# the numbers reflect the code under test, not the harness build.
if(VW_ENABLE_COVERAGE AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
	message(FATAL_ERROR "VW_ENABLE_COVERAGE requires GCC or Clang.")
endif()

# AddressSanitizer + UndefinedBehaviorSanitizer. Applied per test target (the
# harness itself is tiny, so instrumenting it too costs nothing). With ASan a
# heap/stack overflow or use-after-free aborts the run; with UBSan any undefined
# behaviour does the same, and -fno-sanitize-recover=all makes even "recoverable"
# UBSan reports fatal so CI turns red instead of merely logging. -g keeps the
# frame info that makes the sanitizer reports point at a source line, and
# -fno-omit-frame-pointer keeps the stack traces readable.
if(VW_ENABLE_SANITIZERS AND NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
	message(FATAL_ERROR "VW_ENABLE_SANITIZERS requires GCC or Clang.")
endif()

function(vw_add_test name)
	add_executable(${name} ${ARGN})
	target_compile_features(${name} PRIVATE cxx_std_20)
	# Tests include the sources under "src" and the local "TestFramework.h".
	target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/src"
											   "${CMAKE_CURRENT_SOURCE_DIR}")
	if(VW_ENABLE_COVERAGE)
		target_compile_options(
			${name}
			PRIVATE -O0
					-g
					--coverage
					-fprofile-arcs
					-ftest-coverage)
		target_link_options(${name} PRIVATE --coverage)
	endif()
	if(VW_ENABLE_SANITIZERS)
		target_compile_options(
			${name} PRIVATE -g -fno-omit-frame-pointer -fsanitize=address,undefined
							-fno-sanitize-recover=all)
		target_link_options(${name} PRIVATE -fsanitize=address,undefined)
	endif()
	add_test(NAME ${name} COMMAND ${name})
endfunction()

# Header-only helpers.
vw_add_test(UpdaterParseTests UpdaterParseTests.cpp)

# The flows, compiled with the fake host. src/UpdaterFlow.cpp includes no SDK
# header, so it links straight into the test binary.
vw_add_test(UpdaterFlowTests UpdaterFlowTests.cpp "${CMAKE_SOURCE_DIR}/src/UpdaterFlow.cpp")

# Robustness / adversarial-input tests: feed the parsers unexpected external
# input (malformed script output, embedded NULs, huge/degenerate lines, random
# byte soup) to prove they never read out of bounds or invoke undefined
# behaviour. Most valuable under VW_ENABLE_SANITIZERS, where a mistake aborts;
# they also guard against a refactor quietly breaking the parsing contract.
vw_add_test(UpdaterRobustnessTests UpdaterRobustnessTests.cpp)

# Back-end tests for the bundled updater scripts. Both SOURCE (dot-source) their
# script and drive its q-stable / q-dev / do-install functions with the network
# boundary stubbed, so they run SDK- and network-free — the same idea as the C++
# tests, and cross-platform: PowerShell (pwsh) runs on the Linux CI runner too.
# Each is registered only when its interpreter is found, and each harness itself
# skips (exit 0) when a tool it needs is missing.

# macOS updater (scripts/vw-update.sh) via bash + stubbed curl/plutil.
if(NOT WIN32)
	find_program(BASH_PROGRAM bash)
	if(BASH_PROGRAM)
		add_test(NAME UpdaterScriptTests COMMAND "${BASH_PROGRAM}"
												 "${CMAKE_CURRENT_SOURCE_DIR}/vw-update.test.sh")
	endif()
endif()

# Windows updater (scripts/vw-update.ps1) via PowerShell 7 (pwsh) + stubbed
# Invoke-GH/Invoke-WebRequest. No Pester needed — the harness is self-contained.
find_program(PWSH_PROGRAM pwsh)
if(PWSH_PROGRAM)
	add_test(NAME UpdaterScriptTestsPs COMMAND "${PWSH_PROGRAM}" -NoProfile -File
											   "${CMAKE_CURRENT_SOURCE_DIR}/vw-update.Tests.ps1")
endif()
