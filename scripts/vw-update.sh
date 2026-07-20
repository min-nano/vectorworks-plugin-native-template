#!/usr/bin/env bash
#
# vw-update.sh — download the latest CI build of the SamplePlugin Vectorworks plug-in
# and install it into your Vectorworks 2026 Plug-Ins folder.
#
# Two channels, two separately-named plug-ins that can be installed at once:
#
#   stable  -> "SamplePlugin.vwlibrary"     from the rolling "stable" release (main).
#   dev     -> "SamplePluginDev.vwlibrary"  from a per-branch "dev-<branch>" prerelease;
#              you pick which branch's build to install.
#
# Flow: check the latest build, tell you whether a newer one is available, then
# let you choose: 更新しない / 更新だけ / 更新して再起動 (skip / update only /
# update and restart Vectorworks).
#
# Usage:
#   ./scripts/vw-update.sh            # ask which channel (or double-click)
#   ./scripts/vw-update.sh stable
#   ./scripts/vw-update.sh dev
#
# Requirements: macOS only. Uses tools that ship with macOS (curl, plutil,
# unzip, codesign, xattr, osascript) — no Homebrew, no `gh`, and because the
# repository is public, no authentication.
#
# Overridable via environment:
#   VW_REPO         owner/repo             (default below)
#   VW_PLUGINS_DIR  Vectorworks Plug-Ins   (default: user folder for VW 2026)
#   VW_APP_NAME     app to restart         (default: auto-detect / "Vectorworks 2026")
#
set -euo pipefail

VW_REPO="${VW_REPO:-min-nano/vectorworks-plugin-native-template}"
VW_PLUGINS_DIR="${VW_PLUGINS_DIR:-$HOME/Library/Application Support/Vectorworks/2026/Plug-Ins}"
VW_APP_NAME="${VW_APP_NAME:-}"
VW_API="https://api.github.com/repos/${VW_REPO}"

# ---------------------------------------------------------------------------
# Small AppleScript UI helpers. Values are passed as argv (never interpolated
# into the script text) so titles/messages can't break the AppleScript.
# ---------------------------------------------------------------------------
alert() { # title, message
	osascript - "$1" "$2" <<'APPLESCRIPT' >/dev/null 2>&1 || true
on run argv
	display dialog (item 2 of argv) with title (item 1 of argv) buttons {"OK"} default button "OK"
end run
APPLESCRIPT
}

notify() { # title, message
	osascript - "$1" "$2" <<'APPLESCRIPT' >/dev/null 2>&1 || true
on run argv
	display notification (item 2 of argv) with title (item 1 of argv)
end run
APPLESCRIPT
}

die() { # message
	echo "error: $1" >&2
	alert "SamplePlugin アップデート" "エラー: $1"
	exit 1
}

# ask3: show the three-way choice. Echoes the chosen button; cancel -> skip.
ask3() { # title, message
	osascript - "$1" "$2" <<'APPLESCRIPT' 2>/dev/null || echo "更新しない"
on run argv
	try
		set r to button returned of (display dialog (item 2 of argv) with title (item 1 of argv) buttons {"更新しない", "更新だけ", "更新して再起動"} default button "更新して再起動" cancel button "更新しない")
		return r
	on error number -128
		return "更新しない"
	end try
end run
APPLESCRIPT
}

# choose_one: prompt + list of items (as argv). Echoes the chosen item, or "".
choose_one() { # prompt, item1, item2, ...
	osascript - "$@" <<'APPLESCRIPT' 2>/dev/null || echo ""
on run argv
	set thePrompt to item 1 of argv
	set theItems to items 2 thru -1 of argv
	set chosen to choose from list theItems with prompt thePrompt without multiple selections allowed
	if chosen is false then return ""
	return item 1 of chosen
end run
APPLESCRIPT
}

# ---------------------------------------------------------------------------
# GitHub REST helpers (public repo -> unauthenticated). JSON is parsed with
# plutil, which ships with macOS and reads JSON natively.
# ---------------------------------------------------------------------------
api_get() { # api-subpath -> path to a temp file holding the JSON, or fail
	local f; f="$(mktemp)"
	if curl -fsSL -H "Accept: application/vnd.github+json" "${VW_API}/$1" -o "$f"; then
		printf '%s' "$f"
	else
		rm -f "$f"; return 1
	fi
}

jval() { # json-file, keypath -> raw scalar value (empty if missing)
	plutil -extract "$2" raw -o - "$1" 2>/dev/null || true
}

