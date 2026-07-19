# vectorworks-plugin-import-ifc-homeskz

Vectorworks 2026 plug-in developed with the C++ SDK.

Right now this repository contains a **minimal, working plug-in** used to get the
build environment and CI in place. The plug-in adds a single menu command that,
when run, shows an alert dialog announcing that it started. Real functionality
(IFC import) will be built on top of this foundation later.

## Layout

```
CMakeLists.txt              CMake build for the macOS plug-in bundle
src/
  ModuleMain.cpp            Module entry point; registers the extension
  Extensions/ExtMenu.{h,cpp}  The menu command that shows the alert
  PluginPrefix.h            Shared prefix header (pulls in the SDK)
  Module-Info.plist         Bundle Info.plist
.github/workflows/build.yml CI: builds on an Apple Silicon macOS runner
```

The build output is `HelloVW.vwlibrary`, a loadable macOS bundle.

## Building locally

You need macOS with Xcode (Vectorworks 2026 officially targets **Xcode 16.2**)
and the **Vectorworks 2026 mac SDK**.

1. Download and unzip the SDK:
   <https://release.vectorworks.net/latest/Vectorworks/2026-NNA-eng-mac-SDK.zip>
   (~800 MB). After unzipping you get a folder containing `SDKLib/`.

2. Configure and build, pointing `VW_SDK_DIR` at the folder that contains
   `SDKLib`:

   ```sh
   cmake -S . -B build -DVW_SDK_DIR=/path/to/2026-NNA-eng-mac-SDK
   cmake --build build
   ```

   The result is `build/HelloVW.vwlibrary`.

By default the build targets Apple Silicon (`arm64`). For a universal binary:

```sh
cmake -S . -B build -DVW_SDK_DIR=/path/to/sdk \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

## Installing the plug-in

Copy `HelloVW.vwlibrary` into your Vectorworks user folder's `Plug-Ins`
directory, then add the command to a workspace (Tools ▸ Workspaces) so it shows
up in a menu. Note that Vectorworks 2026 requires SDK plug-ins to carry a
developer credentials (satellite) file to load in a shipping build; that is a
distribution concern and is not needed to compile.

## Continuous integration

`.github/workflows/build.yml` builds the plug-in on every push:

- Runs on `macos-15` (Apple Silicon) and selects Xcode 16.2.
- Downloads the SDK once and **caches** the (trimmed) SDK so the ~800 MB zip is
  not re-downloaded on later runs. Bump `VW_SDK_CACHE_KEY` in the workflow to
  force a fresh download.
- Builds `HelloVW.vwlibrary`, checks its architecture, and uploads it as a
  build artifact.
