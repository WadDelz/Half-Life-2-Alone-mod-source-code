#include "cbase.h"
#include "fmtstr.h"
#include "EffectsPanel.h"
#include "filesystem.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/Button.h"

//pages
#include "EffectsPanelViewEffectsPage.h"
#include "EffectsPanelConvarPage.h"
#include "EffectsPanelOverlayPage.h"
#include "EffectsPanelLightingPage.h"
#include "EffectsPanelSettingsPage.h"

//commands
#define COMMAND_LOAD_EFFECTS "LoadEffects"
#define COMMAND_SAVE_EFFECTS "SaveEffects"
#define COMMAND_RESET_EFFECTS "ResetEffects"
#define COMMAND_CONFIRM_RESET "ConfirmReset"
#define COMMAND_LIGHTING_DEBUG "LightingDebug"
#define COMMAND_AUTOLOAD "AutoloadDebug"

//effects panel
class CEffectsPanel : public vgui::PropertyDialog, CAutoGameSystem
{
	DECLARE_CLASS_SIMPLE(CEffectsPanel, vgui::PropertyDialog)
public:	
	//constructor
	CEffectsPanel(vgui::VPANEL parent);
	~CEffectsPanel();

	//keyboard/mouse funcs
	void OnKeyCodePressed(vgui::KeyCode code);

	//map funcs
	void LevelInitPostEntity();
	void LevelShutdownPreEntity();

	//autoload funcs
	void ResetEverything();
	void LoadFile(const char* filepath);
	void CallOnTick();

	//other functions
	void OnCommand(const char* pszCommand);
	void OnTick();

	MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);
private:
	//every single page
	CUtlVector<IEffectsPanelPage*> m_EffectsPages;

	//file dialog stuff for save/load
	vgui::FileOpenDialog* m_FileDialog = nullptr;
	vgui::FileOpenDialogType_t m_FileType;

	//debug for lighting page
	vgui::CheckButton* m_LightingDebugCheckButton;

	//autoload check button
	vgui::CheckButton* m_AutoloadCheckButton;

	//settings page
	CEffectsPanelSettingsPage* m_SettingsPage;
};

#define EFFECTS_PANEL_AUTOLOAD_LIST_FILENAME "cfg/effects_panel_autoload_list.cfg"

//index for overlay page
#define CONVAR_PAGE_INDEX 1

//index for lighting page
#define LIGHTING_PAGE_INDEX 3

//lighting debug convar
extern ConVar amod_lighting_debug;

//autoload debug
extern ConVar amod_effects_panel_autoload_files;

