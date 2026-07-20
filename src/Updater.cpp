//
//	Updater.cpp
//
//	Native-dialog front end for the plug-in's self-update. All user interaction
//	uses the Vectorworks SDK (gSDK->AlertInform / gSDK->AlertQuestion). The
//	actual work (GitHub API, download, install) is delegated to the bundled
//	vw-update.sh script, invoked non-interactively; see Updater.h for the
//	contract.
//

#include "PluginPrefix.h"
#include "BuildConfig.h"
#include "Updater.h"

#include <dlfcn.h>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
	// -----------------------------------------------------------------------
	// Bundled-script discovery + invocation.
	// -----------------------------------------------------------------------

	// Absolute path of the bundled updater script, or "" if it can't be resolved.
	//
	// The installed plug-in is just the .vwlibrary bundle, so the script travels
	// inside it (CMake copies it to Contents/Resources/vw-update.sh). We find our
	// own loaded binary with dladdr() — its path is
	//   <name>.vwlibrary/Contents/MacOS/<name>
	// — and rewrite the trailing "MacOS/<name>" to "Resources/vw-update.sh".
	std::string BundledScriptPath()
	{
		Dl_info info{};
		if (::dladdr(reinterpret_cast<const void*>(&BundledScriptPath), &info) == 0
			|| info.dli_fname == nullptr)
			return "";

		std::string path = info.dli_fname;					// .../Contents/MacOS/<name>
		const std::string marker = "/Contents/MacOS/";
		std::string::size_type at = path.rfind(marker);
		if (at == std::string::npos)
			return "";

		std::string contents = path.substr(0, at + std::string("/Contents/").size());
		return contents + "Resources/vw-update.sh";
	}

	// Directory that CONTAINS this plug-in's .vwlibrary bundle — i.e. the exact
	// Plug-Ins folder Vectorworks actually loaded this build from. Returns "" if
	// it can't be resolved.
	//
	// This is what makes the updater install to the RIGHT place: the plug-in may
	// live in a custom Vectorworks user folder (Vectorworks ▸ 環境設定 ▸ ユーザ
	// フォルダ), not the default path. Installing next to the running bundle
	// guarantees the update replaces the copy that is actually loaded, so the new
	// build is picked up on the next restart. From
	//   .../<PlugIns>/<name>.vwlibrary/Contents/MacOS/<name>
	// we strip back to "<PlugIns>".
	std::string BundlePluginsDir()
	{
		Dl_info info{};
		if (::dladdr(reinterpret_cast<const void*>(&BundlePluginsDir), &info) == 0
			|| info.dli_fname == nullptr)
			return "";

		std::string path = info.dli_fname;					// .../<PlugIns>/<name>.vwlibrary/Contents/MacOS/<name>
		std::string::size_type at = path.rfind("/Contents/MacOS/");
		if (at == std::string::npos)
			return "";

		std::string bundle = path.substr(0, at);			// .../<PlugIns>/<name>.vwlibrary
		std::string::size_type slash = bundle.rfind('/');
		if (slash == std::string::npos)
			return "";
		return bundle.substr(0, slash);						// .../<PlugIns>
	}

	// Wrap a string in single quotes so it is safe as one /bin/sh word, escaping
	// any embedded single quotes (bundle paths and URLs can contain surprises).
	std::string ShellQuote(const std::string& s)
	{
		std::string out = "'";
		for (char c : s)
		{
			if (c == '\'')	out += "'\\''";
			else			out += c;
		}
		out += "'";
		return out;
	}

	// Run "vw-update.sh <args>" and capture its stdout into out. Blocks until the
	// script finishes. Returns false if the script could not be located/started.
	bool RunScript(const std::string& args, std::string& out)
	{
		std::string script = BundledScriptPath();
		if (script.empty())
			return false;

		// Point the script at the folder this build was actually loaded from, so
		// it reads the installed commit from — and installs over — the copy
		// Vectorworks really uses (not a guessed default path).
		std::string env;
		std::string pluginsDir = BundlePluginsDir();
		if (!pluginsDir.empty())
			env = "VW_PLUGINS_DIR=" + ShellQuote(pluginsDir) + " ";

		std::string cmd = env + "/bin/bash " + ShellQuote(script) + " " + args + " 2>/dev/null";
		FILE* pipe = ::popen(cmd.c_str(), "r");
		if (pipe == nullptr)
			return false;

		out.clear();
		char buf[4096];
		size_t n = 0;
		while ((n = ::fread(buf, 1, sizeof(buf), pipe)) > 0)
			out.append(buf, n);
		::pclose(pipe);
		return true;
	}

	// -----------------------------------------------------------------------
	// Tiny parsing helpers for the script's key=value / TSV output.
	// -----------------------------------------------------------------------

	std::string Trim(const std::string& s)
	{
		std::string::size_type b = s.find_first_not_of(" \t\r\n");
		if (b == std::string::npos) return "";
		std::string::size_type e = s.find_last_not_of(" \t\r\n");
		return s.substr(b, e - b + 1);
	}

	// Value of the first "key=value" line whose key matches (key without '='),
	// or "" if absent.
	std::string ValueOf(const std::string& out, const std::string& key)
	{
		std::string needle = key + "=";
		std::string::size_type pos = 0;
		while (pos < out.size())
		{
			std::string::size_type eol = out.find('\n', pos);
			std::string line = out.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
			if (line.compare(0, needle.size(), needle) == 0)
				return Trim(line.substr(needle.size()));
			if (eol == std::string::npos) break;
			pos = eol + 1;
		}
		return "";
	}

	struct DevBuild
	{
		std::string	commit;
		std::string	name;
		std::string	url;
	};

	// Parse the "build<TAB>commit<TAB>name<TAB>url" lines from q-dev output.
	std::vector<DevBuild> ParseDevBuilds(const std::string& out)
	{
		std::vector<DevBuild> builds;
		std::string::size_type pos = 0;
		while (pos < out.size())
		{
			std::string::size_type eol = out.find('\n', pos);
			std::string line = out.substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
			pos = (eol == std::string::npos) ? out.size() : eol + 1;

			if (line.compare(0, 6, "build\t") != 0)
				continue;

			// Split the three tab-separated fields after "build".
			std::string rest = line.substr(6);
			std::string::size_type t1 = rest.find('\t');
			if (t1 == std::string::npos) continue;
			std::string::size_type t2 = rest.find('\t', t1 + 1);
			if (t2 == std::string::npos) continue;

			DevBuild b;
			b.commit = Trim(rest.substr(0, t1));
			b.name   = Trim(rest.substr(t1 + 1, t2 - (t1 + 1)));
			b.url    = Trim(rest.substr(t2 + 1));
			if (!b.url.empty())
				builds.push_back(b);
		}
		return builds;
	}

	// -----------------------------------------------------------------------
	// Native dialog wrappers.
	// -----------------------------------------------------------------------

	// Note: the SDK's TXString constructs implicitly from a (UTF-8) const char*,
	// which is how the existing menu-command AlertInform call passes its text, so
	// we pass std::string::c_str() directly and let that conversion happen.

	void Inform(const std::string& text, const std::string& advice)
	{
		// false => modal dialog (not a minor/status-bar alert), so the advice
		// line is shown too. Matches the existing menu-command alert.
		gSDK->AlertInform(text.c_str(), advice.c_str(), false);
	}

	// Yes/no question. Returns true if the user chose the affirmative button.
	bool Ask(const std::string& text, const std::string& advice,
			 const std::string& okText, const std::string& cancelText)
	{
		// AlertQuestion returns 0 = negative/cancel, 1 = positive/OK, 2/3 = custom
		// buttons A/B. defaultButton 1 = the OK button is the default.
		short r = gSDK->AlertQuestion(
			text.c_str(), advice.c_str(),
			/*defaultButton*/ 1,
			okText.c_str(), cancelText.c_str(),
			/*customButtonA*/ "", /*customButtonB*/ "");
		return r == 1;
	}

	// Run the bundled installer for one bundle. Returns true on success; fills
	// errorOut with the script's message on failure.
	bool Install(const std::string& url, const std::string& name, std::string& errorOut)
	{
		std::string out;
		if (!RunScript("do-install " + ShellQuote(url) + " " + ShellQuote(name), out))
		{
			errorOut = "アップデータを起動できませんでした。";
			return false;
		}
		if (Trim(out) == "ok")
			return true;

		errorOut = ValueOf(out, "error");
		if (errorOut.empty())
			errorOut = "インストールに失敗しました。";
		return false;
	}
}

