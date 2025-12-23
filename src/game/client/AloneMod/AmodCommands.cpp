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
		"amod_day_ravenholm",						//ravenholm day or not
#endif

		"amod_vignette",							//vignette
		"amod_do_core_timer",						//ep1 core timer
		"amod_do_citadel_timer",					//ep1 citadel timer
		"amod_epic_filter",							//post processing type filter

		"amod_filter_brightness_on",				//filter on value
		"amod_filter_brightness_on_exp",			//filter on exponent value
		"amod_filter_brightness_off",				//filter off value
		"amod_sky",									//current sky name
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
			file->SetString(var->GetName(), var->GetString());
	}

	//write to the amod config file
	if (!file->SaveToFile(filesystem, "cfg/AloneMod_Config.txt", "MOD"))
		ConWarning("Warning: Failed to write alone mod config!\n");

	//delete the keyvalues
	file->deleteThis();
	file = nullptr;
}

CON_COMMAND(amod_writeconfig, "")
{
	Amod_WriteConfig();
}

CON_COMMAND_F(map_random, "Goes to a random map", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	int i = random->RandomInt(0, (sizeof(g_szMapNames) / sizeof(g_szMapNames[0]) - 1));
	engine->ClientCmd_Unrestricted(CFmtStr("map %s", g_szMapNames[i]));
}

//is obsolete
CON_COMMAND(amod_rain_stopsounds, "stops rain sounds")
{
	CUtlVector<SndInfo_t> vec;
	enginesound->GetActiveSounds(vec);
	for (int i = 0; i < vec.Count(); i++)
	{
		SndInfo_t snd = vec[i];
		char buffer[512];
		filesystem->String(snd.m_filenameHandle, buffer, sizeof(buffer));
		if (Q_strstr(buffer, "ambient\\weather\\rain"))
			enginesound->StopSoundByGuid(snd.m_nGuid);
	}
}

CON_COMMAND(amod_startcreditssong, "starts the credits song BUT makes it so if you pause the game the music also pauses so the credits and music arnt out of sync")
{
	enginesound->EmitAmbientSound("music/credits.wav", 7, 100, SND_SHOULDPAUSE);
}