//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel
//---------------------------------------------------------------------------------
CEffectsPanel::CEffectsPanel(vgui::VPANEL parent) : BaseClass(nullptr, "EffectsPanel")
{
	//set the parent and all the other stuff
	SetParent(parent);

	//keyboard and mouse
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	
	//other stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(false);

	//add all the pages needed
	m_EffectsPages.AddToTail(new CEffectsPanelViewEffects(this, "#Amod_EffectsPanel_PageTitle_ViewEffects"));
	m_EffectsPages.AddToTail(new CEffectsPanelConvarPage(this, "#Amod_EffectsPanel_PageTitle_Convars"));			//if you change this then change CONVAR_PAGE_INDEX
	m_EffectsPages.AddToTail(new CEffectsPanelOverlayPage(this, "#Amod_EffectsPanel_PageTitle_Overlays"));
	m_EffectsPages.AddToTail(new CEffectsPanelLightingPage(this, "#Amod_EffectsPanel_PageTitle_Lighting"));			//if you change this then change LIGHTING_PAGE_INDEX

	//add each page
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		AddPage(m_EffectsPages[i], m_EffectsPages[i]->GetName());

	//add the settings page
	m_SettingsPage = new CEffectsPanelSettingsPage(this, "SettingsPage");
	AddPage(m_SettingsPage, "#Amod_EffectsPanel_PageTitle_Autoload");

	//load the settings
	KeyValues* settings = new KeyValues("Settings");
	settings->LoadFromFile(filesystem, EFFECTS_PANEL_AUTOLOAD_LIST_FILENAME, "MOD");

	m_SettingsPage->ReadFromFile(settings);

	settings->deleteThis();

	//bounds and title
	SetTitle("#Amod_EffectsPanel_Title", false);
	SetSize(EFFECTS_PANEL_WIDTH, EFFECTS_PANEL_HEIGHT);
	MoveToCenterOfScreen();

	//bottom buttons
	SetOKButtonVisible(true);
	SetOKButtonText("#Amod_EffectsPanel_Buttons_Reset");
	_okButton->SetEnabled(true);
	_okButton->SetCommand(COMMAND_RESET_EFFECTS);
	_okButton->SetReleasedSound("ui/buttonclickrelease.wav");

	SetCancelButtonVisible(true);
	SetCancelButtonText("#Amod_EffectsPanel_Buttons_Save");
	_cancelButton->SetEnabled(true);
	_cancelButton->SetCommand(COMMAND_SAVE_EFFECTS);
	_cancelButton->SetReleasedSound("ui/buttonclickrelease.wav");

	SetApplyButtonVisible(true);
	SetApplyButtonText("#Amod_EffectsPanel_Buttons_Load");
	_applyButton->SetEnabled(true);
	_applyButton->SetCommand(COMMAND_LOAD_EFFECTS);
	_applyButton->SetReleasedSound("ui/buttonclickrelease.wav");

	//create the lighting debug button
	m_LightingDebugCheckButton = new vgui::CheckButton(this, "LightingDebug", "#Amod_EffectsPanel_Buttons_LightingDebug");
	m_LightingDebugCheckButton->SetBounds(10, EFFECTS_PANEL_HEIGHT - 32, 150, 22);
	m_LightingDebugCheckButton->AddActionSignalTarget(this);
	m_LightingDebugCheckButton->SetSelected(false);
	m_LightingDebugCheckButton->SetCommand(COMMAND_LIGHTING_DEBUG);
	
	//create the autoload check buttong
	m_AutoloadCheckButton = new vgui::CheckButton(this, "AutoloadButton", "#Amod_EffectsPanel_Buttons_ShouldAutoload");
	m_AutoloadCheckButton->SetBounds(10, EFFECTS_PANEL_HEIGHT - 32, 165, 22);
	m_AutoloadCheckButton->AddActionSignalTarget(this);
	m_AutoloadCheckButton->SetSelected(amod_effects_panel_autoload_files.GetBool());
	m_AutoloadCheckButton->SetCommand(COMMAND_AUTOLOAD);

	//add tooltips
	ADD_TOOLTIP(m_LightingDebugCheckButton, 100, "#Amod_EffectsPanel_Buttons_LightingDebug_Tooltip", true)
	ADD_TOOLTIP(m_AutoloadCheckButton, 100, "#Amod_EffectsPanel_Buttons_ShouldAutoload_Tooltip", true)

	vgui::ivgui()->AddTickSignal(GetVPanel(), 30);
}

