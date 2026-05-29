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
#define NIGHT_DEFAULT_BLOOM_ENABLED false
#define NIGHT_DEFAULT_BLOOM_SCALE 0
#define NIGHT_DEFAULT_BLOOM_SCALAR 0.0f

#define DAY_DEFAULT_SKY_NAME "sky_day01_01"
#define DAY_DEFAULT_FILTER_NAME "scripts/colorcorrection/cc_daytime.raw"
#define DAY_DEFAULT_FILTER_INTENSITY "0.3"
#define DAY_DEFAULT_CLOUD_COLOR "255 255 255 120"
#define DAY_DEFAULT_BLOOM_ENABLED true
#define DAY_DEFAULT_BLOOM_SCALE 1
#define DAY_DEFAULT_BLOOM_SCALAR 0.4f

//load mod convar
ConVar amod_timeinfo_load_mod("amod_timeinfo_load_mod", "", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, nullptr);

#if FOG_CUBE_TRIGGER_TEST
#include "AloneMod/map properties editor/MapPropertiesEditorPanel.h"

//used to tell if we just started the level. This is so we can tell if we need to lerp
//the values over a period of time or just set its value once.
static bool g_bJustStartedLevel = false;

//static symbols
static CUtlSymbol s_FogUseDefaultFloat;
static CUtlSymbol s_FogUseDefaultColor;
static CUtlSymbol s_FogEnable;
static CUtlSymbol s_FogEnableSkybox;
static CUtlSymbol s_FogColor;
static CUtlSymbol s_FogColorSkybox;
static CUtlSymbol s_FogStart;
static CUtlSymbol s_FogEnd;
static CUtlSymbol s_FogStartSkybox;
static CUtlSymbol s_FogEndSkybox;
static CUtlSymbol s_FogMaxDensity;
static CUtlSymbol s_FogMaxDensitySkybox;
static CUtlSymbol s_FogBlend;
static CUtlSymbol s_FogBlendSkybox;
static CUtlSymbol s_FogBlendAngle;
static CUtlSymbol s_FogBlendAngleSkybox;
static CUtlSymbol s_FogBlendColor;
static CUtlSymbol s_FogBlendColorSkybox;
#endif

//array of time infos
static CUtlVector<MapTimeInfoBase_t> DayNightInfo;
static CUtlSymbolTable gs_DayNightInfoSymbolsTable(32, 1028, true);

//the current day/night info for the current map
MapTimeInfo_t* g_CurrentDayNightInfo = nullptr;

//--------------------------------------------------------------------------------------------
// Purpose: Returns all the time info
//--------------------------------------------------------------------------------------------
CUtlVector<MapTimeInfoBase_t>& GetDayNightInfo()
{
	return DayNightInfo;
}

#if FOG_CUBE_TRIGGER_TEST
//--------------------------------------------------------------------------------------------
// Purpose: Initalizes the fog cube trigger data
//--------------------------------------------------------------------------------------------
void InitalizeDayNightFogTriggersInfo(KeyValues* triggers, CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& info)
{
	//check the keyvalues
	if (!triggers)
		return;

	//go through all the triggers
	FOR_EACH_SUBKEY(triggers, trigger)
	{
		//make a new info
		MapTimeInfo_t::FogCubeTrigger_t& cube = info[info.AddToTail()];
		memset(&cube, 0, sizeof(cube));

		//set the data
		UTIL_StringToVector(cube.mins.Base(), trigger->GetString("mins"));
		UTIL_StringToVector(cube.maxs.Base(), trigger->GetString("maxs"));

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		cube.lerptime = trigger->GetFloat("LerpTime");
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		Q_strncpy(cube.name, trigger->GetName(), sizeof(cube.name));

		//read all the fog variables
		KeyValues* variables = trigger->FindKey("Variables");
		if (!variables)
		{
			//remove the item. No point in having a trigger with no convars
			info.Remove(info.Count() - 1);
			continue;
		}

		FOR_EACH_VALUE(variables, var)
		{
			MapTimeInfo_t::FogInfo_t& fog = cube.foginfo[cube.foginfo.AddToTail()];
			fog.convar = gs_DayNightInfoSymbolsTable.AddString(var->GetName());
			fog.value = gs_DayNightInfoSymbolsTable.AddString(var->GetString());
		}

		//check cube.foginfo to see if we need to remove the cube
		if (cube.foginfo.Count() <= 0)
			info.Remove(info.Count() - 1);
	}
}
#endif

//--------------------------------------------------------------------------------------------
// Purpose: Adds the default fog values
//--------------------------------------------------------------------------------------------
void AddDefaultFogValuesToTimeinfo(MapTimeInfo_t& info)
{
	struct Vars
	{
		const char* key;
		const char* value;
	};

	Vars vars[] = {
		{"fog_override", "0"},
		{"r_pixelfog", "1"},
		{"fog_enable", "-1"},
		{"fog_enableskybox", "-1"},
		{"fog_color", "-1 -1 -1"},
		{"fog_colorskybox", "-1 -1 -1"},
		{"fog_start", "-1"},
		{"fog_end", "-1"},
		{"fog_startskybox", "-1"},
		{"fog_endskybox", "-1"},
		{"fog_maxdensity", "-1"},
		{"fog_maxdensityskybox", "-1"},
		{"fog_blend", "-1"},
		{"fog_blendskybox", "-1"},
		{"fog_blendangle", "-1"},
		{"fog_blendangleskybox", "-1"},
		{"fog_blendcolor", "-1 -1 -1"},
		{"fog_blendcolorskybox", "-1 -1 -1"},
		{"r_farz", "-1"},
	};

	//add the defaults
	for (int i = 0; i < SIZE_OF_ARRAY(vars); i++)
	{
		//add for night info
		MapTimeInfo_t::FogInfo_t& nightinfo = info.NightInfo.FogInfo[info.NightInfo.FogInfo.AddToTail()];
		nightinfo.convar = gs_DayNightInfoSymbolsTable.AddString(vars[i].key);
		nightinfo.value = gs_DayNightInfoSymbolsTable.AddString(vars[i].value);

		//add for dayinfo
		MapTimeInfo_t::FogInfo_t& dayinfo = info.DayInfo.FogInfo[info.DayInfo.FogInfo.AddToTail()];
		dayinfo.convar = nightinfo.convar;
		dayinfo.value = nightinfo.value;
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Loads a keyvalues into a time info internally
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFileInternally(KeyValues* map, MapTimeInfo_t& info)
{
	info.AllowDaytime = map->GetBool("AllowDaytime", true);
	info.AllowNightTime = map->GetBool("AllowNightTime", true);
	info.FlipTimes = map->GetBool("FlipTimes", false);

	//ALWAYS add the default fog values
	AddDefaultFogValuesToTimeinfo(info);

	//get the night info
	KeyValues* night = map->FindKey("Night");
	if (night)
	{
		info.NightInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(night->GetString("DefaultNightSky", NIGHT_DEFAULT_SKY_NAME));

		//set filter values
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterName", NIGHT_DEFAULT_FILTER_NAME));
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(night->GetString("FilterIntensity", NIGHT_DEFAULT_FITLER_INTENSITY));

		//get the clouds color
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(night->GetString("CloudsColor", NIGHT_DEFAULT_CLOUD_COLOR));

		//get the bloom info
		info.NightInfo.BloomEnabled = night->GetBool("BloomEnabled", NIGHT_DEFAULT_BLOOM_ENABLED);
		info.NightInfo.BloomScale = night->GetInt("BloomScale", NIGHT_DEFAULT_BLOOM_SCALE);
		info.NightInfo.BloomScalarFactor = night->GetFloat("BloomScalarFactor", NIGHT_DEFAULT_BLOOM_SCALAR);

		//get the fog info
		KeyValues* fog = night->FindKey("fog");
		if (fog)
		{
			info.NightInfo.FogEnabled = fog->GetBool("fog_override");
			FOR_EACH_VALUE(fog, value)
			{
				//add or update the value
				AddOrUpdateFogInfoInArray(info.NightInfo.FogInfo, value->GetName(), value->GetString());
			}
		}
		
		//get the horizon info
		KeyValues* horizon = night->FindKey("horizon");
		if (horizon)
		{
			info.NightInfo.HorizonEnabled = horizon->GetBool("r_horizonfog");
			FOR_EACH_VALUE(horizon, value)
			{
				//add or update the value
				AddOrUpdateFogInfoInArray(info.NightInfo.HorizonInfo, value->GetName(), value->GetString());
			}
		}

#if FOG_CUBE_TRIGGER_TEST
		//get the fog cube triggers
		InitalizeDayNightFogTriggersInfo(night->FindKey("FogCubeTriggers"), info.NightInfo.FogCubeTriggers);
#endif
	}
	else
	{
		info.NightInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_SKY_NAME);
		info.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FILTER_NAME);
		info.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FITLER_INTENSITY);
		info.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_CLOUD_COLOR);
		info.NightInfo.BloomEnabled = NIGHT_DEFAULT_BLOOM_ENABLED;
		info.NightInfo.BloomScale = NIGHT_DEFAULT_BLOOM_SCALE;
		info.NightInfo.BloomScalarFactor = NIGHT_DEFAULT_BLOOM_SCALAR;
	}




	//get the day info
	KeyValues* day = map->FindKey("Day");
	if (day)
	{
		info.DayInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(day->GetString("DefaultDaySky", DAY_DEFAULT_SKY_NAME));

		//set filter values
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterName", DAY_DEFAULT_FILTER_NAME));
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(day->GetString("FilterIntensity", DAY_DEFAULT_FILTER_INTENSITY));

		//get the clouds color
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(day->GetString("CloudsColor", DAY_DEFAULT_CLOUD_COLOR));

		//get the bloom info
		info.DayInfo.BloomEnabled = day->GetBool("BloomEnabled", DAY_DEFAULT_BLOOM_ENABLED);
		info.DayInfo.BloomScale = day->GetBool("BloomScale", DAY_DEFAULT_BLOOM_SCALE);
		info.DayInfo.BloomScalarFactor = day->GetFloat("BloomScalarFactor", DAY_DEFAULT_BLOOM_SCALAR);

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
				//add or update the value
				AddOrUpdateFogInfoInArray(info.DayInfo.FogInfo, value->GetName(), value->GetString());
			}
		}

		//get the horizon info
		KeyValues* horizon = day->FindKey("horizon");
		if (horizon)
		{
			info.DayInfo.HorizonEnabled = horizon->GetBool("r_horizonfog");
			FOR_EACH_VALUE(horizon, value)
			{
				//add or update the value
				AddOrUpdateFogInfoInArray(info.DayInfo.HorizonInfo, value->GetName(), value->GetString());
			}
		}

