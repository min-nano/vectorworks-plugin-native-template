//
//	ExtMenu.cpp
//
//	Implementation of the "Hello" menu command.
//

#include "PluginPrefix.h"
#include "BuildConfig.h"
#include "Extensions/ExtMenu.h"

using namespace HelloVW;

namespace HelloVW
{
	// Description of the menu command. The SResString entries ({resource,
	// identifier}) point at strings in the plug-in's .vwr resource file; a
	// resource file is optional for a build to succeed. EMenuEnableFlags{}
	// means "no special selection requirements to enable the command".
	// PLUGIN_VWR_ID differs between the stable and dev builds (see
	// BuildConfig.h) so each loads its own strings.
	static SMenuDef		gMenuDef = {
		/*Needs*/			EMenuEnableFlags{},
		/*NeedsNot*/		EMenuEnableFlags{},
		/*Title*/			{ PLUGIN_VWR_ID, "title" },
		/*Category*/		{ PLUGIN_VWR_ID, "category" },
		/*HelpText*/		{ PLUGIN_VWR_ID, "help" },
		/*VersionCreated*/	31,
		/*VersionModified*/	0,
		/*VersionRetired*/	0,
		/*OverrideHelpID*/	""
	};
}

// Every extension needs a globally unique ID and universal name. The stable and
// dev builds MUST use different ones so both plug-ins can be loaded at once.
#ifdef VW_DEV_BUILD
// UUID: cc72fd30-f2f6-4c39-8e7b-d81d5421898a  (dev build)
IMPLEMENT_VWMenuExtension(
	/*Extension class*/	CExtMenuHello,
	/*Event sink*/		CHelloMenu_EventSink,
	/*Universal name*/	PLUGIN_UNIVERSAL_NAME,
	/*Version*/			1,
	/*UUID*/			0xcc72fd30, 0xf2f6, 0x4c39, 0x8e, 0x7b, 0xd8, 0x1d, 0x54, 0x21, 0x89, 0x8a );
#else
// UUID: 4be7d497-0a1b-4c0e-aef9-aee94befc55e  (stable build)
IMPLEMENT_VWMenuExtension(
	/*Extension class*/	CExtMenuHello,
	/*Event sink*/		CHelloMenu_EventSink,
	/*Universal name*/	PLUGIN_UNIVERSAL_NAME,
	/*Version*/			1,
	/*UUID*/			0x4be7d497, 0x0a1b, 0x4c0e, 0xae, 0xf9, 0xae, 0xe9, 0x4b, 0xef, 0xc5, 0x5e );
#endif

// ---------------------------------------------------------------------------
CExtMenuHello::CExtMenuHello(CallBackPtr cbp)
	: VWExtensionMenu( cbp, gMenuDef )
{
}

CExtMenuHello::~CExtMenuHello()
{
}

// ---------------------------------------------------------------------------
CHelloMenu_EventSink::CHelloMenu_EventSink(IVWUnknown* parent)
	: VWMenu_EventSink( parent )
{
}

CHelloMenu_EventSink::~CHelloMenu_EventSink()
{
}

void CHelloMenu_EventSink::DoInterface()
{
	// This is the whole point of the plug-in for now: tell the user it ran,
	// and show exactly which build is loaded so a freshly-installed update can
	// be verified at a glance.
	gSDK->AlertInform(
		PLUGIN_DISPLAY_NAME " plug-in started",
		"channel: " VW_BUILD_CHANNEL "   build: " VW_BUILD_VERSION,
		true /* minor alert */ );
}