# asset_url: find the browser_download_url of an asset by name.
#   file   the JSON file
#   prefix keypath of the assets array ("assets" for a single release object,
#          "<i>.assets" for element i of a releases array)
#   want   the asset file name to match
asset_url() { # file, prefix, want
	local f="$1" pfx="$2" want="$3" j=0 nm
	while [ "$j" -lt 30 ]; do
		nm="$(jval "$f" "${pfx}.${j}.name")"
		[ -n "$nm" ] || break
		if [ "$nm" = "$want" ]; then
			jval "$f" "${pfx}.${j}.browser_download_url"
			return 0
		fi
		j=$((j + 1))
	done
	return 1
}

download() { # url, out-file
	curl -fL --retry 3 "$1" -o "$2"
}

# ---------------------------------------------------------------------------
# Plug-in / Vectorworks helpers.
# ---------------------------------------------------------------------------
installed_commit() { # bundle-path -> stamped VWBuildCommit or "none"
	local plist="$1/Contents/Info.plist"
	if [ -f "$plist" ]; then
		/usr/libexec/PlistBuddy -c "Print :VWBuildCommit" "$plist" 2>/dev/null || echo "none"
	else
		echo "none"
	fi
}

# install_zip: unzip a "<name>.vwlibrary.zip", de-quarantine, ad-hoc re-sign and
# atomically swap it into the Plug-Ins folder.
install_zip() { # zip, name
	local zip="$1" name="$2"
	local work; work="$(mktemp -d)"
	unzip -q "$zip" -d "$work"
	local src="$work/$name.vwlibrary"
	[ -d "$src" ] || { rm -rf "$work"; die "$name.vwlibrary が zip 内に見つかりません。"; }

	# Gatekeeper: clear the download quarantine flag, then re-apply an ad-hoc
	# signature so Apple Silicon will load it even after unzip.
	xattr -dr com.apple.quarantine "$src" 2>/dev/null || true
	codesign --force --deep --sign - "$src" >/dev/null 2>&1 || true

	mkdir -p "$VW_PLUGINS_DIR"
	local dst="$VW_PLUGINS_DIR/$name.vwlibrary"
	rm -rf "$dst.new"
	cp -R "$src" "$dst.new"
	rm -rf "$dst"
	mv "$dst.new" "$dst"
	rm -rf "$work"
	echo "installed: $dst"
}

find_vw_app() { # echo the name of a running Vectorworks app, or ""
	osascript -e 'tell application "System Events" to get name of (first application process whose name starts with "Vectorworks")' 2>/dev/null || echo ""
}

restart_vw() { # app-name (may be empty)
	local app="$1"
	[ -n "$app" ] || app="${VW_APP_NAME:-Vectorworks 2026}"
	osascript -e "tell application \"$app\" to quit" >/dev/null 2>&1 || true
	local i=0
	while [ "$i" -lt 40 ]; do
		pgrep -x "$app" >/dev/null 2>&1 || break
		sleep 1
		i=$((i + 1))
	done
	open -a "$app" >/dev/null 2>&1 || die "Vectorworks ($app) を再起動できませんでした。手動で起動してください。"
}

# apply_choice: run the chosen action (skip / update only / update+restart).
apply_choice() { # choice, zip, name
	local choice="$1" zip="$2" name="$3"
	case "$choice" in
		"更新だけ")
			install_zip "$zip" "$name"
			notify "SamplePlugin アップデート" "更新しました。反映するには Vectorworks を再起動してください。"
			;;
		"更新して再起動")
			local app; app="$(find_vw_app)"
			install_zip "$zip" "$name"
			notify "SamplePlugin アップデート" "更新しました。Vectorworks を再起動します…"
			restart_vw "$app"
			;;
		*)
			echo "skipped."
			;;
	esac
}

