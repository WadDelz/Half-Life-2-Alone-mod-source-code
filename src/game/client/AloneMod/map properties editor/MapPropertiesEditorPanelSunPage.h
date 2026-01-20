#ifndef __MAPPROPERTIESEDITORPANELSUNPAGE_H
#define __MAPPROPERTIESEDITORPANELSUNPAGE_H

#include "MapPropertiesEditorPanelBasePage.h"



//divisor for bloom scalar
#define COMMAND_SUN_ACTIVATE "ActivateSun"
#define COMMAND_PITCH_TO_EYES "PitchToEyes"
#define COMMAND_CHANGE_SUN_COLOR "ChangeSunColors"
#define COMMAND_CHANGE_SUN_OVERLAY_COLOR "ChangeSunOverlayColors"

//skybox + filters page
class CMapPropertiesPanelSunPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSunPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelSunPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void InitSunInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetSunInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
	MESSAGE_FUNC_PARAMS(OnSliderMoved, "SliderMoved", data);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", kv);
private:
	//night time mode
	bool m_bNightTimeMode = false;

	//color picker
	enum ColorSelectorMode { Color_Sun, Color_SunOverlay } m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;

	//sun
private:
	//enable sun
	CheckButton* m_EnableSunButton;

	//sun pitch
	Slider* m_SunPitchSlider;
	Label* m_SunPitchLabel;

	//sun yaw
	Slider* m_SunYawSlider;
	Label* m_SunYawLabel;

	//sun size
	Slider* m_SunSizeSlider;
	Label* m_SunSizeLabel;

	//sun color
	CMapPropertiesPanelButton* m_SunColorButton;
	Color m_SunColor;
	Rect_t m_SunColorDrawRect;

	//sun material text entry
	TextEntry* m_SunMaterialTextEntry;

	//sun overlay size
	Slider* m_SunOverlaySizeSlider;
	Label* m_SunOverlaySizeLabel;

	//sun overlay color
	CMapPropertiesPanelButton* m_SunOverlayColorButton;
	Color m_SunOverlayColor;
	Rect_t m_SunOverlayColorDrawRect;

	//sun overlay material text entry
	TextEntry* m_SunOverlayMaterialTextEntry;

	//set the pitch to the players eyes pitch button
	CMapPropertiesPanelButton* m_PitchToEyesButton;
};




#endif //__MAPPROPERTIESEDITORPANELSUNPAGE_H