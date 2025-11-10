#ifndef __EFFECTSPANELLIGHTINGPAGE_H
#define __EFFECTSPANELLIGHTINGPAGE_H

#include "EffectsPanel.h"
#include "vgui_controls/Button.h"

#ifdef _WIN32
#pragma once
#endif

//overlay page
#define ADD_LIGHT_BUTTON_COMMAWND "AddLight"
#define CHANGE_LIGHT_BUTTON_COMMAWND "ChangeLight"
#define REMOVE_LIGHT_BUTTON_COMMAWND "RemoveLight"

class CEffectsPanelLightingPage : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelLightingPage, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelLightingPage(vgui::Panel* parent, const char* name);
	~CEffectsPanelLightingPage();

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues);
	void WriteToFile(KeyValues* keyvalues);

	//child funcs
	void SetLightingText();

	//other panel functions
	void OnCommand(const char* pszCommand);
	
	//sets all the bounds for each item
	void PerformLayout();

	//updates the lights
	void UpdateLighting();
private:

	//buttons
	vgui::Button* m_AddButton;
	vgui::Button* m_ChangeOverlay;
	vgui::Button* m_RemoveOverlay;
};

#endif //__EFFECTSPANELLIGHTINGPAGE_H