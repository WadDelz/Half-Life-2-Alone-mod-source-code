#include "cbase.h"
#include "MapPropertiesEditorMenuPanel.h"

ConVar map_properties_editor_load_mod("map_properties_editor_load_mod", "", 0/*FCVAR_DEVELOPMENTONLY*/, "The path inside resource/time_editor/<map_properties_editor_load_mod>/* that will get edited instead of resource/time_editor/*");
extern ConVar amod_timeinfo_load_directory;

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
	const char* folder = CFmtStr("resource/time_info%s%s", mod[0] ? "/" : "", mod);

	//we always want to modify the map properties in the "resource/time_info" directory (or resource/time_info/<map_properties_editor_load_mod>). Incase 'amod_timeinfo_load_directory' isnt 
	//that. Then Set it to that.
	while (Q_stricmp(amod_timeinfo_load_directory.GetString(), folder))
	{
		//check the folder exists
		if (!filesystem->IsDirectory(folder, "MOD"))
		{
			//reset map_properties_editor_load_mod (obviously its an invalid path)
			map_properties_editor_load_mod.SetValue("");

			//check the amod_timeinfo_load_directory isnt resource/time_info
			folder = "resource/time_info";
			if (!Q_stricmp(amod_timeinfo_load_directory.GetString(), folder))
				break;
		}

		//reload the timeinfo
		amod_timeinfo_load_directory.SetValue(folder);
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
}

//----------------------------------------------------------------------------------------------------
// Purpose: Returns if the map properties panel is open or not
//----------------------------------------------------------------------------------------------------
const bool IsMapPropertiesPanelOpen()
{
	return g_MapPropertiesPanel != nullptr;
}