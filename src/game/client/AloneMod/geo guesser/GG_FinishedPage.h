//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Page you go to after finishing the game.
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __GG_FINISHEDPAGE_H
#define __GG_FINISHEDPAGE_H

#ifdef WIN32
#pragma once
#endif

//headers
#include "GG_Page.h"
#include "vgui_controls/Label.h"

//name for the main menu page
#define FINISHED_PAGE_NAME "FinishedPage"

//main menu page
class CGG_Finished_Page : public I_GG_Page
{
public:
	//constructor and destructor
	CGG_Finished_Page(CGG_MainPanel* parent);

	//called on command (e.g when a button is pressed)
	void OnCommand(const char* command) override;

	//post navigated to this panel
	void PostNavigateTo() override;
private:
	//labels
	vgui::Label* m_YourScoreLabel;
	vgui::Label* m_HighScoreLabel;
};

#endif //__GG_FINISHEDPAGE_H