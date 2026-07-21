//
//	Updater.h
//
//	Drives the plug-in's self-update using NATIVE Vectorworks dialogs
//	(gSDK->AlertInform / gSDK->AlertQuestion). The network + install mechanics
//	are delegated to the vw-update.sh script bundled inside the .vwlibrary (it
//	runs non-interactively and prints machine-readable output; see its q-stable
//	/ q-dev / do-install modes), while every user-facing dialog is shown by the
//	plug-in itself. So nobody has to open a terminal.
//
//	  * The STABLE plug-in checks for a newer build at Vectorworks start-up:
//	    RunStableStartupCheck() — silent when already current, otherwise asks
//	    (native yes/no) whether to install.
//	  * The DEV plug-in lets the user pick which branch's build to use each time
//	    its command runs: RunDevBranchPicker() — installs the chosen build only
//	    if it differs from the one already installed.
//

#pragma once

namespace SamplePlugin
{
	// Stable plug-in only. At Vectorworks start-up, compare the installed stable
	// build with the latest published one; if a newer one exists, ask (native
	// dialog) whether to install it. Silent when already current or on a network
	// error. Runs only once per session.
	void	RunStableStartupCheck();

	// Dev plug-in only. Ask (native dialogs) which build to use: the currently
	// installed one, or another branch's prerelease. Returns true if the plug-in
	// should now run its normal command (the user kept the installed build);
	// returns false if it should stop instead — either a different build was just
	// installed (you will want to restart to load it) or the user cancelled.
	bool	RunDevBranchPicker();
}