namespace SamplePlugin
{
	void RunStableStartupCheck()
	{
		// plugin_module_main can be called more than once per session; only do
		// the check the first time.
		static bool sDone = false;
		if (sDone)
			return;
		sDone = true;

		std::string out;
		if (!RunScript("q-stable", out))
			return;								// script missing -> stay silent
		if (!ValueOf(out, "error").empty())
			return;								// offline/transient -> stay silent

		std::string installed = ValueOf(out, "installed");
		std::string latest    = ValueOf(out, "latest");
		std::string url       = ValueOf(out, "url");
		if (latest.empty() || url.empty())
			return;
		if (installed == latest)
			return;								// already current -> no dialog

		std::string shownInstalled = installed.empty() ? "none" : installed;
		if (!Ask("新しい安定版ビルドがあります。今すぐインストールしますか？",
				 "インストール済み: " + shownInstalled + "\n最新: " + latest,
				 "インストール", "後で"))
			return;

		std::string err;
		if (Install(url, "SamplePlugin", err))
			Inform("SamplePlugin を更新しました。",
				   "反映するには Vectorworks を再起動してください。");
		else
			Inform("更新に失敗しました。", err);
	}

	// Let the user pick which prerelease build to switch to. Pages through the
	// candidates (already filtered to exclude the running build). Installs the
	// chosen one. Returns false in every case (a switch always ends the command
	// so the user can restart to load it — or they cancelled).
	bool ChooseAndInstallOther(const std::vector<DevBuild>& others)
	{
		const int count = static_cast<int>(others.size());
		for (int i = 0; i < count; ++i)
		{
			const bool last = (i + 1 == count);
			std::string advice = "commit: " + others[i].commit
				+ "  (" + std::to_string(i + 1) + "/" + std::to_string(count) + ")";
			short r = gSDK->AlertQuestion(
				("インストールするビルドを選択:\n" + others[i].name).c_str(),
				advice.c_str(),
				/*defaultButton*/ 1,
				/*OK*/     "これをインストール",
				/*Cancel*/ "キャンセル",
				/*A*/      last ? "" : "次の候補",
				/*B*/      "");

			if (r == 0) return false;				// キャンセル
			if (r == 1)								// これをインストール
			{
				std::string err;
				if (Install(others[i].url, "SamplePluginDev", err))
					Inform("開発版ビルドをインストールしました。",
						   "反映するには Vectorworks を再起動してください。\n"
						   "branch: " + others[i].name + "\ncommit: " + others[i].commit);
				else
					Inform("インストールに失敗しました。", err);
				return false;						// installed -> end the command (restart to load)
			}
			// r == 2 -> 次の候補 -> continue
		}
		return false;
	}

