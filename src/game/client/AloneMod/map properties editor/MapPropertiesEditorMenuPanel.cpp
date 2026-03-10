#include "cbase.h"
#include "MapPropertiesEditorMenuPanel.h"
#include <vgui_controls/PropertySheet.h>

extern ConVar map_properties_editor_load_mod;
extern ConVar amod_timeinfo_load_mod;

//panel singleton
CMapPropertiesEditorPanel* s_MapPropertiesEditorPanel;

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder text entry
//----------------------------------------------------------------------------------------------------
class CSaveToFolderTextEntry : public TextEntry
{
	DECLARE_CLASS_SIMPLE(CSaveToFolderTextEntry, TextEntry)
public:
	CSaveToFolderTextEntry(Panel* parent, const char* name) : BaseClass(parent, name) {}

	//only allow a-z A-Z 0-9 _ - + :
	void OnKeyTyped(wchar_t code) override
	{
		//check
		struct range_t { char min, max; } AllowedKeys[8] = {
			{ 'a', 'z' },
			{ 'A', 'Z' },
			{ '0', '9' },
			{ '_', '_' },
			{ '-', '-' },
			{ '+', '+' },
			{ ':', ':' },
			{ ' ', ' ' }
		};

		//check for ctrl + a
		if ((input()->IsKeyDown(KeyCode::KEY_LSHIFT) || input()->IsKeyDown(KeyCode::KEY_RSHIFT)) && (code == 'a' || code == 'A'))
		{
			BaseClass::OnKeyTyped(code);
			return;
		}

		//check for backspace
		if (code == 8)
		{
			BaseClass::OnKeyTyped(code);
			return;
		}

		//check the key
		for (int i = 0; i < SIZE_OF_ARRAY(AllowedKeys); i++)
		{
			if (code >= AllowedKeys[i].min && code <= AllowedKeys[i].max)
			{
				BaseClass::OnKeyTyped(code);
				return;
			}
		}

		//play error sound
		//surface()->PlaySound("resource/warning.wav");
	}
};







//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder panel
//----------------------------------------------------------------------------------------------------
class CSaveToFolderPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CSaveToFolderPanel, Frame);
public:
	CSaveToFolderPanel(Panel* parent, const char* name);
	~CSaveToFolderPanel();

	//command funcs
	void OnCommand(const char* pszCommand);
private:
	//save text entry
	CSaveToFolderTextEntry* m_TextEntry;
};

//singleton
static CSaveToFolderPanel* gs_SaveToFolderPanel = nullptr;

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder panel
//----------------------------------------------------------------------------------------------------
CSaveToFolderPanel::CSaveToFolderPanel(Panel* parent, const char* name) : BaseClass(parent, name)
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	SetMoveable(false);
	SetTitleBarVisible(true);
	SetCloseButtonVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetSize(300, 85);
	MoveToCenterOfScreen();
	Activate();
	SetTitle("#MapProperties_SaveDialog_Title", true);

	//create the text entry
	m_TextEntry = new CSaveToFolderTextEntry(this, "SaveToFolderTextEntry");
	m_TextEntry->SetBounds(5, 25, 290, 25);
	m_TextEntry->SetMaximumCharCount(64);

	//create the Save and Cancel button
	Button* m_SaveButton = new Button(this, "SaveButton", "#MapProperties_Button_Save");
	m_SaveButton->SetBounds(5, 55, 142, 25);
	m_SaveButton->SetCommand("Save");

	Button* m_CancelButton = new Button(this, "CancelButton", "#MapProperties_Button_Cancel");
	m_CancelButton->SetBounds(152, 55, 142, 25);
	m_CancelButton->SetCommand("Close");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CSaveToFolderPanel::OnCommand(const char* pszCommand)
{
	//check for save
	if (!Q_stricmp(pszCommand, "Save"))
	{
		//get the text
		char buf[512];
		m_TextEntry->GetText(buf, sizeof(buf));

		//post the message
		PostActionSignal(new KeyValues("OnSavePanelSaved", "Folder", buf));

		//close this
		Close();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder destructor
//----------------------------------------------------------------------------------------------------
CSaveToFolderPanel::~CSaveToFolderPanel()
{
	gs_SaveToFolderPanel = nullptr;
}






//----------------------------------------------------------------------------------------------------
// Purpose: load from folder panel
//----------------------------------------------------------------------------------------------------
class CLoadFromFolderPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CLoadFromFolderPanel, Frame);
public:
	CLoadFromFolderPanel(Panel* parent, const char* name);
	~CLoadFromFolderPanel();

	//command funcs
	void OnCommand(const char* pszCommand);
private:
	//load text entry
	CMapPropertiesEditorComboBox* m_ComboBoxList;
};

