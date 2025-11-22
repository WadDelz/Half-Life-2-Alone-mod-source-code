//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main menu page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_OptionsPage.h"
#include "GG_MainMenuPage.h"
#include "fmtstr.h"

//vgui headers
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/QueryBox.h"

#define OPTIONS_RES_FILENAME "resource/geo_guesser/options.res"

#define SELECT_ALL_COMMAND "SelectAll"
#define DESELECT_ALL_COMMAND "DeselectAll"
#define NAVIGATE_BACK_COMMAND "NavigateBack"

//---------------------------------------------------------------------------------
// Purpose: Constructor for main manu page
//---------------------------------------------------------------------------------
CGG_Options_Page::CGG_Options_Page(CGG_MainPanel* parent)
	: I_GG_Page(parent, OPTIONS_PAGE_NAME)
{
	//the elements
	AddChild((m_MapList = new CGG_MapListPanel(parent, "MapList")));
	AddChild((m_RoundsLabel = new vgui::Label(parent, "RoundsText", CFmtStr("Number Of Rounds: %d", parent->GetGGInfo().rounds))));
	AddChild((m_RoundsSlider = new vgui::Slider(parent, "RoundsSlider")));
	AddChild(new vgui::Button(parent, "SelectAllButton", "Select All", parent, SELECT_ALL_COMMAND));
	AddChild(new vgui::Button(parent, "DeselectAllButton", "Deselect All", parent, DESELECT_ALL_COMMAND));
	AddChild(new vgui::Button(parent, "NavigateBackButton", "Navigate Back", parent, NAVIGATE_BACK_COMMAND));

	//set the rounds slider
	m_RoundsSlider->SetRange(2, 30);
	m_RoundsSlider->SetValue(parent->GetGGInfo().rounds);
	m_RoundsSlider->AddActionSignalTarget(parent);

	//load OPTIONS_RES_FILENAME on navigate to
	SetSettingsFile(OPTIONS_RES_FILENAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_Options_Page::OnCommand(const char* command)
{
	//check for navigate back command
	if (!Q_strcmp(command, NAVIGATE_BACK_COMMAND))
	{
		//see if we have any maps selected
		if (!m_MapList->HasAnySelected())
		{
			GetPanel()->ShowError("Error", "Error: You must select atleast 1 map!");
			return;
		}

		GetPanel()->NavigateToPage(MAINMENU_PAGE_NAME);
	}

	//check for select all command
	else if (!Q_strcmp(command, SELECT_ALL_COMMAND))
		m_MapList->SelectAll(true);
	
	//check for deselect all command
	else if (!Q_strcmp(command, DESELECT_ALL_COMMAND))
		m_MapList->SelectAll(false);
}

//---------------------------------------------------------------------------------
// Purpose: Called when a slider is moved
//---------------------------------------------------------------------------------
void CGG_Options_Page::OnSliderMoved(vgui::Panel* slider)
{
	//set the rounds
	GetPanel()->GetGGInfo().rounds = m_RoundsSlider->GetValue();

	//set the rounds label
	m_RoundsLabel->SetText(CFmtStr("Number Of Rounds: %d", m_RoundsSlider->GetValue()));
}