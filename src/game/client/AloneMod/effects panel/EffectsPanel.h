#ifndef __EFFECTSPANEL_H
#define __EFFECTSPANEL_H

#include "vgui/VGUI.h"
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/Tooltip.h"
#include "vgui_controls/Slider.h"

#ifdef _WIN32
#pragma once
#endif

//macro for adding tool tips
#define ADD_TOOLTIP(element, delay, text, multiline) \
element->GetTooltip()->SetText(text); \
element->GetTooltip()->SetEnabled(true); \
element->GetTooltip()->SetTooltipDelay(delay); \
if (multiline) \
	element->GetTooltip()->SetTooltipFormatToMultiLine(); \
else \
	element->GetTooltip()->SetTooltipFormatToSingleLine(); \

//quick mouse wheel slider
class WheelSlider : public vgui::Slider
{
	DECLARE_CLASS_SIMPLE(WheelSlider, vgui::Slider);
public:
	//constructor
	WheelSlider(vgui::Panel* parent, const char* name, int WheelDelta = 1) : BaseClass(parent, name), m_ScrollDelta(WheelDelta)
	{
	}

	//called on mouse wheeled
	void OnMouseWheeled(int delta)
	{
		if (!IsEnabled())
			return;

		int min, max;
		GetRange(min, max);

		int dir = (delta > 0) ? 1 : -1;

		if (min > max)
			dir *= -1;

		SetValue(GetValue() + dir * m_ScrollDelta);
		SendSliderDragStartMessage();
		SendSliderDragEndMessage();
		SendSliderMovedMessage();
	}

	int m_ScrollDelta = 1;
};

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