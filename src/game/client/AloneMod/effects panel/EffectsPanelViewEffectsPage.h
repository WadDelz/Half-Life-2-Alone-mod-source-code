#ifndef __EFFECTSPANELVIEWEFFECTSPAGE_H
#define __EFFECTSPANELVIEWEFFECTSPAGE_H

#include "EffectsPanel.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/Divider.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/TextEntry.h"

#ifdef _WIN32
#pragma once
#endif

//view effects page
class CEffectsPanelViewEffects : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelViewEffects, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelViewEffects(vgui::Panel* parent, const char* name);

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues, bool reset = false);
	void WriteToFile(KeyValues* keyvalues);

	//called every like 30ms
	void OnTick();

	//called on command
	void OnCommand(const char* pszCommand);

	//sets all the bounds for each item
	void PerformLayout();
private:
	//left hand side check buttons
	vgui::CheckButton* m_DontDrawViewmodel;
	vgui::CheckButton* m_EnableBlackAndWhiteView;
	vgui::CheckButton* m_EnableLenseDirtOnScreen;
	vgui::CheckButton* m_EnableTvStyledView;
	vgui::CheckButton* m_EnableColoredTvStyledView;
	vgui::CheckButton* m_EnableBlurredView;
	vgui::CheckButton* m_EnableBlackBoxes;
	vgui::CheckButton* m_EnableClaustraphobia;
	vgui::CheckButton* m_OverrideViewmodelFov;

	//bottom horizontal divider
	vgui::Divider* m_HorizontalDivider;

	//bottom stuff
	vgui::Label* m_BlackBoxWidthText;
	WheelSlider* m_BlackBoxWidthSlider;
	
	vgui::Label* m_BlackBoxHeightText;
	WheelSlider* m_BlackBoxHeightSlider;
	
	vgui::Label* m_ClaustrapphobiaAmountText;
	WheelSlider* m_ClaustrapphobiaAmountSlider;
	
	vgui::Label* m_ClaustrapphobiaFovText;
	WheelSlider* m_ClaustrapphobiaFovSlider;
	
	vgui::Label* m_ViewmodelFovOverrideText;
	WheelSlider* m_ViewmodelFovOverrideSlider;

	//verticle divider
	vgui::Divider* m_VerticalDivider;

	//right hand side items
	vgui::CheckButton* m_EnableCameraEditor;
	vgui::CheckButton* m_EnableCameraEditorViewmodelFix;

	vgui::Label* m_SmoothAngleAmountText;
	WheelSlider* m_SmoothAngleAmountSlider;
	
	vgui::Label* m_SmoothOriginAmountText;
	WheelSlider* m_SmoothOriginAmountSlider;

	vgui::Label* m_OriginOverrideText;
	vgui::TextEntry* m_OriginOverrideTextEntry;
	
	vgui::Label* m_AngleOverrideText;
	vgui::TextEntry* m_AngleOverrideTextEntry;

	vgui::Label* m_MinimumPitchText;
	WheelSlider* m_MinimumPitchSlider;

	vgui::Label* m_MaximumPitchText;
	WheelSlider* m_MaximumPitchSlider;

	//previous fov desired value
	int m_iPrevFovValue;
};

#endif //__EFFECTSPANELVIEWEFFECTSPAGE_H