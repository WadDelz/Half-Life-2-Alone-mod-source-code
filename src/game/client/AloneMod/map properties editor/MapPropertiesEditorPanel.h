#ifndef __MAPPROPERTIESEDITORPANEL_H
#define __MAPPROPERTIESEDITORPANEL_H

#include "MapPropertiesEditorPanelFogPage.h"
#include "MapPropertiesEditorPanelFogTriggersPage.h"
#include "MapPropertiesEditorPanelSkyboxPage.h"
#include "MapPropertiesEditorPanelSunPage.h"
#include "MapPropertiesEditorPanelHorizonFogPage.h"
#include <vgui_controls/PropertyDialog.h>

#define COMMAND_APPLY_PAGE_SETTINGS "ApplyPageSettings"

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

	//update our pages
	void UpdateNonTriggerPages() { m_FogPage->Update(); m_SkyboxFilterPage->Update(); m_HorizonPage->Update(); }
	void UpdateTriggersPage() { m_FogTriggersPage->Update(true); }

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

#if FOG_CUBE_TRIGGER_TEST
	CMapPropertiesPanelFogTriggersPage* m_FogTriggersPage;
#endif //FOG_CUBE_TRIGGER_TEST

	CMapPropertiesPanelSkyboxFiltersPage* m_SkyboxFilterPage;
	CMapPropertiesPanelSunPage* m_SunPage;
	CMapPropertiesPanelHorizonFogPage* m_HorizonPage;

private:
	//were all friends here
	friend const bool IsMapPropertiesFogTriggersPanelOpen();

	//have we closed?
	bool m_bClosed;

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