#if FOG_CUBE_TRIGGER_TEST
		//get the fog cube triggers
		InitalizeDayNightFogTriggersInfo(day->FindKey("FogCubeTriggers"), info.DayInfo.FogCubeTriggers);
#endif

	}
	else
	{
		info.DayInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_SKY_NAME);
		info.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_NAME);
		info.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_INTENSITY);
		info.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_CLOUD_COLOR);
		info.DayInfo.BloomEnabled = DAY_DEFAULT_BLOOM_ENABLED;
		info.DayInfo.BloomScale = DAY_DEFAULT_BLOOM_SCALE;
		info.DayInfo.BloomScalarFactor = DAY_DEFAULT_BLOOM_SCALAR;
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Loads a map info data file
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFileFromBase(const char* file)
{
	KeyValuesAD keyvalues(new KeyValues(file));
	keyvalues->UsesEscapeSequences(true);
	if (!keyvalues->LoadFromFile(filesystem, file))
		return;

	//ptr to base
	MapTimeInfoBase_t* base = nullptr;

	//look for the base first
	const char* basename = CFmtStr("resource/time_info/%s/%s", amod_timeinfo_load_mod.GetString(), V_GetFileName(file));
	for (int i = 0; i < DayNightInfo.Count(); i++)
	{
		if (!Q_stricmp(DayNightInfo[i].filename, basename))
		{
			base = &DayNightInfo[i];
			break;
		}
	}

	//if we didnt find it in the loop then add it
	if (!base)
	{
		base = &DayNightInfo[DayNightInfo.AddToTail()];
		Q_strncpy(base->filename, basename, sizeof(base->filename));
	}

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(keyvalues, map)
	{
		//check the item already exists
		bool found = false;
		GetMapTimeInfo(map->GetName(), &found);
		if (found)
		{
			//dont add the item if it already exists
			continue;
		}

		//add the data
		MapTimeInfo_t& info = base->base[base->base.AddToTail()];
		Q_strncpy(info.mapname, map->GetName(), sizeof(info.mapname));

		InitalizeDayNightInfoFileInternally(map, info);
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Initalizes all the day/night info structs BUT from the base time info so no time info gets missed
//--------------------------------------------------------------------------------------------
void InitalizeDayNightInfoFromBase()
{
	//mod folder
	static const char* modfolder = "resource/time_info";

	//go through all files in the amod_timeinfo_load_directory.GetString()/* directory
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst(CFmtStr("%s/*.txt", modfolder), &handle);
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
			InitalizeDayNightInfoFileFromBase(CFmtStr("%s/%s", modfolder, firstfile));

		firstfile = filesystem->FindNext(handle);
	}

	filesystem->FindClose(handle);
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
		//check the item already exists
		bool found = false;
		GetMapTimeInfo(map->GetName(), &found);
		if (found)
		{
			//dont add the item if it already exists
			continue;
		}

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
	//remove all the current info
	DayNightInfo.RemoveAll();
	gs_DayNightInfoSymbolsTable.RemoveAll();

#if FOG_CUBE_TRIGGER_TEST
	//init our default symbols
	s_FogUseDefaultFloat = gs_DayNightInfoSymbolsTable.AddString("-1");
	s_FogUseDefaultColor = gs_DayNightInfoSymbolsTable.AddString("-1 -1 -1");
	s_FogEnable = gs_DayNightInfoSymbolsTable.AddString("fog_enable");
	s_FogEnableSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_enableskybox");
	s_FogColor = gs_DayNightInfoSymbolsTable.AddString("fog_color");
	s_FogColorSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_colorskybox");
	s_FogStart = gs_DayNightInfoSymbolsTable.AddString("fog_start");
	s_FogStartSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_startskybox");
	s_FogEnd = gs_DayNightInfoSymbolsTable.AddString("fog_end");
	s_FogEndSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_endskybox");
	s_FogMaxDensity = gs_DayNightInfoSymbolsTable.AddString("fog_maxdensity");
	s_FogMaxDensitySkybox = gs_DayNightInfoSymbolsTable.AddString("fog_maxdensityskybox");
	s_FogBlend = gs_DayNightInfoSymbolsTable.AddString("fog_blend");
	s_FogBlendSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_blendskybox");
	s_FogBlendAngle = gs_DayNightInfoSymbolsTable.AddString("fog_blendangle");
	s_FogBlendAngleSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_blendangleskybox");
	s_FogBlendColor = gs_DayNightInfoSymbolsTable.AddString("fog_blendcolor");
	s_FogBlendColorSkybox = gs_DayNightInfoSymbolsTable.AddString("fog_blendcolorskybox");
#endif

	//clear g_CurrentDayNightInfo
	g_CurrentDayNightInfo = nullptr;

	//get the mod
	const char* loadmod = amod_timeinfo_load_mod.GetString();
	char modfolder[512];
	Q_strncpy(modfolder, CFmtStr("resource/time_info%s%s", loadmod[0] ? "/" : "", loadmod), sizeof(modfolder));

	//check the mod file exists
	if (!filesystem->IsDirectory(modfolder))
		Q_strncpy(modfolder, "resource/time_info", sizeof(modfolder));

	//go through all files in the amod_timeinfo_load_directory.GetString()/* directory
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst(CFmtStr("%s/*.txt", modfolder), &handle);
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
			InitalizeDayNightInfoFile(CFmtStr("%s/%s", modfolder, firstfile));

		firstfile = filesystem->FindNext(handle);
	}

	filesystem->FindClose(handle);

	//now load every base resource/time_info/* file so we dont miss out on any (ONLY if amod_timeinfo_load_mod isnt nothing)
	if (*amod_timeinfo_load_mod.GetString())
		InitalizeDayNightInfoFromBase();

#ifdef CLIENT_DLL
	//call open_map_time_properties_editor 1 to reset the map properties editor
	engine->ClientCmd("open_map_time_properties_editor 1");

	void ClearCopiedState();
	ClearCopiedState();

	//set g_CurrentDayNightInfo
	g_CurrentDayNightInfo = &GetMapTimeInfo(szMapName);

#if FOG_CUBE_TRIGGER_TEST
	//hack: dont lerp the fog values
	g_bJustStartedLevel = true;

#endif //FOG_CUBE_TRIGGER_TEST

#else
	//set g_CurrentDayNightInfo
	g_CurrentDayNightInfo = &GetMapTimeInfo(gpGlobals->mapname.ToCStr());
#endif	//CLIENT_DLL

	//reload the map if needed
#ifdef CLIENT_DLL
	bool loaded = engine->IsConnected();
#else
	bool loaded = UTIL_GetLocalPlayer() != nullptr;

	//setup our engine pointer
	extern IVEngineClient* clientengine;
	IVEngineClient* engine = clientengine;

	//get the map name
	const char* szMapName = gpGlobals->mapname.ToCStr();
#endif

	if (reload && loaded)
	{
#if USES_DYNAMIC_SKY
#ifdef CLIENT_DLL
		if (g_PModBase_DynamicSkybox_bUse)
#else
		if (ConVarRef("modbase_dynamic_skybox").GetBool())
#endif	//CLIENT_DLL
		{
			engine->ClientCmd("_amod_day_do");
			return;
		}
#endif	//USES_DYNAMIC_SKY

		//reload if dynamic sky is not enabled
		if (engine->IsLevelMainMenuBackground())
			engine->ClientCmd(CFmtStr("map_background %s", szMapName));
		else
			engine->ClientCmd("save __temp_day01; wait 10; load __temp_day01");
	}
}


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



