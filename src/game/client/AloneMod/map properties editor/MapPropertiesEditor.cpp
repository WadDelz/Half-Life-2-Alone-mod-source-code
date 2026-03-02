#include "cbase.h"
#include "MapPropertiesEditorMenuPanel.h"

//convars
ConVar map_properties_editor_load_mod("map_properties_editor_load_mod", "", FCVAR_DEVELOPMENTONLY, "The path inside resource/time_editor/<map_properties_editor_load_mod>/* that will get edited instead of resource/time_editor/*");
extern ConVar amod_timeinfo_load_mod;






//add more themes if you would like
TimePropertyTheme_t g_TimePropertyThemes[NUM_TIME_PROPERTY_THEMES] = {
	{"#MapProperties_DarkTheme", "resource/panels/MapPropertiesEditor/MapPropertiesEditorDarkScheme.res", "MapPropertiesEditorDarkScheme"},
	{"#MapProperties_LightTheme", "resource/panels/MapPropertiesEditor/MapPropertiesEditorLightScheme.res", "MapPropertiesEditorLightScheme"},
	{"#MapProperties_DefaultTheme", "resource/panels/MapPropertiesEditor/MapPropertiesEditorDefaultScheme.res", "MapPropertiesEditorDefaultScheme"},
};

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the amod_time_properties_editor_theme convar is changed
//----------------------------------------------------------------------------------------------------
void AmodTimePropertiesThemeChangeCallback(IConVar* convar, const char*, float)
{
	//check for s_MapPropertiesEditorPanel
	if (s_MapPropertiesEditorPanel)
	{
		s_MapPropertiesEditorPanel->SetScheme(GetTimePropertiesScheme());
		s_MapPropertiesEditorPanel->InvalidateLayout(false, true);
	}

	//check for g_MapPropertiesPanel
	if (g_MapPropertiesPanel)
	{
		g_MapPropertiesPanel->SetScheme(GetTimePropertiesScheme());
		g_MapPropertiesPanel->InvalidateLayout(false, true);
	}
}

//returns the current theme of the time properties editor
ConVar amod_time_properties_editor_theme("amod_time_properties_editor_theme", "0", FCVAR_ARCHIVE, "0 = dark, 1 = light, 2 = default vgui theme", true, 0, true, NUM_TIME_PROPERTY_THEMES, AmodTimePropertiesThemeChangeCallback);

//----------------------------------------------------------------------------------------------------
// Purpose: Returns the scheme using the amod_time_properties_editor_theme convar
//----------------------------------------------------------------------------------------------------
HScheme GetTimePropertiesScheme()
{
	TimePropertyTheme_t& theme = g_TimePropertyThemes[amod_time_properties_editor_theme.GetInt()];
	return scheme()->LoadSchemeFromFile(theme.filename, theme.tag);
}





//has the time properties panel been opened
bool g_bHasTimePropertiesPanelBeenOpen = false;

//command to open the map time properties editor
CON_COMMAND(open_map_time_properties_editor, "")
{
	//delete if needed
	if (atoi(args.Arg(1)) != 0)
	{
		//delete the panel
		if (s_MapPropertiesEditorPanel)
		{
			delete s_MapPropertiesEditorPanel;
			s_MapPropertiesEditorPanel = nullptr;
		}
		else
			return;
	}

	//get the folder
	const char* mod = map_properties_editor_load_mod.GetString();

	//we always want to modify the map properties in the "resource/time_info" directory (or resource/time_info/<map_properties_editor_load_mod> directory if specified). 
	//Incase 'amod_timeinfo_load_mod' isnt "" or map_properties_editor_load_mod. Then Set it to that.
	while (Q_stricmp(amod_timeinfo_load_mod.GetString(), mod))
	{
		//check the folder exists
		const char* folder = CFmtStr("resource/time_info%s%s", mod[0] ? "/" : "", mod);
		if (!filesystem->IsDirectory(folder, "MOD"))
		{
			//reset map_properties_editor_load_mod (obviously its an invalid path)
			map_properties_editor_load_mod.SetValue("");

			//check the amod_timeinfo_load_mod isnt empty
			mod = "";
			if (!Q_stricmp(amod_timeinfo_load_mod.GetString(), mod))
				break;
		}

		//reload the timeinfo
		amod_timeinfo_load_mod.SetValue(mod);
		cvar->FindCommand("amod_timeinfo_reset")->Dispatch(CCommand{});

		//we need to delete the panel now so the data gets reset
		if (s_MapPropertiesEditorPanel)
		{
			delete s_MapPropertiesEditorPanel;
			s_MapPropertiesEditorPanel = nullptr;
		}

		break;
	}

	//create the panel if it doesnt already exist
	if (!s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel = new CMapPropertiesEditorPanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	s_MapPropertiesEditorPanel->Activate();

	//we have been open
	g_bHasTimePropertiesPanelBeenOpen = true;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Returns if the map properties panel is open or not
//----------------------------------------------------------------------------------------------------
const bool IsMapPropertiesPanelOpen()
{
	return g_MapPropertiesPanel != nullptr;
}