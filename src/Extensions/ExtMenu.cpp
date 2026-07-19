//
//	ExtMenu.cpp
//
//	Implementation of the "Hello" menu command.
//

#include "PluginPrefix.h"
#include "Extensions/ExtMenu.h"

using namespace HelloVW;

namespace HelloVW
{
	// Description of the menu command. The SResString entries ({resource,
	// identifier}) point at strings in the plug-in's .vwr resource file; a
	// resource file is optional for a build to succeed. EMenuEnableFlags{}
	// means "no special selection requirements to enable the command".
	static SMenuDef		gMenuDef = {
		/*Needs*/			EMenuEnableFlags{},
		/*NeedsNot*/		EMenuEnableFlags{},
		/*Title*/			{ "HelloVW", "title" },
		/*Category*/		{ "HelloVW", "category" },
		/*HelpText*/		{ "HelloVW", "help" },
		/*VersionCreated*/	31,
		/*VersionModified*/	0,
		/*VersionRetired*/	0,
		/*OverrideHelpID*/	""
	};
}

// UUID: 4be7d497-0a1b-4c0e-aef9-aee94befc55e
// Every extension needs a globally unique ID. Regenerate this (e.g. `uuidgen`)
// if you copy this file as the basis for another plug-in.
IMPLEMENT_VWMenuExtension(
	/*Extension class*/	CExtMenuHello,
	/*Event sink*/		CHelloMenu_EventSink,
	/*Universal name*/	"CExtMenuHello_HelloVW",
	/*Version*/			1,
	/*UUID*/			0x4be7d497, 0x0a1b, 0x4c0e, 0xae, 0xf9, 0xae, 0xe9, 0x4b, 0xef, 0xc5, 0x5e );

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
	// This is the whole point of the plug-in for now: tell the user it ran.
	gSDK->AlertInform(
		"Hello from the Vectorworks 2026 SDK plug-in!",
		"The plug-in loaded and this menu command ran successfully.",
		true /* minor alert */ );
}
