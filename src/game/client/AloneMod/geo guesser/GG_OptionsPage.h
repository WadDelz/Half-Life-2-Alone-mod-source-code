//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Options page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
//#ifndef __GG_OPTIONSPAGE_H
//#define __GG_OPTIONSPAGE_H

#ifdef WIN32
#pragma once
#endif

//headers
#include "GG_Page.h"
#include "GG_MapList.h"
#include "vgui_controls/Slider.h"

//name for the main menu page
#define OPTIONS_PAGE_NAME "OptionsPage"

//main menu page
class CGG_Options_Page : public I_GG_Page
{
public:
	//constructor and destructor
	CGG_Options_Page(CGG_MainPanel* parent);

	//called on command (e.g when a button is pressed)
	void OnCommand(const char* command) override;

	//called on slider moved
	void OnSliderMoved(vgui::Panel* slider) override;
private:
	//map list
	CGG_MapListPanel* m_MapList;

	//rounds stuff
	vgui::Label *m_RoundsLabel;
	vgui::Slider* m_RoundsSlider;
};

//#endif //__GG_OPTIONSPAGE_H