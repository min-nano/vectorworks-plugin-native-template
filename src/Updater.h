//
//	Updater.h
//
//	Lets the plug-in drive its own updates by invoking the bundled vw-update.sh
//	script (packaged into the .vwlibrary at build time), so the user never has to
//	open a terminal. The script shows all of its own (macOS) dialogs.
//
//	  * The STABLE plug-in checks for a newer build at Vectorworks start-up:
//	    LaunchStableStartupCheck() (non-blocking; silent when already current).
//	  * The DEV plug-in lets the user pick which branch's build to use each time
//	    its command runs: RunDevBranchPicker() (blocks until the user is done).
//

#pragma once

namespace SamplePlugin
{
	// Stable plug-in only. Launch the bundled updater in the BACKGROUND to check
	// for a newer stable build and, if one exists, prompt to install it. Returns
	// immediately so Vectorworks start-up is never blocked. Safe to call more
	// than once per session — only the first call does anything.
	void	LaunchStableStartupCheck();

	// Dev plug-in only. Run the bundled updater SYNCHRONOUSLY so the user can
	// pick which branch's dev build to use; it installs the build only if it
	// differs from the one already installed. Blocks until the picker is done.
	void	RunDevBranchPicker();
}
