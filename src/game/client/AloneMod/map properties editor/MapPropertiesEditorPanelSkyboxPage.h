#ifndef __MAPPROPERTIESEDITORPANELSKYBOXPAGE_H
#define __MAPPROPERTIESEDITORPANELSKYBOXPAGE_H

#include "MapPropertiesEditorPanelBasePage.h"
#include <vgui_controls/ComboBox.h>



//divisor for bloom scalar
#define BLOOM_SCALAR_DIVISOR 100
#define COMMAND_CHANGE_CLOUDS_COLOR "ChangeCloudColors"

//skybox + filters page
class CMapPropertiesPanelSkyboxFiltersPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSkyboxFiltersPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelSkyboxFiltersPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void FormatImage(const char* input, char* output, int outsize);
	virtual void InitSkyboxAndFilter(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetSkyboxFilterInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
private:
	//night time mode
	bool m_bNightTimeMode = false;

private:
	//color picker
	CColorPicker* m_ColorPicker;

	//skybox
private:
	friend class CMapPropertiesPanel;

	//skybox background brush
	ImagePanel* m_SkyboxBackground;

	//skybox foreground brush
	CStretchingImage* m_SkyboxForeground;

	//skybox text entries
	class CMapPropertiesEditorComboBox* m_SkyboxNames;
	int m_PrevSkyboxNamesValue;

	//post processing filter
private:
	//filter list
	class CMapPropertiesEditorComboBox* m_FilterComboBox;
	int m_PrevFilterComboBoxValue;

	//filter intensity text
	Label* m_FilterIntensityText;

	//filter intensity slider
	CMapPropertiesPanelSlider* m_FilterIntensitySlider;

	//clouds
private:
	//clouds colors buttons
	CMapPropertiesPanelButton* m_CloudButton;

	//clouds colors
	Color m_CloudColor;
	Rect_t m_CloudColorRect;

	//bloom
private:
	//enable bloom
	CheckButton* m_EnableBloomCheckButton;

	//bloom slider
	CMapPropertiesPanelSlider* m_BloomScaleSlider;
	Label* m_BloomScaleText;

	//bloom scalar slider
	CMapPropertiesPanelSlider* m_BloomScalarSlider;
	Label* m_BloomScalarText;
};




#endif //__MAPPROPERTIESEDITORPANELSKYBOXPAGE_H