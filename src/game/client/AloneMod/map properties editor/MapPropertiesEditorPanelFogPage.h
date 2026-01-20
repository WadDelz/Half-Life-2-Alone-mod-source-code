#ifndef __MAPPROPERTIESEDITORPANELFOGPAGE_H
#define __MAPPROPERTIESEDITORPANELFOGPAGE_H

#include "MapPropertiesEditorPanelBasePage.h"



#define COMMAND_CHANGE_FOG_COLOR "SetFogColor"
#define COMMAND_CHANGE_FOG_SKYBOX_COLOR "SetFogSkyboxColor"
#define COMMAND_CHANGE_FOG_BLEND_COLOR "SetFogBlendColor"
#define COMMAND_CHANGE_FOG_SKYBOX_BLEND_COLOR "SetFogSkyboxBlendColor"

//fog page
class CMapPropertiesPanelFogPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelFogPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelFogPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void InitFogInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetFogInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
private:
	bool m_bNightTimeMode = false;

	//override fog
	CheckButton* m_OverrideFogButton;

	//pixel fog
	CheckButton* m_EnablePixelFogButton;

	//enable fog
	CheckButton* m_EnableFogCheckButton;
	CheckButton* m_OverrideEnableFogCheckButton;

	//enable skybox fog
	CheckButton* m_EnableSkyboxFogCheckButton;
	CheckButton* m_OverrideEnableSkyboxFogCheckButton;

	//fog color
	CMapPropertiesPanelButton* m_FogColorButton;
	CheckButton* m_FogColorOverride;
	Color m_FogColor;
	Rect_t m_FogColorDrawRect;

	//fog skybox color
	CMapPropertiesPanelButton* m_FogSkyboxColorButton;
	CheckButton* m_FogSkyboxColorOverride;
	Color m_FogSkyboxColor;
	Rect_t m_FogSkyboxColorDrawRect;

	//fog start
	CMapPropertiesPanelSlider* m_FogStartSlider;
	Label* m_FogStartValueLabel;
	CheckButton* m_FogStartOverride;

	//fog end
	CMapPropertiesPanelSlider* m_FogEndSlider;
	Label* m_FogEndValueLabel;
	CheckButton* m_FogEndOverride;

	//fog start skybox
	CMapPropertiesPanelSlider* m_FogStartSkyboxSlider;
	Label* m_FogStartSkyboxValueLabel;
	CheckButton* m_FogStartSkyboxOverride;

	//fog end skybox
	CMapPropertiesPanelSlider* m_FogEndSkyboxSlider;
	Label* m_FogEndSkyboxValueLabel;
	CheckButton* m_FogEndSkyboxOverride;

	//fog density
	CMapPropertiesPanelSlider* m_FogDensitySlider;
	Label* m_FogDensityValueLabel;
	CheckButton* m_FogDensityOverride;

	//fog skybox density
	CMapPropertiesPanelSlider* m_FogSkyboxDensitySlider;
	Label* m_FogSkyboxDensityValueLabel;
	CheckButton* m_FogSkyboxDensityOverride;

	//farz clipping plane
	CMapPropertiesPanelSlider* m_FarzClippingPlaneSlider;
	Label* m_FarzClippingPlaneLabel;
	CheckButton* m_FarzClippingPlaneOverride;



	//enable fog blend
	CheckButton* m_EnableFogBlendCheckButton;
	CheckButton* m_OverrideFogBlendCheckButton;

	//fog blend color
	CMapPropertiesPanelButton* m_FogBlendColorButton;
	CheckButton* m_FogBlendColorOverride;
	Color m_FogBlendColor;
	Rect_t m_FogBlendColorDrawRect;

	//fog blend angle
	CMapPropertiesPanelSlider* m_FogBlendAngleSlider;
	Label* m_FogBlendAngleLabel;
	CheckButton* m_FogBlendAngleOverride;



	//enable skybox fog blend
	CheckButton* m_EnableFogBlendSkyboxCheckButton;
	CheckButton* m_OverrideFogBlendSkyboxCheckButton;

	//fog blend color
	CMapPropertiesPanelButton* m_FogBlendSkyboxColorButton;
	CheckButton* m_FogBlendSkyboxColorOverride;
	Color m_FogBlendSkyboxColor;
	Rect_t m_FogBlendSkyboxColorDrawRect;

	//fog blend angle
	CMapPropertiesPanelSlider* m_FogBlendSkyboxAngleSlider;
	Label* m_FogBlendSkyboxAngleLabel;
	CheckButton* m_FogBlendSkyboxAngleOverride;

	//current color selector mode
	enum ColorSelectorMode { Color_Fog, Color_SkyboxFog, Color_BlendFog, Color_BlendSkyboxFog } m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;
};



#endif //__MAPPROPERTIESEDITORPANELFOGPAGE_H