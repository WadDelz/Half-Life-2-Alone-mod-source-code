//alone mod time info data
#ifndef __ALONEMODTIMEINFO_H
#define __ALONEMODTIMEINFO_H

#ifdef CLIENT_DLL
#define FOG_CUBE_TRIGGER_TEST 1
#define FOG_CUBE_TRIGGER_TEST_VERSION_1 0
#else
#define FOG_CUBE_TRIGGER_TEST 0		//always 0. This is not needed for the server
#endif

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

#if FOG_CUBE_TRIGGER_TEST
	//fog cube triggers
	struct FogCubeTrigger_t
	{
		//cube trigger value
		struct Value_t
		{
			float a;
			float b;
			float c;
		};

		//fog name
		char name[256];

		//bounds
		Vector mins, maxs;

		//fog array
		CUtlVector<FogInfo_t> foginfo;

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		//lerp stuff
		bool inlerp;
		float lerptime;
		float lerpstarttime;
		float lerpendtime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		//is this active
		bool active = false;
	};
#endif

	//base time info
	struct BaseTimeInfo_t
	{
		//fog info stuff
		bool FogEnabled = false;
		CUtlVector<FogInfo_t> FogInfo;

#if FOG_CUBE_TRIGGER_TEST
		//fog cube triggers
		CUtlVector<FogCubeTrigger_t> FogCubeTriggers;
#endif

		//optional filter name + value
		UtlSymId_t FilterName;
		float FilterIntensity;

		//optional clouds color
		UtlSymId_t CloudsColor;

		//bloom
		bool BloomEnabled;
		int BloomScale;
		float BloomScalarFactor;

		//skybox
		UtlSymId_t Skybox;

		//horizon info. Just use the FogInfo_t struct cause it uses the same convar/value structure the
		//horizon convars need + i can use the AddOrUpdateFogInfoInArray function.
		bool HorizonEnabled = false;
		CUtlVector<FogInfo_t> HorizonInfo;
	};

	//night info
	struct NightInfo_t : public BaseTimeInfo_t
	{
	};

	//day info
	struct DayInfo_t : public BaseTimeInfo_t
	{
		//sun info
		struct SunInfo_t
		{
			//key,value for env_sun
			UtlSymId_t key;
			UtlSymId_t value;
		};
		bool SunInfoEnabled = false;
		CUtlVector<SunInfo_t> SunInfo;
	};

	char mapname[128];		//mapname (e.g d2_coast_01_d). do NOT keep the / of a map name (like bonus_maps/d1_trainstation_01_snowey).
	NightInfo_t NightInfo;	//night info
	DayInfo_t DayInfo;		//day info

	//allow daytime and night time
	bool AllowDaytime = true;
	bool AllowNightTime = true;

	//should the times flip? For example if this was true and the current time was day, It would make the time be night and vice versa.
	bool FlipTimes = false;
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
MapTimeInfo_t& GetMapTimeInfo(const char* mapname, bool* found = nullptr);

//returns the current day night info
extern MapTimeInfo_t* g_CurrentDayNightInfo;
MapTimeInfo_t& GetCurrentMapTimeInfo();

//returns the string from the index
const char* StringFromMapTimeStringTableIndex(UtlSymId_t id);
UtlSymId_t StringToMapTimeStringTableIndex(const char* string);

//writes the time info to the keyvalues
void WriteTimeInfoToKeyvalues(MapTimeInfo_t& info, KeyValues* out);
void WriteAllTimeInfosToFiles(const char* prefix = nullptr);

//copy fuincs
void CopyTimeInfoData(MapTimeInfo_t& from, MapTimeInfo_t& to, bool fromnight = true, bool tonight = true);

//Returns if the current map is invalid for the day/night sky change
void InitalizeDayNightInfo(bool reload = false);		//initalizes the day/night info

//fog array helpers
const char* FindFogInfoFromArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* def = "");
int AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, UtlSymId_t value);
int AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* value);

#if FOG_CUBE_TRIGGER_TEST
int AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, MapTimeInfo_t::FogCubeTrigger_t::Value_t value);
#endif

void RemoveFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key);

//sun array helpers
const char* FindSunInfoFromArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* def = "");
void AddOrUpdateSunInfoInArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* value);

#endif //__ALONEMODTIMEINFO_H