#ifndef CLIENT_DLL
//reloads the time info and resets the map/save
CON_COMMAND_F(amod_timeinfo_reset_s, "", FCVAR_DEVELOPMENTONLY)
{
	InitalizeDayNightInfo(true);
}

//reloads the time info but does not reload the game
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

	//clear the current fog/sun info then reload the info
	info.DayInfo.FogInfo.RemoveAll();
	info.DayInfo.SunInfo.RemoveAll();
	info.DayInfo.HorizonInfo.RemoveAll();
	info.NightInfo.FogInfo.RemoveAll();
	info.NightInfo.HorizonInfo.RemoveAll();

#if FOG_CUBE_TRIGGER_TEST
	info.DayInfo.FogCubeTriggers.RemoveAll();
	info.NightInfo.FogCubeTriggers.RemoveAll();
#endif

	InitalizeDayNightInfoFileInternally(file, info);
}
#else
//reloads the client and server info
CON_COMMAND(amod_timeinfo_reset, "")
{
	InitalizeDayNightInfo(false);

	//HACK: call the servers amod_timeinfo_reset_c
	ConCommand* cmd = cvar->FindCommand("amod_timeinfo_reset_s");
	if (cmd)
		cmd->Dispatch(CCommand{});
}

//reloads the client and server info but doesnt reload
CON_COMMAND(amod_timeinfo_reset_noreload, "")
{
	InitalizeDayNightInfo(false);

	//HACK: call the servers amod_timeinfo_reset_c
	ConCommand* cmd = cvar->FindCommand("amod_timeinfo_reset_s_noreload");
	if (cmd)
		cmd->Dispatch(CCommand{});
}
#endif

//----------------------------------------------------------------------------------------------------
// Purpose: Writes the map time into to the keyvalues
//----------------------------------------------------------------------------------------------------
void WriteTimeInfoToKeyvalues(MapTimeInfo_t& info, KeyValues* out)
{
	//write our force mode
	out->SetInt("AllowDayTime", info.AllowDaytime);
	out->SetInt("AllowNightTime", info.AllowNightTime);
	out->SetBool("FlipTimes", info.FlipTimes);

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
			const char* skyesarray[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.Skybox), StringFromMapTimeStringTableIndex(info.NightInfo.Skybox) };

			//write the filter
			times[i]->SetString(namesarray[i], skyesarray[i]);

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
			bool enablebloom[2] = { info.DayInfo.BloomEnabled, info.NightInfo.BloomEnabled };
			int bloomscale[2] = { info.DayInfo.BloomScale, info.NightInfo.BloomScale };
			float bloomscalar[2] = { info.DayInfo.BloomScalarFactor, info.NightInfo.BloomScalarFactor };

			//write the filter
			times[i]->SetBool("BloomEnabled", enablebloom[i]);
			times[i]->SetInt("BloomScale", bloomscale[i]);
			times[i]->SetFloat("BloomScalarFactor", bloomscalar[i]);

		} while (false);

		//write clouds fog color
		do
		{
			const char* cloudcolors[2] = { StringFromMapTimeStringTableIndex(info.DayInfo.CloudsColor), StringFromMapTimeStringTableIndex(info.NightInfo.CloudsColor) };

			//write the filter
			times[i]->SetString("CloudsColor", cloudcolors[i]);

		} while (false);

		//write fog info
		do
		{
			CUtlVector<MapTimeInfo_t::FogInfo_t>* fogarray[2] = { &info.DayInfo.FogInfo, &info.NightInfo.FogInfo };

			//write all keyvalues if the fog is invalid
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

		//write all the horizon stuff
		do
		{
			CUtlVector<MapTimeInfo_t::FogInfo_t>* horizonarray[2] = { &info.DayInfo.HorizonInfo, &info.NightInfo.HorizonInfo };

			//write all keyvalues if the fog is invalid
			if (horizonarray[i]->Count() <= 0)
				break;

			KeyValues* horizon = new KeyValues("horizon");

			//write all the info
			for (int j = 0; j < horizonarray[i]->Count(); j++)
			{
				horizon->SetString(StringFromMapTimeStringTableIndex((*horizonarray[i])[j].convar), StringFromMapTimeStringTableIndex((*horizonarray[i])[j].value));
			}
			horizon->SetBool("r_horizonfog", i == 0 ? info.DayInfo.HorizonEnabled : info.NightInfo.HorizonEnabled);

			//add to day/night
			times[i]->AddSubKey(horizon);

		} while (false);

#if FOG_CUBE_TRIGGER_TEST
		//write fog cube triggers
		do
		{
			CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>* fogarray[2] = { &info.DayInfo.FogCubeTriggers, &info.NightInfo.FogCubeTriggers };

			//write all keyvalues if the fog is invalid
			if (fogarray[i]->Count() <= 0)
				break;

			KeyValues* triggers = new KeyValues("FogCubeTriggers");

			//write all the info
			for (int j = 0; j < fogarray[i]->Count(); j++)
			{
				MapTimeInfo_t::FogCubeTrigger_t& trigger = (*fogarray[i])[j];

				//create a new subkey
				KeyValues* triggerkv = new KeyValues(trigger.name);
				triggerkv->SetString("mins", CFmtStr("%f %f %f", trigger.mins.x, trigger.mins.y, trigger.mins.z));
				triggerkv->SetString("maxs", CFmtStr("%f %f %f", trigger.maxs.x, trigger.maxs.y, trigger.maxs.z));

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
				triggerkv->SetFloat("lerptime", trigger.lerptime);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

				//get all the vars
				KeyValues* variables = new KeyValues("Variables");
				for (int z = 0; z < trigger.foginfo.Count(); z++)
				{
					KeyValues* variable = new KeyValues(gs_DayNightInfoSymbolsTable.String(trigger.foginfo[z].convar));
					variable->SetString(nullptr, gs_DayNightInfoSymbolsTable.String(trigger.foginfo[z].value));
					variables->AddSubKey(variable);
				}
				//add variables to triggerkv
				triggerkv->AddSubKey(variables);

				//add the trigger
				triggers->AddSubKey(triggerkv);
			}

			//add to day/night
			times[i]->AddSubKey(triggers);

		} while (false);
#endif
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
// Purpose: Returns the default day/night info
//--------------------------------------------------------------------------------------------
MapTimeInfo_t& GetDefaultMapTimeInfo()
{
	static MapTimeInfo_t def;
	def.mapname[0] = '\0';
	def.AllowDaytime = true;
	def.AllowNightTime = true;
	def.FlipTimes = false;

	//reset set skyboxs/data INCASE ive ever used amod_timeinfo_reset to reset the symbol table
	def.DayInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_SKY_NAME);
	def.DayInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_NAME);
	def.DayInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_FILTER_INTENSITY);
	def.DayInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(DAY_DEFAULT_CLOUD_COLOR);
	def.DayInfo.BloomEnabled = DAY_DEFAULT_BLOOM_ENABLED;
	def.DayInfo.BloomScale = DAY_DEFAULT_BLOOM_SCALE;
	def.DayInfo.BloomScalarFactor = DAY_DEFAULT_BLOOM_SCALAR;

	def.NightInfo.Skybox = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_SKY_NAME);
	def.NightInfo.FilterName = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FILTER_NAME);
	def.NightInfo.FilterIntensity = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_FITLER_INTENSITY);
	def.NightInfo.CloudsColor = gs_DayNightInfoSymbolsTable.AddString(NIGHT_DEFAULT_CLOUD_COLOR);
	def.NightInfo.BloomEnabled = NIGHT_DEFAULT_BLOOM_ENABLED;
	def.NightInfo.BloomScale = NIGHT_DEFAULT_BLOOM_SCALE;
	def.NightInfo.BloomScalarFactor = NIGHT_DEFAULT_BLOOM_SCALAR;
	return def;
}

