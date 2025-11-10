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

//commands
#define COMMAND_LOAD_EFFECTS "LoadEffects"
#define COMMAND_SAVE_EFFECTS "SaveEffects"
#define COMMAND_RESET_EFFECTS "ResetEffects"

//effects panel
class CEffectsPanel : public vgui::PropertyDialog, CAutoGameSystem
{
	DECLARE_CLASS_SIMPLE(CEffectsPanel, vgui::PropertyDialog)
public:	
	//constructor
	CEffectsPanel(vgui::VPANEL parent);

	//keyboard/mouse funcs
	void OnKeyCodePressed(vgui::KeyCode code);

	//called on map load
	void LevelInitPostEntity();

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
};

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

	//bottom buttons
	SetOKButtonVisible(true);
	SetOKButtonText("Reset");
	_okButton->SetEnabled(true);
	_okButton->SetCommand(COMMAND_RESET_EFFECTS);
	_okButton->SetReleasedSound("ui/buttonclickrelease.wav");

	SetCancelButtonVisible(true);
	SetCancelButtonText("Save");
	_cancelButton->SetEnabled(true);
	_cancelButton->SetCommand(COMMAND_SAVE_EFFECTS);
	_cancelButton->SetReleasedSound("ui/buttonclickrelease.wav");

	SetApplyButtonVisible(true);
	SetApplyButtonText("Load");
	_applyButton->SetEnabled(true);
	_applyButton->SetCommand(COMMAND_LOAD_EFFECTS);
	_applyButton->SetReleasedSound("ui/buttonclickrelease.wav");

	//add all the pages needed
	m_EffectsPages.AddToTail(new CEffectsPanelViewEffects(this, "View Effects"));
	m_EffectsPages.AddToTail(new CEffectsPanelConvarPage(this, "Console Variables"));
	m_EffectsPages.AddToTail(new CEffectsPanelOverlayPage(this, "Screen Overlays"));
	m_EffectsPages.AddToTail(new CEffectsPanelLightingPage(this, "Lighting"));

	//add each page
	for (int i = 0; i < m_EffectsPages.Count(); i++)
		AddPage(m_EffectsPages[i], m_EffectsPages[i]->GetName());

	//bounds and title
	SetTitle("Effects Panel", false);
	SetSize(EFFECTS_PANEL_WIDTH, EFFECTS_PANEL_HEIGHT);
	MoveToCenterOfScreen();

	vgui::ivgui()->AddTickSignal(GetVPanel(), 30);
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

	CAutoGameSystem::LevelInitPostEntity();
}

//---------------------------------------------------------------------------------
// Purpose: Called on panel command
//---------------------------------------------------------------------------------
void CEffectsPanel::OnCommand(const char* pszCommand)
{
	//check for reset
	if (!Q_strcmp(pszCommand, COMMAND_RESET_EFFECTS))
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
		m_FileDialog = new vgui::FileOpenDialog(this, "Save Preset", false);
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
		m_FileDialog = new vgui::FileOpenDialog(this, "Save Preset", true);
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

	//if this isnt visible then dont bother with the next code
	if (!IsVisible())
		return;

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

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr1024("Failed to load preset file:\n%s", filename));
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(this);
			return;
		}

		//read each page
		for (int i = 0; i < m_EffectsPages.Count(); i++)
			m_EffectsPages[i]->ReadFromFile(file);
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

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr1024("Failed to save preset to file:\n%s", filename));
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

//effects panel singleton
static CEffectsPanelInterface s_EffectsPanelInterface;
IEffectsPanel* g_EffectsPanelInterface = &s_EffectsPanelInterface;

//toggles the effects panel
CON_COMMAND(toggleeffectspanel, "Toggles the alone mod effects panel")
{
	s_EffectsPanelInterface.ToggleVisibility();
}