//---------------------------------------------------------------------------------
// Purpose: Destructor
//---------------------------------------------------------------------------------
CEffectsPanel::~CEffectsPanel()
{
	//save the settings
	KeyValues* settings = new KeyValues("Settings");

	m_SettingsPage->WriteToFile(settings);

	settings->SaveToFile(filesystem, EFFECTS_PANEL_AUTOLOAD_LIST_FILENAME, "MOD");
	settings->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Called when a key is pressed
//---------------------------------------------------------------------------------
void CEffectsPanel::OnKeyCodePressed(vgui::KeyCode code)
{
	//call the base function
	BaseClass::OnKeyCodePressed(code);

	//get the bind to toggle the song panel
	const char* bind = engine->Key_LookupBinding("ToggleEffectsPanel");
	if (!bind)
		return;

	//get the string of the key that was pressed
	const char* DisplayCode = (const char*)KeyCodeToDisplayString(code);

	//compare the 2 strings and if they match. close the panel
	if (!Q_stricmp(bind, DisplayCode))
		g_EffectsPanelInterface->ToggleVisibility();
}

//---------------------------------------------------------------------------------
// Purpose: Called on map load
//---------------------------------------------------------------------------------
void CEffectsPanel::LevelInitPostEntity()
{
	//call all page functions
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		m_EffectsPages[i]->OnMapLoad();

	//call for settings
	m_SettingsPage->OnMapLoad();

	CAutoGameSystem::LevelInitPostEntity();
}

//---------------------------------------------------------------------------------
// Purpose: Called on map load
//---------------------------------------------------------------------------------
void CEffectsPanel::LevelShutdownPreEntity()
{
	//call all page functions
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		m_EffectsPages[i]->OnMapShutdown();

	//call for settings
	m_SettingsPage->OnMapShutdown();

	CAutoGameSystem::LevelInitPostEntity();
}

//---------------------------------------------------------------------------------
// Purpose: Resets everything. This is for the autoload functionality
//---------------------------------------------------------------------------------
void CEffectsPanel::ResetEverything()
{
	//load for all effect pages
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		m_EffectsPages[i]->ResetEffects();
}

//---------------------------------------------------------------------------------
// Purpose: Loads a file. This is for the autoload functionality
//---------------------------------------------------------------------------------
void CEffectsPanel::LoadFile(const char* filepath)
{
	//load the keyvalues
	KeyValues* file = new KeyValues("");
	if (file->LoadFromFile(g_pFullFileSystem, filepath))
	{
		//load for all effect pages
		for (int i = 0; i < m_EffectsPages.Count(); i++)
		{
			m_EffectsPages[i]->ReadFromFile(file);
		}
	}

	//delete the keyvalues
	file->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Calls the ontick funciton for every page
//---------------------------------------------------------------------------------
void CEffectsPanel::CallOnTick()
{
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		m_EffectsPages[i]->OnTick();
}

//---------------------------------------------------------------------------------
// Purpose: Called on panel command
//---------------------------------------------------------------------------------
void CEffectsPanel::OnCommand(const char* pszCommand)
{
	//check for reset
	if (!Q_strcmp(pszCommand, COMMAND_RESET_EFFECTS))
	{
		//prompt the user
		vgui::QueryBox* prompt = new vgui::QueryBox("#Amod_EffectsPanel_ResetPrompt_Title", "#Amod_EffectsPanel_ResetPrompt_Desc");
		prompt->SetOKButtonText("Ok");
		prompt->SetOKCommand(new KeyValues("Command", "command", COMMAND_CONFIRM_RESET));
		prompt->SetCancelButtonVisible(false);
		prompt->AddActionSignalTarget(this);
		prompt->DoModal(this);
	}

	//confirm reset
	if (!Q_strcmp(pszCommand, COMMAND_CONFIRM_RESET))
	{
		//reset every page
		for (int i = 0; i < m_EffectsPages.Count(); i++)
		{
			m_EffectsPages[i]->ResetEffects();
		}
	}

	//check for save file
	else if (!Q_strcmp(pszCommand, COMMAND_SAVE_EFFECTS))
	{
		//delete m_FileDialog if it isnt nullptr
		if (m_FileDialog)
		{
			delete m_FileDialog;
			m_FileDialog = nullptr;
		}

		//create the dialog
		m_FileDialog = new vgui::FileOpenDialog(this, "#Amod_EffectsPanel_SavePreset_Title", false);
		m_FileDialog->AddFilter("*.amf", "Amod Filter Files (*.amf)", true);
		m_FileDialog->AddFilter("*.*", "All Files (*.*)", false);
		m_FileDialog->AddActionSignalTarget(this);

		//set the starting directory
		char buf[512];
		g_pFullFileSystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));
		m_FileDialog->SetStartDirectory(buf);

		//activate the file dialog
		m_FileDialog->DoModal(false);
		m_FileDialog->Activate();

		//set the file type
		m_FileType = vgui::FileOpenDialogType_t::FOD_SAVE;
	}
	
	//check for load file
	else if (!Q_strcmp(pszCommand, COMMAND_LOAD_EFFECTS))
	{
		//delete m_FileDialog if it isnt nullptr
		if (m_FileDialog)
		{
			delete m_FileDialog;
			m_FileDialog = nullptr;
		}

		//create the dialog
		m_FileDialog = new vgui::FileOpenDialog(this, "#Amod_EffectsPanel_LoadPreset_Title", true);
		m_FileDialog->AddFilter("*.amf", "Amod Filter Files (*.amf)", true);
		m_FileDialog->AddFilter("*.*", "All Files (*.*)", false);
		m_FileDialog->AddActionSignalTarget(this);

		//set the starting directory
		char buf[512];
		g_pFullFileSystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));
		m_FileDialog->SetStartDirectory(buf);

		//activate the file dialog
		m_FileDialog->DoModal(false);
		m_FileDialog->Activate();

		//set the file type
		m_FileType = vgui::FileOpenDialogType_t::FOD_OPEN;
	}
	
	//check for lighting debug
	else if (!Q_strcmp(pszCommand, COMMAND_LIGHTING_DEBUG))
	{
		amod_lighting_debug.SetValue(m_LightingDebugCheckButton->IsSelected());
	}
	
	//check for lighting debug
	else if (!Q_strcmp(pszCommand, COMMAND_AUTOLOAD))
	{
		amod_effects_panel_autoload_files.SetValue(m_AutoloadCheckButton->IsSelected());
	}

	//call base function
	else
	{
		BaseClass::OnCommand(pszCommand);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called every 30ms
//---------------------------------------------------------------------------------
void CEffectsPanel::OnTick()
{
	BaseClass::OnTick();

	vgui::Panel* activepage = GetActivePage();

	//see if settings page
	m_AutoloadCheckButton->SetVisible(activepage == m_SettingsPage);
	_cancelButton->SetVisible(activepage != m_SettingsPage);
	_okButton->SetVisible(activepage != m_SettingsPage);
	_applyButton->SetVisible(activepage != m_SettingsPage);

	//see if it is the lighting page
	m_LightingDebugCheckButton->SetVisible(activepage == m_EffectsPages[LIGHTING_PAGE_INDEX]);

	//if this isnt visible then dont bother with the next code
	if (!IsVisible())
	{
		//call on tick for the lighting and convar page
		m_EffectsPages[LIGHTING_PAGE_INDEX]->OnTick();
		m_EffectsPages[CONVAR_PAGE_INDEX]->OnTick();

		return;
	}

	//call each page's on tick function
	for (int i = 0; i < m_EffectsPages.Count(); i++)
	{
		m_EffectsPages[i]->OnTick();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called when a file is loaded/saved
//---------------------------------------------------------------------------------
void CEffectsPanel::OnFileSelected(const char* pszFileName)
{
	//get the filename
	char filename[1028];
	Q_strcpy(filename, pszFileName);

	//create the keyvalues
	KeyValuesAD file(new KeyValues("AloneModFilter"));

	//check the type
	if (m_FileType == vgui::FileOpenDialogType_t::FOD_OPEN)
	{
		//load the file
		if (!file->LoadFromFile(g_pFullFileSystem, filename))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//get the text
			wchar_t* format = g_pVGuiLocalize->Find("#Amod_EffectsPanel_FailedToLoadPreset_Desc");

			//get the filename
			wchar_t name[256];
			g_pVGuiLocalize->ConvertANSIToUnicode(filename, name, sizeof(name));

			//get the output text
			wchar_t text[1024];
			swprintf(text, SIZE_OF_ARRAY(text), format, name);

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox(L"#Amod_Panel_Error", text);
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(this);
			return;
		}

		//read each page
		for (int i = 0; i < m_EffectsPages.Count(); i++)
			m_EffectsPages[i]->ReadFromFile(file, true);
	}
	else
	{
		//ensure .amf extention
		if (Q_strcmp(Q_GetFileExtension(filename), "amf"))
			Q_SetExtension(filename, "amf", sizeof(filename));

		//write each page
		for (int i = 0; i < m_EffectsPages.Count(); i++)
			m_EffectsPages[i]->WriteToFile(file);

		if (!file->SaveToFile(g_pFullFileSystem, filename, nullptr, false, true))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//get the text
			wchar_t* format = g_pVGuiLocalize->Find("#Amod_EffectsPanel_FailedToSavePreset_Desc");

			//get the filename
			wchar_t name[256];
			g_pVGuiLocalize->ConvertANSIToUnicode(filename, name, sizeof(name));

			//get the output text
			wchar_t text[1024];
			swprintf(text, SIZE_OF_ARRAY(text), format, name);

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox(L"#Amod_Panel_Error", text);
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(this);
			return;
		}
	}
}

//effects panel interface
class CEffectsPanelInterface : public IEffectsPanel
{
public:
	virtual void Create(vgui::VPANEL parent);
	virtual void Destroy();
	virtual void ToggleVisibility();

	//autoload stuff
	virtual void ResetEverything();
	virtual void LoadFile(const char* filepath);
	virtual void CallOnTick();
private:
	//effects panel instance
	CEffectsPanel* m_EffectsPanelInstance = nullptr;
};

//---------------------------------------------------------------------------------
// Purpose: Creates the effects panel
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::Create(vgui::VPANEL parent)
{
	//destroy the old one if needed
	Destroy();

	//create a new effects panel
	m_EffectsPanelInstance = new CEffectsPanel(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Destroys the effects panel
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::Destroy()
{
	if (m_EffectsPanelInstance)
	{
		delete m_EffectsPanelInstance;
		m_EffectsPanelInstance = nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Toggles the visibility of the effects panel
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::ToggleVisibility()
{
	//check for effects panel
	if (!m_EffectsPanelInstance)
		return;

	//set visibility
	m_EffectsPanelInstance->SetVisible(!m_EffectsPanelInstance->IsVisible());

	//if visible then move to center of the screen
	if (m_EffectsPanelInstance->IsVisible())
	{
		m_EffectsPanelInstance->MoveToCenterOfScreen();
		m_EffectsPanelInstance->MoveToFront();
		m_EffectsPanelInstance->vgui::Panel::RequestFocus();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Resets everything
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::ResetEverything()
{
	//check for effects panel
	if (!m_EffectsPanelInstance)
		return;

	m_EffectsPanelInstance->ResetEverything();
}

//---------------------------------------------------------------------------------
// Purpose: Loads a file
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::LoadFile(const char* filepath)
{
	//check for effects panel
	if (!m_EffectsPanelInstance)
		return;

	m_EffectsPanelInstance->LoadFile(filepath);
}

//---------------------------------------------------------------------------------
// Purpose: Calls on tick for every page
//---------------------------------------------------------------------------------
void CEffectsPanelInterface::CallOnTick()
{
	//check for effects panel
	if (!m_EffectsPanelInstance)
		return;

	m_EffectsPanelInstance->CallOnTick();
}

//effects panel singleton
static CEffectsPanelInterface s_EffectsPanelInterface;
IEffectsPanel* g_EffectsPanelInterface = &s_EffectsPanelInterface;

//toggles the effects panel
CON_COMMAND(toggleeffectspanel, "Toggles the alone mod effects panel")
{
	s_EffectsPanelInterface.ToggleVisibility();
}