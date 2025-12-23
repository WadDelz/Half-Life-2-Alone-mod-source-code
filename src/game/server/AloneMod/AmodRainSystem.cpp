#include "cbase.h"
#include "cdll_int.h"
#include "fmtstr.h"
#include <filesystem.h>
#include <engine/IEngineSound.h>

//the file containing all the info on the thunder sounds
static KeyValues* g_kvThunderFile;

static bool bDidInit = false;
extern IVEngineClient* clientengine;

void AmodRainChangeCallback(IConVar* cvar, const char*, float);
void AmodRainWaitMaxChangeCallback(IConVar* cvar, const char*, float);

//rain/weather convars
ConVar amod_rain_type("amod_rain_type", "1", 0, "", AmodRainChangeCallback);
ConVar amod_rain_enable("amod_rain_enable", "0", 0, "", AmodRainChangeCallback);
ConVar amod_rain_thunder("amod_rain_thunder", "0", 0);
ConVar amod_rain_wait_min("amod_rain_wait_min", "300", 0);
ConVar amod_rain_wait_max("amod_rain_wait_max", "600", 0, "", AmodRainWaitMaxChangeCallback);
ConVar amod_do_breathing("amod_do_breathing", "0");
ConVar amod_weather_info("amod_weather_info", "0");

//change callback for alone mod rain convars
void AmodRainChangeCallback(IConVar* cvar, const char*, float val)
{
	//check for background map
	if (gpGlobals->eLoadType == MapLoad_Background || !clientengine->IsConnected())
		return;

	//check to see if rain is enabled
	bool bRainEnabled = true;

	int type = amod_rain_type.GetInt();
	if (!type || !amod_rain_enable.GetBool())
	{
		bRainEnabled = false;
	}
	else
	{
		if (type == 2)
			bRainEnabled = random->RandomInt(0, 1) != 0;
	}

	if (bRainEnabled)
	{
		//trigger all the _rainrel entities
		CBaseEntity* FirstEnt = gEntList.FirstEnt();
		while (FirstEnt)
		{
			if (!Q_strcmp(FirstEnt->GetEntityName().ToCStr(), "_rainrel"))
				FirstEnt->AcceptInput("Trigger", nullptr, nullptr, variant_t{}, 0);

			FirstEnt = gEntList.NextEnt(FirstEnt);
			continue;
		}

		clientengine->ClientCmd("wait 15; amod_soundscape_startrain");
	}
	else
	{
		//stop the rain sounds
		clientengine->ClientCmd_Unrestricted("amod_soundscape_stoprain");
	}

	//disable/enable rain windows
	CBaseEntity* FirstEnt = gEntList.FirstEnt();
	while (FirstEnt)
	{
		if (!Q_stricmp(FirstEnt->GetEntityName().ToCStr(), "brush_rain_windows"))
		{
			if (bRainEnabled)
				FirstEnt->AcceptInput("Enable", nullptr, nullptr, variant_t{}, 0);
			else
				FirstEnt->AcceptInput("Disable", nullptr, nullptr, variant_t{}, 0);
		}

		FirstEnt = gEntList.NextEnt(FirstEnt);
		continue;
	}

	//set the player's variable
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer)
	{
		pPlayer->m_fNextRainTime = gpGlobals->curtime + random->RandomFloat(amod_rain_wait_min.GetFloat(), amod_rain_wait_max.GetFloat());
		pPlayer->m_bInRain = bRainEnabled;
	}
}

void AmodRainWaitMaxChangeCallback(IConVar* cvar, const char*, float)
{
	if (!amod_rain_enable.GetBool() || amod_rain_type.GetInt() != 2)
		return;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer)
	{
		if (pPlayer->m_fNextRainTime - gpGlobals->curtime > amod_rain_wait_max.GetInt())
			pPlayer->m_fNextRainTime = gpGlobals->curtime + amod_rain_wait_max.GetInt();
	}
}

