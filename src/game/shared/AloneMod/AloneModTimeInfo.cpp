#include "cbase.h"
#include "utlsymbol.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "cdll_int.h"
#include "AloneMod/Amod_SharedDefs.h"
#include "../client/AloneMod/DynamicSky.h"

//defaults
#define NIGHT_DEFAULT_SKY_NAME "sky_borealis01"
#define NIGHT_DEFAULT_FILTER_NAME "scripts/colorcorrection/cc_epic_filter.raw"
#define NIGHT_DEFAULT_FITLER_INTENSITY "0.7"
#define NIGHT_DEFAULT_CLOUD_COLOR "255 255 255 255"
#define NIGHT_DEFAULT_BLOOM_ENABLED "0"
#define NIGHT_DEFAULT_BLOOM_SCALE "0"
#define NIGHT_DEFAULT_BLOOM_SCALAR "0"

#define DAY_DEFAULT_SKY_NAME "sky_day01_01"
#define DAY_DEFAULT_FILTER_NAME "scripts/colorcorrection/cc_daytime.raw"
#define DAY_DEFAULT_FILTER_INTENSITY "0.325"
#define DAY_DEFAULT_CLOUD_COLOR "255 255 255 120"
#define DAY_DEFAULT_BLOOM_ENABLED "1"
#define DAY_DEFAULT_BLOOM_SCALE "1"
#define DAY_DEFAULT_BLOOM_SCALAR "0.4"

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
		info.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString(night->GetString("DefaultNightSky", NIGHT_DEFAULT_SKY_NAME));

		//set filter values
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterName", NIGHT_DEFAULT_FILTER_NAME));
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterIntensity", NIGHT_DEFAULT_FITLER_INTENSITY));

		//get the clouds color
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(night->GetString("CloudsColor", NIGHT_DEFAULT_CLOUD_COLOR));

		//get the bloom info
		info.NightInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(night->GetString("BloomEnabled", NIGHT_DEFAULT_BLOOM_ENABLED));
		info.NightInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(night->GetString("BloomScale", NIGHT_DEFAULT_BLOOM_SCALE));
		info.NightInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(night->GetString("BloomScalarFactor", NIGHT_DEFAULT_BLOOM_SCALAR));

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
		info.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_SKY_NAME);
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FILTER_NAME);
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FITLER_INTENSITY);
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_CLOUD_COLOR);
		info.NightInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_ENABLED);
		info.NightInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_SCALE);
		info.NightInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_SCALAR);
	}

	//get the day info
	KeyValues* day = map->FindKey("Day");
	if (day)
	{
		info.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString(day->GetString("DefaultDaySky", DAY_DEFAULT_SKY_NAME));

		//set filter values
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterName", DAY_DEFAULT_FILTER_NAME));
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterIntensity", DAY_DEFAULT_FILTER_INTENSITY));

		//get the clouds color
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(day->GetString("CloudsColor", DAY_DEFAULT_CLOUD_COLOR));

		//get the bloom info
		info.DayInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(day->GetString("BloomEnabled", DAY_DEFAULT_BLOOM_ENABLED));
		info.DayInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(day->GetString("BloomScale", DAY_DEFAULT_BLOOM_SCALE));
		info.DayInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(day->GetString("BloomScalarFactor", DAY_DEFAULT_BLOOM_SCALAR));

		//get the sun info
		KeyValues* sun = day->FindKey("sun");
		if (sun)
		{
			FOR_EACH_VALUE(sun, value)
			{
				MapTimeInfo_t::DayInfo_t::SunInfo_t& suninfo = info.DayInfo.SunInfo[info.DayInfo.SunInfo.AddToTail()];
				suninfo.key = gs_DayNightInfoSymbolsTable.AddString(value->GetName());
				suninfo.value = gs_DayNightInfoSymbolsTable.AddString(value->GetString());
			}
			info.DayInfo.SunInfoEnabled = sun->GetBool("enabled", info.DayInfo.SunInfo.Count() > 0);

			//ALWAYS ADD THIS:
			MapTimeInfo_t::DayInfo_t::SunInfo_t& useangles = info.DayInfo.SunInfo[info.DayInfo.SunInfo.AddToTail()];
			useangles.key = gs_DayNightInfoSymbolsTable.AddString("use_angles");
			useangles.value = gs_DayNightInfoSymbolsTable.AddString("1");
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
		info.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_SKY_NAME);
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_NAME);
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_INTENSITY);
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_CLOUD_COLOR);
		info.DayInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_ENABLED);
		info.DayInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_SCALE);
		info.DayInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_SCALAR);
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Loads a map info data file
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFile(const char* file)
{
	KeyValuesAD keyvalues(new KeyValues(file));
	keyvalues->UsesEscapeSequences(true);
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

ConVar amod_timeinfo_load_directory("amod_timeinfo_load_directory", "resource/time_info", FCVAR_REPLICATED);

//--------------------------------------------------------------------------------------------
// Purpose: Initalizes all the day/night info structs
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfo(bool reload)
{
#ifdef CLIENT_DLL
	//call open_map_time_properties_editor 1
	engine->ClientCmd("open_map_time_properties_editor 1");

	void ClearCopiedState();
	ClearCopiedState();
#endif

	DayNightInfo.RemoveAll();
	gs_DayNightInfoSymbolsTable.RemoveAll();

	//go through all files in the amod_timeinfo_load_directory.GetString()/* directory
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst(CFmtStr("%s/*.txt", amod_timeinfo_load_directory.GetString()), &handle);
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
			InitalizeDayNightInfoFile(CFmtStr("%s/%s", amod_timeinfo_load_directory.GetString(), firstfile));

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

//noreload
CON_COMMAND_F(amod_timeinfo_reset_s_noreload, "", FCVAR_DEVELOPMENTONLY)
{
	InitalizeDayNightInfo(false);
}

//for the map properties panel
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

//noreload
CON_COMMAND(amod_timeinfo_reset_noreload, "")
{
	InitalizeDayNightInfo(false);

	//HACK: call the servers amod_timeinfo_reset_c
	ConCommand* cmd = cvar->FindCommand("amod_timeinfo_reset_s_noreload");
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
			for (int j = 0; j < fogarray[i]->Count(); j++)
			{
				fog->SetString(StringFromMapTimeStringTableIndex((*fogarray[i])[j].convar), StringFromMapTimeStringTableIndex((*fogarray[i])[j].value));
			}
			fog->SetBool("fog_override", i == 0 ? info.DayInfo.FogEnabled : info.NightInfo.FogEnabled);

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

		//write bloom settings
		do
		{
			const char* enablebloom[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.BloomEnabled), StringFromMapTimeStringTableIndex(info.NightInfo.BloomEnabled) };
			const char* bloomscale[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.BloomScale), StringFromMapTimeStringTableIndex(info.NightInfo.BloomScale) };
			const char* bloomscalar[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.BloomScalarFactor), StringFromMapTimeStringTableIndex(info.NightInfo.BloomScalarFactor) };

			//write the filter
			times[i]->SetString("BloomEnabled", enablebloom[i]);
			times[i]->SetString("BloomScale", bloomscale[i]);
			times[i]->SetString("BloomScalarFactor", bloomscalar[i]);

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
	if (info.DayInfo.SunInfo.Count() > 0)
	{
		KeyValues* sundata = new KeyValues("sun");

		//always add 'enabled' key
		sundata->SetInt("enabled", info.DayInfo.SunInfoEnabled);
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
void WriteAllTimeInfosToFiles(const char* prefix)
{
	//make sure the filepath exists
	if (!filesystem->IsDirectory(CFmtStr("resource/time_info/%s", prefix), "MOD"))
		filesystem->CreateDirHierarchy(CFmtStr("resource/time_info/%s", prefix), "MOD");

	for (int i = 0; i < DayNightInfo.Count(); i++)
	{
		//create the new keyvalues
		KeyValuesAD file(new KeyValues(DayNightInfo[i].filename));

		//build the filename
		char filename[512];
		if (prefix)
			Q_snprintf(filename, sizeof(filename), "resource/time_info/%s/%s", prefix, V_GetFileName(DayNightInfo[i].filename));
		else
			Q_strncpy(filename, DayNightInfo[i].filename, sizeof(filename));

		//go through each info
		for (int j = 0; j < DayNightInfo[i].base.Count(); j++)
		{
			KeyValues* map = new KeyValues(DayNightInfo[i].base[j].mapname);
			WriteTimeInfoToKeyvalues(DayNightInfo[i].base[j], map);
			file->AddSubKey(map);
		}

		//save the file
		file->SaveToFile(filesystem, filename, "MOD", true);
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
	def.DayInfo.DefaultDaySky = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_SKY_NAME);
	def.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_NAME);
	def.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_INTENSITY);
	def.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_CLOUD_COLOR);
	def.DayInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_ENABLED);
	def.DayInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_SCALE);
	def.DayInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_BLOOM_SCALAR);

	def.NightInfo.DefaultNightSky = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_SKY_NAME);
	def.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FILTER_NAME);
	def.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FITLER_INTENSITY);
	def.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_CLOUD_COLOR);
	def.NightInfo.BloomEnabled = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_ENABLED);
	def.NightInfo.BloomScale = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_SCALE);
	def.NightInfo.BloomScalarFactor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_BLOOM_SCALAR);

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

