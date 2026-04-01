#ifndef __MAPPROPERTIESEDITORPANELHORIZONFOGPAGE_H
#define __MAPPROPERTIESEDITORPANELHORIZONFOGPAGE_H

#include "MapPropertiesEditorPanelBasePage.h"


#define HORIZON_TALL_DIVISOR 50
#define HORIZON_SCALE_DIVISOR 100
#define COMMAND_HORIZON_SET_TOP_COLOR "SetTopColor"
#define COMMAND_HORIZON_SET_MIDDLE_COLOR "SetMiddleColor"
#define COMMAND_HORIZON_SET_BOTTOM_COLOR "SetBottomColor"

//fog page
class CMapPropertiesPanelHorizonFogPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelHorizonFogPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelHorizonFogPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void InitHorizonInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetHorizonInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
private:
	bool m_bNightTimeMode = false;

	//enable horizon button
	CheckButton* m_EnableHorizonButton;

	//should the horizon clip through the skybox?
	CheckButton* m_ShouldHorizonClip3dSkybox;

	//horizon pitch + yaw sliders
	CMapPropertiesPanelSlider* m_HorizonPitchSlider;
	Label* m_HorizonPitchText;

	CMapPropertiesPanelSlider* m_HorizonYawSlider;
	Label* m_HorizonYawText;

	//wide slider
	CMapPropertiesPanelSlider* m_HorizonWideSlider;
	Label* m_HorizonWideText;
	
	//tall slider
	CMapPropertiesPanelSlider* m_HorizonTallSlider;
	Label* m_HorizonTallText;
	
	//offset slider
	CMapPropertiesPanelSlider* m_HorizonOffsetXSlider;
	CMapPropertiesPanelSlider* m_HorizonOffsetYSlider;
	CMapPropertiesPanelSlider* m_HorizonOffsetZSlider;
	Label* m_HorizonOffsetXText;
	Label* m_HorizonOffsetYText;
	Label* m_HorizonOffsetZText;

	//scale
	CMapPropertiesPanelSlider* m_HorizonScaleSlider;
	Label* m_HorizonScaleText;

	//top color
	CMapPropertiesPanelButton* m_SetHorizonTopColorButton;
	Rect_t m_HorizonTopColorRect;
	Color m_HorizonTopColor;
	
	//middle color
	CMapPropertiesPanelButton* m_SetHorizonMiddleColorButton;
	Rect_t m_HorizonMiddleColorRect;
	Color m_HorizonMiddleColor;
	
	//bottom color
	CMapPropertiesPanelButton* m_SetHorizonBottomColorButton;
	Rect_t m_HorizonBottomColorRect;
	Color m_HorizonBottomColor;

	//current color selector mode
	enum ColorSelectorMode { Color_Top, Color_Middle, Color_Bottom } m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;
};



#endif //__MAPPROPERTIESEDITORPANELHORIZONFOGPAGE_H