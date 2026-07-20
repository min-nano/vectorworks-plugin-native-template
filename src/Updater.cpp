//
//	Updater.cpp
//
//	Locates the vw-update.sh script bundled inside this plug-in's .vwlibrary and
//	launches it to perform the actual update work (GitHub API, download, install,
//	and all user dialogs). See Updater.h for the contract.
//

#include "PluginPrefix.h"
#include "Updater.h"

#include <dlfcn.h>
#include <cstdlib>
#include <string>

namespace
{
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

		// Keep everything up to and including "/Contents/".
		std::string contents = path.substr(0, at + std::string("/Contents/").size());
		return contents + "Resources/vw-update.sh";
	}

	// Wrap a string in single quotes so it is safe as one /bin/sh word, escaping
	// any embedded single quotes (the bundle path can contain spaces).
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

	// Run "vw-update.sh <mode>". background=true detaches it (start-up check),
	// background=false blocks until it finishes (dev picker).
	void RunUpdater(const char* mode, bool background)
	{
		std::string script = BundledScriptPath();
		if (script.empty())
			return;

		std::string cmd = "/bin/bash " + ShellQuote(script) + " " + mode
			+ " >/dev/null 2>&1";
		if (background)
			cmd += " &";		// system() returns immediately; the job is reparented.

		std::system(cmd.c_str());
	}
}

namespace SamplePlugin
{
	void LaunchStableStartupCheck()
	{
		// plugin_module_main can be called more than once per session; only the
		// first call should kick off the background check.
		static bool sLaunched = false;
		if (sLaunched)
			return;
		sLaunched = true;

		RunUpdater("startup-stable", /*background*/ true);
	}

	void RunDevBranchPicker()
	{
		RunUpdater("dev-pick", /*background*/ false);
	}
}
