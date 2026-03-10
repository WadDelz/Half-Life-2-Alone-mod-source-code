#include "cbase.h"
#include "AloneMod/AmodCvars.h"
#include "AloneMod/Amod_SharedDefs.h"
#include "movevars_shared.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "engine/IEngineSound.h"

//------------------------------------------------------------------------------------------
// Purpose: Writes the config for the alone mod settings
//------------------------------------------------------------------------------------------
void Amod_WriteConfig()
{
	//cvar must be not null
	if (!cvar)
		return;

	//here is a list of all the commands to write to the cfg
	static const char* s_CommandsToWrite[] = {
		"amod_viewbob_enabled",						//viewbob
		"sv_rollangle",								//roll angle
		"amod_jump_punch_enable",					//jump punch
		"amod_land_punch_enable",					//launch punch
		"r_flashlightfar",							//flashlight far
		"r_flashlightfov",							//flashlight fov
		"filter_brightness_on",						//filter on brightness
		"filter_brightness_on_exp",					//filter on exponent
		"filter_brightness_off",					//filter off brightness
		"amod_flashlightlag",						//flashlight lag
		"amod_mirrored",							//mirrored world
		"amod_flashlightflicker",					//flashlight flicker
		"amod_soundscapes_disable",					//disable/enable soundscapes
		"amod_music_disable",						//disable/enable music
		"amod_songs_transition_through_levels",		//transition music through levels
		"sv_footsteps",								//enable/disable footstep sounds
		"hidehud",									//hide the hud or not
		"amod_fog_disabled",						//disable/enable the fog
		"amod_enable_god",							//god mode
		"amod_standbob_enabled",					//viewbob whilst standing

#if !AMOD_DAYTIME_EDITION
		"amod_day",									//daytime or not
#endif

		"amod_vignette",							//vignette
		"amod_do_core_timer",						//ep1 core timer
		"amod_do_citadel_timer",					//ep1 citadel timer
		"amod_epic_filter",							//post processing type filter

		"amod_filter_brightness_on",				//filter on value
		"amod_filter_brightness_on_exp",			//filter on exponent value
		"amod_filter_brightness_off",				//filter off value
		"amod_sky",									//current sky name

		//"amod_day_sky",								//current day skybox
		//"amod_night_sky",							//current night skybox
		//"amod_epic_filter_night_filename",			//current night filter filename
		//"amod_epic_filter_night_intensity",			//current night filter intensity
		//"amod_epic_filter_day_filename",			//current day filter filename
		//"amod_epic_filter_day_intensity"			//current day filter intensity
	};


	//write all the stuff to a keyvalues* file
	KeyValues* file = new KeyValues("AmodConfigFile");
	
	//load previous settings
	file->LoadFromFile(filesystem, "cfg/AloneMod_Config.txt", "MOD");

	//write every console var's value
	for (int i = 0; i < ARRAYSIZE(s_CommandsToWrite); i++)
	{
		//find the convar and write it to the file
		ConVar* var = cvar->FindVar(s_CommandsToWrite[i]);
		if (var)
			file->SetString(s_CommandsToWrite[i], var->GetString());
	}

	//write to the amod config file
	if (!file->SaveToFile(filesystem, "cfg/AloneMod_Config.txt", "MOD", false, true))
		ConWarning("Warning: Failed to write alone mod config!\n");

	//delete the keyvalues
	file->deleteThis();
	file = nullptr;
}

//writes to the alone mod config
CON_COMMAND(amod_writeconfig, "")
{
	Amod_WriteConfig();
}



//------------------------------------------------------------------------------------------
// Purpose: Recursivly scans the map directory for maps
//------------------------------------------------------------------------------------------
static void ScanMapsRecursive(const char* dir, CUtlVector<char*>& maps)
{
	//get the first file
	FileFindHandle_t handle;
	const char* file = g_pFullFileSystem->FindFirstEx(CFmtStr("%s/*", dir), "GAME", &handle);

	//go through the files
	while (file)
	{
		//check for . or ..
		if (!Q_strcmp(file, ".") || !Q_strcmp(file, ".."))
		{
			file = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		//get the full path
		char fullpath[512];
		Q_snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, file);

		//check the file is a directory
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			//recurse but check for maps/graphs path first
			if (Q_strnicmp(fullpath, "maps/graphs", Q_strlen("maps/graphs")))
				ScanMapsRecursive(fullpath, maps);
		}
		else
		{
			//check the extention
			const char* ext = V_GetFileExtension(fullpath);
			if (ext && !Q_stricmp(ext, "bsp"))
			{
				//get the path relative to maps/
				const char* relative = strstr(fullpath, "maps/");
				if (!relative) { file = g_pFullFileSystem->FindNext(handle); continue; }
				relative += Q_strlen("maps/");

				//skip background and credits maps
				if (Q_strnicmp(relative, "background", Q_strlen("background")) && Q_strnicmp(relative, "credits", Q_strlen("credits")))
				{
					//add the path
					char mapname[512];
					Q_strcpy(mapname, relative);
					V_StripExtension(mapname, mapname, sizeof(mapname));

					maps.AddToTail(strdup(mapname));
				}
			}
		}

		file = g_pFullFileSystem->FindNext(handle);
	}

	g_pFullFileSystem->FindClose(handle);
}

//chooses a random map out of all the maps in the maps/* directory
CON_COMMAND_F(map_random, "Goes to a random map", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	//get the relative path
	char diskpath[MAX_PATH];
	g_pFullFileSystem->RelativePathToFullPath("gameinfo.txt", "MOD", diskpath, sizeof(diskpath));
	Q_StripFilename(diskpath);

	//scan the maps directory
	CUtlVector<char*> maps;
	ScanMapsRecursive(CFmtStr("%s/maps", diskpath), maps);

	//check the maps count (shouldnt be 0 but could be)
	if (maps.Count() == 0)
	{
		Msg("No maps found.\n");
		return;
	}

	//choose a random index
	int index = random->RandomInt(0, maps.Count() - 1);
	engine->ClientCmd(CFmtStr("map %s\n", maps[index]));

	//free all the map names
	for (int i = 0; i < maps.Count(); i++)
		free(maps[i]);
}

////is obsolete
//CON_COMMAND(amod_rain_stopsounds, "stops rain sounds")
//{
//	CUtlVector<SndInfo_t> vec;
//	enginesound->GetActiveSounds(vec);
//	for (int i = 0; i < vec.Count(); i++)
//	{
//		SndInfo_t snd = vec[i];
//		char buffer[512];
//		filesystem->String(snd.m_filenameHandle, buffer, sizeof(buffer));
//		if (Q_strstr(buffer, "ambient\\weather\\rain"))
//			enginesound->StopSoundByGuid(snd.m_nGuid);
//	}
//}

CON_COMMAND(amod_startcreditssong, "starts the credits song BUT makes it so if you pause the game the music also pauses so the credits and music arnt out of sync. TODO: CHANGEME")
{
	enginesound->EmitAmbientSound("music/credits.wav", 7, 100, SND_SHOULDPAUSE);
}