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
resources/
  HelloVW.vwr/Strings/HelloVW.vwstrings  Menu title/category/help text
.github/workflows/build.yml CI: builds on an Apple Silicon macOS runner
```

The build output is `HelloVW.vwlibrary`, a loadable macOS bundle. The menu
command's display text comes from `resources/HelloVW.vwr`, which the SDK's
`BuildVWR` tool packages into `HelloVW.vwlibrary/Contents/Resources/HelloVW.vwr`
during the build, so the bundle is self-contained.

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

## Installing and running (internal / unsigned use)

This plug-in is for internal use and is shipped **unsigned** (no Apple Developer
ID signing and no Vectorworks developer credentials). To install and run it:

1. **Put the bundle on a local disk** (not iCloud Drive — iCloud can re-apply
   the download quarantine flag) and into your Vectorworks 2026 user folder's
   `Plug-Ins` directory (find it via Vectorworks ▸ Preferences ▸ *User Folders*).
   The `.vwr` resource is already inside the bundle, so just the
   `HelloVW.vwlibrary` folder is needed.

2. **Clear the macOS quarantine flag** so Gatekeeper doesn't block the
   downloaded bundle:

   ```sh
   xattr -dr com.apple.quarantine HelloVW.vwlibrary
   ```

   CI builds are already **ad-hoc code-signed** (required so Apple Silicon will
   load the binary at all — this is free and is not Developer ID signing). If
   you built locally the linker ad-hoc-signs automatically. If macOS still says
   the bundle is "damaged", re-sign it yourself:

   ```sh
   codesign --force --deep --sign - HelloVW.vwlibrary
   ```

3. **Launch Vectorworks.** Because the plug-in is unsigned, Vectorworks 2026
   shows an "unknown/unsigned plug-in" warning at startup and may disable it by
   default. Acknowledge the warning and enable the plug-in — this is expected
   for internal, uncredentialed plug-ins and is fine for in-house use.

4. **Add the command to your workspace:** Tools ▸ Workspaces ▸ Edit Current
   Workspace ▸ *Menus*. Under the **HomesKZ** category you'll find the
   **起動確認** command; drag it into a menu. Running it shows the alert.

Getting Vectorworks developer credentials (the 2026 "satellite" file) is only
needed to ship a *signed* plug-in with no warning; it is not required to build
or to run internally.

## Continuous integration

`.github/workflows/build.yml` builds the plug-in on every push:

- Runs on `macos-15` (Apple Silicon) and selects Xcode 16.2.
- Downloads the SDK once and **caches** the (trimmed) SDK so the ~800 MB zip is
  not re-downloaded on later runs. Bump `VW_SDK_CACHE_KEY` in the workflow to
  force a fresh download.
- Builds `HelloVW.vwlibrary`, checks its architecture, and uploads it as a
  build artifact.
