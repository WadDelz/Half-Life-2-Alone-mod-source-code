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
const char* g_szMapNames[] = {
	//Station
	"d1_trainstation_01_d",
	"d1_trainstation_02_d",
	"d1_trainstation_03_d",
	"d1_trainstation_04_d",
	"d1_trainstation_05_d",
	"d1_trainstation_06_d",

	//Canals
	"d1_canals_01_d",
	"d1_canals_01a_d",
	"d1_canals_02_d",
	"d1_canals_03_d",
	"d1_canals_05_d",
	"d1_canals_06_d",
	"d1_canals_07_d",
	"d1_canals_08_d",
	"d1_canals_09_d",
	"d1_canals_10_d",
	"d1_canals_11_d",
	"d1_canals_12_d",
	"d1_canals_13_d",

	//Elis Lab
	"d1_eli_01_d",
	"d1_eli_02_d",

	//Town
	"d1_town_01_d",
	"d1_town_01a_d",
	"d1_town_02_d",
	"d1_town_02a_d",
	"d1_town_03_d",
	"d1_town_04_d",
	"d1_town_05_d",

	//Coast
	"d2_coast_01_d",
	"d2_coast_03_d",
	"d2_coast_04_d",
	"d2_coast_05_d",
	"d2_coast_07_d",
	"d2_coast_08_d",
	"d2_coast_09_d",
	"d2_coast_10_d",
	"d2_coast_11_d",
	"d2_coast_12_d",

	//Prison
	"d2_prison_01_d",
	"d2_prison_02_d",
	"d2_prison_03_d",
	"d2_prison_04_d",
	"d2_prison_05_d",
	"d2_prison_06_d",
	"d2_prison_07_d",
	"d2_prison_08_d",

	//City
	"d3_c17_01_d",
	"d3_c17_02_d",
	"d3_c17_03_d",
	"d3_c17_04_d",
	"d3_c17_05_d",
	"d3_c17_06a_d",
	"d3_c17_06b_d",
	"d3_c17_07_d",
	"d3_c17_08_d",
	"d3_c17_09_d",
	"d3_c17_10a_d",
	"d3_c17_10b_d",
	"d3_c17_11_d",
	"d3_c17_12b_d",
	"d3_c17_13_d",

	//citadel
	"d3_citadel_01_d",
	"d3_citadel_02_d",
	"d3_citadel_03_d",
	"d3_citadel_04_d",
	"d3_citadel_05_d",

	//final map for hl2
	"d3_breen_01_d",

	//episode 1
	"ep1_citadel_00_d",
	"ep1_citadel_01_d",
	"ep1_citadel_02_d",
	"ep1_citadel_02b_d",
	"ep1_citadel_03_d",
	"ep1_citadel_04_d",
	"ep1_c17_00_d",
	"ep1_c17_00a_d",
	"ep1_c17_01_d",
	"ep1_c17_02_d",
	"ep1_c17_02b_d",
	"ep1_c17_02a_d",
	"ep1_c17_05_d",
	"ep1_c17_06_d",

	//episode 2
	"ep2_outland_01_d",
	"ep2_outland_01a_d",
	"ep2_outland_02_d",
	"ep2_outland_03_d",
	"ep2_outland_04_d",
	"ep2_outland_05_d",
	"ep2_outland_06_d",
	"ep2_outland_06a_d",
	"ep2_outland_07_d",
	"ep2_outland_08_d",
	"ep2_outland_09_d",
	"ep2_outland_10_d",
	"ep2_outland_10a_d",
	"ep2_outland_11_d",
	"ep2_outland_11a_d",
	"ep2_outland_12_d",
	"ep2_outland_12a_d",
};

