#ifndef __MAPPROPERTIESEDITORPANEL_H
#define __MAPPROPERTIESEDITORPANEL_H

#include "MapPropertiesEditorPanelFogPage.h"
#include "MapPropertiesEditorPanelSkyboxPage.h"
#include "MapPropertiesEditorPanelSunPage.h"
#include <vgui_controls/PropertyDialog.h>

#define COMMAND_APPLY_PAGE_SETTINGS "ApplySettings"

//map properties panel
class CMapPropertiesPanel : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanel, PropertyDialog)
public:
	CMapPropertiesPanel(Panel* parent);
	~CMapPropertiesPanel();

	//paint/tick
	void OnThink();
	void OnClose();
	void OnCommand(const char* pszCommand);

	//scheme
	void ApplySchemeSettings(IScheme* scheme);

	//loads the settings into the panel
	void PerformLayout();

	//keyboard/mouse
	void OnKeyCodePressed(KeyCode code);
	void OnKeyCodeReleased(KeyCode code);
	void OnMousePressed(KeyCode code);

	//init funcs
	void Init(MapTimeInfo_t& info, bool IsNightPage);

	void GetData(MapTimeInfo_t& info);
	void FormatImage(const char* input, char* output, int outside);

	MESSAGE_FUNC_PARAMS(OnResetUndoSteps, "ResetUndoSteps", data);
private:
	//pages
	CMapPropertiesPanelFogPage* m_FogPage;
	CMapPropertiesPanelSkyboxFiltersPage* m_SkyboxFilterPage;
	CMapPropertiesPanelSunPage* m_SunPage;

private:
	//day or night?
	bool m_bNightTimeMode;

	//previous convar values to re-set when we close
	char m_PreviousFilterConvarValue[256];
	char m_PreviousFilterIntensityConvarValue[16];
	char m_PreviousCloudsColorValue[16];
	char m_PreviousCloudsShowValue[16];
	char m_PreviousCloudsOverrideValue[4];
	char m_PreviousGodConvarValue[4];
	char m_PreviousEpicFilterConvarValue[4];
};

//singleton
extern CMapPropertiesPanel* g_MapPropertiesPanel;

#endif //__MAPPROPERTIESEDITORPANEL_H