//--------------------------------------------------------------------------------------------
// Purpose: Returns the current day/night time infos
//--------------------------------------------------------------------------------------------
MapTimeInfo_t& GetMapTimeInfo(const char* mapname, bool* found)
{
	//strip the extention
	mapname = V_GetFileName(mapname);

	//go through all the files
	for (int i = 0; i < DayNightInfo.Count(); i++)
	{
		//go through all the maps
		for (int j = 0; j < DayNightInfo[i].base.Count(); j++)
		{
			if (!Q_stricmp(DayNightInfo[i].base[j].mapname, mapname))
			{
				//set found
				if (found)
					*found = true;

				//return the time info
				return DayNightInfo[i].base[j];
			}
		}
	}

	//set found
	if (found)
		*found = false;

	//default
	return GetDefaultMapTimeInfo();
}

//--------------------------------------------------------------------------------------------
// Purpose: Returns the current day/night time infos
//--------------------------------------------------------------------------------------------
MapTimeInfo_t& GetCurrentMapTimeInfo()
{
	if (!g_CurrentDayNightInfo)
		return GetDefaultMapTimeInfo();

	return *g_CurrentDayNightInfo;
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
void CopyTimeInfoData(MapTimeInfo_t& from, MapTimeInfo_t& to, bool fromnight, bool tonight)
{
	//handle to night
	if (tonight)
	{
		//bloom
		to.NightInfo.BloomEnabled = fromnight ? from.NightInfo.BloomEnabled : from.DayInfo.BloomEnabled;
		to.NightInfo.BloomScale = fromnight ? from.NightInfo.BloomScale : from.DayInfo.BloomScale;
		to.NightInfo.BloomScalarFactor = fromnight ? from.NightInfo.BloomScalarFactor : from.DayInfo.BloomScalarFactor;

		//skybox
		to.NightInfo.Skybox = fromnight ? from.NightInfo.Skybox : from.DayInfo.Skybox;

		//filter info
		to.NightInfo.FilterIntensity = fromnight ? from.NightInfo.FilterIntensity : from.DayInfo.FilterIntensity;
		to.NightInfo.FilterName = fromnight ? from.NightInfo.FilterName : from.DayInfo.FilterName;

		//clouds
		to.NightInfo.CloudsColor = fromnight ? from.NightInfo.CloudsColor : from.DayInfo.CloudsColor;

		//fog
		to.NightInfo.FogEnabled = fromnight ? from.NightInfo.FogEnabled : from.DayInfo.FogEnabled;
		to.NightInfo.FogInfo.RemoveAll();
		
		CUtlVector<MapTimeInfo_t::FogInfo_t>& fromfog = fromnight ? from.NightInfo.FogInfo : from.DayInfo.FogInfo;
		for (int i = 0; i < fromfog.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& info = to.NightInfo.FogInfo[to.NightInfo.FogInfo.AddToTail()];
			info.convar = fromfog[i].convar;
			info.value = fromfog[i].value;
		}

		//horizon
		to.NightInfo.HorizonEnabled = fromnight ? from.NightInfo.HorizonEnabled : from.DayInfo.HorizonEnabled;
		to.NightInfo.HorizonInfo.RemoveAll();
		
		fromfog = fromnight ? from.NightInfo.HorizonInfo : from.DayInfo.HorizonInfo;
		for (int i = 0; i < fromfog.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& info = to.NightInfo.HorizonInfo[to.NightInfo.HorizonInfo.AddToTail()];
			info.convar = fromfog[i].convar;
			info.value = fromfog[i].value;
		}

#if FOG_CUBE_TRIGGER_TEST
		//fog cube triggers
		to.NightInfo.FogCubeTriggers.RemoveAll();

		CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& fromtriggers = fromnight ? from.NightInfo.FogCubeTriggers : from.DayInfo.FogCubeTriggers;
		for (int i = 0; i < fromtriggers.Count(); i++)
		{
			MapTimeInfo_t::FogCubeTrigger_t& info = to.NightInfo.FogCubeTriggers[to.NightInfo.FogCubeTriggers.AddToTail()];

			//copy
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
			info.lerptime = fromtriggers[i].lerptime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

			info.mins = fromtriggers[i].mins;
			info.maxs = fromtriggers[i].maxs;
			Q_strncpy(info.name, fromtriggers[i].name, sizeof(info.name));

			//copy the vars
			for (int j = 0; j < fromtriggers[i].foginfo.Count(); j++)
			{
				MapTimeInfo_t::FogInfo_t& fogdata = info.foginfo[info.foginfo.AddToTail()];
				fogdata.convar = fromtriggers[i].foginfo[j].convar;
				fogdata.value = fromtriggers[i].foginfo[j].value;
			}
		}
#endif
	}
	else
	{
		//bloom
		to.DayInfo.BloomEnabled = fromnight ? from.NightInfo.BloomEnabled : from.DayInfo.BloomEnabled;
		to.DayInfo.BloomScale = fromnight ? from.NightInfo.BloomScale : from.DayInfo.BloomScale;
		to.DayInfo.BloomScalarFactor = fromnight ? from.NightInfo.BloomScalarFactor : from.DayInfo.BloomScalarFactor;

		//skybox
		to.DayInfo.Skybox = fromnight ? from.NightInfo.Skybox : from.DayInfo.Skybox;

		//filter info
		to.DayInfo.FilterIntensity = fromnight ? from.NightInfo.FilterIntensity : from.DayInfo.FilterIntensity;
		to.DayInfo.FilterName = fromnight ? from.NightInfo.FilterName : from.DayInfo.FilterName;

		//clouds
		to.DayInfo.CloudsColor = fromnight ? from.NightInfo.CloudsColor : from.DayInfo.CloudsColor;

		//fog
		to.DayInfo.FogEnabled = fromnight ? from.NightInfo.FogEnabled : from.DayInfo.FogEnabled;
		to.DayInfo.FogInfo.RemoveAll();
		
		CUtlVector<MapTimeInfo_t::FogInfo_t>& fromfog = fromnight ? from.NightInfo.FogInfo : from.DayInfo.FogInfo;
		for (int i = 0; i < fromfog.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& info = to.DayInfo.FogInfo[to.DayInfo.FogInfo.AddToTail()];
			info.convar = fromfog[i].convar;
			info.value = fromfog[i].value;
		}

		//horizon
		to.DayInfo.HorizonEnabled = fromnight ? from.NightInfo.HorizonEnabled : from.DayInfo.HorizonEnabled;
		to.DayInfo.HorizonInfo.RemoveAll();

		fromfog = fromnight ? from.NightInfo.HorizonInfo : from.DayInfo.HorizonInfo;
		for (int i = 0; i < fromfog.Count(); i++)
		{
			MapTimeInfo_t::FogInfo_t& info = to.DayInfo.HorizonInfo[to.DayInfo.HorizonInfo.AddToTail()];
			info.convar = fromfog[i].convar;
			info.value = fromfog[i].value;
		}

		//sun
		to.DayInfo.SunInfoEnabled = from.DayInfo.SunInfoEnabled;
		to.DayInfo.SunInfo.RemoveAll();
		
		if (!fromnight)
		{
			CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& fromfog = from.DayInfo.SunInfo;
			for (int i = 0; i < fromfog.Count(); i++)
			{
				MapTimeInfo_t::DayInfo_t::SunInfo_t& info = to.DayInfo.SunInfo[to.DayInfo.SunInfo.AddToTail()];
				info.key = fromfog[i].key;
				info.value = fromfog[i].value;
			}
		}

#if FOG_CUBE_TRIGGER_TEST
		//fog cube triggers
		to.DayInfo.FogCubeTriggers.RemoveAll();

		CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& fromtriggers = fromnight ? from.NightInfo.FogCubeTriggers : from.DayInfo.FogCubeTriggers;
		for (int i = 0; i < fromtriggers.Count(); i++)
		{
			MapTimeInfo_t::FogCubeTrigger_t& info = to.DayInfo.FogCubeTriggers[to.DayInfo.FogCubeTriggers.AddToTail()];

			//copy
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
			info.lerptime = fromtriggers[i].lerptime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

			info.mins = fromtriggers[i].mins;
			info.maxs = fromtriggers[i].maxs;
			Q_strncpy(info.name, fromtriggers[i].name, sizeof(info.name));

			//copy the vars
			for (int j = 0; j < fromtriggers[i].foginfo.Count(); j++)
			{
				MapTimeInfo_t::FogInfo_t& fogdata = info.foginfo[info.foginfo.AddToTail()];
				fogdata.convar = fromtriggers[i].foginfo[j].convar;
				fogdata.value = fromtriggers[i].foginfo[j].value;
			}
		}
#endif
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the fog info array
//----------------------------------------------------------------------------------------------------
const char* FindFogInfoFromArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* def)
{
	for (int i = 0; i < info.Count(); i++)
	{
		if (!Q_stricmp(StringFromMapTimeStringTableIndex(info[i].convar), key))
			return StringFromMapTimeStringTableIndex(info[i].value);
	}

	return def;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a key/value to the fog info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
int AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, UtlSymId_t value)
{
	//search for existing key
	for (int i = 0; i < info.Count(); i++)
	{
		if (info[i].convar == key)
		{
			//update existing value
			info[i].value = value;
			return i;
		}
	}

	//key not found, add new entry
	MapTimeInfo_t::FogInfo_t& newInfo = info[info.AddToTail()];
	newInfo.convar = key;
	newInfo.value = value;

	return info.Count() - 1;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a key/value to the fog info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
int AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* value)
{
	return AddOrUpdateFogInfoInArray(info, StringToMapTimeStringTableIndex(key), StringToMapTimeStringTableIndex(value));
}

//----------------------------------------------------------------------------------------------------
// Purpose: Removes the item from the fog info array
//----------------------------------------------------------------------------------------------------
void RemoveFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key)
{
	UtlSymId_t sym = StringToMapTimeStringTableIndex(key);

	//search for existing key
	for (int i = 0; i < info.Count(); i++)
	{
		if (info[i].convar == sym)
		{
			info.Remove(i);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the sun info array
//----------------------------------------------------------------------------------------------------
const char* FindSunInfoFromArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* def)
{
	for (int i = 0; i < info.Count(); i++)
	{
		if (!Q_stricmp(StringFromMapTimeStringTableIndex(info[i].key), key))
			return StringFromMapTimeStringTableIndex(info[i].value);
	}

	return def;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a key/value to the sun info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
void AddOrUpdateSunInfoInArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* value)
{
	//search for existing key
	for (int i = 0; i < info.Count(); i++)
	{
		if (!Q_stricmp(StringFromMapTimeStringTableIndex(info[i].key), key))
		{
			//update existing value
			info[i].value = StringToMapTimeStringTableIndex(value);
			return;
		}
	}

	//key not found, add new entry
	MapTimeInfo_t::DayInfo_t::SunInfo_t& newInfo = info[info.AddToTail()];
	newInfo.key = StringToMapTimeStringTableIndex(key);
	newInfo.value = StringToMapTimeStringTableIndex(value);
}

#ifdef CLIENT_DLL
//get the fog convars
extern ConVar fog_override;
extern ConVar fog_start;
extern ConVar fog_end;
extern ConVar fog_color;
extern ConVar fog_enable;
extern ConVar fog_startskybox;
extern ConVar fog_endskybox;
extern ConVar fog_maxdensityskybox;
extern ConVar fog_colorskybox;
extern ConVar fog_enableskybox;
extern ConVar fog_maxdensity;
extern ConVar fog_blend;
extern ConVar fog_blendangle;
extern ConVar fog_blendcolor;
extern ConVar fog_blendskybox;
extern ConVar fog_blendangleskybox;
extern ConVar fog_blendcolorskybox;
extern ConVar fog_lerp_system_lerp_time;
extern ConVar fog_lerp_system_lerp_type;
extern ConVar fog_lerp_system_lerp_parameter;
#endif

#ifdef CLIENT_DLL

//for the cube debug overlays
#if FOG_CUBE_TRIGGER_TEST
#include "debugoverlay_shared.h"

//debug convar 
ConVar amod_fog_cubeinfo_debug("amod_fog_cubeinfo_debug", "0");

//this it the cube trigger that will use the maps fog.
static MapTimeInfo_t::FogCubeTrigger_t s_MapCubeTrigger;

//------------------------------------------------------------------------------------------
// Purpose: Resets the map cube trigger activated states
//------------------------------------------------------------------------------------------
void ResetCubeTriggerData()
{
	//set the s_MapCubeTrigger
	memset(&s_MapCubeTrigger, 0, sizeof(s_MapCubeTrigger));
	
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	s_MapCubeTrigger.lerptime = 1.0f;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

	//de-activate every active trigger
	CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers = IsDaytimeEnabled() ? g_CurrentDayNightInfo->DayInfo.FogCubeTriggers : g_CurrentDayNightInfo->NightInfo.FogCubeTriggers;
	for (int i = 0; i < CubeTriggers.Count(); i++)
	{
		CubeTriggers[i].active = false;
	}

	//we just started the level
	g_bJustStartedLevel = true;
}

//update time for the cube trigger update func
static float s_NextUpdateTime = 0.0f;
#endif

//update the vars this frame?
bool s_bUpdateTriggerValuesThisFrame = false;


//time info class
class CAutoTimeInfoInitializer : public CAutoGameSystemPerFrame
{
public:
	void LevelInitPreEntity()
	{
		//set our current day night info
		g_CurrentDayNightInfo = &GetMapTimeInfo(MapName());

#if FOG_CUBE_TRIGGER_TEST
		ResetCubeTriggerData();

		//set s_NextUpdateTime
		s_NextUpdateTime = gpGlobals->curtime;
#endif
	}

#if FOG_CUBE_TRIGGER_TEST
	//returns a pointer to a fog convar
	ConVar* FindFogConvar(const char* name)
	{
		//here is a list of commonly used convars so we dont have to use cvar->FindConvar() unless the convar isnt inside this
		//array of convar pointers.
		static ConVar* s_CommonlyUsedConvars[] = {
			&fog_override,
			&fog_enable,
			&fog_enableskybox,
			&fog_start,
			&fog_startskybox,
			&fog_end,
			&fog_endskybox,
			&fog_maxdensity,
			&fog_maxdensityskybox,
			&fog_color,
			&fog_colorskybox,
			&fog_blend,
			&fog_blendangle,
			&fog_blendcolor,
			&fog_blendskybox,
			&fog_blendangleskybox,
			&fog_blendcolorskybox,
		};

		//check the common list first
		for (int i = 0; i < SIZE_OF_ARRAY(s_CommonlyUsedConvars); i++)
		{
			if (!Q_stricmp(name, s_CommonlyUsedConvars[i]->GetName()))
				return s_CommonlyUsedConvars[i];
		}

		//fall back to cvar->FindVar(name)
		return cvar->FindVar(name);
	}

	//collects the fog cubes
	void CollectFogCubes(Vector pos, CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers, CUtlVector<MapTimeInfo_t::FogCubeTrigger_t*>& CubeTriggersOutsideThis, MapTimeInfo_t::FogCubeTrigger_t** OldActive, MapTimeInfo_t::FogCubeTrigger_t** CurrentActive)
	{
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		//get the old active trigger
		for (int i = 0; i < CubeTriggers.Count(); i++)
		{
			if (CubeTriggers[i].active)
			{
				*CurrentActive = &CubeTriggers[i];
				*OldActive = *CurrentActive;
				break;
			}
		}

		//did we find a cube or not
		bool bDid = false;

		//all the triggers outside the cube we are inside.
		CUtlVector<float> CubeTriggerDistances;

		//go through all cube triggers
		for (int i = 0; i < CubeTriggers.Count(); i++)
		{
			//get the cube trigger
			MapTimeInfo_t::FogCubeTrigger_t& trigger = CubeTriggers[i];

			//are we in the trigger or not?
			bool InBounds = (pos.x >= trigger.mins.x && pos.y >= trigger.mins.y && pos.z >= trigger.mins.z) &&
				(pos.x <= trigger.maxs.x && pos.y <= trigger.maxs.y && pos.z <= trigger.maxs.z);

			//dont bother if we arnt
			if (!InBounds)
				continue;

			//distance to each side
			float dx1 = pos.x - trigger.mins.x;
			float dx2 = trigger.maxs.x - pos.x;
			float dy1 = pos.y - trigger.mins.y;
			float dy2 = trigger.maxs.y - pos.y;
			float dz1 = pos.z - trigger.mins.z;
			float dz2 = trigger.maxs.z - pos.z;

			//distance to closest face
			float faceDist = dx1;
			if (dx2 < faceDist) faceDist = dx2;
			if (dy1 < faceDist) faceDist = dy1;
			if (dy2 < faceDist) faceDist = dy2;
			if (dz1 < faceDist) faceDist = dz1;
			if (dz2 < faceDist) faceDist = dz2;

			//distance to center
			Vector center = (trigger.mins + trigger.maxs) * 0.5f;
			Vector delta = pos - center;
			float centerDist = delta.LengthSqr(); // squared for speed

			//final priority metric
			float dist = faceDist - centerDist * 0.00001f;

			//insert by distance to the sides. THe closer you are to a side, The later in the array it gets added.
			int insertIndex;
			for (insertIndex = 0; insertIndex < CubeTriggerDistances.Count(); insertIndex++)
			{
				if (dist > CubeTriggerDistances[insertIndex])
					break;
			}

			//insert it
			CubeTriggersOutsideThis.InsertBefore(insertIndex, &trigger);
			CubeTriggerDistances.InsertBefore(insertIndex, dist);
			bDid = true;
		}

		//did we add the item. If not then add the default map trigger
		if (bDid)
		{
			*CurrentActive = CubeTriggersOutsideThis[CubeTriggersOutsideThis.Count() - 1];
		}
		else
		{
			*CurrentActive = &s_MapCubeTrigger;
		}
#else 
		//get the old active trigger
		for (int i = 0; i < CubeTriggers.Count(); i++)
		{
			if (CubeTriggers[i].active)
			{
				*CurrentActive = &CubeTriggers[i];
				*OldActive = *CurrentActive;
				break;
			}
		}

		//did we add an item?
		bool bDid = false;

		//best (most inner) trigger
		MapTimeInfo_t::FogCubeTrigger_t* best = NULL;

		//go through all cube triggers
		for (int i = 0; i < CubeTriggers.Count(); i++)
		{
			//get the cube trigger
			MapTimeInfo_t::FogCubeTrigger_t& trigger = CubeTriggers[i];

			//are we in the trigger or not?
			bool InBounds = (pos.x >= trigger.mins.x && pos.y >= trigger.mins.y && pos.z >= trigger.mins.z) &&
				(pos.x <= trigger.maxs.x && pos.y <= trigger.maxs.y && pos.z <= trigger.maxs.z);

			if (!InBounds)
				continue;

			//we are inside at least one trigger
			bDid = true;

			//assume this is the most inner trigger
			bool isMostInner = true;

			//compare against all other triggers
			for (int j = 0; j < CubeTriggers.Count(); j++)
			{
				if (i == j)
					continue;

				MapTimeInfo_t::FogCubeTrigger_t& other = CubeTriggers[j];

				//are we inside the other trigger?
				bool InBoundsOther = (pos.x >= other.mins.x && pos.y >= other.mins.y && pos.z >= other.mins.z) &&
					(pos.x <= other.maxs.x && pos.y <= other.maxs.y && pos.z <= other.maxs.z);

				if (!InBoundsOther)
					continue;

				//check if THIS trigger contains the other trigger
				bool ContainsOther =
					(other.mins.x >= trigger.mins.x && other.mins.y >= trigger.mins.y && other.mins.z >= trigger.mins.z) &&
					(other.maxs.x <= trigger.maxs.x && other.maxs.y <= trigger.maxs.y && other.maxs.z <= trigger.maxs.z);

				//if we contain another trigger, we are NOT the most inner
				if (ContainsOther)
				{
					isMostInner = false;
					break;
				}
			}

			//add to list regardless (keeps your original behavior)
			CubeTriggersOutsideThis.AddToTail(&CubeTriggers[i]);

			//if this is the most inner, store it
			if (isMostInner)
				best = &CubeTriggers[i];
		}

		//set the active trigger
		if (bDid && best)
		{
			*CurrentActive = best;
		}
		else
		{
			*CurrentActive = &s_MapCubeTrigger;
		}
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1
	}

	//updates the fog triggers
	void UpdateFogTriggers(CBasePlayer* pPlayer, CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers, bool forcend = false, bool fromeditor = false)
	{
		//get the cube triggers count
		if (CubeTriggers.Count() <= 0)
			return;					//dont bother with 0 triggers

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		//current time
		float curtime = gpGlobals->curtime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		//get the players pos
		Vector pos = pPlayer->WorldSpaceCenter();

		//current + old active cube trigger.
		MapTimeInfo_t::FogCubeTrigger_t* CurrentActive = &s_MapCubeTrigger;
		MapTimeInfo_t::FogCubeTrigger_t* OldActive = CurrentActive;

		//collect the fog cubes we are inside
		CUtlVector<MapTimeInfo_t::FogCubeTrigger_t*> CubeTriggersOutsideThis;
		CollectFogCubes(pos, CubeTriggers, CubeTriggersOutsideThis, &OldActive, &CurrentActive);

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		//static array of starting values
		static CUtlVector<MapTimeInfo_t::FogCubeTrigger_t::Value_t> s_FogStartingValues;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		//check the current trigger
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		if (CurrentActive != OldActive)
#else
		if (CurrentActive != OldActive || forcend || fromeditor)
#endif
		{
			//check and set OldActive
			if (OldActive) OldActive->active = false;

			//set CurrentActive
			CurrentActive->active = true;

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
			CurrentActive->inlerp = true;
			CurrentActive->lerpstarttime = curtime;
			CurrentActive->lerpendtime = curtime + CurrentActive->lerptime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

			//get the fog array. If CurrentActive is s_MapCubeTrigger then use the maps current fog
			CUtlVector<MapTimeInfo_t::FogInfo_t> foginfo;
			MapTimeInfo_t::BaseTimeInfo_t* baseinfo = IsDaytimeEnabled() ? (MapTimeInfo_t::BaseTimeInfo_t*)&g_CurrentDayNightInfo->DayInfo : (MapTimeInfo_t::BaseTimeInfo_t*)&g_CurrentDayNightInfo->NightInfo;

#if !FOG_CUBE_TRIGGER_TEST_VERSION_1
			//HACKA-REE-DOO - Always add the default value for these vars. Add to this list if needed
			static ConVar* s_HackAddDefaultFromVars[] = {
				cvar->FindVar("amod_epic_filter_lerp_time"),
			};

			for (int j = 0; j < SIZE_OF_ARRAY(s_HackAddDefaultFromVars); j++)
			{
				//check for the var
				if (!s_HackAddDefaultFromVars[j])
					continue;

				AddOrUpdateFogInfoInArray(foginfo, s_HackAddDefaultFromVars[j]->GetName(), s_HackAddDefaultFromVars[j]->GetDefault());
			}

			//always add these
			if (CurrentActive != OldActive)
			{
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("fog_lerp_system_lerp_time"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%f", fog_lerp_system_lerp_time.GetFloat())));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("fog_lerp_system_lerp_type"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%f", fog_lerp_system_lerp_type.GetFloat())));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("fog_lerp_system_lerp_parameter"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%f", fog_lerp_system_lerp_parameter.GetFloat())));
			}

			//add the defaults
			if (!fromeditor)
			{
				//add these values
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("amod_trigger_filterintensity"), gs_DayNightInfoSymbolsTable.AddString(""));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("amod_trigger_filtername"), gs_DayNightInfoSymbolsTable.AddString(""));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("mat_force_bloom"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%d", baseinfo->BloomEnabled)));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("mat_bloomscale"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%d", baseinfo->BloomScale)));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("mat_bloom_scalefactor_scalar"), gs_DayNightInfoSymbolsTable.AddString(CFmtStr("%.3f", baseinfo->BloomScalarFactor)));
				AddOrUpdateFogInfoInArray(foginfo, gs_DayNightInfoSymbolsTable.AddString("sv_skyname"), baseinfo->Skybox);

				//always add to the base array above
				CUtlVector<MapTimeInfo_t::FogInfo_t>& BaseOutsideFog = baseinfo->FogInfo;
				for (int j = 0; j < BaseOutsideFog.Count(); j++)
				{
					//wow so easy!!
					AddOrUpdateFogInfoInArray(foginfo, BaseOutsideFog[j].convar, BaseOutsideFog[j].value);
				}

				//always add the base horizon fog info
				for (int j = 0; j < baseinfo->HorizonInfo.Count(); j++)
				{
					//wow so easy x2!!
					AddOrUpdateFogInfoInArray(foginfo, baseinfo->HorizonInfo[j].convar, baseinfo->HorizonInfo[j].value);
				}
			}
			else
			{
				//HACK: update the time properties editor. This is because the filter name + skybox name + fog + other stuff doesnt get reset if we exit the zone
				if (g_MapPropertiesPanel && CurrentActive != OldActive)
				{
					void HACKShouldUpdateSkyboxAndFilter();
					HACKShouldUpdateSkyboxAndFilter();

					g_MapPropertiesPanel->UpdateNonTriggerPages();
				}
			}
