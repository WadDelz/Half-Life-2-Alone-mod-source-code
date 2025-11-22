//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main panel for the half-life 2 geoguesser
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
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
			AnySelected = true;
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
		map_states->SetBool(m_SymbolTable.String(m_Info.mapdata[i]->MapName), m_Info.mapdata[i]->enabled);

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
		//go through each value
		FOR_EACH_VALUE(map_states, state)
		{
			//get the name
			const char* name = state->GetName();

			//check the map data for a matching name
			for (int i = 0; i < m_Info.mapdata.Count(); i++)
			{
				if (!Q_strcmp(name, m_SymbolTable.String(m_Info.mapdata[i]->MapName)))
				{
					m_Info.mapdata[i]->enabled = state->GetBool();
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

	//loop through all files
	do
	{
		//skip "." and ".."
		if (!Q_strcmp(filename, ".") || !Q_strcmp(filename, ".."))
			continue;

		//must be .res
		if (Q_strcmp(Q_GetFileExtension(filename), "res"))
			continue;

		//make the filepath
		char path[FILENAME_MAX];
		Q_snprintf(path, sizeof(path), GEO_GUESSER_MAPS_FOLDER "%s", filename);

		//load the file
		KeyValues* keyvalues = new KeyValues("MapFile");
		if (keyvalues->LoadFromFile(filesystem, path, "MOD"))
			InitalizeMapData(keyvalues);

		//delete the keyvalues
		keyvalues->deleteThis();
	} 
	while ((filename = filesystem->FindNext(handle)) != nullptr);
}

//---------------------------------------------------------------------------------
// Purpose: Initalizes some geo-guesser map data from a file
//---------------------------------------------------------------------------------
void CGG_MainPanel::InitalizeMapData(KeyValues* file)
{
	//go through all the subkeys. The format is like this:
	//
	//"mapname"
	//{
	//    "positions"
	//    {
	//		  "example_position_imagepath"
	//		  {
	//		      "Easy" "0 20"
	//		      "Medium" "40 20"
	//		      "Hard" "120 -53"
	//        }
	//    }
	// 
	//	  "EasyImage" "mapname/easy"
	//	  "MediumImage" "mapname/medium"
	//	  "HardImage" "mapname/hard"
	//}
	//
	//The images will work like this: All folders should be located in 'materials/vgui/geo_guesser/*'.
	//
	// The 'EasyImage' 'MediumImage' and 'HardImage' image files will be located inside of 'materials/vgui/geo_guesser/full_maps/<keyvalue string>"
	// where <keyvalue string> is the string value of "EasyImage", "MediumImage" and "HardImage"
	//
	// When you make a map position the name of the subkey is the image path that you will use to try and find the position. The image path
	// starts in 'materials/vgui/geo_guesser/positions/<mapname>/*'.
	// Inside the subkey you need to make 3 keyvalue pairs for the "easy", "medium", and "hard", position in pixels.
	
	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(file, map)
	{
		//see if the map already exists
		if (m_SymbolTable.Find(map->GetName()) != UTL_INVAL_SYMBOL)
			continue;

		MapData_t* item = new MapData_t;

		//set the map name
		item->MapName = m_SymbolTable.AddString(map->GetName());

		//set the difficulty images
		item->MapImages[MapData_t::MapType_e::Easy] = m_SymbolTable.AddString(map->GetString("EasyImage"));
		item->MapImages[MapData_t::MapType_e::Medium] = m_SymbolTable.AddString(map->GetString("MediumImage"));
		item->MapImages[MapData_t::MapType_e::Hard] = m_SymbolTable.AddString(map->GetString("HardImage"));

		//now get the positions
		FOR_EACH_TRUE_SUBKEY(map, position)
		{
			//create the location data
			MapData_t::MapLocation_t& location = item->MapLocations[item->MapLocations.AddToTail()];

			//get the image
			location.image = m_SymbolTable.AddString(position->GetName());

			//get the positions
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Easy].Base(), 2, position->GetString("Easy", "0 0"));
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Medium].Base(), 2, position->GetString("Medium", "0 0"));
			UTIL_StringToFloatArray(location.positions[MapData_t::MapType_e::Hard].Base(), 2, position->GetString("Hard", "0 0"));
		}

		//if no position data then delete the data
		if (item->MapLocations.Count() <= 0)
		{
			delete item;
			continue;
		}

		//add the item to the map datas
		m_Info.mapdata.AddToTail(item);
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