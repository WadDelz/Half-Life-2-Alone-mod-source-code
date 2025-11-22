//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main panel for the half-life 2 geoguesser
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __GG_MAINPANEL_H
#define __GG_MAINPANEL_H

#ifdef WIN32
#pragma once
#endif

//vgui headers
#include "vgui_controls/Frame.h"
#include "utlsymbol.h"

//main geo-guesser panel.
class CGG_MainPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CGG_MainPanel, vgui::Frame)
public:
	//constructor and destructor
	CGG_MainPanel(const char* name);
	~CGG_MainPanel();

	//config stuff
	void WriteToConfig();
	void ReadFromConfig();

	//map funcs
	void LoadAllMapData();
	void InitalizeMapData(KeyValues* file);

	//error message
	void ShowError(const char* title, const char* message, bool playsound = true);

	//page functions
	void NavigateToPage(class I_GG_Page* page);
	void NavigateToPage(const char* PageName);

	void AddPage(class I_GG_Page* page);

	//other panel functions
	void OnCommand(const char* pszCommand);

	//message funcs
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PTR(OnSliderMoved, "SliderMoved", panel);
private:
	//pages
	CUtlVector<class I_GG_Page*> m_Pages;

private:
	friend class CGG_Game_Page;

	//map data struct
	struct MapData_t
	{
		//map name
		UtlSymId_t MapName;

		//enabled
		bool enabled = true;

		//map position enum
		enum MapType_e
		{
			Easy,
			Medium,
			Hard,
			Count
		};

		//map location struct
		struct MapLocation_t
		{
			Vector2D positions[MapType_e::Count];
			UtlSymId_t image;
		};
		CUtlVector<MapLocation_t> MapLocations;

		//easy/medium/hard map images
		UtlSymId_t MapImages[MapType_e::Count];
	};

	//string table for map names/images
	CUtlSymbolTable m_SymbolTable;

public:
	//geo-guesser info struct
	struct GeoGuesserInfo_t
	{
		int difficulty = 0;
		int rounds = 5;
		int rounds_played = 0;
		int high_score = 0;
		int current_score = 0;
		CUtlVector<MapData_t*> mapdata;
	} m_Info;

	//returns the config info
	inline GeoGuesserInfo_t& GetGGInfo() { return m_Info; };

	//symbol stuff
	inline const char* GetStringForSymbol(UtlSymId_t id) { return m_SymbolTable.String(id); }
};

#endif //__GG_MAINPANEL_H