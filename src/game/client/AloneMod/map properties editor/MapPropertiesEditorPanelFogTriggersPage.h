#ifndef __MAPPROPERTIESEDITORPANELFOGTRIGGERSPAGE_H
#define __MAPPROPERTIESEDITORPANELFOGTRIGGERSPAGE_H

#include "MapPropertiesEditorPanelBasePage.h"
#include "vgui_controls/ListPanel.h"

#if FOG_CUBE_TRIGGER_TEST

//add trigger dialog
class CAddTriggerDialogPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CAddTriggerDialogPanel, Frame);
public:
	//constructor
	CAddTriggerDialogPanel(Panel* parent, bool add, const char* currenttext = "");		//add = true for add, false for rename
	~CAddTriggerDialogPanel();

	//on command
	void OnCommand(const char* pszCommand);
private:
	//text entry to store the name
	TextEntry* m_TextEntry;

	//are we the add or rename panel
	bool m_AddPanel;
};


//max fog variables
#define LERP_SLIDER_DIVISOR 100

#define SIZE_EDITOR_MODE_NONE 0
#define SIZE_EDITOR_MODE_MINS 1
#define SIZE_EDITOR_MODE_MAXS 2

#define COMMAND_ADD_TRIGGER "AddTrigger"
#define COMMAND_RENAME_TRIGGER "RenameTrigger"
#define COMMAND_REMOVE_TRIGGER "RemoveTrigger"
#define COMMAND_SET_SIZE "SetSize"
#define COMMAND_SET_COLOR "SetColor"
#define COMMAND_SHOULD_OVERRIDE "ShouldOverride"
#define COMMAND_APPLY_SETTING "ApplyFogSetting"


//fog page
class CMapPropertiesPanelFogTriggersPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelFogTriggersPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelFogTriggersPage(Panel* parent, const char* name);
	~CMapPropertiesPanelFogTriggersPage();

	//panel funcs
	virtual void Update(bool ForceUpdate);
	virtual void Update();
	virtual void Paint();
	virtual void PerformLayout();
	virtual void OnCommand(const char* pszCommand);
	virtual void OnKeyCodePressed(KeyCode code);
	virtual void OnPageShow();

	//init funcs
	virtual void InitFogTriggerInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetFogTriggerInfo(MapTimeInfo_t& info);

	//data funcs
	void AddFogTrigger(const char* name, MapTimeInfo_t::FogCubeTrigger_t* Data = nullptr, bool addtoarray = true);
	void RenameFogTrigger(int index, const char* newname);
	void RepopulateList();
	bool IsInSizeEditor() { return m_iInSizeEditor != SIZE_EDITOR_MODE_NONE; }

	void OnPageHide();

	//teleports the player to an edge of a trigger
	void TeleportToTrigger(int num);

	//callbacks
	void OnOverrideButtonEnabled();
	void OnOverrideButtonDisabled();

	//update funcs
	void UpdateSizes();
	void UpdateArrayValue();

	//override OnCheckButtonChecked
	void OnCheckButtonChecked(KeyValues* subkey);

	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PARAMS(OnSliderMoved, "SliderMoved", data);
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
	MESSAGE_FUNC_PARAMS(OnAddTrigger, "OnAddTrigger", data);
	MESSAGE_FUNC_PARAMS(OnRenameTrigger, "OnRenameTrigger", data);
	MESSAGE_FUNC_PARAMS(OnItemSelected, "ItemSelected", data);
	MESSAGE_FUNC_PARAMS(OnItemDeselected, "ItemDeselected", data);
private:
	bool m_bNightTimeMode = false;

	//trigger info
	CUtlVector<MapTimeInfo_t::FogCubeTrigger_t> m_TriggerInfos;

	//data stuff
	Label* m_DataLabel;
	CheckButton* m_ShouldOverrideButton;

	//slider data
	CMapPropertiesPanelSlider* m_DataSlider;

	//filter combo box
	class CMapPropertiesEditorComboBox* m_FilterBox;

	//skybox combo box
	class CMapPropertiesEditorComboBox* m_SkyboxBox;

	//color data stuff
	Button* m_DataButton;
	Rect_t m_DataButtonRect;
	Color m_DataButtonColor;
	CheckButton* m_ShouldOverrideColorButton;	//this determines if the color for the fog should be overriden

	//apply button
	//Button* m_ApplyButton;

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	//fog lerp slider
	Label* m_FogLerpSliderText;
	CMapPropertiesPanelSlider* m_FogLerpSlider;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

	//list for each var
	ListPanel* m_VarList;

	//list view for each trigger
	ListPanel* m_TriggerList;

	//bottom buttons
	Button* m_AddButton;
	Button* m_RenameButton;
	Button* m_RemoveButton;

	//set size button
	TextEntry* m_MinsTextEntry, *m_MaxsTextEntry;

	Button* m_SetSizeButton;

	//for setting the size
	Vector m_Sizes[2];
	int m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;		//0 for false, 1 for mins, 2 for maxs

	//color picker dialog
	CColorPicker* m_ColorPicker;
};

#endif //FOG_CUBE_TRIGGER_TEST

#endif //__MAPPROPERTIESEDITORPANELFOGTRIGGERSPAGE_H