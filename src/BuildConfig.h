//
//	BuildConfig.h
//
//	Central place for the plug-in's build-time identity. The exact same source
//	code is compiled into two coexisting plug-ins:
//
//	  * the STABLE plug-in ("HelloVW"),    built from the `main` branch, and
//	  * the DEV plug-in    ("HelloVWDev"), built from feature / PR branches.
//
//	They must have DIFFERENT identifiers (bundle name, .vwr identifier, VCOM
//	universal name and extension UUID) so Vectorworks can load BOTH at the same
//	time without them colliding. The DEV build is selected by defining
//	VW_DEV_BUILD (done per-target in CMakeLists.txt); everything else here is
//	derived from that single switch.
//

#pragma once

#ifdef VW_DEV_BUILD
	// Dev plug-in identity.
	#define PLUGIN_VWR_ID			"HelloVWDev"
	#define PLUGIN_UNIVERSAL_NAME	"CExtMenuHello_HelloVWDev"
	#define PLUGIN_DISPLAY_NAME		"HelloVW (Dev)"
#else
	// Stable plug-in identity.
	#define PLUGIN_VWR_ID			"HelloVW"
	#define PLUGIN_UNIVERSAL_NAME	"CExtMenuHello_HelloVW"
	#define PLUGIN_DISPLAY_NAME		"HelloVW"
#endif

// Build channel, as a human-readable string ("stable" / "dev").
#ifndef VW_BUILD_CHANNEL
	#ifdef VW_DEV_BUILD
		#define VW_BUILD_CHANNEL	"dev"
	#else
		#define VW_BUILD_CHANNEL	"stable"
	#endif
#endif

// Short identifier of the exact build (git commit, or "local" for a local
// build). CMake passes this in via -DVW_BUILD_VERSION=...; it is stamped into
// the bundle's Info.plist and shown in the menu command's alert so you can tell
// which build is actually loaded.
#ifndef VW_BUILD_VERSION
	#define VW_BUILD_VERSION		"local"
#endif
