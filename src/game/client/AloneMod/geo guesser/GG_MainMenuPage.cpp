//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main menu page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_MainMenuPage.h"
#include "GG_OptionsPage.h"
#include "GG_GamePage.h"
#include "GG_MiniMap.h"
#include "fmtstr.h"

//vgui headers
#include "vgui_controls/Button.h"
#include "vgui_controls/Divider.h"

#define MAINMENU_RES_FILENAME "resource/geo_guesser/main_menu.res"

#define MAINMENU_COMMAND_OPTIONS "NavigateToOptions"
#define MAINMENU_COMMAND_STARTGAME "NavigateToGamePage"

//---------------------------------------------------------------------------------
// Purpose: Constructor for main manu page
//---------------------------------------------------------------------------------
CGG_MainMenu_Page::CGG_MainMenu_Page(CGG_MainPanel* parent)
	: I_GG_Page(parent, MAINMENU_PAGE_NAME)
{
	//icon
	CStretchingImage* icon = new CStretchingImage(parent, "IconImage");
	icon->SetImage("geo_guesser/icon");
	AddChild(icon);

	//the buttons
	AddChild(new vgui::Button(parent, "StartNewGameButton", "Start New Game", parent, MAINMENU_COMMAND_STARTGAME));
	AddChild(new vgui::Button(parent, "ChangeSettingsButton", "Change Settings", parent, MAINMENU_COMMAND_OPTIONS));
	
	//difficulty stuff
	AddChild(new vgui::Label(parent, "DifficultyLabel", "Difficulty:"));
	AddChild((DifficultyComboBox = new vgui::ComboBox(parent, "DifficultyComboBox", 3, false)));
	
	//score stuff
	AddChild(new vgui::Divider(parent, "HighScoreDivider"));
	AddChild((m_HighScoreText = new vgui::Label(parent, "HighScoreText", CFmtStr("High Score: %d", parent->GetGGInfo().high_score))));

	//set the combo box data
	DifficultyComboBox->AddItem("Easy", nullptr);		//The map display will only show the map that the photo was take from
	DifficultyComboBox->AddItem("Medium", nullptr);		//The map display will show the chapter/section the photo was taken from
	DifficultyComboBox->AddItem("Hard", nullptr);		//The map display will show the entire map (like the hl2 map, ep1 map, ep2 map etc...)
	DifficultyComboBox->ActivateItem(parent->GetGGInfo().difficulty);

	//load MAINMENU_RES_FILENAME on navigate to
	SetSettingsFile(MAINMENU_RES_FILENAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_MainMenu_Page::OnCommand(const char* command)
{
	//check for options command
	if (!Q_strcmp(command, MAINMENU_COMMAND_OPTIONS))
		GetPanel()->NavigateToPage(OPTIONS_PAGE_NAME);
	
	//check for start game command
	if (!Q_strcmp(command, MAINMENU_COMMAND_STARTGAME))
		GetPanel()->NavigateToPage(GAME_PAGE_NAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called when a text is changed
//---------------------------------------------------------------------------------
void CGG_MainMenu_Page::OnTextChanged()
{
	GetPanel()->GetGGInfo().difficulty = DifficultyComboBox->GetActiveItem();
}

//---------------------------------------------------------------------------------
// Purpose: Called after this page is navigated to
//---------------------------------------------------------------------------------
void CGG_MainMenu_Page::PostNavigateTo()
{
	//then set the text
	m_HighScoreText->SetText(CFmtStr("High Score: %d", GetPanel()->GetGGInfo().high_score));
}