//--------------------------------------------------------------------------------------------
// Purpose: Returns if the current map is invalid for the day/night sky change
//--------------------------------------------------------------------------------------------
bool IsInvalidChangeMap(const char* map)
{
	if (Q_strstr(map, "ep1") || Q_strstr(map, "amod_outro") || !Q_strcmp(map, "background08_d") || !Q_strcmp(map, "background10_d") || !Q_stricmp(map, "portal_06"))
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------
// Purpose: Returns if daytime for alone mod is enabled or not
//--------------------------------------------------------------------------------------------
bool IsDaytimeEnabled()
{
	static ConVar* amod_day = cvar->FindVar("amod_day");
	if (!amod_day)
		return false;

	return amod_day->GetBool();
}

//array of time infos
static CUtlVector<MapTimeInfoBase_t> DayNightInfo;
static CUtlSymbolTable gs_DayNightInfoSymbolsTable(32, 1028, true);

//--------------------------------------------------------------------------------------------
// Purpose: Returns all the time info
//--------------------------------------------------------------------------------------------
CUtlVector<MapTimeInfoBase_t>& GetDayNightInfo()
{
	return DayNightInfo;
}

//--------------------------------------------------------------------------------------------
// Purpose: Loads a keyvalues into a time info internally
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFileInternally(KeyValues* map, MapTimeInfo_t& info)
{
	//get the night info
	KeyValues* night = map->FindKey("Night");
	if (night)
	{
		info.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString(night->GetString("DefaultNightSky", "sky_borealis01"));

		//set filter values
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterName", "scripts/colorcorrection/cc_epic_filter.raw"));
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterIntensity", "0.8"));

		//get the clouds color
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(night->GetString("CloudsColor", "255 255 255 255"));

		//get the fog info
		KeyValues* fog = night->FindKey("fog");
		if (fog)
		{
			info.NightInfo.FogEnabled = fog->GetBool("fog_override");
			FOR_EACH_VALUE(fog, value)
			{
				MapTimeInfo_t::FogInfo_t& foginfo = info.NightInfo.FogInfo[info.NightInfo.FogInfo.AddToTail()];
				foginfo.convar = gs_DayNightInfoSymbolsTable.AddString(value->GetName());
				foginfo.value = gs_DayNightInfoSymbolsTable.AddString(value->GetString());
			}
		}
	}
	else
	{
		info.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString("sky_borealis01");
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString("scripts/colorcorrection/cc_epic_filter.raw");
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString("0.8");
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString("255 255 255 255");
	}

	//get the day info
	KeyValues* day = map->FindKey("Day");
	if (day)
	{
		info.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString(day->GetString("DefaultDaySky", "sky_day02_09"));

		//set filter values
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterName", "scripts/colorcorrection/cc_daytime.raw"));
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterIntensity", "0.325"));

		//get the clouds color
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(day->GetString("CloudsColor", "255 255 255 255"));

		//get the sun info
		KeyValues* sun = day->FindKey("sun");
		if (sun)
		{
			info.DayInfo.SunInfoEnabled = sun->GetBool("enabled", true);
			FOR_EACH_VALUE(sun, value)
			{
				MapTimeInfo_t::DayInfo_t::SunInfo_t& suninfo = info.DayInfo.SunInfo[info.DayInfo.SunInfo.AddToTail()];
				suninfo.key = gs_DayNightInfoSymbolsTable.AddString(value->GetName());
				suninfo.value = gs_DayNightInfoSymbolsTable.AddString(value->GetString());
			}
		}

		//get the fog info
		KeyValues* fog = day->FindKey("fog");
		if (fog)
		{
			info.DayInfo.FogEnabled = fog->GetBool("fog_override");
			FOR_EACH_VALUE(fog, value)
			{
				MapTimeInfo_t::FogInfo_t& foginfo = info.DayInfo.FogInfo[info.DayInfo.FogInfo.AddToTail()];
				foginfo.convar = gs_DayNightInfoSymbolsTable.AddString(value->GetName());
				foginfo.value = gs_DayNightInfoSymbolsTable.AddString(value->GetString());
			}
		}
	}
	else
	{
		info.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString("sky_day02_09");
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString("scripts/colorcorrection/cc_daytime.raw");
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString("0.325");
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString("255 255 255 255");
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Loads a map info data file
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFile(const char* file)
{
	KeyValuesAD keyvalues(new KeyValues(file));
	if (!keyvalues->LoadFromFile(filesystem, file))
		return;

	//load the info
	MapTimeInfoBase_t& baseinfo = DayNightInfo[DayNightInfo.AddToTail()];
	Q_strncpy(baseinfo.filename, file, sizeof(baseinfo.filename));

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(keyvalues, map)
	{
		//make the info
		MapTimeInfo_t& info = baseinfo.base[baseinfo.base.AddToTail()];
		Q_strncpy(info.mapname, map->GetName(), sizeof(info.mapname));

		InitalizeDayNightInfoFileInternally(map, info);
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Initalizes all the day/night info structs
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfo(bool reload)
{
	DayNightInfo.RemoveAll();
	gs_DayNightInfoSymbolsTable.RemoveAll();

	//go through all files in the resource/time_info/* directory
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst("resource/time_info/*.txt", &handle);
	while (firstfile)
	{
		//dont read the . filenames
		if (!Q_stricmp(firstfile, ".") || !Q_stricmp(firstfile, ".."))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//load the file
		if (!filesystem->FindIsDirectory(handle) && strchr(firstfile, '.'))
			InitalizeDayNightInfoFile(CFmtStr("resource/time_info/%s", firstfile));

		firstfile = filesystem->FindNext(handle);
	}

	filesystem->FindClose(handle);

	//reload the map if needed
#if CLIENT_DLL
	if (reload && engine->IsConnected())
#else
	if (reload && UTIL_GetLocalPlayer())
#endif
	{
#ifdef CLIENT_DLL

#if USES_DYNAMIC_SKY
		if (g_PModBase_DynamicSkybox_bUse)
		{
			engine->ClientCmd("_amod_day_do");
			return;
		}
#endif

		//client version
		if (engine->IsLevelMainMenuBackground())
			engine->ClientCmd(CFmtStr("map_background %s", szMapName));
		else
			engine->ClientCmd("save __temp_day01; wait 10; load __temp_day01");
#else
		//server version
		extern IVEngineClient* clientengine;

#if USES_DYNAMIC_SKY
		if (ConVarRef("modbase_dynamic_skybox").GetBool())
		{
			clientengine->ClientCmd("_amod_day_do");
			return;
		}
#endif

		if (clientengine->IsLevelMainMenuBackground())
			clientengine->ClientCmd(CFmtStr("map_background %s", gpGlobals->mapname.ToCStr()));
		else
			clientengine->ClientCmd("save __temp_day01; wait 10; load __temp_day01");
#endif
	}
}

#ifndef CLIENT_DLL
CON_COMMAND_F(amod_timeinfo_reset_s, "", FCVAR_DEVELOPMENTONLY)
{
	InitalizeDayNightInfo(true);
}

CON_COMMAND_F(_amod_daytimeinfo_reset, "", FCVAR_HIDDEN)
{
	//open the temp file
	KeyValuesAD file(new KeyValues("tempfile"));
	if (!file->LoadFromFile(filesystem, "__temptime.txt", "MOD"))
		return;

	//check the indexs
	int _file = atoi(args.Arg(1));
	int _map = atoi(args.Arg(2));

	//bounds checking
	if (_file < 0 || _file >= DayNightInfo.Count())
		return;

	if (_map < 0 || _map >= DayNightInfo[_file].base.Count())
		return;

	//get and set the info
	MapTimeInfo_t& info = DayNightInfo[_file].base[_map];
	InitalizeDayNightInfoFileInternally(file, info);
}
#else
CON_COMMAND(amod_timeinfo_reset, "")
{
	InitalizeDayNightInfo(false);

	//HACK: call the servers amod_timeinfo_reset_c
	ConCommand* cmd = cvar->FindCommand("amod_timeinfo_reset_s");
	if (cmd)
		cmd->Dispatch(CCommand{});
}
#endif

//simple class to initalize the day/night info
static class CAutoInitalizeDayInfo : public CAutoGameSystem
{
public:
	bool Init()
	{
		InitalizeDayNightInfo();
		return true;
	}
} s_InitalizeDayInfoSystem;



//----------------------------------------------------------------------------------------------------
// Purpose: Writes the map time into to the keyvalues
//----------------------------------------------------------------------------------------------------
void WriteTimeInfoToKeyvalues(MapTimeInfo_t& info, KeyValues* out)
{
	//get day/night keys
	KeyValues* day = new KeyValues("day");
	KeyValues* night = new KeyValues("night");

	KeyValues* times[2] = { day, night };

	//write times
	for (int i = 0; i < sizeof(times) / sizeof(times[i]); i++)
	{
		//write skybox names
		do
		{
			const char* namesarray[2] = { "DefaultDaySky", "DefaultNightSky" };
			const char* skyesarray[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.DefaultDaySky), StringFromMapTimeStringTableIndex(info.NightInfo.DefaultNightSky) };

			//write the filter
			times[i]->SetString(namesarray[i], skyesarray[i]);

		} while (false);

		//write fog info
		do
		{
			CUtlVector<MapTimeInfo_t::FogInfo_t>* fogarray[2] = { &info.DayInfo.FogInfo, &info.NightInfo.FogInfo };

			//write all keyvalues if the fog isnt invalid
			if (fogarray[i]->Count() <= 0)
				break;

			KeyValues* fog = new KeyValues("fog");

			//write all the info
			fog->SetString("fog_override", "1");
			for (int j = 0; j < fogarray[i]->Count(); j++)
			{
				fog->SetString(StringFromMapTimeStringTableIndex((*fogarray[i])[j].convar), StringFromMapTimeStringTableIndex((*fogarray[i])[j].value));
			}

			//add to day/night
			times[i]->AddSubKey(fog);

		} while (false);

		//write color correction settings
		do
		{
			const char* filterarray[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.FilterName), StringFromMapTimeStringTableIndex(info.NightInfo.FilterName) };
			const char* intensitiesarray[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.FilterIntensity), StringFromMapTimeStringTableIndex(info.NightInfo.FilterIntensity) };

			//write the filter
			times[i]->SetString("FilterName", filterarray[i]);
			times[i]->SetString("FilterIntensity", intensitiesarray[i]);

		} while (false);

		//write clouds fog color
		do
		{
			const char* cloudcolors[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.CloudsColor), StringFromMapTimeStringTableIndex(info.NightInfo.CloudsColor) };

			//write the filter
			times[i]->SetString("CloudsColor", cloudcolors[i]);

		} while (false);
	}

	//write the sun info (if there is any)
	if (info.DayInfo.SunInfoEnabled)
	{
		KeyValues* sundata = new KeyValues("sun");
		for (int i = 0; i < info.DayInfo.SunInfo.Count(); i++)
		{
			sundata->SetString(StringFromMapTimeStringTableIndex(info.DayInfo.SunInfo[i].key), StringFromMapTimeStringTableIndex(info.DayInfo.SunInfo[i].value));
		}

		day->AddSubKey(sundata);
	}

	//add the keys
	out->AddSubKey(night);
	out->AddSubKey(day);
}

//--------------------------------------------------------------------------------------------
// Purpose: Writes all the time info to the files
//--------------------------------------------------------------------------------------------
void WriteAllTimeInfosToFiles()
{
	for (int i = 0; i < DayNightInfo.Count(); i++)
	{
		//create the new keyvalues
		KeyValuesAD file(new KeyValues(DayNightInfo[i].filename));
		
		//go through each info
		for (int j = 0; j < DayNightInfo[i].base.Count(); j++)
		{
			KeyValues* map = new KeyValues(DayNightInfo[i].base[j].mapname);
			WriteTimeInfoToKeyvalues(DayNightInfo[i].base[j], map);
			file->AddSubKey(map);
		}

		//save the file
		file->SaveToFile(filesystem, DayNightInfo[i].filename, "MOD");
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Returns the current day/night time infos
//--------------------------------------------------------------------------------------------
MapTimeInfo_t& GetMapTimeInfo(const char* mapname)
{
	//go through all the files
	for (int i = 0; i < DayNightInfo.Count(); i++)
	{
		//go through all the maps
		for (int j = 0; j < DayNightInfo[i].base.Count(); j++)
		{
			if (!Q_stricmp(DayNightInfo[i].base[j].mapname, mapname))
				return DayNightInfo[i].base[j];
		}
	}

	//default
	static MapTimeInfo_t def;

	//reset set skyboxs INCASE ive ever used amod_timeinfo_reset to reset the symbol table
	def.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString("sky_day02_09");
	def.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString("scripts/colorcorrection/cc_daytime.raw");
	def.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString("0.325");
	def.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString("255 255 255 255");

	def.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString("sky_borealis01");
	def.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString("scripts/colorcorrection/cc_epic_filter.raw");
	def.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString("0.8");
	def.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString("255 255 255 255");
	
	return def;
}

//--------------------------------------------------------------------------------------------
// Purpose: returns the string from the index
//--------------------------------------------------------------------------------------------
const char* StringFromMapTimeStringTableIndex(UtlSymId_t id)
{
	return gs_DayNightInfoSymbolsTable.String(id);
}

//--------------------------------------------------------------------------------------------
// Purpose: adds a string to the string table
//--------------------------------------------------------------------------------------------
UtlSymId_t StringToMapTimeStringTableIndex(const char* string)
{
	return gs_DayNightInfoSymbolsTable.AddString(string);
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