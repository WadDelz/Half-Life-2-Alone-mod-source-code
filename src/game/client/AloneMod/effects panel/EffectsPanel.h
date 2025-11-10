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
	virtual void ReadFromFile(KeyValues* keyvalues) = 0;
	virtual void WriteToFile(KeyValues* keyvalues) = 0;

	//called on map load
	virtual void OnMapLoad() {};
};

#define EFFECTS_PANEL_WIDTH 550
#define EFFECTS_PANEL_HEIGHT 490
#define EFFECTS_PAGE_WIDTH EFFECTS_PANEL_WIDTH - 25
#define EFFECTS_PAGE_HEIGHT EFFECTS_PANEL_HEIGHT - 25

//effects panel interface
class IEffectsPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy() = 0;
	virtual void		ToggleVisibility() = 0;
};

//global effects panel interface
extern IEffectsPanel* g_EffectsPanelInterface;

#endif //__EFFECTSPANEL_H