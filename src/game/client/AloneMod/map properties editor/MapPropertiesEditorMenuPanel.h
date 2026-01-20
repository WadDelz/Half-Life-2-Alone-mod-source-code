#ifndef __MAPPROPERTIESEDITORMENUPANEL_H
#define __MAPPROPERTIESEDITORMENUPANEL_H

#include "MapPropertiesEditorMenuPages.h"

#define COMMAND_LOAD_FROM_FILE "LoadFromFile"
#define COMMAND_SAVE_TO_FILE "SaveToFile"
#define COMMAND_SAVE_AS_FILE "SaveAsFile"
#define COMMAND_SAVE_CONFIRM "ConfirmSave"
#define COMMAND_RELOAD_SCRIPTS "ReloadScripts"
#define COMMAND_RELOAD_SCRIPTS_CONFIRM "ReloadScripts_C"

//main map properties editor panel!!
class CMapPropertiesEditorPanel : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorPanel, PropertyDialog);
public:
	CMapPropertiesEditorPanel(VPANEL parent);
	~CMapPropertiesEditorPanel();

	//called on command
	void PerformLayout();
	void OnCommand(const char* pszCommand);
	void ApplySchemeSettings(IScheme* settings);

	//called when the save panel saves
	MESSAGE_FUNC_PARAMS(OnFileSaved, "OnSavePanelSaved", kv);
	MESSAGE_FUNC_PARAMS(OnFileLoaded, "OnLoadPanelLoaded", kv);
private:
	//each page
	CMapPropertiesEditorNightPage* m_NightPage;
	CMapPropertiesEditorDayPage* m_DayPage;

	//button stuff
private:
	void InitButtons();
	Button* _saveAsButton = nullptr;
};

//singleton panel
extern CMapPropertiesEditorPanel* s_MapPropertiesEditorPanel;

#endif //__MAPPROPERTIESEDITORMENUPANEL_H