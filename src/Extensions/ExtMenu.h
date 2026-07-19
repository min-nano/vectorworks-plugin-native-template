//
//	ExtMenu.h
//
//	A minimal Vectorworks menu command extension. When the user runs the
//	command it pops up an alert announcing that the plug-in has started.
//

#pragma once

#include "VectorworksSDK.h"

namespace HelloVW
{
	using namespace VWFC::PluginSupport;

	// ------------------------------------------------------------------------
	// The code that actually runs when the menu command is picked.
	class CHelloMenu_EventSink : public VWMenu_EventSink
	{
	public:
						CHelloMenu_EventSink(IVWUnknown* parent);
		virtual			~CHelloMenu_EventSink();

		virtual void	DoInterface() override;
	};

	// ------------------------------------------------------------------------
	// The menu command extension itself.
	class CExtMenuHello : public VWExtensionMenu
	{
		DEFINE_VWMenuExtension;
	public:
						CExtMenuHello(CallBackPtr cbp);
		virtual			~CExtMenuHello();
	};
}
