#ifndef __AMOD_SHARED_DEFS_H
#define __AMOD_SHARED_DEFS_H

extern const char* g_szMapNames[98];

#ifdef CLIENT_DLL
extern const char* szMapName;
#endif

//TODO: make this not hard coded in
extern const char* g_pBonusMaps[4];

//TODO: this is now obsolete since i have created the MapTimeInfo_t. Remove me!!
#define AMOD_DAYTIME_EDITION 0

//map times info
struct MapTimeInfo_t
{
	//fog info
	struct FogInfo_t
	{
		//convar,value for fog_* convars
		UtlSymId_t convar;
		UtlSymId_t value;
	};


	//night info
	struct NightInfo_t
	{
		//fog info stuff
		bool FogEnabled = false;
		CUtlVector<FogInfo_t> FogInfo;

		UtlSymId_t DefaultNightSky;		//the default night sky name

		//optional filter name + value
		UtlSymId_t FilterName;
		UtlSymId_t FilterIntensity;

		//optional clouds color
		UtlSymId_t CloudsColor;

		//bloom
		UtlSymId_t BloomEnabled;
		UtlSymId_t BloomScale;
		UtlSymId_t BloomScalarFactor;
	};
	
	//day info
	struct DayInfo_t
	{
		//sun info
		struct SunInfo_t
		{
			//key,value for env_sun
			UtlSymId_t key;
			UtlSymId_t value;
		};
		UtlSymId_t DefaultDaySky;			//default day sky name

		//fog info stuff
		bool FogEnabled = false;
		CUtlVector<FogInfo_t> FogInfo;

		//the sun info
		bool SunInfoEnabled = false;
		CUtlVector<SunInfo_t> SunInfo;

		//optional filter name + value
		UtlSymId_t FilterName;
		UtlSymId_t FilterIntensity;

		//optional clouds color
		UtlSymId_t CloudsColor;

		//bloom
		UtlSymId_t BloomEnabled;
		UtlSymId_t BloomScale;
		UtlSymId_t BloomScalarFactor;
	};

	char mapname[128];		//mapname (e.g d2_coast_01_d). do NOT keep the / of a map name (like bonus_maps/d1_trainstation_01_snowey).
	NightInfo_t NightInfo;	//night info
	DayInfo_t DayInfo;		//day info
};

//map times info base
struct MapTimeInfoBase_t
{
	char filename[512];
	CUtlVector<MapTimeInfo_t> base;
};

//returns all the time info
CUtlVector<MapTimeInfoBase_t>& GetDayNightInfo();

//is daytime enabled or not
bool IsDaytimeEnabled();
MapTimeInfo_t& GetMapTimeInfo(const char* mapname);

//returns the string from the index
const char* StringFromMapTimeStringTableIndex(UtlSymId_t id);
UtlSymId_t StringToMapTimeStringTableIndex(const char* string);

//writes the time info to the keyvalues
void WriteTimeInfoToKeyvalues(MapTimeInfo_t& info, KeyValues* out);
void WriteAllTimeInfosToFiles();

//copy fuincs
void CopyTimeInfoData(MapTimeInfo_t& from, MapTimeInfo_t& to, bool copynight = true, bool copyday = true);

//Returns if the current map is invalid for the day/night sky change
bool IsInvalidChangeMap(const char* map);
void InitalizeDayNightInfo(bool reload = false);		//initalizes the day/night info

#endif