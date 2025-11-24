//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: The game page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
//#ifndef __GG_GAMEPAGE_H
//#define __GG_GAMEPAGE_H

#ifdef WIN32
#pragma once
#endif

//headers
#include "GG_MiniMap.h"
#include "GG_Page.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ComboBox.h"

//name for the main menu page
#define GAME_PAGE_NAME "Gamepage"

//main menu page
class CGG_Game_Page : public I_GG_Page
{
public:
	//constructor and destructor
	CGG_Game_Page(CGG_MainPanel* parent);

	//called on command (e.g when a button is pressed)
	void OnCommand(const char* command) override;

	//text changed
	void OnTextChanged();

	//called on navigated to
	void OnMapFinished();
	void NavigateTo() override;
	void PostNavigateTo() override;

	//sets the finished state of the current map position
	void SetFinishedState(bool state);
private:
	//search image
	CStretchingImage* m_CurrentFindFromImage;
	vgui::ImagePanel* m_CurrentFindFromImageBackground;

	//map combo box
	vgui::ComboBox* m_MapList;

	//minimap
	CGG_MiniMap* m_MiniMap;

	//top labels
	vgui::Label* m_CurrentScoreLabel;
	vgui::Label* m_CurrentRoundLabel;

	//buttons
	vgui::Button* m_SubmitAnswerButton;
	vgui::Button* m_SkipButtonm;
	vgui::Button* m_FinishButton;

	//map data list
	CUtlVector<CGG_MainPanel::MapData_t*> m_MapData;

	//map queue struct and array
	struct MapQueueItem_t
	{
		//map and position index
		int map;
		int position;
	};
	CUtlVector<MapQueueItem_t> m_MapQueue;

	//current selected map and position index
	int CurrentSelectedMap = 0;
	int CurrentSelectedPosition = 0;
};

//#endif //__GG_GAMEPAGE_H