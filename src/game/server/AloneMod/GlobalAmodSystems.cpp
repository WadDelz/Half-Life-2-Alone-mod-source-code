#include "cbase.h"
#include "cdll_int.h"
#include "fmtstr.h"
#include "AloneMod/Amod_SharedDefs.h"
#include <filesystem.h>
#include "../client/AloneMod/DynamicSky.h"

#if !AMOD_DAYTIME_EDITION

#define AMOD_EPIC_FILTER_INTENSITY_DEFAULT_DAYTIME "0.325"

extern IVEngineClient* clientengine;

//change callbacks
void AmodSkyCallback(IConVar* var, const char*, float);
void AmodEpicFilterCallback(IConVar* var, const char*, float);
void AmodCloudsChangeCallback(IConVar* var, const char*, float);
void AmodSunChangeCallback(IConVar* var, const char*, float);

//sky
//ConVar amod_sky_override("amod_sky_override", "1");
ConVar amod_night_sky("amod_night_sky", "", 0, "", AmodSkyCallback);
ConVar amod_day_sky("amod_day_sky", "", 0, "", AmodSkyCallback);

//epic filter
ConVar amod_epic_filter("amod_epic_filter", "0", 0, "", AmodEpicFilterCallback);
ConVar amod_epic_filter_night_filename("amod_epic_filter_night_filename", "", 0, "", AmodEpicFilterCallback);
ConVar amod_epic_filter_night_intensity("amod_epic_filter_night_intensity", "", 0, "", AmodEpicFilterCallback);
ConVar amod_epic_filter_day_filename("amod_epic_filter_day_filename", "", 0, "", AmodEpicFilterCallback);
ConVar amod_epic_filter_day_intensity("amod_epic_filter_day_intensity", "", 0, "", AmodEpicFilterCallback);

//alone mod clouds convar
ConVar amod_clouds("amod_clouds", "0", 0, "", AmodCloudsChangeCallback);
ConVar amod_clouds_color("amod_clouds_color", "255 255 255 255", 0, "", AmodCloudsChangeCallback);
ConVar amod_clouds_color_override("amod_clouds_color_override", "0", 0, "", AmodCloudsChangeCallback);

//sun
ConVar amod_sun_disable("amod_sun_disable", "0", 0, "", AmodSunChangeCallback);