	bool RunDevBranchPicker()
	{
		std::string out;
		if (!RunScript("q-dev", out) || !ValueOf(out, "error").empty())
		{
			// Offline / transient: don't block the user — just run the current
			// build (the command's original behaviour).
			return true;
		}

		std::vector<DevBuild> all = ParseDevBuilds(out);

		// The build that is actually loaded and running right now. Compiled in,
		// so it is unambiguous even if a different build is staged on disk.
		const std::string runningBranch = VW_BUILD_BRANCH;
		const std::string runningCommit = VW_BUILD_VERSION;

		// Candidates to switch TO: every prerelease except the running build.
		std::vector<DevBuild> others;
		for (const DevBuild& b : all)
			if (b.commit != runningCommit)
				others.push_back(b);

		// Entry dialog: show the running (installed) build; offer to run it, or
		// switch to another branch when other prereleases exist.
		const bool hasOthers = !others.empty();
		std::string advice = "現在: " + runningBranch + " (" + runningCommit + ")  ← インストール済み";
		advice += hasOthers
			? "\n\n他のブランチのビルドに切り替えられます。"
			: "\n\n切り替え可能な他のブランチのビルドはありません。";

		short r = gSDK->AlertQuestion(
			"使用する開発版ビルドを選択",
			advice.c_str(),
			/*defaultButton*/ 1,
			/*OK*/     "現在のビルドを実行",
			/*Cancel*/ "キャンセル",
			/*A*/      hasOthers ? "別のブランチ…" : "",
			/*B*/      "");

		if (r == 1) return true;					// 現在のビルドを実行 -> run
		if (r == 2) return ChooseAndInstallOther(others);	// 別のブランチ… -> switch
		return false;								// キャンセル -> do nothing
	}
}
