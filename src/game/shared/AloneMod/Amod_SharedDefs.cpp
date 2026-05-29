#include "cbase.h"
#include "Amod_SharedDefs.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "cdll_int.h"

#ifdef CLIENT_DLL
#include "AloneMod/DynamicSky.h"
#else
#include "../client/AloneMod/DynamicSky.h"
#endif

//list of maps
//const char* g_szMapNames[] = {
//	//Station
//	"d1_trainstation_01_d",
//	"d1_trainstation_02_d",
//	"d1_trainstation_03_d",
//	"d1_trainstation_04_d",
//	"d1_trainstation_05_d",
//	"d1_trainstation_06_d",
//
//	//Canals
//	"d1_canals_01_d",
//	"d1_canals_01a_d",
//	"d1_canals_02_d",
//	"d1_canals_03_d",
//	"d1_canals_05_d",
//	"d1_canals_06_d",
//	"d1_canals_07_d",
//	"d1_canals_08_d",
//	"d1_canals_09_d",
//	"d1_canals_10_d",
//	"d1_canals_11_d",
//	"d1_canals_12_d",
//	"d1_canals_13_d",
//
//	//Elis Lab
//	"d1_eli_01_d",
//	"d1_eli_02_d",
//
//	//Town
//	"d1_town_01_d",
//	"d1_town_01a_d",
//	"d1_town_02_d",
//	"d1_town_02a_d",
//	"d1_town_03_d",
//	"d1_town_04_d",
//	"d1_town_05_d",
//
//	//Coast
//	"d2_coast_01_d",
//	"d2_coast_03_d",
//	"d2_coast_04_d",
//	"d2_coast_05_d",
//	"d2_coast_07_d",
//	"d2_coast_08_d",
//	"d2_coast_09_d",
//	"d2_coast_10_d",
//	"d2_coast_11_d",
//	"d2_coast_12_d",
//
//	//Prison
//	"d2_prison_01_d",
//	"d2_prison_02_d",
//	"d2_prison_03_d",
//	"d2_prison_04_d",
//	"d2_prison_05_d",
//	"d2_prison_06_d",
//	"d2_prison_07_d",
//	"d2_prison_08_d",
//
//	//City
//	"d3_c17_01_d",
//	"d3_c17_02_d",
//	"d3_c17_03_d",
//	"d3_c17_04_d",
//	"d3_c17_05_d",
//	"d3_c17_06a_d",
//	"d3_c17_06b_d",
//	"d3_c17_07_d",
//	"d3_c17_08_d",
//	"d3_c17_09_d",
//	"d3_c17_10a_d",
//	"d3_c17_10b_d",
//	"d3_c17_11_d",
//	"d3_c17_12b_d",
//	"d3_c17_13_d",
//
//	//citadel
//	"d3_citadel_01_d",
//	"d3_citadel_02_d",
//	"d3_citadel_03_d",
//	"d3_citadel_04_d",
//	"d3_citadel_05_d",
//
//	//final map for hl2
//	"d3_breen_01_d",
//
//	//episode 1
//	"ep1_citadel_00_d",
//	"ep1_citadel_01_d",
//	"ep1_citadel_02_d",
//	"ep1_citadel_02b_d",
//	"ep1_citadel_03_d",
//	"ep1_citadel_04_d",
//	"ep1_c17_00_d",
//	"ep1_c17_00a_d",
//	"ep1_c17_01_d",
//	"ep1_c17_02_d",
//	"ep1_c17_02b_d",
//	"ep1_c17_02a_d",
//	"ep1_c17_05_d",
//	"ep1_c17_06_d",
//
//	//episode 2
//	"ep2_outland_01_d",
//	"ep2_outland_01a_d",
//	"ep2_outland_02_d",
//	"ep2_outland_03_d",
//	"ep2_outland_04_d",
//	"ep2_outland_05_d",
//	"ep2_outland_06_d",
//	"ep2_outland_06a_d",
//	"ep2_outland_07_d",
//	"ep2_outland_08_d",
//	"ep2_outland_09_d",
//	"ep2_outland_10_d",
//	"ep2_outland_10a_d",
//	"ep2_outland_11_d",
//	"ep2_outland_11a_d",
//	"ep2_outland_12_d",
//	"ep2_outland_12a_d",
//};

//--------------------------------------------------------------------------------------------
// Purpose: Returns if daytime for alone mod is enabled or not
//--------------------------------------------------------------------------------------------
bool IsDaytimeEnabled()
{
	static ConVar* amod_day = cvar->FindVar("amod_day");
	if (!amod_day)
		return false;

	//get the current time info
	MapTimeInfo_t& info = GetCurrentMapTimeInfo();

	//check to see if both are disabled
	if (!info.AllowDaytime && !info.AllowNightTime)
		return amod_day->GetBool();

	//check for only day or only night
	if (!info.AllowDaytime)
		return false;
	else if (!info.AllowNightTime)
		return true;

	//should we flip the time. If so then return the opposite of amod_day
	if (info.FlipTimes)
		return !amod_day->GetBool();

	//just return the value of amod_day
	return amod_day->GetBool();
}

#ifdef CLIENT_DLL
const char* szMapName = "";
#endif

const char* g_pBonusMaps[] = {
	"d1_trainstation_01_snowey",
	"d1_trainstation_02_snowey",
	"ep2_outland_01_snowey",
	"gm_snowey"
};