//--------------------------------------------------------------------------------------------
// Purpose: Alone mod clouds change callback
//--------------------------------------------------------------------------------------------
void AmodCloudsChangeCallback(IConVar* var, const char*, float)
{
	//check to see if we are connected or not
	if (!clientengine->IsConnected())
		return;

	//find the clouds brush
	CBaseEntity* pEntity = gEntList.FindEntityByName(nullptr, "brush_clouds");
	if (!pEntity)
		return;

	//set the clouds color
	int icolor[4];

	//convert alpha
	if (amod_clouds_color_override.GetBool())
	{
		UTIL_StringToIntArray(icolor, 4, amod_clouds_color.GetString());
		float a = icolor[3] / 255.0f;
		icolor[0] = (int)((float)icolor[0] * a);
		icolor[1] = (int)((float)icolor[1] * a);
		icolor[2] = (int)((float)icolor[2] * a);
	}
	else
	{
		//get the string
		MapTimeInfo_t& info = GetCurrentMapTimeInfo();
		const char* color = StringFromMapTimeStringTableIndex(IsDaytimeEnabled() ? info.DayInfo.CloudsColor : info.NightInfo.CloudsColor);

		//get the color now
		UTIL_StringToIntArray(icolor, 4, color);
		float a = icolor[3] / 255.0f;
		icolor[0] = (int)((float)icolor[0] * a);
		icolor[1] = (int)((float)icolor[1] * a);
		icolor[2] = (int)((float)icolor[2] * a);
	}

	pEntity->SetRenderColor(icolor[0], icolor[1], icolor[2]);

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

//--------------------------------------------------------------------------------------------
// Purpose: Creates and returns the epic color correction filter
//--------------------------------------------------------------------------------------------
CBaseEntity* CreateEpicFilter()
{
	//look for the epic filter first
	CBaseEntity* cc = gEntList.FindEntityByName(nullptr, "_cc_epic_filter_");
	if (cc)
		return cc;

	cc = CreateEntityByName("color_correction");
	cc->KeyValue("targetname", "_cc_epic_filter_");
	cc->KeyValue("minfalloff", "-1");
	cc->KeyValue("maxfalloff", "-1");

	//dont set the filename or weight here. The stuff gets set in the AmodEpicFilterCallback and CAmodAutoSystem::LevelInitPreEntity()

	cc->Precache();
	DispatchSpawn(cc);
	cc->Activate();

	return cc;
}

//--------------------------------------------------------------------------------------------
// Purpose: Change callback for the amod_sky convars
//--------------------------------------------------------------------------------------------
void AmodSkyCallback(IConVar* var, const char* oldstring, float)
{
	//see if we should override the sky or not
	//if (!amod_sky_override.GetBool())
	//	return;

	//get the convar type
	bool CalledFromDayConvar = cvar->FindVar(var->GetName()) == &amod_day_sky;

	//see if the sky is already \amod_*_sky
	if ((CalledFromDayConvar && !Q_stricmp(amod_day_sky.GetString(), oldstring)) || (!CalledFromDayConvar && !Q_stricmp(amod_night_sky.GetString(), oldstring)))
		return;

	//if dynamic sky is enabled then dont bother
#if USES_DYNAMIC_SKY
	if (ConVarRef("modbase_dynamic_skybox").GetBool())
	{
		clientengine->ClientCmd("_amod_day_do");
		return;
	}
#endif

	//check if its currently day or night
	if ((IsDaytimeEnabled() && CalledFromDayConvar) || (!IsDaytimeEnabled() && !CalledFromDayConvar))
	{
		if (gpGlobals->eLoadType == MapLoad_Background)
			clientengine->ExecuteClientCmd(CFmtStr("map_background %s", gpGlobals->mapname.ToCStr()));
		else
			clientengine->ExecuteClientCmd("save quick001_sky; load quick001_sky");
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Enables/disables the epic filter
//--------------------------------------------------------------------------------------------
void AmodEpicFilterCallback(IConVar* var, const char*, float)
{
	if (!clientengine->IsConnected())
		return;

	//if the epic filter is invalid. Then we are calling this before entities are initalized OR whilst shutting down.
	//This is because the epic filter gets created when all the entities are created.
	CBaseEntity* cc = gEntList.FindEntityByName(nullptr, "_cc_epic_filter_");
	if (!cc)
		return;

	//enable or disable
	if (amod_epic_filter.GetBool())
		cc->AcceptInput("Enable", nullptr, nullptr, variant_t{}, 0);
	else
		cc->AcceptInput("Disable", nullptr, nullptr, variant_t{}, 0);

	//check if this was called by amod_epic_filter convar
	if (var == &amod_epic_filter)
		return;

	//get the map info
	MapTimeInfo_t& info = GetCurrentMapTimeInfo();

	//set the color correction differently if daytime
	if (IsDaytimeEnabled())
	{
		//check the convars
		const char* filename = amod_epic_filter_day_filename.GetString();
		if (!filename[0])
			filename = StringFromMapTimeStringTableIndex(info.DayInfo.FilterName);

		const char* intensity = amod_epic_filter_day_intensity.GetString();
		if (!intensity[0])
			intensity = StringFromMapTimeStringTableIndex(info.DayInfo.FilterIntensity);

		cc->KeyValue("filename", filename);
		cc->KeyValue("maxweight", atof(intensity));
	}
	else
	{
		//check the convars
		const char* filename = amod_epic_filter_night_filename.GetString();
		if (!filename[0])
			filename = StringFromMapTimeStringTableIndex(info.NightInfo.FilterName);

		const char* intensity = amod_epic_filter_night_intensity.GetString();
		if (!intensity[0])
			intensity = StringFromMapTimeStringTableIndex(info.NightInfo.FilterIntensity);

		//set vars
		cc->KeyValue("filename", filename);
		cc->KeyValue("maxweight", atof(intensity));
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Sets the fog for the daytime stuff
//--------------------------------------------------------------------------------------------
void SetFogForTime(MapTimeInfo_t& info)
{
	bool disabled = !info.DayInfo.FogEnabled || info.DayInfo.FogInfo.Count() <= 0;
	if (!IsDaytimeEnabled())
		disabled = !info.NightInfo.FogEnabled || info.NightInfo.FogInfo.Count() <= 0;

	//is fog override disabled?
	if (disabled)
	{
		clientengine->ClientCmd("fog_override 0");
		return;
	}

	CUtlVector<MapTimeInfo_t::FogInfo_t>& finfo = IsDaytimeEnabled() ? info.DayInfo.FogInfo : info.NightInfo.FogInfo;

	for (int i = 0; i < finfo.Count(); i++)
	{
		//execute the fog vars
		clientengine->ClientCmd_Unrestricted(CFmtStr("%s %s", StringFromMapTimeStringTableIndex(finfo[i].convar), StringFromMapTimeStringTableIndex(finfo[i].value)));
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Sets the horizon for the day/night stuff
//--------------------------------------------------------------------------------------------
void SetHorizonForTime(MapTimeInfo_t& info)
{
	bool disabled = !info.DayInfo.HorizonEnabled || info.DayInfo.HorizonInfo.Count() <= 0;
	if (!IsDaytimeEnabled())
		disabled = !info.NightInfo.HorizonEnabled || info.NightInfo.HorizonInfo.Count() <= 0;

	//is fog override disabled?
	if (disabled)
	{
		clientengine->ClientCmd("r_horizonfog 0");
		return;
	}

	CUtlVector<MapTimeInfo_t::FogInfo_t>& finfo = IsDaytimeEnabled() ? info.DayInfo.HorizonInfo : info.NightInfo.HorizonInfo;

	for (int i = 0; i < finfo.Count(); i++)
	{
		//execute the fog vars
		clientengine->ClientCmd_Unrestricted(CFmtStr("%s %s", StringFromMapTimeStringTableIndex(finfo[i].convar), StringFromMapTimeStringTableIndex(finfo[i].value)));
	}
}

//--------------------------------------------------------------------------------------------
// Purpose: Sets the env_sun for the daytime stuff
//--------------------------------------------------------------------------------------------
void SetSunForDaytime(MapTimeInfo_t& info)
{
	//remove current sun
	CBaseEntity* currentsun = gEntList.FindEntityByClassname(nullptr, "env_sun");
	if (currentsun)
		UTIL_RemoveImmediate(currentsun);

	if (!info.DayInfo.SunInfoEnabled || info.DayInfo.SunInfo.Count() <= 0 || !IsDaytimeEnabled() || amod_sun_disable.GetBool())
		return;

	//create the new sun
	currentsun = CreateEntityByName("env_sun");

	for (int i = 0; i < info.DayInfo.SunInfo.Count(); i++)
		currentsun->KeyValue(StringFromMapTimeStringTableIndex(info.DayInfo.SunInfo[i].key), StringFromMapTimeStringTableIndex(info.DayInfo.SunInfo[i].value));

	currentsun->Precache();
	DispatchSpawn(currentsun);
	currentsun->Activate();
}

//--------------------------------------------------------------------------------------------
// Purpose: Alone mod clouds change callback
//--------------------------------------------------------------------------------------------
void AmodSunChangeCallback(IConVar* var, const char*, float)
{
	//remove current sun
	if (amod_sun_disable.GetBool())
	{
		CBaseEntity* currentsun = gEntList.FindEntityByClassname(nullptr, "env_sun");
		if (currentsun)
			UTIL_RemoveImmediate(currentsun);
	}
	else
	{
		SetSunForDaytime(GetCurrentMapTimeInfo());
	}
}

//static ConVar amod_ep2_bg_brush_color_day("amod_ep2_bg_brush_color_day", "255 255 255", 0, "The color the _bg_brush (skybox trees) will be for the episodes if amod_day is enabled.");
//static ConVar amod_ep2_bg_brush_color_night("amod_ep2_bg_brush_color_night", "41 41 41", 0, "The color the _bg_brush (skybox trees) will be for the episodes if amod_day is not enabled.");

//should we call the music transition commands
static bool s_ShouldTransitionSongs = true;

//alone mod auto game system for epic filter and day/night system
class CAmodAutoGameSystem : public CAutoGameSystem
{
public:
	void LevelInitPreEntity()
	{
		//set g_CurrentDayNightInfo. There used to be a seperate system to call this but i removed
		//that because i only really need to set it here.
		g_CurrentDayNightInfo = &GetMapTimeInfo(gpGlobals->mapname.ToCStr());

		//check for the sv_skyname convar
		static ConVar* sname = cvar->FindVar("sv_skyname");
		if (!sname)
			return;

		//disable fog lerping
		clientengine->ClientCmd_Unrestricted("fog_lerp_system_enable 0");

		bool daytime = IsDaytimeEnabled();

		//reset all the vars that can get set
		clientengine->ClientCmd_Unrestricted("mat_force_bloom 0");
		clientengine->ClientCmd_Unrestricted("fog_override 0");
		clientengine->ClientCmd_Unrestricted("fog_enable -1");
		clientengine->ClientCmd_Unrestricted("fog_enableskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_color -1 -1 -1");
		clientengine->ClientCmd_Unrestricted("fog_colorskybox -1 -1 -1");
		clientengine->ClientCmd_Unrestricted("fog_start -1");
		clientengine->ClientCmd_Unrestricted("fog_end -1");
		clientengine->ClientCmd_Unrestricted("fog_startskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_endskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_maxdensity -1");
		clientengine->ClientCmd_Unrestricted("fog_maxdensityskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_blend -1");
		clientengine->ClientCmd_Unrestricted("fog_blendangle -1");
		clientengine->ClientCmd_Unrestricted("fog_blendcolor -1 -1 -1");
		clientengine->ClientCmd_Unrestricted("fog_blendskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_blendangleskybox -1");
		clientengine->ClientCmd_Unrestricted("fog_blendcolorskybox -1 -1 -1");
		clientengine->ClientCmd_Unrestricted("fog_lerp_system_lerp_time 0.0");
		clientengine->ClientCmd_Unrestricted("r_pixelfog 1");
		clientengine->ClientCmd_Unrestricted("r_farz -1");

		//reset the horizon variables
		clientengine->ClientCmd_Unrestricted("r_horizonfog 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_no3dskyclip 1");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_top_r 0.5");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_top_g 0.7");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_top_b 1.0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_mid_r 1.0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_mid_g 0.5");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_mid_b 0.2");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_bot_r 0.1");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_bot_g 0.1");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_bot_b 0.05");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_pitch 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_yaw 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_offset_x 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_offset_y 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_offset_z 0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_height 1.0");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_width 360");
		clientengine->ClientCmd_Unrestricted("r_horizonfog_scale 0.5");

		//get the time info
		MapTimeInfo_t& info = GetCurrentMapTimeInfo();

		//enable bloom
		if (daytime)
		{
			clientengine->ClientCmd(CFmtStr("mat_force_bloom %d", info.DayInfo.BloomEnabled));
			clientengine->ClientCmd(CFmtStr("mat_bloomscale %d", info.DayInfo.BloomScale));
			clientengine->ClientCmd(CFmtStr("mat_bloom_scalefactor_scalar %f", info.DayInfo.BloomScalarFactor));
		}
		else
		{
			clientengine->ClientCmd(CFmtStr("mat_force_bloom %d", info.NightInfo.BloomEnabled));
			clientengine->ClientCmd(CFmtStr("mat_bloomscale %d", info.NightInfo.BloomScale));
			clientengine->ClientCmd(CFmtStr("mat_bloom_scalefactor_scalar %f", info.NightInfo.BloomScalarFactor));
		}

		//get the current sky name
		const char* skyname = "";
		if (daytime)
		{
			skyname = StringFromMapTimeStringTableIndex(info.DayInfo.Skybox);
			if (amod_day_sky.GetString()[0])
				skyname = amod_day_sky.GetString();
		}
		else
		{
			skyname = StringFromMapTimeStringTableIndex(info.NightInfo.Skybox);
			if (amod_night_sky.GetString()[0])
				skyname = amod_night_sky.GetString();
		}

		//set the skyname convar
		sname->SetValue(skyname);

		//enable/disable the filter if day or not
		static bool bDidFilterOn = false;
		if (daytime && !bDidFilterOn)
		{
			clientengine->ClientCmd_Unrestricted("tf2");
			clientengine->ClientCmd_Unrestricted("alias Amod_ToggleFilter");
			bDidFilterOn = true;
		}
		else if (!daytime && bDidFilterOn)
		{
			clientengine->ClientCmd_Unrestricted("alias Amod_ToggleFilter tf2; tf1");
			bDidFilterOn = false;
		}

		//set the fog + horizon
		SetFogForTime(info);
		SetHorizonForTime(info);

		//fog lerp stuff
		ConVarRef fog_lerp_system_enable("fog_lerp_system_enable");
		clientengine->ClientCmd_Unrestricted(CFmtStr("fog_lerp_system_enable %d", fog_lerp_system_enable.GetBool()));

		clientengine->ClientCmd_Unrestricted("amod_update_triggers_page");
		clientengine->ClientCmd_Unrestricted("fog_lerp_startdisabletimer");
	}

	void LevelInitPostEntity()
	{
		//create/enabble the epic filter
		CBaseEntity* cc = CreateEpicFilter();

		//get the map info
		MapTimeInfo_t& info = GetCurrentMapTimeInfo();

		//set the color correction differently if daytime
		if (IsDaytimeEnabled())
		{
			//check the convars
			const char* filename = amod_epic_filter_day_filename.GetString();
			if (!filename[0])
				filename = StringFromMapTimeStringTableIndex(info.DayInfo.FilterName);
			
			const char* intensity = amod_epic_filter_day_intensity.GetString();
			if (!intensity[0])
				intensity = StringFromMapTimeStringTableIndex(info.DayInfo.FilterIntensity);

			cc->KeyValue("filename", filename);
			cc->KeyValue("maxweight", atof(intensity));
		}
		else
		{
			//check the convars
			const char* filename = amod_epic_filter_night_filename.GetString();
			if (!filename[0])
				filename = StringFromMapTimeStringTableIndex(info.NightInfo.FilterName);

			const char* intensity = amod_epic_filter_night_intensity.GetString();
			if (!intensity[0])
				intensity = StringFromMapTimeStringTableIndex(info.NightInfo.FilterIntensity);

			//set vars
			cc->KeyValue("filename", filename);
			cc->KeyValue("maxweight", atof(intensity));
		}

		if (amod_epic_filter.GetBool())
			cc->AcceptInput("Enable", nullptr, nullptr, variant_t{}, 0);
		else
			cc->AcceptInput("Disable", nullptr, nullptr, variant_t{}, 0);

		if (s_ShouldTransitionSongs)
		{
			//transition the song panel
			if (gpGlobals->eLoadType == MapLoad_Transition)
				clientengine->ClientCmd_Unrestricted("amod_songpanel_changelevel");
			else
				clientengine->ClientCmd_Unrestricted("amod_songpanel_newlevel");
		}

		//set the sun for the daytime info
		SetSunForDaytime(info);

		do
		{
			//find the clouds brush
			CBaseEntity* pEntity = gEntList.FindEntityByName(nullptr, "brush_clouds");
			if (!pEntity)
				break;

			//enable or disable
			if (amod_clouds.GetBool())
				pEntity->AcceptInput("enable", nullptr, nullptr, variant_t{}, 0);
			else
				pEntity->AcceptInput("disable", nullptr, nullptr, variant_t{}, 0);

			//set the clouds color
			int icolor[4];

			//convert alpha
			if (amod_clouds_color_override.GetBool())
			{
				//get the color
				UTIL_StringToIntArray(icolor, 4, amod_clouds_color.GetString());
				float a = icolor[3] / 255.0f;
				icolor[0] = (int)((float)icolor[0] * a);
				icolor[1] = (int)((float)icolor[1] * a);
				icolor[2] = (int)((float)icolor[2] * a);
			}
			else
			{
				//get the string
				const char* color = StringFromMapTimeStringTableIndex(IsDaytimeEnabled() ? info.DayInfo.CloudsColor : info.NightInfo.CloudsColor);

				//get the color now
				UTIL_StringToIntArray(icolor, 4, color);
				float a = icolor[3] / 255.0f;
				icolor[0] = (int)((float)icolor[0] * a);
				icolor[1] = (int)((float)icolor[1] * a);
				icolor[2] = (int)((float)icolor[2] * a);
			}

			pEntity->SetRenderColor(icolor[0], icolor[1], icolor[2]);
		} while (false);

		//enable the _brush_night for the bottom of the episode 2s skybox
		CBaseEntity* pEntity = gEntList.FindEntityByName(nullptr, "_brush_night");
		while (pEntity)
		{
			//enable or disable
			//if (IsDaytimeEnabled())
			//	pEntity->AcceptInput("disable", nullptr, nullptr, variant_t{}, 0);
			//else
				pEntity->AcceptInput("enable", nullptr, nullptr, variant_t{}, 0);

			pEntity = gEntList.FindEntityByName(pEntity, "_brush_night");
		}

		//sets the _brush_bg for episode 2. Now obsolete (kind of)
		
		pEntity = gEntList.FindEntityByName(nullptr, "_brush_bg");
		while (pEntity)
		{
		//	//enable or disable
		//	if (IsDaytimeEnabled())
		//	{
		//		int array[3];
		//		UTIL_StringToIntArray(array, 3, amod_ep2_bg_brush_color_day.GetString());
		//		pEntity->SetRenderColor(array[0], array[1], array[2]);
		//	}
		//	else
		//	{
		//		int array[3];
		//		UTIL_StringToIntArray(array, 3, amod_ep2_bg_brush_color_night.GetString());
		//		pEntity->SetRenderColor(array[0], array[1], array[2]);
		//	}
		//
			pEntity->SetRenderColor(255, 255, 255);
			pEntity = gEntList.FindEntityByName(pEntity, "_brush_bg");
		}
	}
};
static CAmodAutoGameSystem g_AmodAutoGameSystem;

//resets the time system
CON_COMMAND_F(_amod_day_do, "", FCVAR_HIDDEN)
{
	s_ShouldTransitionSongs = false;
	g_AmodAutoGameSystem.LevelInitPreEntity();
	g_AmodAutoGameSystem.LevelInitPostEntity();
	s_ShouldTransitionSongs = true;
}







//this convar is used for the cube triggers.
void AmodTriggerFilterIntensityChange(IConVar* var, const char*, float);
void AmodTriggerFilterNameChange(IConVar* var, const char*, float);

ConVar amod_trigger_filterintensity("amod_trigger_filterintensity", "0", FCVAR_HIDDEN, nullptr, AmodTriggerFilterIntensityChange);
ConVar amod_trigger_filtername("amod_trigger_filtername", "0", FCVAR_HIDDEN, nullptr, AmodTriggerFilterNameChange);

//changes the intensity of the epic filter
void AmodTriggerFilterIntensityChange(IConVar* var, const char*, float)
{
	CBaseEntity* cc = gEntList.FindEntityByName(nullptr, "_cc_epic_filter_");
	if (!cc)
		return;

	//get the map info
	MapTimeInfo_t& info = GetCurrentMapTimeInfo();
	const char* string = ConVarRef(var).GetString();

	//set the color correction differently if daytime
	if (IsDaytimeEnabled())
	{
		if (!string[0])
			string = StringFromMapTimeStringTableIndex(info.DayInfo.FilterIntensity);

		cc->KeyValue("maxweight", atof(string));
	}
	else
	{
		if (!string[0])
			string = StringFromMapTimeStringTableIndex(info.NightInfo.FilterIntensity);

		//set vars
		cc->KeyValue("maxweight", atof(string));
	}
}

//changes the name of the epic filter
void AmodTriggerFilterNameChange(IConVar* var, const char*, float)
{
	CBaseEntity* cc = gEntList.FindEntityByName(nullptr, "_cc_epic_filter_");
	if (!cc)
		return;

	//get the map info
	MapTimeInfo_t& info = GetCurrentMapTimeInfo();
	const char* string = ConVarRef(var).GetString();

	//set the color correction differently if daytime
	if (IsDaytimeEnabled())
	{
		//check the convars
		if (!string[0])
			string = StringFromMapTimeStringTableIndex(info.DayInfo.FilterName);

		cc->KeyValue("filename", string);
	}
	else
	{
		//check the convars
		if (!string[0])
			string = StringFromMapTimeStringTableIndex(info.NightInfo.FilterName);

		//set vars
		cc->KeyValue("filename", string);
	}

}

//simple fog setter for background06_d
//class CAmodFogSetter : public CBaseEntity
//{
//public:
//	DECLARE_CLASS(CAmodFogSetter, CBaseEntity);
//	DECLARE_DATADESC();
//
//	void InputSetFog(inputdata_t& input);
//	void OnRestore();
//private:
//	string_t m_szFogName;
//};
//
//LINK_ENTITY_TO_CLASS(amod_fog_setter, CAmodFogSetter);
//
//BEGIN_DATADESC(CAmodFogSetter)
//DEFINE_KEYFIELD(m_szFogName, FIELD_STRING, "fogname"),
//DEFINE_INPUTFUNC(FIELD_STRING, "SetFog", InputSetFog)
//END_DATADESC()
//
//void CAmodFogSetter::InputSetFog(inputdata_t& input)
//{
//	static ConVarRef amod_day("amod_day");
//	if (!amod_day.GetBool())
//		return;
//
//	//set the fog
//	m_szFogName = AllocPooledString(input.value.String());
//	SetFogForDaytime(GetMapTimeInfo(m_szFogName.ToCStr()));
//}
//
//void CAmodFogSetter::OnRestore()
//{
//	BaseClass::OnRestore();
//	static ConVarRef amod_day("amod_day");
//	if (!amod_day.GetBool())
//		return;
//
//	//set the fog
//	SetFogForDaytime(GetMapTimeInfo(V_GetFileName(gpGlobals->mapname.ToCStr())));
//}

#endif




//core timer thinggy for episode 1
class CAmodCoreTimer : public CBaseEntity
{
public:
	DECLARE_CLASS(CAmodCoreTimer, CBaseEntity);
	DECLARE_DATADESC();

	CAmodCoreTimer()
	{
		m_bEnabled = true;
	}

	//inputs
	void InputStartCoreTimer(inputdata_t& input);
	void InputStartCitadelTimer(inputdata_t& input);
	void InputStopCoreTimer(inputdata_t& input);
	void InputStopCitadelTimer(inputdata_t& input);
	void InputShowCoreTime(inputdata_t& input);
	void InputShowCitadelTime(inputdata_t& input);
	void InputDisable(inputdata_t& input) { m_bEnabled = false; }
private:
	bool m_bEnabled = true;
};

LINK_ENTITY_TO_CLASS(amod_core_timer, CAmodCoreTimer);

BEGIN_DATADESC(CAmodCoreTimer)
DEFINE_INPUTFUNC(FIELD_INTEGER, "StartCoreTimer", InputStartCoreTimer),
DEFINE_INPUTFUNC(FIELD_INTEGER, "StartCitadelTimer", InputStartCitadelTimer),
DEFINE_INPUTFUNC(FIELD_VOID, "StopCoreTimer", InputStopCoreTimer),
DEFINE_INPUTFUNC(FIELD_VOID, "StopCitadelTimer", InputStopCitadelTimer),
DEFINE_INPUTFUNC(FIELD_VOID, "ShowCoreTime", InputShowCoreTime),
DEFINE_INPUTFUNC(FIELD_VOID, "ShowCitadel", InputShowCitadelTime),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
END_DATADESC()

//core timer convars
ConVar amod_do_core_timer("amod_do_core_timer", "1");
ConVar amod_do_citadel_timer("amod_do_citadel_timer", "1");

void CAmodCoreTimer::InputStartCoreTimer(inputdata_t& input)
{
	if (!amod_do_core_timer.GetBool())
		return;

	int seconds = input.value.Int();

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	pPlayer->m_fCoreTimerTime = gpGlobals->curtime + (float)seconds;
	pPlayer->m_bInCoreTimer = true;

	char buf[128];
	Q_snprintf(buf, sizeof(buf), "%d:%02d Minutes Till Core Collapse", seconds / 60, seconds % 60);
	UTIL_HudHintText(pPlayer, buf);
}

void CAmodCoreTimer::InputStartCitadelTimer(inputdata_t& input)
{
	if (!amod_do_citadel_timer.GetBool())
		return;

	int seconds = input.value.Int();

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	pPlayer->m_fCitadelTimerTime = gpGlobals->curtime + (float)seconds;
	pPlayer->m_bInCitadelTimer = true;


	char buf[128];
	Q_snprintf(buf, sizeof(buf), "%d:%02d Minutes Till Citadel Explosion", seconds / 60, seconds % 60);
	UTIL_HudHintText(pPlayer, buf);
}

void CAmodCoreTimer::InputStopCoreTimer(inputdata_t& input)
{
	if (!amod_do_core_timer.GetBool())
		return;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	UTIL_HudHintText(pPlayer, "Citadel Core Neutralized");
	pPlayer->m_bInCoreTimer = false;
}

void CAmodCoreTimer::InputStopCitadelTimer(inputdata_t& input)
{
	if (!amod_do_citadel_timer.GetBool())
		return;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	pPlayer->m_bInCitadelTimer = false;
	UTIL_HudHintText(pPlayer, "Citadel Explosion Starting Now..");
}

void CAmodCoreTimer::InputShowCitadelTime(inputdata_t& input)
{
	if (!m_bEnabled || !amod_do_citadel_timer.GetBool())
		return;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (!pPlayer->m_bInCitadelTimer)
		return;

	char buf[128];
	int ctime = (int)(pPlayer->m_fCitadelTimerTime - gpGlobals->curtime);
	Q_snprintf(buf, sizeof(buf), "%d:%02d Minutes Till Citadel Explosion", (int)(ctime / 60), (int)((int)ctime % 60));
	UTIL_HudHintText(pPlayer, buf);
}

void CAmodCoreTimer::InputShowCoreTime(inputdata_t& input)
{
	if (!m_bEnabled || !amod_do_core_timer.GetBool())
		return;

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (!pPlayer->m_bInCoreTimer)
		return;

	char buf[128];
	int ctime = (int)(pPlayer->m_fCoreTimerTime - gpGlobals->curtime);
	Q_snprintf(buf, sizeof(buf), "%d:%02d Minutes Till Core Collapse", (int)(ctime / 60), (int)((int)ctime % 60));
	UTIL_HudHintText(pPlayer, buf);
}


//-----------------------------------------------------------------------------------------------------------
// Purpose: Map properties editor sun command to create or destroy it
//-----------------------------------------------------------------------------------------------------------
CON_COMMAND_F(_amod_mapedit_server_sun, "", FCVAR_HIDDEN)
{
	//delete or create?
	bool create = atoi(args.Arg(1)) != 0;

	if (create)
	{
		CBaseEntity* pSun = gEntList.FindEntityByClassname(nullptr, "env_sun");
		if (pSun)
			return;

		pSun = CreateEntityByName("env_sun");
		pSun->Precache();
		DispatchSpawn(pSun);
		pSun->Activate();
	}
	else
	{
		CBaseEntity* pSun = gEntList.FindEntityByClassname(nullptr, "env_sun");
		if (pSun)
			UTIL_Remove(pSun);
	}
}

float OppositeAngle(float angle)
{
	float opposite = angle + 180.0f;
	if (opposite >= 360.0f)
		opposite -= 360.0f;
	else if (opposite < 0.0f)
		opposite += 360.0f;
	return opposite;
}

//-----------------------------------------------------------------------------------------------------------
// Purpose: Map properties editor sun command to set its angle to the players eyes
//-----------------------------------------------------------------------------------------------------------
CON_COMMAND_F(_amod_mapedit_server_sun_keyvalue, "", FCVAR_HIDDEN)
{
	//look for the player
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (!pPlayer)
		return;

	//get the sun
	CBaseEntity* pSun = gEntList.FindEntityByClassname(nullptr, "env_sun");
	if (!pSun)
		return;

	pSun->KeyValue(args.Arg(1), args.Arg(2));
}