# ---------------------------------------------------------------------------
# Channel flows.
# ---------------------------------------------------------------------------
update_stable() {
	local f; f="$(api_get "releases/tags/stable")" \
		|| die "安定版リリース (stable) が見つかりません。main のビルドが完了しているか確認してください。"
	local latest_full; latest_full="$(jval "$f" target_commitish)"
	local url; url="$(asset_url "$f" "assets" "SamplePlugin.vwlibrary.zip" || true)"
	rm -f "$f"
	[ -n "$latest_full" ] || die "安定版リリースの情報を取得できませんでした。"
	[ -n "$url" ] || die "安定版のアセット (SamplePlugin.vwlibrary.zip) が見つかりません。"

	local latest="${latest_full:0:7}"
	local installed; installed="$(installed_commit "$VW_PLUGINS_DIR/SamplePlugin.vwlibrary")"

	if [ "$installed" = "$latest" ]; then
		alert "SamplePlugin (stable)" "既に最新です（build ${installed}）。"
		return
	fi

	local choice; choice="$(ask3 "SamplePlugin (stable)" "新しい安定版ビルドがあります。
インストール済み: ${installed}
最新: ${latest}

どうしますか？")"
	[ "$choice" != "更新しない" ] || { echo "skipped."; return; }

	local tmp; tmp="$(mktemp -d)"
	download "$url" "$tmp/SamplePlugin.vwlibrary.zip" || die "安定版アセットのダウンロードに失敗しました。"
	apply_choice "$choice" "$tmp/SamplePlugin.vwlibrary.zip" "SamplePlugin"
	rm -rf "$tmp"
}

update_dev() {
	local f; f="$(api_get "releases?per_page=100")" \
		|| die "リリース一覧を取得できませんでした。"

	# Collect the per-branch dev prereleases.
	local names=() tags=() commits=() urls=()
	local i=0 tag name commit url
	while [ "$i" -lt 100 ]; do
		tag="$(jval "$f" "${i}.tag_name")"
		[ -n "$tag" ] || break
		case "$tag" in
			dev-*)
				name="$(jval "$f" "${i}.name")"
				commit="$(jval "$f" "${i}.target_commitish")"
				url="$(asset_url "$f" "${i}.assets" "SamplePluginDev.vwlibrary.zip" || true)"
				[ -n "$name" ] || name="$tag"
				names+=("$name"); tags+=("$tag"); commits+=("$commit"); urls+=("$url")
				;;
		esac
		i=$((i + 1))
	done
	rm -f "$f"

	[ "${#tags[@]}" -gt 0 ] || die "開発版ビルド (dev-*) がまだありません。対象ブランチを push してビルドを走らせてください。"

	local chosen_name; chosen_name="$(choose_one "確認したい開発版ビルド（ブランチ）を選んでください:" "${names[@]}")"
	[ -n "$chosen_name" ] || { echo "cancelled."; return; }

	# Resolve the chosen entry.
	local idx=-1
	for i in "${!names[@]}"; do
		if [ "${names[$i]}" = "$chosen_name" ]; then idx="$i"; break; fi
	done
	[ "$idx" -ge 0 ] || die "選択したビルドを特定できませんでした。"

	local url2="${urls[$idx]}" latest="${commits[$idx]:0:7}"
	[ -n "$url2" ] || die "選択したビルドのアセット (SamplePluginDev.vwlibrary.zip) が見つかりません。"
	local installed; installed="$(installed_commit "$VW_PLUGINS_DIR/SamplePluginDev.vwlibrary")"

	local same_note=""
	[ "$installed" = "$latest" ] && same_note="（このビルドは既にインストール済みです）
"

	local choice; choice="$(ask3 "SamplePlugin (dev)" "${chosen_name}
${same_note}インストール済み: ${installed}
選択したビルド: ${latest}

どうしますか？")"
	[ "$choice" != "更新しない" ] || { echo "skipped."; return; }

	local tmp; tmp="$(mktemp -d)"
	download "$url2" "$tmp/SamplePluginDev.vwlibrary.zip" || die "開発版アセットのダウンロードに失敗しました。"
	apply_choice "$choice" "$tmp/SamplePluginDev.vwlibrary.zip" "SamplePluginDev"
	rm -rf "$tmp"
}

# ---------------------------------------------------------------------------
main() {
	command -v curl >/dev/null 2>&1 || die "curl が見つかりません（macOS で実行してください）。"

	local channel="${1:-}"
	if [ -z "$channel" ]; then
		channel="$(choose_one "どのビルドを確認しますか？" "stable（安定版 / main）" "dev（開発版 / ブランチ選択）")"
		case "$channel" in
			stable*) channel="stable" ;;
			dev*)    channel="dev" ;;
			*)       echo "cancelled."; exit 0 ;;
		esac
	fi

	case "$channel" in
		stable) update_stable ;;
		dev)    update_dev ;;
		*)      die "不明なチャンネル: '$channel'（stable か dev を指定してください）。" ;;
	esac
}

main "$@"
