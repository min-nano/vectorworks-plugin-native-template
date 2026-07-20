# vectorworks-plugin-import-ifc-homeskz

Vectorworks 2026 plug-in developed with the C++ SDK.

Right now this repository contains a **minimal, working plug-in** used to get the
build environment and CI in place. The plug-in adds a single menu command that,
when run, shows an alert dialog announcing that it started. Real functionality
(IFC import) will be built on top of this foundation later.

## Layout

```
CMakeLists.txt              CMake build for the macOS plug-in bundles
src/
  ModuleMain.cpp            Module entry point; registers the extension
  Extensions/ExtMenu.{h,cpp}  The menu command that shows the alert
  BuildConfig.h             Stable vs. dev identity switch (VW_DEV_BUILD)
  PluginPrefix.h            Shared prefix header (pulls in the SDK)
  Module-Info.plist.in      Bundle Info.plist template (filled in per build)
resources/
  HelloVW.vwr/…             Menu strings for the stable plug-in
  HelloVWDev.vwr/…          Menu strings for the dev plug-in
scripts/
  vw-update.sh              Download the latest CI build and install it
.github/workflows/build.yml CI: builds on an Apple Silicon macOS runner
```

The same source builds **two coexisting plug-ins** from a single switch
(`VW_DEV_BUILD`, see `src/BuildConfig.h`):

- **`HelloVW.vwlibrary`** — the *stable* plug-in, built from `main`. Menu
  category **HomesKZ**.
- **`HelloVWDev.vwlibrary`** — the *dev* plug-in, built from feature/PR
  branches. Menu category **HomesKZ (Dev)**.

They have distinct bundle names, `.vwr` identifiers, VCOM universal names and
extension UUIDs, so both can be installed and loaded at the same time — the
stable build for normal use, the dev build for trying out an in-progress branch.
Each bundle's menu command shows its channel and build commit in the alert, and
that commit is also stamped into the bundle's `Info.plist`
(`VWBuildChannel` / `VWBuildCommit`) so the updater can tell what's installed.

Each menu command's display text comes from its `resources/<name>.vwr` folder,
which the SDK's `BuildVWR` tool packages into
`<name>.vwlibrary/Contents/Resources/<name>.vwr` during the build, so each
bundle is self-contained.

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

`.github/workflows/build.yml` builds the plug-ins on every push:

- Runs on `macos-15` (Apple Silicon) and selects Xcode 16.2.
- Downloads the SDK once and **caches** the (trimmed) SDK so the ~800 MB zip is
  not re-downloaded on later runs. Bump `VW_SDK_CACHE_KEY` in the workflow to
  force a fresh download.
- Builds **both** `HelloVW.vwlibrary` and `HelloVWDev.vwlibrary`, stamps them
  with the commit (`-DVW_BUILD_VERSION`), ad-hoc-signs them, checks their
  architecture, and uploads them as build artifacts.
- **Publishes a downloadable release** so the updater has a stable URL to fetch:
  - a push to **`main`** refreshes the rolling **`stable`** release with
    `HelloVW.vwlibrary.zip`;
  - a push to **any other branch** refreshes a per-branch **`dev-<branch>`**
    prerelease with `HelloVWDev.vwlibrary.zip`.

  Both are rolling — the tag is re-pointed at the newest build each time.

`.github/workflows/cleanup-dev-release.yml` deletes a branch's `dev-<branch>`
prerelease (and its tag) when that branch is deleted, so dev builds don't pile
up. Because it is triggered by the `delete` event, it runs from the copy on the
default branch and therefore only cleans up branches deleted after it lands on
`main`.

## Auto-update (自動アップデート)

`scripts/vw-update.sh` downloads the latest CI build and installs it into your
Vectorworks 2026 `Plug-Ins` folder, so you can verify a new build without
manually downloading, de-quarantining and copying it.

It checks the latest build, tells you whether a newer one is available, then
lets you choose **更新しない / 更新だけ / 更新して再起動** (skip / update only /
update and restart Vectorworks). It de-quarantines and ad-hoc re-signs the
bundle for you, and "更新して再起動" quits and relaunches Vectorworks so the new
build actually loads (compiled plug-ins are only read at startup).

The repository is public, so no authentication or extra tooling is needed — the
script uses only what ships with macOS (`curl`, `plutil`, `unzip`, `codesign`,
`xattr`, `osascript`).

```sh
# Stable channel (main → HelloVW):
./scripts/vw-update.sh stable

# Dev channel — pick which branch's build to install (→ HelloVWDev):
./scripts/vw-update.sh dev

# No argument (or double-click in Finder): asks which channel first.
./scripts/vw-update.sh
```

Overridable via environment: `VW_REPO` (owner/repo), `VW_PLUGINS_DIR` (install
location), `VW_APP_NAME` (app to restart). The two channels install
separately-named bundles, so the stable and dev plug-ins never overwrite each
other.