//singleton
static CLoadFromFolderPanel* gs_LoadFromFolderPanel = nullptr;

//----------------------------------------------------------------------------------------------------
// Purpose: Load from folder panel
//----------------------------------------------------------------------------------------------------
CLoadFromFolderPanel::CLoadFromFolderPanel(Panel* parent, const char* name) : BaseClass(parent, name)
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	SetMoveable(false);
	SetTitleBarVisible(true);
	SetCloseButtonVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetSize(300, 85);
	MoveToCenterOfScreen();
	Activate();
	SetTitle("#MapProperties_LoadDialog_Title", true);

	//create the text entry
	m_ComboBoxList = new CMapPropertiesEditorComboBox(this, "LoadFromFolderList", 10, false);
	m_ComboBoxList->SetBounds(5, 25, 290, 25);

	//load the files
	{
		//always add 'default'
		m_ComboBoxList->AddItem("Default", new KeyValues(""));

		//get the file system search path
		FileFindHandle_t findHandle;
		const char* folderName = filesystem->FindFirst("resource/time_info/*", &findHandle);
		while (folderName)
		{
			//dont load . files
			if (strchr(folderName, '.'))
			{
				folderName = filesystem->FindNext(findHandle);
				continue;
			}

			//check for _ignore
			if (!Q_stricmp(folderName, "_IGNORE"))
			{
				folderName = filesystem->FindNext(findHandle);
				continue;
			}

			//check for the current path
			if (!Q_stricmp(folderName, map_properties_editor_load_mod.GetString()))
			{
				folderName = filesystem->FindNext(findHandle);
				continue;
			}

			//only add directories
			char path[512];
			Q_snprintf(path, sizeof(path), "resource/time_info/%s", folderName);
			if (filesystem->IsDirectory(path, "MOD"))
				m_ComboBoxList->AddItem(folderName, new KeyValues(folderName));

			folderName = filesystem->FindNext(findHandle);
		}

		//set vars
		m_ComboBoxList->ActivateItem(0);
		filesystem->FindClose(findHandle);
	}

	//create the Load and Cancel button
	Button* m_LoadButton = new Button(this, "LoadButton", "#MapProperties_Button_Load");
	m_LoadButton->SetBounds(5, 55, 142, 25);
	m_LoadButton->SetCommand("Load");

	Button* m_CancelButton = new Button(this, "CancelButton", "#MapProperties_Button_Cancel");
	m_CancelButton->SetBounds(152, 55, 142, 25);
	m_CancelButton->SetCommand("Close");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CLoadFromFolderPanel::OnCommand(const char* pszCommand)
{
	//check for load
	if (!Q_stricmp(pszCommand, "Load"))
	{
		//post the message
		PostActionSignal(new KeyValues("OnLoadPanelLoaded", "Folder", m_ComboBoxList->GetActiveItemUserData()->GetName()));

		//close this
		Close();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Load from folder destructor
//----------------------------------------------------------------------------------------------------
CLoadFromFolderPanel::~CLoadFromFolderPanel()
{
	gs_LoadFromFolderPanel = nullptr;
}





//----------------------------------------------------------------------------------------------------
// Purpose: Map properties editor combo box class
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorComboBox::CMapPropertiesEditorComboBox(Panel* parent, const char* name, int numlines, bool allowedit)
	: BaseClass(parent, name, numlines, allowedit)
{
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a key is pressed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorComboBox::OnKeyCodeTyped(KeyCode code)
{
	//check for up or down
	if (code == KeyCode::KEY_UP)
	{
		//get the item
		int item = GetActiveItem() - 1;

		if (item < 0)
			item = GetItemCount() - 1;

		//select the item
		ActivateItem(item);
		SelectMenuItem(item);
		RequestFocus();
	}
	else if (code == KeyCode::KEY_DOWN)
	{
		//get the item
		int item = GetActiveItem() + 1;

		if (item >= GetItemCount())
			item = 0;

		//select the item
		ActivateItem(item);
		SelectMenuItem(item);
		RequestFocus();
	}

	//check for R
	else if (code == KeyCode::KEY_R)
	{
		GetParent()->OnKeyCodePressed(KeyCode::KEY_R);
	}
}






extern ConVar amod_time_properties_editor_theme;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPanel::CMapPropertiesEditorPanel(VPANEL parent) : BaseClass(nullptr, "MapPropertiesEditor")
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSize(480, 600);
	MoveToCenterOfScreen();
	SetTitle(CFmtStr("Map Properties Editor: %s", map_properties_editor_load_mod.GetString()), false);
	SetApplyButtonVisible(true);
	SetDeleteSelfOnClose(true);
	SetRoundedCorners(0);

	//set our scheme
	SetScheme(GetTimePropertiesScheme());

	//add the pages
	AddPage(m_NightPage = new CMapPropertiesEditorNightPage(this), "#MapProperties_ButtomTitle_NightMapProperties");
	AddPage(m_DayPage = new CMapPropertiesEditorDayPage(this), "#MapProperties_ButtomTitle_DayMapProperties");
	GetPropertySheet()->SetActivePage(ConVarRef("amod_day").GetBool() ? (Panel*)m_DayPage : (Panel*)m_NightPage);
	GetPropertySheet()->SetKeyBoardInputEnabled(false);

	//init our buttons
	InitButtons();

	//create the themes combo box
	m_ThemeComboBox = new CMapPropertiesEditorComboBox(this, "ThemesCombnoBox", 10, false);
	m_ThemeComboBox->AddActionSignalTarget(this);

	//add all the themes
	for (int i = 0; i < SIZE_OF_ARRAY(g_TimePropertyThemes); i++)
		m_ThemeComboBox->AddItem(g_TimePropertyThemes[i].name, nullptr);

	//active the current item
	m_ThemeComboBox->ActivateItem(amod_time_properties_editor_theme.GetInt());
}

//----------------------------------------------------------------------------------------------------
// Purpose: Inits our buttons
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::InitButtons()
{
	//set our apply button
	SetApplyButtonText("#MapProperties_Button_Reload");
	_applyButton->SetEnabled(true);
	_applyButton->SetCommand(COMMAND_RELOAD_SCRIPTS);

	//set our reload button
	SetCancelButtonText("#MapProperties_Button_Load");
	_cancelButton->SetCommand(COMMAND_LOAD_FROM_FILE);

	//set our save button
	SetOKButtonText("#MapProperties_Button_Save");
	_okButton->SetCommand(COMMAND_SAVE_TO_FILE);

	//delete _saveAsButton if needed
	if (_saveAsButton)
	{
		delete _saveAsButton;
		_saveAsButton = nullptr;
	}

	//create our save button if needed
	if (map_properties_editor_load_mod.GetString()[0] && !_saveAsButton)
	{
		_saveAsButton = new Button(this, "_saveAsButton", "#MapProperties_Button_SaveAs");
		_saveAsButton->SetCommand(COMMAND_SAVE_AS_FILE);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed from the themes combo box
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnTextChanged(KeyValues* data)
{
	//safe check
	if (data->GetPtr("panel") != m_ThemeComboBox)
		return;

	//set the value of amod_time_properties_editor_theme
	amod_time_properties_editor_theme.SetValue(m_ThemeComboBox->GetActiveItem());
}

//----------------------------------------------------------------------------------------------------
// Purpose: layout
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	//get the bounds for the button
	int x, y, w, h;
	_okButton->GetBounds(x, y, w, h);

	//take (w + (_cancelButton->GetX() - (x + w))) off the x pos
	int cbx, _;
	_cancelButton->GetPos(cbx, _);

	//get the gap
	int gap = (cbx - (x + w));
	x = x - (w + gap);

	//save as button must be valid
	if (_saveAsButton)
		_saveAsButton->SetBounds(x, y, w, h);

	//set the bounds of our theme button
	m_ThemeComboBox->SetBounds(8, y, (x - 8) - gap, h); //yay
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the save panel saves to a folder
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnFileSaved(KeyValues* data)
{
	//get the filename
	const char* filename = data->GetString("Folder");

	//MUST not be empty
	if (!filename || !*filename)
	{
		surface()->PlaySound("resource/warning.wav");
		QueryBox* modal = new QueryBox("#Amod_Panel_Error", "#MapProperties_Query_EmptySaveTextEntry", this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		return;
	}
	
	//check for _ignore or default
	if (!Q_stricmp(filename, "_ignore") || !Q_stricmp(filename, "default"))
	{
		surface()->PlaySound("resource/warning.wav");
		QueryBox* modal = new QueryBox("#Amod_Panel_Error", "#MapProperties_Query_PreservedSaveName", this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		return;
	}

	//check if the folder exists AND it isnt map_properties_editor_load_mod
	if (filesystem->IsDirectory(CFmtStr("resource/time_info/%s", filename), "MOD") && Q_stricmp(filename, map_properties_editor_load_mod.GetString()))
	{
		//get the text
		wchar_t* format = g_pVGuiLocalize->Find("#MapProperties_Query_FolderExists_Desc");

		//get the filename
		wchar_t name[256];
		g_pVGuiLocalize->ConvertANSIToUnicode(filename, name, sizeof(name));

		//get the output text
		wchar_t text[512];
		swprintf(text, SIZE_OF_ARRAY(text), format, name);

		//show the text
		QueryBox* modal = new QueryBox(L"#MapProperties_Query_FolderExists_Title", text, this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		modal->SetOKCommand(new KeyValues("Command", "command", CFmtStr(COMMAND_SAVE_CONFIRM "%s", filename)));
		return;
	}

	//save now
	OnCommand(CFmtStr(COMMAND_SAVE_CONFIRM "%s", filename));
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the file is loaded
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnFileLoaded(KeyValues* data)
{
	//set our convar + title
	const char* filename = data->GetString("folder");
	map_properties_editor_load_mod.SetValue(filename);
	SetTitle(CFmtStr("Map Properties Editor: %s", filename), false);

	//reload this panel
	cvar->FindCommand("open_map_time_properties_editor")->Dispatch(CCommand{});
	return;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Apply the scheme settings
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::ApplySchemeSettings(IScheme* settings)
{
	BaseClass::ApplySchemeSettings(settings);

	//idk why i put this here but its here
	MoveToCenterOfScreen();

	//set the bg color of the property sheet
	_propertySheet->SetBgColor(settings->GetColor("PropertySheet.BgColor", _propertySheet->GetBgColor()));
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnCommand(const char* pszCommand)
{
	//check for COMMAND_SAVE_TO_FILE
	if (!Q_stricmp(pszCommand, COMMAND_SAVE_TO_FILE))
	{
		//if map_properties_editor_load_mod isnt empty then save to that path
		const char* mod = map_properties_editor_load_mod.GetString();
		if (mod[0])
		{
			OnCommand(CFmtStr(COMMAND_SAVE_CONFIRM "%s", mod));
			return;
		}

		//open the save dialog
		if (gs_SaveToFolderPanel)
			gs_SaveToFolderPanel->DeletePanel();

		gs_SaveToFolderPanel = new CSaveToFolderPanel(this, "SaveToFolderPanel");
		gs_SaveToFolderPanel->DoModal();
		gs_SaveToFolderPanel->AddActionSignalTarget(this);
		gs_SaveToFolderPanel->Activate();
		return;
	}

	//check for COMMAND_SAVE_AS_FILE
	if (!Q_stricmp(pszCommand, COMMAND_SAVE_AS_FILE))
	{
		//open the save dialog
		if (gs_SaveToFolderPanel)
			gs_SaveToFolderPanel->DeletePanel();

		gs_SaveToFolderPanel = new CSaveToFolderPanel(this, "SaveAsFolderPanel");
		gs_SaveToFolderPanel->DoModal();
		gs_SaveToFolderPanel->AddActionSignalTarget(this);
		gs_SaveToFolderPanel->Activate();
		return;
	}

	//check for COMMAND_LOAD_FROM_FILE
	if (!Q_stricmp(pszCommand, COMMAND_LOAD_FROM_FILE))
	{
		//open the load dialog
		if (gs_LoadFromFolderPanel)
			gs_LoadFromFolderPanel->DeletePanel();

		gs_LoadFromFolderPanel = new CLoadFromFolderPanel(this, "LoadFromFolderPanel");
		gs_LoadFromFolderPanel->DoModal();
		gs_LoadFromFolderPanel->AddActionSignalTarget(this);
		gs_LoadFromFolderPanel->Activate();
		return;
	}

	//check for COMMAND_SAVE_CONFIRM
	else if (!Q_strnicmp(pszCommand, COMMAND_SAVE_CONFIRM, Q_strlen(COMMAND_SAVE_CONFIRM)))
	{
		//get the filename
		const char* filename = pszCommand + Q_strlen(COMMAND_SAVE_CONFIRM);
		if (!filename || !*filename)
			return;

		//save
		WriteAllTimeInfosToFiles(filename);

		//if map_properties_editor_load_mod isnt set then set the stuff
		if (!map_properties_editor_load_mod.GetString()[0])
		{
			//set our convar + title
			map_properties_editor_load_mod.SetValue(filename);
			amod_timeinfo_load_mod.SetValue(filename);
			SetTitle(CFmtStr("Map Properties Editor: %s", filename), false);

			//reset our buttons
			InitButtons();
			PerformLayout();
		}

		return;
	}

	//check for COMMAND_RELOAD_SCRIPTS
	else if (!Q_stricmp(pszCommand, COMMAND_RELOAD_SCRIPTS))
	{
		//get the text
		wchar_t* format = g_pVGuiLocalize->Find("#MapProperties_Query_Reload_Desc");

		//get the map name
		wchar_t map[256];
		g_pVGuiLocalize->ConvertANSIToUnicode(map_properties_editor_load_mod.GetString(), map, sizeof(map));

		//get the output text
		wchar_t text[512];
		swprintf(text, SIZE_OF_ARRAY(text), format, map);

		//show the text
		QueryBox* modal = new QueryBox(L"#Amod_Panel_AreYouSure", text, this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		modal->SetOKCommand(new KeyValues("Command", "command", COMMAND_RELOAD_SCRIPTS_CONFIRM));
	}

	//check for COMMAND_RELOAD_SCRIPTS_CONFIRM
	else if (!Q_stricmp(pszCommand, COMMAND_RELOAD_SCRIPTS_CONFIRM))
	{
		//reload the time info
		cvar->FindCommand("amod_timeinfo_reset")->Dispatch(CCommand{});

		//reload the pages
		int activepage = GetPropertySheet()->GetActivePageNum();
		
		//delete our pages
		GetPropertySheet()->DeletePage(m_NightPage);
		GetPropertySheet()->DeletePage(m_DayPage);

		delete m_DayPage;

		//ALWAYS delete g_MapPropertiesPanel if its open
		if (g_MapPropertiesPanel)
			delete g_MapPropertiesPanel;

		//add the pages
		AddPage(m_NightPage = new CMapPropertiesEditorNightPage(this), "#MapProperties_Button_NightMapProperties");
		AddPage(m_DayPage = new CMapPropertiesEditorDayPage(this), "#MapProperties_Button_DayMapProperties");
		GetPropertySheet()->SetActivePage(activepage ? (Panel*)m_DayPage : (Panel*)m_NightPage);
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPanel::~CMapPropertiesEditorPanel()
{
	s_MapPropertiesEditorPanel = nullptr;

	//check for the actuall editor
	if (g_MapPropertiesPanel)
		g_MapPropertiesPanel->DeletePanel();

	g_MapPropertiesPanel = nullptr;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Deletes and clears s_MapPropertiesEditorPanel if its open/active
//----------------------------------------------------------------------------------------------------
void DeleteMapPropertiesPanel()
{
	if (s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel->DeletePanel();

	s_MapPropertiesEditorPanel = nullptr;
}