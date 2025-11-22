//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main menu page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __GG_MAINMENUPAGE_H
#define __GG_MAINMENUPAGE_H

#ifdef WIN32
#pragma once
#endif

//headers
#include "GG_Page.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Label.h"

//name for the main menu page
#define MAINMENU_PAGE_NAME "MainMenu"

//main menu page
class CGG_MainMenu_Page : public I_GG_Page
{
public:
	//constructor and destructor
	CGG_MainMenu_Page(CGG_MainPanel* parent);

	//called on command (e.g when a button is pressed)
	void OnCommand(const char* command) override;
	
	//called on text changed
	void OnTextChanged() override;
	
	//called on navigated to
	void PostNavigateTo() override;
private:
	vgui::ComboBox* DifficultyComboBox;
	vgui::Label* m_HighScoreText;
};

#endif //__GG_MAINMENUPAGE_H