//alone mod clouds convar change callbacl
void AmodCloudsChangeCallback(IConVar* var, const char*, float);

//alone mod clouds convar
ConVar amod_clouds("amod_clouds", "0", 0, "", AmodCloudsChangeCallback);

void AmodCloudsChangeCallback(IConVar* var, const char*, float)
{
	//check to see if we are connected or not
	if (!clientengine->IsConnected())
		return;

	//find the clouds brush
	CBaseEntity* pEntity = gEntList.FindEntityByName(nullptr, "brush_clouds");
	if (!pEntity)
		return;

	//enable or disable
	if (amod_clouds.GetBool())
	{
		pEntity->AcceptInput("enable", nullptr, nullptr, variant_t{}, 0);
	}
	else
	{
		pEntity->AcceptInput("disable", nullptr, nullptr, variant_t{}, 0);
	}
}

//play a random thunder sound
void PlayRandomThunder()
{
	if (!g_kvThunderFile)
		return;

	static ConCommand* playgamesound = cvar->FindCommand("playgamesound");
	if (!playgamesound)
		return;

	KeyValues* mapkv = g_kvThunderFile->FindKey(gpGlobals->mapname.ToCStr());
	if (!mapkv)
		return;

	CUtlVector<KeyValues*> tmpvec;
	FOR_EACH_VALUE(mapkv, kv)
	{
		tmpvec.AddToTail(kv);
	}

	const char* origin = tmpvec[random->RandomInt(0, tmpvec.Count() >= 1 ? tmpvec.Count() - 1 : 0)]->GetString();

	CCommand args;
	args.Tokenize(CFmtStr("playgamesound weather.thunder%d \"%s\"", random->RandomInt(1, 3), origin));
	playgamesound->Dispatch(args);
}

//debugging command
CON_COMMAND(amod_play_random_thunder, "")
{
	PlayRandomThunder();
}

//prints the rain info
CON_COMMAND(amod_raininfo, "")
{
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (!pPlayer)
		return;

	ConMsg("Currently In rain %d\n", pPlayer->m_bInRain);
	ConMsg("Next Rain Time %f\n", pPlayer->m_fNextRainTime - gpGlobals->curtime);
	ConMsg("Next Thunder Time %f\n", pPlayer->m_fNextThunder - gpGlobals->curtime);
}

class CAutoAmodRainSystem : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		clientengine->ClientCmd_Unrestricted("exec \"rain/background_main");	//ALWAYS run this

		//if the r_rainradius value is to high it will crash. i made .cfg files for each map so i can make the r_rainradius
		//value the highest i can on maps where there isnt that much rain like d1_trainstation_01_d
		clientengine->ClientCmd_Unrestricted(CFmtStr("exec \"rain/%s", gpGlobals->mapname.ToCStr()));
		clientengine->ClientCmd_Unrestricted("rain_density_check");	//do te rain density check after the file's get executed

		if (!bDidInit)
		{
			g_kvThunderFile = new KeyValues("RainSystem");
			if (!g_kvThunderFile->LoadFromFile(filesystem, "resource/thunder_locations.txt", "MOD"))
			{
				ConWarning("Failed To Load resource/thunder_locations.txt Thunder Sounds Will Not Play\n");
				g_kvThunderFile->deleteThis();
				g_kvThunderFile = nullptr;
			}
			bDidInit = true;
		}
	}

	void LevelInitPostEntity()
	{
		//find the clouds brush
		CBaseEntity* pEntity = gEntList.FindEntityByName(nullptr, "brush_clouds");
		if (!pEntity)
			return;

		//enable or disable
		if (amod_clouds.GetBool())
			pEntity->AcceptInput("enable", nullptr, nullptr, variant_t{}, 0);
		else
			pEntity->AcceptInput("disable", nullptr, nullptr, variant_t{}, 0);
	}
};
static CAutoAmodRainSystem g_RainSystem;