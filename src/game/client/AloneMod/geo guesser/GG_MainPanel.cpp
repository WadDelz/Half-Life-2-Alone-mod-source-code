//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main panel for the half-life 2 geoguesser
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "fmtstr.h"
#include "ienginevgui.h"
#include "filesystem.h"
#include "GG_MainPanel.h"
#include "vgui/ISurface.h"
#include "vgui_controls/QueryBox.h"

//all the pages
#include "GG_MainMenuPage.h"
#include "GG_OptionsPage.h"
#include "GG_GamePage.h"
#include "GG_FinishedPage.h"

//static geo-guesser singleton page
static CGG_MainPanel* gs_GeoGuesserMainPage = nullptr;

#define GEO_GUESSER_CONFIG_FILE "cfg/geo_guesser_config.cfg"
#define GEO_GUESSER_MAPS_FOLDER "resource/geo_guesser/maps/"
#define GEO_GUESSER_MAPS_MACROS_FILENAME "macros.res"

//---------------------------------------------------------------------------------
// Purpose: Constructor for main geo-guesser panel.
//---------------------------------------------------------------------------------
CGG_MainPanel::CGG_MainPanel(const char* name)
	: BaseClass(nullptr, name), m_SymbolTable(16, 32, true)
{
	//set the parent
	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	//set default stuff
	SetMoveable(true);
	SetSizeable(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//move this to the center of the screen
	MoveToCenterOfScreen();

	//get the map data
	LoadAllMapData();

	//read the config
	ReadFromConfig();

	//add each page
	AddPage(new CGG_MainMenu_Page(this));
	AddPage(new CGG_Options_Page(this));
	AddPage(new CGG_Game_Page(this));
	AddPage(new CGG_Finished_Page(this));

	//see if we have ANY maps that are selected. If not then navigate to the options panel first instead of the main menu
	bool AnySelected = false;
	for (int i = 0; i < m_Info.mapdata.Count(); i++)
	{
		if (m_Info.mapdata[i]->enabled)
		{
			//see if we have any children that are enabled
			for (int j = 0; j < m_Info.mapdata[i]->MapLocations.Count(); j++)
			{
				if (m_Info.mapdata[i]->MapLocations[j].enabled)
				{
					AnySelected = true;
					break;
				}
			}

			//break if we found a child that is enabled
			if (AnySelected)
				break;
		}
	}

	//navigate to the main menu if we have any map selected (index 0. change if you change the AddPage order)
	if (AnySelected)
	{
		m_Pages[0]->NavigateTo();
	}
	else
	{
		//navigate to the options panel and show an error(index 1. change if you change the AddPage order)
		m_Pages[1]->NavigateTo();
		ShowError("Choose A Map", "There are currently no map selected.\nYou must first select maps you want in your game to play", false);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Destructor for main geo-guesser panel
//---------------------------------------------------------------------------------
CGG_MainPanel::~CGG_MainPanel()
{
	//write the config
	WriteToConfig();

	//delete all pages
	for (int i = 0; i < m_Pages.Count(); i++)
		delete m_Pages[i];

	//delete all the map data
	for (int i = 0; i < m_Info.mapdata.Count(); i++)
		delete m_Info.mapdata[i];

	//clear the string table
	m_SymbolTable.RemoveAll();
}

//---------------------------------------------------------------------------------
// Purpose: Writes the settings to a config file
//---------------------------------------------------------------------------------
void CGG_MainPanel::WriteToConfig()
{
	//make the keyvalues
	KeyValues* file = new KeyValues("config");

	//set the difficulty and rounds
	file->SetInt("difficulty", m_Info.difficulty);
	file->SetInt("rounds", m_Info.rounds);
	file->SetInt("high_score", m_Info.high_score);

	//now set all the map states
	KeyValues* map_states = new KeyValues("map_states");
	for (int i = 0; i < m_Info.mapdata.Count(); i++)
	{
		//get the map name
		const char* mapname = m_SymbolTable.String(m_Info.mapdata[i]->MapName);

		//make map states subkey
		KeyValues* substate = new KeyValues(mapname, "enabled", m_Info.mapdata[i]->enabled);

		//write sub map states
		for (int j = 0; j < m_Info.mapdata[i]->MapLocations.Count(); j++)
			substate->SetBool(m_SymbolTable.String(m_Info.mapdata[i]->MapLocations[j].image), m_Info.mapdata[i]->MapLocations[j].enabled);

		//add into map states
		map_states->AddSubKey(substate);
	}

	//add the map states
	file->AddSubKey(map_states);

	//save the file
	file->SaveToFile(filesystem, GEO_GUESSER_CONFIG_FILE, "MOD");

	//delete the keyvalues
	file->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Reads the settings from a config file
//---------------------------------------------------------------------------------
void CGG_MainPanel::ReadFromConfig()
{
	//make the keyvalues and load the file
	KeyValues* file = new KeyValues("config");
	if (!file->LoadFromFile(filesystem, GEO_GUESSER_CONFIG_FILE, "MOD"))
	{
		file->deleteThis();
		return;
	}

	//get the difficulty and rounds
	m_Info.difficulty = file->GetInt("difficulty");
	m_Info.rounds = file->GetInt("rounds", 5);
	m_Info.high_score = file->GetInt("high_score");

	//now get all the map states
	KeyValues* map_states = file->FindKey("map_states");
	if (map_states)
	{
		//go through each subkey
		FOR_EACH_TRUE_SUBKEY(map_states, map)
		{
			//get the name
			const char* name = map->GetName();

			//check the map data for a matching name
			for (int i = 0; i < m_Info.mapdata.Count(); i++)
			{
				if (!Q_strcmp(name, m_SymbolTable.String(m_Info.mapdata[i]->MapName)))
				{
					//set enabled
					m_Info.mapdata[i]->enabled = map->GetBool("enabled", true);

					//get sub states
					for (int j = 0; j < m_Info.mapdata[i]->MapLocations.Count(); j++)
						m_Info.mapdata[i]->MapLocations[j].enabled = map->GetBool(m_SymbolTable.String(m_Info.mapdata[i]->MapLocations[j].image), true);

					break;
				}
			}
		}
	}

	//delete the keyvalues
	file->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Initalizes all the geo-guesser map files
//---------------------------------------------------------------------------------
void CGG_MainPanel::LoadAllMapData()
{
	//find the first file
	FileFindHandle_t handle;
	const char* filename = filesystem->FindFirstEx(GEO_GUESSER_MAPS_FOLDER "*", "MOD", &handle);

	//if nothing found then exit
	if (handle == FILESYSTEM_INVALID_FIND_HANDLE || !filename)
		return;

	//load the macro files
	KeyValues* macros = new KeyValues("$Macros");
	macros->LoadFromFile(filesystem, GEO_GUESSER_MAPS_FOLDER GEO_GUESSER_MAPS_MACROS_FILENAME, "MOD");

	//loop through all files
	do
	{
		//skip "." and ".."
		if (!Q_strcmp(filename, ".") || !Q_strcmp(filename, ".."))
			continue;

		//must be .res and not the macros file
		if (Q_strcmp(Q_GetFileExtension(filename), "res") || !Q_stricmp(filename, GEO_GUESSER_MAPS_MACROS_FILENAME))
			continue;

		//make the filepath
		char path[FILENAME_MAX];
		Q_snprintf(path, sizeof(path), GEO_GUESSER_MAPS_FOLDER "%s", filename);

		//load the file
		KeyValues* keyvalues = new KeyValues("MapFile");
		if (keyvalues->LoadFromFile(filesystem, path, "MOD"))
			InitalizeMapData(keyvalues, macros);

		//delete the keyvalues
		keyvalues->deleteThis();
	} 
	while ((filename = filesystem->FindNext(handle)) != nullptr);

	//delete the macros
	macros->deleteThis();
}

//---------------------------------------------------------------------------------
// Purpose: Initalizes some geo-guesser map data from a file
//---------------------------------------------------------------------------------
void CGG_MainPanel::LoadDifficultyImages(KeyValues* images, KeyValues* macros, MapData_t::MapType_e type, MapData_t* item)
{
	//check for images pointer
	if (!images)
		return;

	//check for subkey
	if (!images->GetFirstSubKey())
	{
		//see if we find the image in the macros file
		const char* macro_image = nullptr;
		if ((macro_image = macros->GetString(images->GetString(), nullptr)) == nullptr)
			macro_image = images->GetString();

		//just get the image
		MapData_t::MapImages_t& image = item->MapImages[type][item->MapImages[type].AddToTail()];
		image.MapImage = m_SymbolTable.AddString(macro_image);
	}

	//go through each subkey and add the images and names
	else
	{
		FOR_EACH_VALUE(images, value)
		{
			//check to see if we should use macros
			if (!Q_strcmp(value->GetName(), "$UseMacro$") && macros)
			{
				LoadDifficultyImages(macros->FindKey(value->GetString()), macros, type, item);
			}
			else
			{
				//store the macro name and image name
				const char* macro_name = nullptr;
				const char* macro_image = nullptr;

				//see if we find it in the macros file
				if ((macro_name = macros->GetString(value->GetName(), nullptr)) == nullptr)
					macro_name = value->GetName();
				
				//see if we find it in the macros file
				if ((macro_image = macros->GetString(value->GetString(), nullptr)) == nullptr)
					macro_image = value->GetString();

				UtlSymId_t MapName = m_SymbolTable.AddString(macro_name);

				//check for already existing map name
				bool ShouldBreak = false;
				for (int i = 0; i < item->MapImages[type].Count(); i++)
				{
					if (item->MapImages[type][i].MapName == MapName)
					{
						//already existing item was found
						ShouldBreak = true;
						break;
					}
				}

				//should we break?
				if (ShouldBreak)
					continue;

				//get the image and name
				MapData_t::MapImages_t& image = item->MapImages[type][item->MapImages[type].AddToTail()];
				image.MapImage = m_SymbolTable.AddString(macro_image);
				image.MapName = MapName;
			}
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Initalizes some geo-guesser map data from a file
//---------------------------------------------------------------------------------
void CGG_MainPanel::InitalizeMapData(KeyValues* file, KeyValues* macros)
{
	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(file, map)
	{
		//see if the map already exists
		bool ShouldBreak = false;
		for (int i = 0; i < m_Info.mapdata.Count(); i++)
		{
			if (!Q_strcmp(m_SymbolTable.String(m_Info.mapdata[i]->MapName), map->GetName()))
			{
				ShouldBreak = true;
				break;
			}
		}

		//should we break?
		if (ShouldBreak)
			break;

		//check map name for :
		if (Q_strrchr(map->GetName(), ':'))
			continue;

		MapData_t* item = new MapData_t;

		//set the map name
		item->MapName = m_SymbolTable.AddString(map->GetName());

		//get images subkey
		KeyValues* images = map->FindKey("Images");
		if (images)
		{
			//load the difficulty images
			LoadDifficultyImages(images->FindKey("EasyImages"), macros, MapData_t::MapType_e::Easy, item);
			LoadDifficultyImages(images->FindKey("MediumImages"), macros, MapData_t::MapType_e::Medium, item);
			LoadDifficultyImages(images->FindKey("HardImages"), macros, MapData_t::MapType_e::Hard, item);
		}

		//if no image data for the easy, medium or hard mode then delete the item
		if (item->MapImages[MapData_t::MapType_e::Easy].Count() <= 0 || item->MapImages[MapData_t::MapType_e::Medium].Count() <= 0 || item->MapImages[MapData_t::MapType_e::Hard].Count() <= 0)
		{
			//warning
			ConWarning("Error: Geo-Guesser Failed to load minimap images data for map: \"%s\". Deleting item!\n", map->GetName());

			//delet the item
			delete item;
			continue;
		}

		//now get the positions
		FOR_EACH_TRUE_SUBKEY(map, position)
		{
			//check for 'Images'. This subkey is used for the images
			if (!Q_strcmp(position->GetName(), "Images"))
				continue;

			//check position name for :
			if (Q_strrchr(position->GetName(), ':'))
				continue;

			//create the location data
			MapData_t::MapLocation_t& location = item->MapLocations[item->MapLocations.AddToTail()];

			//get the image
			location.image = m_SymbolTable.AddString(position->GetName());

			//get easy name
			const char* name = macros->GetString(position->GetString("EasyName", nullptr));
			if (!name)
				name = position->GetString("EasyName");

			location.actuall_map[MapData_t::MapType_e::Easy] = m_SymbolTable.AddString(name);

			//get medium name
			name = macros->GetString(position->GetString("MediumName", nullptr));
			if (!name)
				name = position->GetString("MediumName");

			location.actuall_map[MapData_t::MapType_e::Medium] = m_SymbolTable.AddString(name);
				
			//get hard name
			name = macros->GetString(position->GetString("HardName", nullptr));
			if (!name)
				name = position->GetString("HardName");

			location.actuall_map[MapData_t::MapType_e::Hard] = m_SymbolTable.AddString(name);

			//get the positions
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Easy].Base(), 2, position->GetString("Easy", "0 0"));
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Medium].Base(), 2, position->GetString("Medium", "0 0"));
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Hard].Base(), 2, position->GetString("Hard", "0 0"));
		}

		//if no position data then delete the data
		if (item->MapLocations.Count() <= 0)
		{
			//warning
			ConWarning("Error: Geo-Guesser Failed to get any locations for map: \"%s\". Deleting item!\n", map->GetName());

			delete item;
			continue;
		}

		//add the item to the map datas
		m_Info.mapdata.AddToTail(item);
		continue;	
	}
}

//---------------------------------------------------------------------------------
// Purpose: Shows an error message
//---------------------------------------------------------------------------------
void CGG_MainPanel::ShowError(const char* title, const char* message, bool playsound)
{
	//play an error sound
	if (playsound)
		vgui::surface()->PlaySound("resource/warning.wav");

	//show the error message
	vgui::QueryBox* error = new vgui::QueryBox(title, message, nullptr);
	error->SetOKButtonText("Ok");
	error->SetCancelButtonVisible(false);
	error->DoModal(this);
	error->MoveToCenterOfScreen();
}

//---------------------------------------------------------------------------------
// Purpose: Navigates to a page
//---------------------------------------------------------------------------------
void CGG_MainPanel::NavigateToPage(I_GG_Page* page)
{
	//check for the page first
	if (m_Pages.Find(page) == m_Pages.InvalidIndex())
		return;

	//go through all the pages and set the visibility
	for (int i = 0; i < m_Pages.Count(); i++)
		m_Pages[i]->SetVisible(m_Pages[i] == page);
}

//---------------------------------------------------------------------------------
// Purpose: Navigates to a page
//---------------------------------------------------------------------------------
void CGG_MainPanel::NavigateToPage(const char* PageName)
{
	//go through all the pages and set the visibility
	for (int i = 0; i < m_Pages.Count(); i++)
	{
		if (!Q_stricmp(m_Pages[i]->GetName(), PageName))
		{
			//navigate to the page. This then calls NavigateToPage(I_GG_Page* page)
			m_Pages[i]->NavigateTo();
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Adds a page
//---------------------------------------------------------------------------------
void CGG_MainPanel::AddPage(I_GG_Page* page)
{
	//check for the page first
	if (!page || m_Pages.Find(page) != m_Pages.InvalidIndex())
		return;

	//add the page
	m_Pages.AddToTail(page);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_MainPanel::OnCommand(const char* command)
{
	//close is a reserved command
	if (!Q_stricmp(command, "close"))
		return BaseClass::OnCommand(command);

	//see which page is active and send the command to that page
	for (int i = 0; i < m_Pages.Count(); i++)
	{
		if (m_Pages[i]->IsVisible())
		{
			m_Pages[i]->OnCommand(command);
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on text/combo box changed
//-----------------------------------------------------------------------------
void CGG_MainPanel::OnTextChanged(KeyValues* kv)
{
	//see which page is active and send the command to that page
	for (int i = 0; i < m_Pages.Count(); i++)
	{
		if (m_Pages[i]->IsVisible())
		{
			m_Pages[i]->OnTextChanged();
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when a slider is moved
//-----------------------------------------------------------------------------
void CGG_MainPanel::OnSliderMoved(vgui::Panel* panel)
{
	//see which page is active and send the command to that page
	for (int i = 0; i < m_Pages.Count(); i++)
	{
		if (m_Pages[i]->IsVisible())
		{
			m_Pages[i]->OnSliderMoved(panel);
			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Command to toggle the geo-guesser main page
//---------------------------------------------------------------------------------
CON_COMMAND(gg_toggle, "Toggles the hl2 geo-guesser panel")
{
	//check for the geo-guesser panel
	if (!gs_GeoGuesserMainPage)
		gs_GeoGuesserMainPage = new CGG_MainPanel("Geo-GuesserPanel");

	//move/set the panel
	gs_GeoGuesserMainPage->SetVisible(!gs_GeoGuesserMainPage->IsVisible());
	gs_GeoGuesserMainPage->MoveToCenterOfScreen();
	gs_GeoGuesserMainPage->RequestFocus();
	gs_GeoGuesserMainPage->MoveToFront();
}

//---------------------------------------------------------------------------------
// Purpose: Command to reset and open the geo-guesser main page
//---------------------------------------------------------------------------------
CON_COMMAND(gg_reset_open, "Resets and opens the hl2 geo-guesser panel")
{
	if (gs_GeoGuesserMainPage)
		delete gs_GeoGuesserMainPage;

	//check for the geo-guesser panel
	gs_GeoGuesserMainPage = new CGG_MainPanel("Geo-GuesserPanel");

	//move/set the panel
	gs_GeoGuesserMainPage->SetVisible(!gs_GeoGuesserMainPage->IsVisible());
	gs_GeoGuesserMainPage->MoveToCenterOfScreen();
	gs_GeoGuesserMainPage->RequestFocus();
	gs_GeoGuesserMainPage->MoveToFront();
}