//--------------------------------------------------------------------------------------------
// Purpose: Copies time info from 1 instance to another
//--------------------------------------------------------------------------------------------
void CopyTimeInfoData(MapTimeInfo_t& from, MapTimeInfo_t& to, bool copynight, bool copyday)
{
	//night info
	{
		//clear current data
		to.NightInfo.FogInfo.RemoveAll();

		//set new state
		to.NightInfo.CloudsColor = from.NightInfo.CloudsColor;
		to.NightInfo.DefaultNightSky = from.NightInfo.DefaultNightSky;
		to.NightInfo.FilterIntensity = from.NightInfo.FilterIntensity;
		to.NightInfo.FilterName = from.NightInfo.FilterName;
		to.NightInfo.FogEnabled = from.NightInfo.FogEnabled;
		to.NightInfo.BloomEnabled = from.NightInfo.BloomEnabled;
		to.NightInfo.BloomScale = from.NightInfo.BloomScale;
		to.NightInfo.BloomScalarFactor = from.NightInfo.BloomScalarFactor;

		//copy the fog states
		for (int i = 0; i < from.NightInfo.FogInfo.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& finfo = to.NightInfo.FogInfo[to.NightInfo.FogInfo.AddToTail()];
			finfo.convar = from.NightInfo.FogInfo[i].convar;
			finfo.value = from.NightInfo.FogInfo[i].value;
		}
	}

	//day info
	{
		//clear current data
		to.DayInfo.FogInfo.RemoveAll();
		to.DayInfo.SunInfo.RemoveAll();

		//set new state
		to.DayInfo.CloudsColor = from.DayInfo.CloudsColor;
		to.DayInfo.DefaultDaySky = from.DayInfo.DefaultDaySky;
		to.DayInfo.FilterIntensity = from.DayInfo.FilterIntensity;
		to.DayInfo.FilterName = from.DayInfo.FilterName;
		to.DayInfo.FogEnabled = from.DayInfo.FogEnabled;
		to.DayInfo.SunInfoEnabled = from.DayInfo.SunInfoEnabled;
		to.DayInfo.BloomEnabled = from.DayInfo.BloomEnabled;
		to.DayInfo.BloomScale = from.DayInfo.BloomScale;
		to.DayInfo.BloomScalarFactor = from.DayInfo.BloomScalarFactor;

		//copy the fog states
		for (int i = 0; i < from.DayInfo.FogInfo.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& finfo = to.DayInfo.FogInfo[to.DayInfo.FogInfo.AddToTail()];
			finfo.convar = from.DayInfo.FogInfo[i].convar;
			finfo.value = from.DayInfo.FogInfo[i].value;
		}

		//copy the sun states
		for (int i = 0; i < from.DayInfo.SunInfo.Count(); i++)
		{
			MapTimeInfo_t::DayInfo_t::SunInfo_t& finfo = to.DayInfo.SunInfo[to.DayInfo.SunInfo.AddToTail()];
			finfo.key = from.DayInfo.SunInfo[i].key;
			finfo.value = from.DayInfo.SunInfo[i].value;
		}
	}
}