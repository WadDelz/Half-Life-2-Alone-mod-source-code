#ifndef __MAPPROPERTIESEDITORMENUPANEL_H
#define __MAPPROPERTIESEDITORMENUPANEL_H

#include "MapPropertiesEditorMenuPages.h"

#define COMMAND_LOAD_FROM_FILE "LoadFromFile"
#define COMMAND_SAVE_TO_FILE "SaveToFile"
#define COMMAND_SAVE_AS_FILE "SaveAsFile"
#define COMMAND_SAVE_CONFIRM "ConfirmSave"
#define COMMAND_RELOAD_SCRIPTS "ReloadScripts"
#define COMMAND_RELOAD_SCRIPTS_CONFIRM "ReloadScripts_C"



//theme struct
struct TimePropertyTheme_t
{
	const char* name;
	const char* filename;
	const char* tag;
};

#define NUM_TIME_PROPERTY_THEMES 3
extern TimePropertyTheme_t g_TimePropertyThemes[NUM_TIME_PROPERTY_THEMES];

//returns the time properties scheme
HScheme GetTimePropertiesScheme();



//time properties combo box. This allows the user to use the up + down arrow keys to navigate without it messing
//up the current selected item
class CMapPropertiesEditorComboBox : public ComboBox
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorComboBox, ComboBox);
public:
	CMapPropertiesEditorComboBox(Panel* parent, const char* name, int numlines, bool allowedit);

	//called when a key is pressed
	void OnKeyCodeTyped(KeyCode code);
	void OnKeyCodePressed(KeyCode code) {}
	void OnKeyTyped(wchar_t code) {}
};




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

	//called when text is changed (from the combo box)
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
private:
	//each page
	CMapPropertiesEditorNightPage* m_NightPage;
	CMapPropertiesEditorDayPage* m_DayPage;

	//button stuff
private:
	void InitButtons();
	Button* _saveAsButton = nullptr;

private:
	//theme combo box
	CMapPropertiesEditorComboBox* m_ThemeComboBox = nullptr;
};

//singleton panel
extern CMapPropertiesEditorPanel* s_MapPropertiesEditorPanel;

#endif //__MAPPROPERTIESEDITORMENUPANEL_H