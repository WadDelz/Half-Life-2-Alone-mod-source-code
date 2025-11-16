#ifndef __EFFECTSPANEL_H
#define __EFFECTSPANEL_H

#include "vgui/VGUI.h"
#include "vgui_controls/PropertyPage.h"

#ifdef _WIN32
#pragma once
#endif

//base page for each effects panel
class IEffectsPanelPage : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE(IEffectsPanelPage, vgui::PropertyPage);
public:
	//constructor
	IEffectsPanelPage(vgui::Panel* parent, const char* name) : vgui::PropertyPage(parent, name) {};

	//save/load functions
	virtual void ResetEffects() = 0;
	virtual void ReadFromFile(KeyValues* keyvalues, bool reset = false) = 0;
	virtual void WriteToFile(KeyValues* keyvalues) = 0;

	//map funcs
	virtual void OnMapLoad() {};
	virtual void OnMapShutdown() {};
};

#define EFFECTS_PANEL_WIDTH 550
#define EFFECTS_PANEL_HEIGHT 490
#define EFFECTS_PAGE_WIDTH (EFFECTS_PANEL_WIDTH - 25)
#define EFFECTS_PAGE_HEIGHT (EFFECTS_PANEL_HEIGHT - 25)

//effects panel interface
class IEffectsPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy() = 0;
	virtual void		ToggleVisibility() = 0;

	//autoload
	virtual void		ResetEverything() = 0;
	virtual void		LoadFile(const char* filepath) = 0;
	virtual void		CallOnTick() = 0;
};

//global effects panel interface
extern IEffectsPanel* g_EffectsPanelInterface;

#endif //__EFFECTSPANEL_H