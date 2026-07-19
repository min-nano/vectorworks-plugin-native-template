//
//	ModuleMain.cpp
//
//	Main entry point for the Vectorworks plug-in module. Vectorworks loads the
//	built .vwlibrary and calls plugin_module_main to register the extensions it
//	provides.
//

#include "PluginPrefix.h"
#include "BuildConfig.h"
#include "Extensions/ExtMenu.h"

// Identifier used by Vectorworks to locate this plug-in's resources (.vwr) at
// run time. Must match the base name of the packaged .vwr ("HelloVW.vwr" for
// the stable build, "HelloVWDev.vwr" for the dev build). See BuildConfig.h.
const char* DefaultPluginVWRIdentifier() { return PLUGIN_VWR_ID; }

//------------------------------------------------------------------
// Report the SDK version this plug-in was compiled against so Vectorworks can
// decide whether it is safe to load.
extern "C" Sint32 GS_EXTERNAL_ENTRY plugin_module_ver() { return SDK_VERSION; }

//------------------------------------------------------------------
// Module entry point.
// More info: http://developer.vectorworks.net/index.php?title=SDK:Module_Plug-in
//
extern "C" Sint32 GS_EXTERNAL_ENTRY plugin_module_main(
	Sint32			action,
	void*			moduleInfo,
	const VWIID&	iid,
	IVWUnknown*&	inOutInterface,
	CallBackPtr		cbp )
{
	// Initialize the VCOM (Vectorworks Component Object Model) mechanism.
	::GS_InitializeVCOM( cbp );

	Sint32	reply	= 0L;

	using namespace VWFC::PluginSupport;

	// Register our single menu command extension.
	REGISTER_Extension<HelloVW::CExtMenuHello>(
		GROUPID_ExtensionMenu, action, moduleInfo, iid, inOutInterface, cbp, reply );

	return reply;
}
