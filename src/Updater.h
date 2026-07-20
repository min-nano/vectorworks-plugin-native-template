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

	// Dev plug-in only. Ask (native dialogs) which branch's dev build to use and
	// install it, but only if it differs from the one already installed;
	// otherwise nothing is installed and the plug-in just runs as loaded.
	void	RunDevBranchPicker();
}