#endif

			if (CurrentActive != &s_MapCubeTrigger && (!fromeditor || s_bUpdateTriggerValuesThisFrame || (fromeditor && CurrentActive != OldActive)))
			{
				//add all the fog infos from the CubeTriggersOutsideThis fog info arrays
				for (int i = 0; i < CubeTriggersOutsideThis.Count(); i++)
				{
					CUtlVector<MapTimeInfo_t::FogInfo_t>& OutsideThisFog = CubeTriggersOutsideThis[i]->foginfo;
					for (int j = 0; j < OutsideThisFog.Count(); j++)
					{
						//wow so easy again!!
						AddOrUpdateFogInfoInArray(foginfo, OutsideThisFog[j].convar, OutsideThisFog[j].value);
					}
				}

				//reset s_bUpdateTriggerValuesThisFrame
				s_bUpdateTriggerValuesThisFrame = false;
			}

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
			//set the s_MapCubeTrigger's lerptime to be the current time + the old triggers lerp time
			else if (OldActive && CurrentActive == &s_MapCubeTrigger)
				CurrentActive->lerpendtime = curtime + OldActive->lerptime;

			//clear then reset the fog info
			s_FogStartingValues.RemoveAll();
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

			for (int i = 0; i < foginfo.Count(); i++)
			{
				//look for the convar
				ConVar* var = FindFogConvar(gs_DayNightInfoSymbolsTable.String(foginfo[i].convar));
				if (!var)
				{
					//remove the element
					foginfo.Remove(i--);
					break;
				}


				//set the values
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
				MapTimeInfo_t::FogCubeTrigger_t::Value_t& value = s_FogStartingValues[s_FogStartingValues.AddToTail(MapTimeInfo_t::FogCubeTrigger_t::Value_t{ 0, 0, 0 })];
				sscanf(var->GetString(), "%f %f %f", &value.a, &value.b, &value.c);
#else
				var->SetValue(CFmtStr("%s", gs_DayNightInfoSymbolsTable.String(foginfo[i].value)));

				//check for sv_skyname and it being empty
				if (!Q_stricmp(var->GetName(), "sv_skyname") && !*var->GetString())
				{
					var->SetValue(gs_DayNightInfoSymbolsTable.String(baseinfo->Skybox));
				}
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1
			}
		}

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		//if we just started the level. Set currentactive->LerpEndTime to the start time
		if (g_bJustStartedLevel || forcend || fromeditor)
		{
			CurrentActive->lerpendtime = curtime;
			g_bJustStartedLevel = false;
		}

		//handle the lerp for the current active trigger
		if (CurrentActive->inlerp)
		{
			//get the fog array. If CurrentActive is s_MapCubeTrigger then use the maps current fog
			CUtlVector<MapTimeInfo_t::FogInfo_t> foginfo;

			//always add to the base array above
			CUtlVector<MapTimeInfo_t::FogInfo_t>& BaseOutsideFog = IsDaytimeEnabled() ? g_CurrentDayNightInfo->DayInfo.FogInfo : g_CurrentDayNightInfo->NightInfo.FogInfo;
			for (int j = 0; j < BaseOutsideFog.Count(); j++)
			{
				//wow so easy!!
				AddOrUpdateFogInfoInArray(foginfo, BaseOutsideFog[j].convar, BaseOutsideFog[j].value);
			}

			if (CurrentActive != &s_MapCubeTrigger)
			{
				//add all the fog infos from the CubeTriggersOutsideThis fog info arrays
				for (int i = 0; i < CubeTriggersOutsideThis.Count(); i++)
				{
					CUtlVector<MapTimeInfo_t::FogInfo_t>& OutsideThisFog = CubeTriggersOutsideThis[i]->foginfo;
					for (int j = 0; j < OutsideThisFog.Count(); j++)
					{
						//wow so easy!!
						AddOrUpdateFogInfoInArray(foginfo, OutsideThisFog[j].convar, OutsideThisFog[j].value);
					}
				}
			}

			//go through the vars
			for (int i = 0; i < foginfo.Count(); i++)
			{
				//check that i is in range of s_FogStartingValues
				if (i >= s_FogStartingValues.Count())
					break;

				//get the ending value
				MapTimeInfo_t::FogCubeTrigger_t::Value_t ending_value{ 0, 0, 0 };
				int count = sscanf(gs_DayNightInfoSymbolsTable.String(foginfo[i].value), "%f %f %f", &ending_value.a, &ending_value.b, &ending_value.c);

				//check the starting color + ending color match
				if (s_FogStartingValues[i].a == ending_value.a && s_FogStartingValues[i].b == ending_value.b && s_FogStartingValues[i].c == ending_value.c)
					continue;

				//look for the convar
				ConVar* var = FindFogConvar(gs_DayNightInfoSymbolsTable.String(foginfo[i].convar));
				if (!var)
				{
					//remove the element
					foginfo.Remove(i--);
					break;
				}

				//get the current value
				float pos = (curtime - CurrentActive->lerpstarttime) / (CurrentActive->lerpendtime - CurrentActive->lerpstarttime);
				if (pos > 1.0f)
				{
					pos = 1.0f;
					CurrentActive->inlerp = false;
				}

				MapTimeInfo_t::FogCubeTrigger_t::Value_t value{ 0,0,0 };
				value.a = ((ending_value.a - s_FogStartingValues[i].a) * pos) + s_FogStartingValues[i].a;
				value.b = ((ending_value.b - s_FogStartingValues[i].b) * pos) + s_FogStartingValues[i].b;
				value.c = ((ending_value.c - s_FogStartingValues[i].c) * pos) + s_FogStartingValues[i].c;

				//get the value string
				const char* setvalue = "0";
				if (count <= 1)
					setvalue = CFmtStr("%f", value.a);
				else
					setvalue = CFmtStr("%f %f %f", value.a, value.b, value.c);

				//set the convar
				var->SetValue(setvalue);
			}
		}
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		//draw the debug overlays
		if (amod_fog_cubeinfo_debug.GetBool())
		{
			//cube colors
			static Color s_CubeColors[] = {
				Color(150, 250, 0, 1),		//lime
				Color(0, 240, 255, 1),		//cyan
				Color(0, 255, 140, 1),		//aqua green
				Color(0, 50, 255, 1),		//dark blue
				Color(255, 0, 255, 1),		//magenta
				Color(240, 255, 0, 1),		//yellow
				Color(255, 100, 0, 1),		//orange
				Color(0, 255, 0, 1),		//green
			};
			static int s_CubeColorsCount = SIZE_OF_ARRAY(s_CubeColors);

			//go through all the CubeTriggersOutsideThis and draw a bbox
			for (int i = 0; i < CubeTriggersOutsideThis.Count(); i++)
			{
				Color color;

				//if i is the last trigger then make it the last s_CubeColors color
				if (i == CubeTriggersOutsideThis.Count() - 1)
					color = s_CubeColors[s_CubeColorsCount - 1];
				else
					color = s_CubeColors[(s_CubeColorsCount - 2) - (i % (s_CubeColorsCount - 1))];

				//show the bbox
				NDebugOverlay::Box(vec3_origin, CubeTriggersOutsideThis[i]->mins, CubeTriggersOutsideThis[i]->maxs, color.r(), color.g(), color.b(), color.a(), 0.05f);

				//show the text
				Vector origin = (CubeTriggersOutsideThis[i]->mins + CubeTriggersOutsideThis[i]->maxs) * 0.5f;
				NDebugOverlay::Text(origin, CFmtStr("Trigger: %s", CubeTriggersOutsideThis[i]->name), false, 0.05f);
				NDebugOverlay::Text(Vector(origin.x, origin.y, origin.z - 10), CFmtStr("Inside Level: %d", (i * -1) + (CubeTriggersOutsideThis.Count() - 1)), false, 0.05f);
			}

			//draw all the cubes we are NOT inside
			for (int i = 0; i < CubeTriggers.Count(); i++)
			{
				if (CubeTriggersOutsideThis.Find(&CubeTriggers[i]) == CubeTriggersOutsideThis.InvalidIndex() && CubeTriggers[i].mins != CubeTriggers[i].maxs)
				{
					//show the bbox
					NDebugOverlay::Box(vec3_origin, CubeTriggers[i].mins, CubeTriggers[i].maxs, 255, 0, 0, 1, 0.05f);

					//show the text
					Vector origin = (CubeTriggers[i].mins + CubeTriggers[i].maxs) * 0.5f;
					NDebugOverlay::Text(origin, CFmtStr("Trigger: %s", CubeTriggers[i].name), false, 0.05f);
					NDebugOverlay::Text(Vector(origin.x, origin.y, origin.z - 10), "Inside: 0", false, 0.05f);
				}
			}
		}
	}

	//updates this system. Called every frame
	void Update(float dt)
	{
		//only call this every 0.02 seconds
		if (gpGlobals->curtime < s_NextUpdateTime)
			return;

		//if g_MapPropertiesPanel is open then dont call this. It will get called inside the Update() function of the panels fog cube page.
		if (g_MapPropertiesPanel)
			return;

		//get the player
		CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		//get our cube triggers
		CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers = IsDaytimeEnabled() ? g_CurrentDayNightInfo->DayInfo.FogCubeTriggers : g_CurrentDayNightInfo->NightInfo.FogCubeTriggers;

		//update the stuff
		UpdateFogTriggers(pPlayer, CubeTriggers);

		//set s_NextUpdateTime
		s_NextUpdateTime = gpGlobals->curtime + 0.03f;	//30 times a second
	}
#endif //FOG_CUBE_TRIGGER_TEST

} g_TimeInfoInitalizer;


#if FOG_CUBE_TRIGGER_TEST
//------------------------------------------------------------------------------------------------------------------------------------
// Purpose: For updating the cube triggers for the 
//------------------------------------------------------------------------------------------------------------------------------------
void UpdateFogTriggers(CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers, bool fromeditor)
{
	//get the player
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	//update
	g_TimeInfoInitalizer.UpdateFogTriggers(pPlayer, CubeTriggers, false, fromeditor);
}
#endif //FOG_CUBE_TRIGGER_TEST

#endif