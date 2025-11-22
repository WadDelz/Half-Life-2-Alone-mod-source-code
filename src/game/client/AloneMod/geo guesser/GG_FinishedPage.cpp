//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Page you go to after finishing the game.
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_FinishedPage.h"
#include "GG_MainMenuPage.h"
#include "GG_MiniMap.h"
#include "fmtstr.h"

//vgui headers
#include "vgui_controls/Button.h"

#define FINISHED_RES_FILENAME "resource/geo_guesser/finished_page.res"

#define FINISHED_PAGE_MAINMENU "NavigateToMainMenu"

//---------------------------------------------------------------------------------
// Purpose: Constructor for main manu page
//---------------------------------------------------------------------------------
CGG_Finished_Page::CGG_Finished_Page(CGG_MainPanel* parent)
	: I_GG_Page(parent, FINISHED_PAGE_NAME)
{
	//icon
	CStretchingImage* icon = new CStretchingImage(parent, "IconImage");
	icon->SetImage("geo_guesser/icon");
	AddChild(icon);

	//add stuff
	AddChild((m_YourScoreLabel = new vgui::Label(parent, "YourScoreLabel", "Your Score: 0")));
	AddChild((m_HighScoreLabel = new vgui::Label(parent, "HighScoreLabel", "High Score: 0")));
	AddChild(new vgui::Button(parent, "GoBackButton", "Back To Menu", parent, FINISHED_PAGE_MAINMENU));

	//load FINISHED_RES_FILENAME on navigate to
	SetSettingsFile(FINISHED_RES_FILENAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_Finished_Page::OnCommand(const char* command)
{
	//check for options command
	if (!Q_strcmp(command, FINISHED_PAGE_MAINMENU))
		GetPanel()->NavigateToPage(MAINMENU_PAGE_NAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_Finished_Page::PostNavigateTo()
{
	//set score labels
	m_YourScoreLabel->SetText(CFmtStr("Your Score: %d", GetPanel()->GetGGInfo().current_score));
	m_HighScoreLabel->SetText(CFmtStr("High Score: %d", GetPanel()->GetGGInfo().high_score));
}