//HOLY FUCK IM FINALLY DONE (i fucking hope so) I HAVBE BEEN DOING THIS FOR THE LAST 8 HOURS AND NOW IM FINALLY DONE
//IT HAS TAKEN OVER 3 DAYS YIPPE
#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <filesystem.h>
#include <ienginevgui.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/label.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ToolTip.h>
#include <vgui/ISurface.h>
#include "fmtstr.h"
#include "IoptionsPanel.h"
#include "AloneMod/AmodCvars.h"
#include "movevars_shared.h"
#include "AloneMod/Amod_SharedDefs.h"

using namespace vgui;

//convar declirations
void AmodDayChangeCallback(IConVar* var, const char*, float);
void AmodDayRavChangeCallback(IConVar* var, const char*, float);

//writes the alone mod config
void Amod_WriteConfig();

#if !AMOD_DAYTIME_EDITION
//day time convars
ConVar amod_day("amod_day", "0", 0, "", AmodDayChangeCallback);
ConVar amod_day_ravenholm("amod_day_ravenholm", "0", 0, "", AmodDayRavChangeCallback);
#endif

//use the new ending?
ConVar amod_new_ending("amod_new_ending", "0", 0, "");

//HACK HACK: set if the soundscape button should be disabled or not
static bool g_NeedSoundscapeEnable = false;



//quick class to get the name of the current map
class CAutoAmodDaySystem : public CAutoGameSystemPerFrame
{
public:
	void LevelInitPreEntity()
	{
		szMapName = MapName();
	}
};
CAutoAmodDaySystem g_AutoAmodDaySys;



//alone mod credits panel
class CAModCreditsPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CAModCreditsPanel, vgui::Frame);
public:

	//constructor
	CAModCreditsPanel(Panel* parent);

	//destructor
	~CAModCreditsPanel();
};

//------------------------------------------------------------------------------------------
// Purpose: Constructor for alone mod credits panel
//------------------------------------------------------------------------------------------
CAModCreditsPanel::CAModCreditsPanel(Panel* parent) : vgui::Frame(parent, "AloneModCreditsPanel")
{
	//set the parent
	SetParent(parent);

	//set the keyboard stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set other panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
	SetDeleteSelfOnClose(true);

	//load the panel scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//set the size and pos of the panel
	SetBounds(100, 100, 245, 360);
	SetTitle("Credits", false);

	//create the actual credits text box
	RichText* CreditsText = new RichText(this, "Credits");
	CreditsText->SetBounds(10, 25, 225, 330);
	CreditsText->SetVerticalScrollbar(false);
	CreditsText->SetText("Creator: Waddelz\n\nGame: Valve\n\nSoundtrack:\n portal ->\n Self Esteem Fund, Android Hell\n Stop What You Are Doing,\n HL2 Episode 2 ->\n Dark Inverval\n\nHuge Thanks To dominicb\nFor The Song: Gone\nAnd TheAR3Guy For The Song\nEnd\n\nMirrored Source Code: NvC-DmN-CH\n\nModdb: For Containing The Mod\n\n");
}

//------------------------------------------------------------------------------------------
// Purpose: Destructor
//------------------------------------------------------------------------------------------
CAModCreditsPanel::~CAModCreditsPanel()
{
	//set this panel to nullptr for the credits panel interface
	creditspanel->ClearPanel();
}



//alone mod weather panel
#define WEATHER_PANEL_COMMAND_APPLY "Apply"

class CAModWeatherPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CAModWeatherPanel, vgui::Frame);
public:

	//constructor
	CAModWeatherPanel(Panel* parent);

	//panel functions
	void OnTick();
	void OnCommand(const char* command);

	//destructor
	~CAModWeatherPanel();
private:

	//panel items
	CheckButton* m_cbEnable;
	CheckButton* m_cbThunder;
	ComboBox* m_cbType;
};

//------------------------------------------------------------------------------------------
// Purpose: Constructor for the alone mod weather panel
//------------------------------------------------------------------------------------------
CAModWeatherPanel::CAModWeatherPanel(Panel* parent) : vgui::Frame(parent, "CAModWeatherPanel")
{
	//set the parent
	SetParent(parent);

	//set keyboard stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set the other panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
	SetDeleteSelfOnClose(true);

	//load panel scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//set bounds and title
	SetBounds(100, 100, 180, 190);
	SetTitle("Weather Panel", false);

	//load the convars for the settings
	ConVarRef amod_rain_type("amod_rain_type");
	ConVarRef amod_rain_enable("amod_rain_enable");
	ConVarRef amod_rain_thunder("amod_rain_thunder");

	//create the 'enable rain' check button
	m_cbEnable = new CheckButton(this, "CheckButtonEnableRain", "Enable Rain");
	m_cbEnable->SetSelected(amod_rain_enable.GetBool());
	m_cbEnable->SetBounds(2, 21, 150, 25);

	//create the 'enable thunder sounds' check button
	m_cbThunder = new CheckButton(this, "CheckButtonEnableThunder", "Enable Thunder Sounds");
	m_cbThunder->SetSelected(amod_rain_thunder.GetBool());
	m_cbThunder->SetBounds(2, 42, 175, 25);
	m_cbThunder->SetEnabled(amod_rain_enable.GetBool());

	//create 'rain type' label
	Label* RainType = new Label(this, "RainType", "Rain Type");
	RainType->SetBounds(60, 64, 160, 20);

	//create the type of rain button
	m_cbType = new ComboBox(this, "RainType", 2, false);
	m_cbType->SetBounds(7, 85, 167, 20);
	m_cbType->AddItem("Always Rain", nullptr);
	m_cbType->AddItem("Rain In Intervals", nullptr);
	m_cbType->ActivateItem(amod_rain_type.GetInt() ? amod_rain_type.GetInt() - 1 : 0);
	m_cbType->SetEnabled(amod_rain_enable.GetBool());

	//make the apply button
	Button* ApplyButton = new Button(this, "", "Apply Settings");
	ApplyButton->SetBounds(5, 164, 167, 20);
	ApplyButton->SetCommand(WEATHER_PANEL_COMMAND_APPLY);

	//create a tick signal for this panel so every 100ms the OnTick function gets called
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
}

//------------------------------------------------------------------------------------------
// Purpose: Called every 100ms
//------------------------------------------------------------------------------------------
void CAModWeatherPanel::OnTick()
{
	//enable or disable the check buttons/combo box's
	m_cbThunder->SetEnabled(m_cbEnable->IsSelected());
	m_cbType->SetEnabled(m_cbEnable->IsSelected());

	//call the base function
	BaseClass::OnTick();
}

//------------------------------------------------------------------------------------------
// Purpose: Called when a command for the panel is ran
//------------------------------------------------------------------------------------------
void CAModWeatherPanel::OnCommand(const char* command)
{
	//check for the 'close' command
	if (!Q_strcmp(command, WEATHER_PANEL_COMMAND_APPLY))
	{
		//get the convars
		static ConVarRef amod_rain_enable("amod_rain_enable");
		static ConVarRef amod_rain_type("amod_rain_type");

		//enable/disable the rain
		if (m_cbEnable->IsSelected() ^ amod_rain_enable.GetBool())
			engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_enable %d", m_cbEnable->IsSelected()));

		//set the rain type
		if (m_cbType->GetActiveItem() + 1 ^ amod_rain_type.GetInt())
			engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_type %d", m_cbType->GetActiveItem() + 1));

		//enable/disable the thunder
		engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_thunder %d", m_cbThunder->IsSelected()));

		//set g_NeedSoundscapeEnable
		g_NeedSoundscapeEnable = m_cbEnable->IsSelected();
		return;
	}

	BaseClass::OnCommand(command);
}

//------------------------------------------------------------------------------------------
// Purpose: destructor
//------------------------------------------------------------------------------------------
CAModWeatherPanel::~CAModWeatherPanel()
{
	//tell the weather panel interface to clear the pointer to this panel
	weatherpanel->ClearPanel();
}



//skybox panel
#define SKYBOX_COMMAND_SET_BUTTON "SetSkybox"

class CAModSkyboxPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CAModSkyboxPanel, vgui::Frame);
public:

	//constructor
	CAModSkyboxPanel(Panel* parent);

	//panel functions
	void OnTick();
	void OnCommand(const char* pcCommand);

	//destructor
	~CAModSkyboxPanel();

private:

	//variables
	KeyValues* m_SkyboxFile;
	CUtlVector<const char*> m_Skyboxes;

	//panel items
	ComboBox* m_skyComboBox;
	ImagePanel* m_skyImage;
	CheckButton* m_FogDisable;

	//other variables
	bool m_bPrevSelected = false;
	bool m_bInit = false;
};

//------------------------------------------------------------------------------------------
// Purpose: Constructor for the alone mod skybox panel
//------------------------------------------------------------------------------------------
CAModSkyboxPanel::CAModSkyboxPanel(Panel* parent) : vgui::Frame(parent, "AloneModSkyboxPanel")
{
	//set the parent
	SetParent(parent);

	//enable/disable keyboard input
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
	SetDeleteSelfOnClose(true);

	//load the scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//add a tick signal so OnTick gets called once every 100ms
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	//set the bounds and title of the panel
	SetBounds(150, 150, 180, 230);
	SetTitle("Skybox Panel", false);

	//load all the skybox's from SkyPanel.txt
	m_SkyboxFile = new KeyValues("SkyboxFile");
	if (!m_SkyboxFile->LoadFromFile(filesystem, "resource/panels/SkyPanel.txt"))
	{
		m_SkyboxFile->deleteThis();
		m_SkyboxFile = nullptr;

		//send warning
		ConWarning("Failed To Open resource/panels/SkyPanel.txt\n");
		return;
	}

	//get the current sky name
	ConVarRef amod_sky("amod_sky");

	int index = 0, current = 0;

	//before anything make the skybox's combo box so i can add the skybox names to it
	m_skyComboBox = new ComboBox(this, "m_skyComboBox", 14, false);
	m_skyComboBox->SetBounds(10, 180, 160, 20);

	FOR_EACH_VALUE(m_SkyboxFile, sub)
	{
		//add the skybox name
		m_Skyboxes.AddToTail(sub->GetString());

		//add the name to the m_skyComboBox
		m_skyComboBox->AddItem(sub->GetName(), nullptr);

		//if the skyname == amod_sky then set index to current
		if (!Q_strcmp(amod_sky.GetString(), sub->GetString()))
			index = current;

		//increment current
		current++;
	}

	//active the item
	m_skyComboBox->ActivateItem(index);

	//make the fog disable button
	m_FogDisable = new CheckButton(this, "", "Disable Fog");
	m_FogDisable->SetBounds(29, 159, 161, 20);
	m_FogDisable->SetSelected(amod_fog_disabled.GetBool());

	//get the sky name
	char buf[218];
	Q_snprintf(buf, sizeof(buf), "skybox/%s", amod_sky.GetString());

	m_skyImage = new ImagePanel(this, "m_SkyImage");
	m_skyImage->SetImage((m_Skyboxes.Size() != 0) ? m_Skyboxes[0] : buf);
	m_skyImage->SetBounds(10, 30, 160, 130);

	//make the apply button
	Button* SetButton = new Button(this, "SetButton", "Set Skybox");
	SetButton->SetBounds(10, 205, 160, 20);
	SetButton->SetCommand(SKYBOX_COMMAND_SET_BUTTON);

	//initalized
	m_bInit = true;
}

//------------------------------------------------------------------------------------------
// Purpose: Called on command
//------------------------------------------------------------------------------------------
void CAModSkyboxPanel::OnCommand(const char* pcCommand)
{
	//check the command
	if (!Q_strcmp(pcCommand, SKYBOX_COMMAND_SET_BUTTON))
	{
		//check the current skybox name
		ConVar* sv_skyname = cvar->FindVar("sv_skyname");
		if (sv_skyname)
		{
			if (!Q_strcmp(sv_skyname->GetString(), (m_Skyboxes.Size() != 0) ? m_Skyboxes[m_skyComboBox->GetActiveItem()] : "skybox/borealis01"))
				return;
		}

		//set amod_sky to the current sky
		engine->ExecuteClientCmd(CFmtStr("amod_sky %s", (m_Skyboxes.Size() != 0) ? m_Skyboxes[m_skyComboBox->GetActiveItem()] : "skybox/borealis01"));
	}

	BaseClass::OnCommand(pcCommand);
}

//------------------------------------------------------------------------------------------
// Purpose: Called every 100ms
//------------------------------------------------------------------------------------------
void CAModSkyboxPanel::OnTick()
{
	//if this isnt initalized then return
	if (!m_bInit)
		return;

	//check to see if we need to toggle the fog or not
	if (m_bPrevSelected != m_FogDisable->IsSelected())
	{
		m_bPrevSelected = m_FogDisable->IsSelected();
		amod_fog_disabled.SetValue(m_bPrevSelected);
		engine->ClientCmd_Unrestricted(CFmtStr("amod_fog_disabled %d", m_bPrevSelected));
	}

	//set the skybox image
	m_skyImage->SetImage((m_Skyboxes.Size() != 0) ? m_Skyboxes[m_skyComboBox->GetActiveItem()] : "skybox/borealis01");
}

//------------------------------------------------------------------------------------------
// Purpose: Destructor for skybox panel
//------------------------------------------------------------------------------------------
CAModSkyboxPanel::~CAModSkyboxPanel()
{
	//delete m_SkyboxFile if needed
	if (m_SkyboxFile)
		m_SkyboxFile->deleteThis();

	//set pointer in interface to nullptr
	skyboxpanel->ClearPanel();
}

extern ConVar r_flashlightfar;
extern ConVar r_flashlightfov;

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when on) using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOnValue(int fv)
{
	return 2.5f - (0.05f * fv);
}

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when on)'s exponent using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOnExponentValue(int fv)
{
	return 1.15f + (0.05f * fv);
}

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when off) by using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOffValue(int fv)
{
	return 2.3f - (0.05f * fv);
}



//options panel class
#define AMOD_OPTIONS_PANEL_COMMAND_CREDITS_PANEL "OpenCreditsPanel"
#define AMOD_OPTIONS_PANEL_COMMAND_SKYBOX_PANEL "OpenSkyboxPanel"
#define AMOD_OPTIONS_PANEL_COMMAND_WEATHER_PANEL "OpenWeatherPanel"
#define AMOD_OPTIONS_PANEL_COMMAND_EFFECTS_PANEL "OpenEffectsPanel"
#define AMOD_OPTIONS_PANEL_COMMAND_APPLY "Apply"

class COptionsPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(COptionsPanel, vgui::Frame);
public:

	//constructor and destructor
	COptionsPanel(vgui::VPANEL parent);
	~COptionsPanel();

	//called on panel close
	void OnClose();

protected:
	//special panel functions
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

	//initalize functions
	void InitConVars();
	void Init();

private:
	//keyvalues for file
	KeyValues* kvMain;

	//---------- view settings -----------//
	CheckButton* m_DisableFootstepsCheckButton;
	CheckButton* m_NoHudCheckButton;
	CheckButton* m_MirroredCheckButton;
	CheckButton* m_VignetteCheckButton;
	CheckButton* m_ViewBobCheckButton;
	CheckButton* m_StandBobCheckButton;
	CheckButton* m_JumpViewpunchCheckButton;
	CheckButton* m_LandViewpunchCheckButton;
	CheckButton* m_RollAngleCheckButton;
	Slider* m_RollAngleSlider;

	//---------- filters and effects -----------//
	CheckButton* m_EpicFilterCheckButton;
	Slider* m_FilterOnBrightnessSlider;
	Slider* m_FilterOnExponentSlider;
	Slider* m_FilterOffBrightnessSlider;

	//---------- flashlight settings -----------//
	ComboBox* m_FlashlightFarComboBox;
	ComboBox* m_FlashlightFovComboBox;
	CheckButton* m_FlashlightLagCheckButton;
	CheckButton* m_FlashlightFlickerCheckButton;

	//---------- bottom right -----------//
	ComboBox* m_EndingComboBox;

#if !AMOD_DAYTIME_EDITION
	Button* m_SkyboxButton;
#endif


	CheckButton* cbNoSoundscapes;
	CheckButton* cbNoMusic;
	CheckButton* m_cbDayAfterNP;
	CheckButton* m_cbDayAfterRav;
	Label* m_labelCheat;
	CheckButton* EnableGod;
	CheckButton* CoreTimerCB;
	CheckButton* CitadelTimerCB;

	//sou
	CUtlVector<const char*> m_FlashlightFarData;
	CUtlVector<const char*> m_FlashlightFovData;

	//filter variables
	int m_FilterOnValue = 0, m_FilterOnExponent = 0, m_FilterOffValue = 0;

	//is this panel initalized yet
	bool m_bInit = false;
};

//------------------------------------------------------------------------------------------
// Purpose: Constructor for options panel
//------------------------------------------------------------------------------------------
COptionsPanel::COptionsPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "OptionsPanel")
{
	//set m_bInit to false
	m_bInit = false;

	//set the parent
	SetParent(parent);

	//keyboard + mouse stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//other panel properties
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	//load the scheme settings
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//add a 100 ms tick signal
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

	kvMain = new KeyValues("OptionsPanel");
	kvMain->UsesEscapeSequences(true);

	if (!kvMain->LoadFromFile(filesystem, "resource/panels/OptionsPanel.txt", "MOD"))
	{
		ConWarning("Error Opening OptionsPanel.txt Aborting Panel Load\n");
		return;
	}

	//load the convars from the .cfg file
	InitConVars();

#if !AMOD_DAYTIME_EDITION

	//set the filter values
	float FilterOnValue =	  GetFilterOnValue(m_FilterOnValue);
	float FilterExponentValue	= GetFilterOnExponentValue(m_FilterOnExponent);
	float FilterOffValue		=   GetFilterOffValue(m_FilterOffValue);

	//set the monitor gamma values for the filter
	engine->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma_tv_exp %f", FilterExponentValue));
	engine->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma %f", FilterOnValue));

	//alias the toggle filter commands
	engine->ClientCmd_Unrestricted(CFmtStr("alias tf1 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 1; Amod_FilterSet 1; alias Amod_ToggleFilter tf2\"", FilterOnValue));			//when the filter is set off
	engine->ClientCmd_Unrestricted(CFmtStr("alias tf2 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 0; Amod_FilterSet 0; alias Amod_ToggleFilter tf1\"", FilterOffValue));			//when filter is set off
#endif

	//initalize all the panel elements
	Init();
}

//------------------------------------------------------------------------------------------
// Purpose: Initalizes all the convars. TODO: change sstream to keyvalues
//------------------------------------------------------------------------------------------
void COptionsPanel::InitConVars()
{
	//load the keyvalues file
	KeyValues* file = new KeyValues("AloneModConfig");
	if (!file->LoadFromFile(filesystem, "cfg/AloneMod_Config.txt", "MOD"))
	{
		//write the config and delete the keyvalues
		Amod_WriteConfig();
		file->deleteThis();
		return;
	}

	//go through each key (convar name) and set its value
	FOR_EACH_VALUE(file, value)
	{
		const char* varname = value->GetName();
		const char* varvalue = value->GetString();

		//find the var
		ConVar* var = cvar->FindVar(varname);
		
		//HACK: do these first
		if (!Q_strcmp(varname, "filter_brightness_on"))
		{
			m_FilterOnValue = atoi(varvalue);
		}
		else if (!Q_strcmp(varname, "filter_brightness_on_exp"))
		{
			m_FilterOnExponent = atoi(varvalue);
		}
		else if (!Q_strcmp(varname, "filter_brightness_off"))
		{
			m_FilterOffValue = atoi(varvalue);
		}
		else if (!Q_strcmp(varname, "amod_epic_filter"))
		{
			//check for the var
			if (!var)
				continue;

			//HACK: fixes "No Edict's yet" error
			auto t = var->m_fnChangeCallback;
			var->m_fnChangeCallback = nullptr;
			var->SetValue(atoi(varvalue));
			var->m_fnChangeCallback = t;
		}
		else
		{
			//check for the var
			if (!var)
				continue;

			//HACK: do this. idk why but just do it
			engine->ClientCmd(CFmtStr("%s %s", varname, varvalue));

			//set the convar value
			var->SetValue(varvalue);
		}
	}
}

//macro for finding keyvalues
#define OPTIONS_KEYVALUES_FIND(name, string)																\
KeyValues* name = kvMain->FindKey(string);																	\
if (!name)																								\
{																											\
	kvMain->deleteThis();																					\
	ConWarning("Got NULL Subkey: \"" string "\"In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");		\
	return;																									\
}

//macto for adding tool tips
#define OPTIONS_ELEMENT_ADD_TOOLTIP(element, keyvalues)								\
if (element && element->GetTooltip() && keyvalues->GetBool("ToolTipEnabled"))		\
{																					\
	element->GetTooltip()->SetTooltipDelay(keyvalues->GetInt("TooltipDelay"));		\
	element->GetTooltip()->SetText(keyvalues->GetString("TooltipText"));			\
																					\
	if (keyvalues->GetBool("ToolTipMultiline"))										\
		element->GetTooltip()->SetTooltipFormatToMultiLine();						\
	else																			\
		element->GetTooltip()->SetTooltipFormatToSingleLine();						\
}

//------------------------------------------------------------------------------------------
// Purpose: Initalizes all the panel items
//------------------------------------------------------------------------------------------
void COptionsPanel::Init()
{
	//find all the panel item keyvalues

	//bottom right
	OPTIONS_KEYVALUES_FIND(EndingButtonKV, "EndingComboBox");			//weather button
	OPTIONS_KEYVALUES_FIND(WeatherButtonKv, "WeatherButton");			//weather button

#if !AMOD_DAYTIME_EDITION
	OPTIONS_KEYVALUES_FIND(SkyboxButtonKv, "SkyboxButton");				//skybox button
#endif

	OPTIONS_KEYVALUES_FIND(CreditsButtonKv, "CreditsButton");			//credits button
	OPTIONS_KEYVALUES_FIND(ApplyButtonKv,	"ApplyButton");				//apply button
	
	//bottom left
	OPTIONS_KEYVALUES_FIND(FilterSettingsLabelKv,		"FilterSettingsLabel");		//filter settings label
	OPTIONS_KEYVALUES_FIND(FilterOnBrightnessLabelKv,	"FilterBOnLabel");			//filter on brightness label
	OPTIONS_KEYVALUES_FIND(FilterOnBrightnessSliderKv,	"FilterOnBSlider");			//filter on brightness
	OPTIONS_KEYVALUES_FIND(FilterOnExponentLabelKv,		"FilterBOnExpLabel");		//filter exponent label
	OPTIONS_KEYVALUES_FIND(FilterOnExponentSliderKv,	"FilterOnExpBSlider");		//filter exponent
	OPTIONS_KEYVALUES_FIND(FilterOffBrightnessLabelKv,	"FilterBOffLabel");			//filter off brightness label
	OPTIONS_KEYVALUES_FIND(FilterOffBrightnessSliderKv, "FilterOffBSlider");		//filter off brightness
	OPTIONS_KEYVALUES_FIND(EpicFilterCheckButtonKV,		"EpicFilterCB");			//epic filter check button
	OPTIONS_KEYVALUES_FIND(EffectsButtonKv,				"EffectsButton");			//effects button

	//top left
	OPTIONS_KEYVALUES_FIND(ViewSettingsLabelKv,					"ViewSettingsLabel");
	OPTIONS_KEYVALUES_FIND(DisableFootstepSoundsCheckButtonKV,	"CBDFootstebSnds");
	OPTIONS_KEYVALUES_FIND(DisableHudCheckButtonKV,				"CBDisableHud");
	OPTIONS_KEYVALUES_FIND(MirroredViewCheckButtonKv,			"MirroredCB");
	OPTIONS_KEYVALUES_FIND(VignetteCheckButtonKv,				"VignetteCB");
	OPTIONS_KEYVALUES_FIND(ViewBobCheckButtonKv,				"ViewBobbingCB");
	OPTIONS_KEYVALUES_FIND(StandBobCheckButtonKv,				"StandBobCB");
	OPTIONS_KEYVALUES_FIND(JumpViewpunchCheckButtonKv,			"JumpViewpunchCB");
	OPTIONS_KEYVALUES_FIND(LandViewpunchCheckButtonKv,			"LandViewpunchCB");
	OPTIONS_KEYVALUES_FIND(RollAngleCheckButtonKv,				"RollAngleCB");
	OPTIONS_KEYVALUES_FIND(RollAngleSliderKv,					"RollAngleSlider");
	OPTIONS_KEYVALUES_FIND(RollAngleLabelKv,					"RollAngleLabel");

	//flashlight settings
	OPTIONS_KEYVALUES_FIND(FlashlightSettingsLabelKv,		"FlashlightSettingsLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFarLabelKv,			"FlashlightFarLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFovLabelKv,			"FlashlightFovLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFarComboBoxKv,			"FlashlightFarCombobox");
	OPTIONS_KEYVALUES_FIND(FlashlightFovComboBoxKv,			"FlashlightFovCombobox");
	OPTIONS_KEYVALUES_FIND(FlashlightFlickerCheckButtonKv,	"FFlickerCbox");
	OPTIONS_KEYVALUES_FIND(FlashlightLagCheckButtonKv,		"FLagCBox");

	//get the far data
	KeyValues* FlashlightFarDataKv = FlashlightFarComboBoxKv->FindKey("Data");
	if (!FlashlightFarDataKv)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"Data\" In Subkey \"FlashlightFarCombobox\" File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//get the fov data
	KeyValues* FlashlightFovDataKv = FlashlightFovComboBoxKv->FindKey("Data");
	if (!FlashlightFovDataKv)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"Data\" In Subkey \"FlashlightFovCombobox\" File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//store the names for the combo box's
	CUtlVector<const char*> FlashlightFarNames;
	CUtlVector<const char*> FlashlightFovNames;

	//loop through both keyvalues to get each flashlight value
	FOR_EACH_VALUE(FlashlightFarDataKv, FlashlightDistance)
	{
		FlashlightFarNames.AddToTail(FlashlightDistance->GetName());
		m_FlashlightFarData.AddToTail(FlashlightDistance->GetString());
	}

	FOR_EACH_VALUE(FlashlightFovDataKv, FlashlightFov)
	{
		FlashlightFovNames.AddToTail(FlashlightFov->GetName());
		m_FlashlightFovData.AddToTail(FlashlightFov->GetString());
	}

	//dividers
	OPTIONS_KEYVALUES_FIND(Divider1Kv, "SettingsDivider1");
	OPTIONS_KEYVALUES_FIND(Divider2Kv, "SettingsDivider2");
	OPTIONS_KEYVALUES_FIND(Divider3Kv, "SettingsDivider3");
	OPTIONS_KEYVALUES_FIND(Divider4Kv, "SettingsDivider4");
	OPTIONS_KEYVALUES_FIND(Divider5Kv, "SettingsDivider5");

	KeyValues* NoSoundscapesCB = kvMain->FindKey("NoSoundscapesCB");
	if (!NoSoundscapesCB)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"NoSoundscapesCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* CoreTimerKV = kvMain->FindKey("CoreTimerCB");
	if (!CoreTimerKV)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"CoreTimerCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* CitadelTimerKV = kvMain->FindKey("CitadelTimerCB");
	if (!CitadelTimerKV)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"CitadelTimerCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* NoMusicCB = kvMain->FindKey("NoMusicCB");
	if (!NoMusicCB)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"NoMusicCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* DayAfterNPCBKv = kvMain->FindKey("DayAfterNPCBox");
	if (!DayAfterNPCBKv)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"DayAfterNPCBox\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* DayAfterRavCBKv = kvMain->FindKey("DayAfterRavCB");
	if (!DayAfterRavCBKv)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"DayAfterRavCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}
		
	KeyValues* GModeCB = kvMain->FindKey("GodModeCB");
	if (!GModeCB)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"GodModeCB\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	KeyValues* CheatSettingsKV = kvMain->FindKey("CheatSettings");
	if (!CheatSettingsKV)
	{
		kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"CheatSettings\" In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//set the title
	SetTitle(kvMain->GetString("Title", "Title"), false);

	//set size and bounds of the panel
	int wide = kvMain->GetInt("Wide"), tall = kvMain->GetInt("Tall");
	int screenWidth, screenHeight;

	//get the screen size and center the panel if needed
	vgui::surface()->GetScreenSize(screenWidth, screenHeight);

	//get x and y string
	const char* x = kvMain->GetString("XPos"), * y = kvMain->GetString("YPos");

	//get x and y pos
	int xPos = atoi(x);
	int yPos = atoi(y);

	//if x or y is center then center the panel
	if (!Q_strcmp(x, "center"))
		xPos = (screenWidth - wide) / 2;

	if (!Q_strcmp(y, "center"))
		yPos = (screenHeight - tall) / 2;

	SetSize(wide, tall);
	SetPos(xPos, yPos);

	//-------------------------------------- VIEW SETTINGS --------------------------------------//

	//view settings label
	Label* LabelView = new Label(this, "ViewLabel", ViewSettingsLabelKv->GetString("Text"));
	LabelView->SetBounds(ViewSettingsLabelKv->GetInt("XPos"), ViewSettingsLabelKv->GetInt("YPos"), ViewSettingsLabelKv->GetInt("Wide"), ViewSettingsLabelKv->GetInt("Tall"));

	//footstep sounds
	ConVarRef sv_footstep("sv_footsteps");
	m_DisableFootstepsCheckButton = new CheckButton(this, "CheckButtonDisableFootsteps", DisableFootstepSoundsCheckButtonKV->GetString("Text"));
	m_DisableFootstepsCheckButton->SetBounds(DisableFootstepSoundsCheckButtonKV->GetInt("XPos"), DisableFootstepSoundsCheckButtonKV->GetInt("YPos"), DisableFootstepSoundsCheckButtonKV->GetInt("Wide"), DisableFootstepSoundsCheckButtonKV->GetInt("Tall"));
	m_DisableFootstepsCheckButton->SetSelected(!sv_footstep.GetInt());

	//hidehud
	ConVarRef hidehud("hidehud");
	m_NoHudCheckButton = new CheckButton(this, "DisableHudCheckButton", DisableHudCheckButtonKV->GetString("Text"));
	m_NoHudCheckButton->SetBounds(DisableHudCheckButtonKV->GetInt("XPos"), DisableHudCheckButtonKV->GetInt("YPos"), DisableHudCheckButtonKV->GetInt("Wide"), DisableHudCheckButtonKV->GetInt("Tall"));
	m_NoHudCheckButton->SetSelected(hidehud.GetInt() == 8 ? true : false);

	//mirrored view check button
	m_MirroredCheckButton = new CheckButton(this, "MirroredCB", MirroredViewCheckButtonKv->GetString("Text", ""));
	m_MirroredCheckButton->SetBounds(MirroredViewCheckButtonKv->GetInt("XPos"), MirroredViewCheckButtonKv->GetInt("YPos"), MirroredViewCheckButtonKv->GetInt("Wide"), MirroredViewCheckButtonKv->GetInt("Tall"));
	m_MirroredCheckButton->SetSelected(amod_mirrored.GetBool());

	//vignette
	m_VignetteCheckButton = new CheckButton(this, "VignetteCheckButton", VignetteCheckButtonKv->GetString("Text", ""));
	m_VignetteCheckButton->SetBounds(VignetteCheckButtonKv->GetInt("XPos"), VignetteCheckButtonKv->GetInt("YPos"), VignetteCheckButtonKv->GetInt("Wide"), VignetteCheckButtonKv->GetInt("Tall"));
	m_VignetteCheckButton->SetSelected(amod_vignette.GetInt());

	//view bobbing
	m_ViewBobCheckButton = new CheckButton(this, "ViewBobCheckButton", ViewBobCheckButtonKv->GetString("Text", ""));
	m_ViewBobCheckButton->SetBounds(ViewBobCheckButtonKv->GetInt("XPos"), ViewBobCheckButtonKv->GetInt("YPos"), ViewBobCheckButtonKv->GetInt("Wide"), ViewBobCheckButtonKv->GetInt("Tall"));
	m_ViewBobCheckButton->SetSelected(amod_viewbob_enabled.GetInt());

	//stand bob
	ConVarRef amod_standbob_enabled("amod_standbob_enabled");
	m_StandBobCheckButton = new CheckButton(this, "StandBobCheckButton", StandBobCheckButtonKv->GetString("Text", ""));
	m_StandBobCheckButton->SetBounds(StandBobCheckButtonKv->GetInt("XPos"), StandBobCheckButtonKv->GetInt("YPos"), StandBobCheckButtonKv->GetInt("Wide"), StandBobCheckButtonKv->GetInt("Tall"));
	m_StandBobCheckButton->SetSelected(amod_standbob_enabled.GetBool());

	//jump viewpunch
	m_JumpViewpunchCheckButton = new CheckButton(this, "JumpViewpunch", JumpViewpunchCheckButtonKv->GetString("Text", ""));
	m_JumpViewpunchCheckButton->SetBounds(JumpViewpunchCheckButtonKv->GetInt("XPos"), JumpViewpunchCheckButtonKv->GetInt("YPos"), JumpViewpunchCheckButtonKv->GetInt("Wide"), JumpViewpunchCheckButtonKv->GetInt("Tall"));
	m_JumpViewpunchCheckButton->SetSelected(amod_jump_punch_enable.GetInt());

	//land viewpunch
	m_LandViewpunchCheckButton = new CheckButton(this, "LandViewpunch", LandViewpunchCheckButtonKv->GetString("Text", ""));
	m_LandViewpunchCheckButton->SetBounds(LandViewpunchCheckButtonKv->GetInt("XPos"), LandViewpunchCheckButtonKv->GetInt("YPos"), LandViewpunchCheckButtonKv->GetInt("Wide"), LandViewpunchCheckButtonKv->GetInt("Tall"));
	m_LandViewpunchCheckButton->SetSelected(amod_land_punch_enable.GetInt());

	//roll angle check button
	m_RollAngleCheckButton = new CheckButton(this, "RollAngleSlider", RollAngleCheckButtonKv->GetString("Text", ""));
	m_RollAngleCheckButton->SetBounds(RollAngleCheckButtonKv->GetInt("XPos"), RollAngleCheckButtonKv->GetInt("YPos"), RollAngleCheckButtonKv->GetInt("Wide"), RollAngleCheckButtonKv->GetInt("Tall"));
	m_RollAngleCheckButton->SetSelected(sv_rollangle.GetInt());

	//roll angle slider
	m_RollAngleSlider = new Slider(this, "RollAngleSlider");
	m_RollAngleSlider->SetBounds(RollAngleSliderKv->GetInt("XPos"), RollAngleSliderKv->GetInt("YPos"), RollAngleSliderKv->GetInt("Wide"), RollAngleSliderKv->GetInt("Tall"));
	m_RollAngleSlider->SetRange(RollAngleSliderKv->GetInt("Min"), RollAngleSliderKv->GetInt("Max"));
	m_RollAngleSlider->SetValue(sv_rollangle.GetInt());

	//roll angle label
	Label* RollAnglelabel = new Label(this, "RollAngleLabel", RollAngleLabelKv->GetString("Text"));
	RollAnglelabel->SetBounds(RollAngleLabelKv->GetInt("XPos"), RollAngleLabelKv->GetInt("YPos"), RollAngleLabelKv->GetInt("Wide"), RollAngleLabelKv->GetInt("Tall"));

	//-------------------------------------- FILTERS AND EFFECTS --------------------------------------//

	//make 'Filter Settings' label
	Label* FilterSettingsLabel = new Label(this, "FilterSettings", FilterSettingsLabelKv->GetString("text"));
	FilterSettingsLabel->SetBounds(FilterSettingsLabelKv->GetInt("XPos"), FilterSettingsLabelKv->GetInt("YPos"), FilterSettingsLabelKv->GetInt("Wide"), FilterSettingsLabelKv->GetInt("Tall"));

	//make 'filter on brightness' label
	Label* FilterOnBrightnessLabel = new Label(this, "FilterOnBLabel", FilterOnBrightnessLabelKv->GetString("text"));
	FilterOnBrightnessLabel->SetBounds(FilterOnBrightnessLabelKv->GetInt("XPos"), FilterOnBrightnessLabelKv->GetInt("YPos"), FilterOnBrightnessLabelKv->GetInt("Wide"), FilterOnBrightnessLabelKv->GetInt("Tall"));

	//make 'Filter on brightness' label
	m_FilterOnBrightnessSlider = new Slider(this, "FilterOnBrightness");
	m_FilterOnBrightnessSlider->SetBounds(FilterOnBrightnessSliderKv->GetInt("XPos"), FilterOnBrightnessSliderKv->GetInt("YPos"), FilterOnBrightnessSliderKv->GetInt("Wide"), FilterOnBrightnessSliderKv->GetInt("Tall"));
	m_FilterOnBrightnessSlider->SetRange(0, 10);
	m_FilterOnBrightnessSlider->SetValue(m_FilterOnValue);

	//make 'filter on exponent' label
	Label* FilterOnExponentLabel = new Label(this, "FilterOnExponentLabel", FilterOnExponentLabelKv->GetString("text"));
	FilterOnExponentLabel->SetBounds(FilterOnExponentLabelKv->GetInt("XPos"), FilterOnExponentLabelKv->GetInt("YPos"), FilterOnExponentLabelKv->GetInt("Wide"), FilterOnExponentLabelKv->GetInt("Tall"));

	//make 'filter on exponent' slider
	m_FilterOnExponentSlider = new Slider(this, "FilterOnExponent");
	m_FilterOnExponentSlider->SetBounds(FilterOnExponentSliderKv->GetInt("XPos"), FilterOnExponentSliderKv->GetInt("YPos"), FilterOnExponentSliderKv->GetInt("Wide"), FilterOnExponentSliderKv->GetInt("Tall"));
	m_FilterOnExponentSlider->SetRange(0, 10);
	m_FilterOnExponentSlider->SetValue(m_FilterOnExponent);

	//make 'Filter off brightness' label
	Label* FilterOffBrightnessLabel = new Label(this, "FilterOffBLabel", FilterOffBrightnessLabelKv->GetString("text"));
	FilterOffBrightnessLabel->SetBounds(FilterOffBrightnessLabelKv->GetInt("XPos"), FilterOffBrightnessLabelKv->GetInt("YPos"), FilterOffBrightnessLabelKv->GetInt("Wide"), FilterOffBrightnessLabelKv->GetInt("Tall"));

	//make 'Filter on brightness' label
	m_FilterOffBrightnessSlider = new Slider(this, "FilterOffBrightness");
	m_FilterOffBrightnessSlider->SetBounds(FilterOffBrightnessSliderKv->GetInt("XPos"), FilterOffBrightnessSliderKv->GetInt("YPos"), FilterOffBrightnessSliderKv->GetInt("Wide"), FilterOffBrightnessSliderKv->GetInt("Tall"));
	m_FilterOffBrightnessSlider->SetRange(0, 10);
	m_FilterOffBrightnessSlider->SetValue(m_FilterOffValue);

	//epic filter (post processing type color-correction) check button
	ConVarRef amod_epic_filter("amod_epic_filter");
	m_EpicFilterCheckButton = new CheckButton(this, "EpicFilterCB", EpicFilterCheckButtonKV->GetString("Text", ""));
	m_EpicFilterCheckButton->SetBounds(EpicFilterCheckButtonKV->GetInt("XPos"), EpicFilterCheckButtonKV->GetInt("YPos"), EpicFilterCheckButtonKV->GetInt("Wide"), EpicFilterCheckButtonKV->GetInt("Tall"));
	m_EpicFilterCheckButton->SetSelected(amod_epic_filter.GetInt());

	//Effects button
	vgui::Button* EffectsButton = new Button(this, "EffectsButton", EffectsButtonKv->GetString("Text", "Button"));
	EffectsButton->SetBounds(EffectsButtonKv->GetInt("XPos"), EffectsButtonKv->GetInt("YPos"), EffectsButtonKv->GetInt("Wide"), EffectsButtonKv->GetInt("Tall"));
	EffectsButton->SetCommand("EffectsPanel");

	//-------------------------------------- APPLY AND OTHER --------------------------------------//

	//make the ending combo box
	m_EndingComboBox = new ComboBox(this, "EndingComboBox", 2, false);
	m_EndingComboBox->AddItem("Mod Ending: Ending 1", nullptr);
	m_EndingComboBox->AddItem("Mod Ending: Ending 2", nullptr);
	m_EndingComboBox->ActivateItem(0);
	m_EndingComboBox->SetBounds(EndingButtonKV->GetInt("XPos"), EndingButtonKV->GetInt("YPos"), EndingButtonKV->GetInt("Wide"), EndingButtonKV->GetInt("Tall"));

	//credits button
	vgui::Button* CreditButton = new Button(this, "CreditButton", CreditsButtonKv->GetString("Text", "Button"));
	CreditButton->SetBounds(CreditsButtonKv->GetInt("XPos"), CreditsButtonKv->GetInt("YPos"), CreditsButtonKv->GetInt("Wide"), CreditsButtonKv->GetInt("Tall"));
	CreditButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_CREDITS_PANEL);

	//weather button
	vgui::Button* WeatherButton = new Button(this, "WeatherButton", WeatherButtonKv->GetString("Text", "Button"));
	WeatherButton->SetBounds(WeatherButtonKv->GetInt("XPos"), WeatherButtonKv->GetInt("YPos"), WeatherButtonKv->GetInt("Wide"), WeatherButtonKv->GetInt("Tall"));
	WeatherButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_WEATHER_PANEL);

#if !AMOD_DAYTIME_EDITION
	//skybox button
	m_SkyboxButton = new Button(this, "SkyboxButton", SkyboxButtonKv->GetString("Text", "Button"));
	m_SkyboxButton->SetBounds(SkyboxButtonKv->GetInt("XPos"), SkyboxButtonKv->GetInt("YPos"), SkyboxButtonKv->GetInt("Wide"), SkyboxButtonKv->GetInt("Tall"));
	m_SkyboxButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_SKYBOX_PANEL);
#endif

	//apply button
	vgui::Button* ApplyButton = new Button(this, "ApplyButton", ApplyButtonKv->GetString("Text", "Button"));
	ApplyButton->SetBounds(ApplyButtonKv->GetInt("XPos"), ApplyButtonKv->GetInt("YPos"), ApplyButtonKv->GetInt("Wide"), ApplyButtonKv->GetInt("Tall"));
	ApplyButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_APPLY);

	//-------------------------------------- FLASHLIGHT --------------------------------------//
	
	//heading
	Label* FlashlightSettingsLabel = new Label(this, "FlashlightSettingsLabel", FlashlightSettingsLabelKv->GetString("Text"));
	FlashlightSettingsLabel->SetBounds(FlashlightSettingsLabelKv->GetInt("XPos"), FlashlightSettingsLabelKv->GetInt("YPos"), FlashlightSettingsLabelKv->GetInt("Wide"), FlashlightSettingsLabelKv->GetInt("Tall"));

	//flashlight strength label
	Label* FlashlightFarLabel = new Label(this, "FlashlightFarLabel", FlashlightFarLabelKv->GetString("text"));
	FlashlightFarLabel->SetBounds(FlashlightFarLabelKv->GetInt("XPos"), FlashlightFarLabelKv->GetInt("YPos"), FlashlightFarLabelKv->GetInt("Wide"), FlashlightFarLabelKv->GetInt("Tall"));

	//flashlight fov label
	Label* FlashlightFovLabel = new Label(this, "FlashlightFovLabel", FlashlightFovLabelKv->GetString("text"));
	FlashlightFovLabel->SetBounds(FlashlightFovLabelKv->GetInt("XPos"), FlashlightFovLabelKv->GetInt("YPos"), FlashlightFovLabelKv->GetInt("Wide"), FlashlightFovLabelKv->GetInt("Tall"));

	//flashlight far checkbox
	m_FlashlightFarComboBox = new ComboBox(this, "FlashlightFarComboBox", 12, false);
	m_FlashlightFarComboBox->SetBounds(FlashlightFarComboBoxKv->GetInt("XPos"), FlashlightFarComboBoxKv->GetInt("YPos"), FlashlightFarComboBoxKv->GetInt("Wide"), FlashlightFarComboBoxKv->GetInt("Tall"));

	//add all the names
	for (int i = 0; i < FlashlightFarNames.Size(); i++)
		m_FlashlightFarComboBox->AddItem(FlashlightFarNames[i], nullptr);

	//activate item 0
	m_FlashlightFarComboBox->ActivateItem(0);

	//now check each far to see if any of the values == r_flashlightfar
	for (int j = 0; j < m_FlashlightFarData.Size(); j++)
	{
		if (r_flashlightfar.GetInt() == atoi(m_FlashlightFarData[j]))
		{
			m_FlashlightFarComboBox->ActivateItem(j);
			break;
		}
	}

	//flashlight fov combo box
	m_FlashlightFovComboBox = new ComboBox(this, "FlashlightFovComboBox", 12, false);
	m_FlashlightFovComboBox->SetBounds(FlashlightFovComboBoxKv->GetInt("XPos"), FlashlightFovComboBoxKv->GetInt("YPos"), FlashlightFovComboBoxKv->GetInt("Wide"), FlashlightFovComboBoxKv->GetInt("Tall"));

	//add all the names
	for (int i = 0; i < FlashlightFovNames.Size(); i++)
		m_FlashlightFovComboBox->AddItem(FlashlightFovNames[i], nullptr);
	
	//activate item 0
	m_FlashlightFovComboBox->ActivateItem(0);

	//now check each far to see if any of the values == r_flashlightfov
	for (int j = 0; j < m_FlashlightFovData.Size(); j++)
	{
		if (r_flashlightfov.GetInt() == atoi(m_FlashlightFovData[j]))
		{
			m_FlashlightFovComboBox->ActivateItem(j);
			break;
		}
	}

	//create the flashlight lag button
	m_FlashlightLagCheckButton = new CheckButton(this, "FlashlightLagComboBox", FlashlightLagCheckButtonKv->GetString("Text"));
	m_FlashlightLagCheckButton->SetBounds(FlashlightLagCheckButtonKv->GetInt("XPos"), FlashlightLagCheckButtonKv->GetInt("YPos"), FlashlightLagCheckButtonKv->GetInt("Wide"), FlashlightLagCheckButtonKv->GetInt("Tall"));
	m_FlashlightLagCheckButton->SetSelected(amod_flashlightlag.GetBool());

	//flashlight flicker combo box
	m_FlashlightFlickerCheckButton = new CheckButton(this, "FlashlightFlickerComboBox", FlashlightFlickerCheckButtonKv->GetString("Text"));
	m_FlashlightFlickerCheckButton->SetBounds(FlashlightFlickerCheckButtonKv->GetInt("XPos"), FlashlightFlickerCheckButtonKv->GetInt("YPos"), FlashlightFlickerCheckButtonKv->GetInt("Wide"), FlashlightFlickerCheckButtonKv->GetInt("Tall"));
	m_FlashlightFlickerCheckButton->SetSelected(amod_flashlightflicker.GetBool());

	//-------------------------------------- DIVIDERS --------------------------------------//

	//divider 1
	Divider* SettingsDivider1 = new Divider(this, "SettingsDivider1");
	SettingsDivider1->SetBounds(Divider1Kv->GetInt("XPos"), Divider1Kv->GetInt("YPos"), Divider1Kv->GetInt("Wide"), Divider1Kv->GetInt("Tall"));

	//divider 2
	Divider* SettingsDivider2 = new Divider(this, "SettingsDivider2");
	SettingsDivider2->SetBounds(Divider2Kv->GetInt("XPos"), Divider2Kv->GetInt("YPos"), Divider2Kv->GetInt("Wide"), Divider2Kv->GetInt("Tall"));

	//divider 3
	Divider* SettingsDivider3 = new Divider(this, "SettingsDivider3");
	SettingsDivider3->SetBounds(Divider3Kv->GetInt("XPos"), Divider3Kv->GetInt("YPos"), Divider3Kv->GetInt("Wide"), Divider3Kv->GetInt("Tall"));

	//divider 4
	Divider* SettingsDivider4 = new Divider(this, "SettingsDivider4");
	SettingsDivider4->SetBounds(Divider4Kv->GetInt("XPos"), Divider4Kv->GetInt("YPos"), Divider4Kv->GetInt("Wide"), Divider4Kv->GetInt("Tall"));

	//divider 5
	Divider* SettingsDivider5 = new Divider(this, "SettingsDivider5");
	SettingsDivider5->SetBounds(Divider5Kv->GetInt("XPos"), Divider5Kv->GetInt("YPos"), Divider5Kv->GetInt("Wide"), Divider5Kv->GetInt("Tall"));

	m_labelCheat = new Label(this, "m_labelCheat", CheatSettingsKV->GetString("text"));
	m_labelCheat->SetBounds(CheatSettingsKV->GetInt("XPos"), CheatSettingsKV->GetInt("YPos"), CheatSettingsKV->GetInt("Wide"), CheatSettingsKV->GetInt("Tall"));

	EnableGod = new CheckButton(this, "EnableGod", GModeCB->GetString("Text", ""));
	EnableGod->SetBounds(GModeCB->GetInt("XPos"), GModeCB->GetInt("YPos"), GModeCB->GetInt("Wide"), GModeCB->GetInt("Tall"));

	ConVarRef amo_god("amod_enable_god");

	EnableGod->SetSelected(amo_god.GetInt());


	ConVarRef amod_soundscapes_disable("amod_soundscapes_disable");
	ConVarRef amod_music_disable_tmp("amod_music_disable");

	cbNoSoundscapes = new CheckButton(this, "NoSoundscapes", NoSoundscapesCB->GetString("Text", ""));
	cbNoSoundscapes->SetBounds(NoSoundscapesCB->GetInt("XPos"), NoSoundscapesCB->GetInt("YPos"), NoSoundscapesCB->GetInt("Wide"), NoSoundscapesCB->GetInt("Tall"));
	cbNoSoundscapes->SetSelected(amod_soundscapes_disable.GetBool());

	cbNoMusic = new CheckButton(this, "NoSoundscapes", NoMusicCB->GetString("Text", ""));
	cbNoMusic->SetBounds(NoMusicCB->GetInt("XPos"), NoMusicCB->GetInt("YPos"), NoMusicCB->GetInt("Wide"), NoMusicCB->GetInt("Tall"));
	cbNoMusic->SetSelected(amod_music_disable_tmp.GetBool());

	m_cbDayAfterNP = new CheckButton(this, "DayAfterNP", DayAfterNPCBKv->GetString("Text", ""));
	m_cbDayAfterNP->SetBounds(DayAfterNPCBKv->GetInt("XPos"), DayAfterNPCBKv->GetInt("YPos"), DayAfterNPCBKv->GetInt("Wide"), DayAfterNPCBKv->GetInt("Tall"));

#if !AMOD_DAYTIME_EDITION
	m_cbDayAfterNP->SetSelected(amod_day.GetBool());
#endif

	m_cbDayAfterRav = new CheckButton(this, "m_cbDayAfterRav", DayAfterRavCBKv->GetString("Text", ""));
	m_cbDayAfterRav->SetBounds(DayAfterRavCBKv->GetInt("XPos"), DayAfterRavCBKv->GetInt("YPos"), DayAfterRavCBKv->GetInt("Wide"), DayAfterRavCBKv->GetInt("Tall"));

#if !AMOD_DAYTIME_EDITION
	m_cbDayAfterRav->SetSelected(amod_day_ravenholm.GetBool());
#endif



#if AMOD_DAYTIME_EDITION
	FilterOnBrightness->SetEnabled(false);
	FilterOnExps->SetEnabled(false);
	FilterOffBrightness->SetEnabled(false);
	m_cbDayAfterNP->SetEnabled(false);
	m_cbDayAfterRav->SetEnabled(false);
#endif

	ConVarRef amod_do_core_timer("amod_do_core_timer");
	ConVarRef amod_do_citadel_timer("amod_do_citadel_timer");

	CoreTimerCB = new CheckButton(this, "", CoreTimerKV->GetString("Text"));
	CoreTimerCB->SetSelected(amod_do_core_timer.GetBool());
	CoreTimerCB->SetBounds(CoreTimerKV->GetInt("XPos"), CoreTimerKV->GetInt("YPos"), CoreTimerKV->GetInt("Wide"), CoreTimerKV->GetInt("Tall"));

	CitadelTimerCB = new CheckButton(this, "", CitadelTimerKV->GetString("Text"));
	CitadelTimerCB->SetSelected(amod_do_citadel_timer.GetBool());
	CitadelTimerCB->SetBounds(CitadelTimerKV->GetInt("XPos"), CitadelTimerKV->GetInt("YPos"), CitadelTimerKV->GetInt("Wide"), CitadelTimerKV->GetInt("Tall"));

	//finally add all the tooltips
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_DisableFootstepsCheckButton,	DisableFootstepSoundsCheckButtonKV);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_NoHudCheckButton,				DisableHudCheckButtonKV);
	//OPTIONS_ELEMENT_ADD_TOOLTIP(m_MirroredCheckButton,			MirroredViewCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_VignetteCheckButton,			VignetteCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_ViewBobCheckButton,			ViewBobCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_StandBobCheckButton,			StandBobCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_JumpViewpunchCheckButton,		JumpViewpunchCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_LandViewpunchCheckButton,		LandViewpunchCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_RollAngleCheckButton,			RollAngleCheckButtonKv);

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_FilterOnBrightnessSlider,		FilterOnBrightnessSliderKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_FilterOnExponentSlider,		FilterOnExponentLabelKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_FilterOffBrightnessSlider,	FilterOffBrightnessSliderKv);

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_FlashlightLagCheckButton,		FlashlightLagCheckButtonKv);
	OPTIONS_ELEMENT_ADD_TOOLTIP(m_FlashlightFlickerCheckButton,	FlashlightFlickerCheckButtonKv);

	//if (m_MirroredCheckButton && m_MirroredCheckButton->GetTooltip() && MirroredViewCheckButtonKv->GetBool("ToolTipEnabled"))
	//{
	//	m_MirroredCheckButton->GetTooltip()->SetEnabled(true);
	//	m_MirroredCheckButton->GetTooltip()->SetTooltipDelay(MirroredViewCheckButtonKv->GetInt("ToolTipDelay"));
	//	m_MirroredCheckButton->GetTooltip()->SetText(MirroredViewCheckButtonKv->GetString("ToolTipText"));
	//	
	//	if (MirroredViewCheckButtonKv->GetBool("ToolTipMultiline"))
	//		m_MirroredCheckButton->GetTooltip()->SetTooltipFormatToMultiLine();
	//	else
	//		m_MirroredCheckButton->GetTooltip()->SetTooltipFormatToSingleLine();
	//}

	m_DisableFootstepsCheckButton->GetTooltip()->SetTooltipDelay(0);
	m_DisableFootstepsCheckButton->GetTooltip()->SetText("Hello World");
	m_DisableFootstepsCheckButton->GetTooltip()->SetTooltipFormatToSingleLine();

	m_bInit = true;
}

ConVar cl_drawoptionspanel("cl_drawoptionspanel", "0", FCVAR_CLIENTDLL, "Sets the state of myPanel <state>");


//------------------------------------------------------------------------------------------
// Purpose: Called on panel close
//------------------------------------------------------------------------------------------
void COptionsPanel::OnClose()
{
	cl_drawoptionspanel.SetValue(0);

	//close all the sub panels
	creditspanel->Close();
	weatherpanel->Close();
	skyboxpanel->Close();
}

//------------------------------------------------------------------------------------------
// Purpose: Called on tick (every 100ms)
//------------------------------------------------------------------------------------------
void COptionsPanel::OnTick()
{
	//call the base class
	BaseClass::OnTick();
	SetVisible(cl_drawoptionspanel.GetBool());

	//if initalized then enable some of the elements based on check buttons
	if (m_bInit)
	{
		m_RollAngleSlider->SetEnabled(m_RollAngleCheckButton->IsSelected());

#if !AMOD_DAYTIME_EDITION
		if (szMapName)
			m_SkyboxButton->SetEnabled(Q_strcmp(szMapName, "ep1"));
#endif

		cbNoSoundscapes->SetEnabled(!g_NeedSoundscapeEnable);
	}
}

//these are the current values of the filter. When changing the filter settings it will check
//to see if the filter is on or off, Then set the monitor gamma to the on value or off value.
static bool gs_FilterIsOn = true;

//sets if the filter is on or off
CON_COMMAND_F(Amod_FilterSet, "", FCVAR_HIDDEN)
{
	gs_FilterIsOn = atoi(args.Arg(1));
}

//------------------------------------------------------------------------------------------
// Purpose: Initalizes all the panel items
//------------------------------------------------------------------------------------------
void COptionsPanel::OnCommand(const char* pcCommand)
{
	//not sure why this is needed but if i dont want to remove it
	static bool s_bWasSoundscapesChecked = cbNoSoundscapes->IsSelected();

	//handle apply
	if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_APPLY))
	{
		//write convar struct
		struct WriteConvar_t
		{
			const char* ConvarName;				//convar name
			int Value;							//value to set convar
			CUtlVector<const char*>* Array;		//If not set to nullptr. Then it will set the convar's value to Array[value]
		};

		//all the convars and stuff to write
		WriteConvar_t WriteConvars[] = {
			//view settings
			{"sv_footsteps",				!m_DisableFootstepsCheckButton->IsSelected(),									nullptr}, 
			{"hidehud",						m_NoHudCheckButton->IsSelected() ? 8 : 0,										nullptr}, 
			{"amod_mirrored",				m_MirroredCheckButton->IsSelected(),											nullptr}, 
			{"amod_vignette",				m_VignetteCheckButton->IsSelected(),											nullptr},
			{"amod_viewbob_enabled",		m_ViewBobCheckButton->IsSelected(),												nullptr},
			{"amod_standbob_enabled",		m_StandBobCheckButton->IsSelected(),											nullptr},
			{"amod_jump_punch_enable",		m_JumpViewpunchCheckButton->IsSelected(),										nullptr},
			{"amod_land_punch_enable",		m_LandViewpunchCheckButton->IsSelected(),										nullptr},
			{"sv_rollangle",				m_RollAngleCheckButton->IsSelected() ? m_RollAngleSlider->GetValue() : 0,		nullptr},

			//effects
			{"amod_epic_filter",			m_EpicFilterCheckButton->IsSelected(),											nullptr},

			//flashlight settings
			{"r_flashlightfar",				m_FlashlightFarComboBox->GetActiveItem(),										&m_FlashlightFarData},
			{"r_flashlightfov",				m_FlashlightFovComboBox->GetActiveItem(),										&m_FlashlightFovData},
			{"amod_flashlightlag",			m_FlashlightLagCheckButton->IsSelected(),										nullptr},
			{"amod_flashlightflicker",		m_FlashlightFlickerCheckButton->IsSelected(),									nullptr},

			//other settings
			{"amod_enable_god",				EnableGod->IsSelected(),														nullptr},
			{"amod_soundscapes_disable",	cbNoSoundscapes->IsSelected(),													nullptr},
			{"amod_music_disable",			cbNoMusic->IsSelected(),														nullptr},
			{"amod_do_citadel_timer",		CitadelTimerCB->IsSelected(),													nullptr},
			{"amod_do_core_timer",			CoreTimerCB->IsSelected(),														nullptr},

			//ending
			{"amod_new_ending",				m_EndingComboBox->GetActiveItem(),												nullptr}
		};

		//HACK: these require a hack to work
		if (amod_day.GetBool() ^ m_cbDayAfterNP->IsSelected())
		{
			amod_day.SetValue(m_cbDayAfterNP->IsSelected());
			engine->ClientCmd_Unrestricted(CFmtStr("amod_day %d", m_cbDayAfterNP->IsSelected()));
		}
		
		//HACK: these require a hack to work
		if (amod_day_ravenholm.GetBool() ^ m_cbDayAfterRav->IsSelected())
		{
			amod_day_ravenholm.SetValue(m_cbDayAfterNP->IsSelected());
			engine->ClientCmd_Unrestricted(CFmtStr("amod_day_ravenholm %d", m_cbDayAfterRav->IsSelected()));
		}

		//check the soundscapes to see if thet can be disabled or not
		if (s_bWasSoundscapesChecked ^ (cbNoSoundscapes->IsSelected() && !g_NeedSoundscapeEnable))
			engine->ExecuteClientCmd("playsoundscape Nothing");

		s_bWasSoundscapesChecked = (cbNoSoundscapes->IsSelected() && !g_NeedSoundscapeEnable);

#if !AMOD_DAYTIME_EDITION
		
		//we need to set the filter values. So do that now
		float FilterOnValue = GetFilterOnValue(m_FilterOnBrightnessSlider->GetValue());
		float FilterExponentValue = GetFilterOnExponentValue(m_FilterOnExponentSlider->GetValue());
		float FilterOffValue = GetFilterOffValue(m_FilterOffBrightnessSlider->GetValue());

		//set the brightness (filter values)
		engine->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma_tv_exp %f", FilterExponentValue));
		engine->ClientCmd_Unrestricted(CFmtStr("alias tf1 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 1; Amod_FilterSet 1; alias Amod_ToggleFilter tf2\"", FilterOnValue));
		engine->ClientCmd_Unrestricted(CFmtStr("alias tf2 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 0; Amod_FilterSet 0; alias Amod_ToggleFilter tf1\"", FilterOffValue));

		//if the filter is on then execute the alias command to apply the new filter values
		if (gs_FilterIsOn && !(amod_day.GetBool() && IsCityMap(szMapName)))
			engine->ClientCmd_Unrestricted("tf1");
		else
			engine->ClientCmd_Unrestricted("tf2");
#endif

		//loop through all the WriteConvars and set the values of the convars
		for (int i = 0; i < ARRAYSIZE(WriteConvars); i++)
		{
			//store the var data
			WriteConvar_t& data = WriteConvars[i];

			//find the convar
			ConVar* var = cvar->FindVar(data.ConvarName);
			if (!var)
				continue;

			//check to see if we use the array or not
			if (data.Array && data.Value >= 0 && data.Value < data.Array->Count())
			{
				engine->ClientCmd(CFmtStr("%s %s", data.ConvarName, (*data.Array)[data.Value]));
				var->SetValue((*data.Array)[data.Value]);
			}
			else
			{
				//set the value
				engine->ClientCmd(CFmtStr("%s %d", data.ConvarName, data.Value));
				var->SetValue(data.Value);
			}
		}

		//write the config file
		Amod_WriteConfig();
	}
	else if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_CREDITS_PANEL))
	{
		//open up the credits panel
		creditspanel->Close();
		creditspanel->Open(this);
	}
#if !AMOD_DAYTIME_EDITION
	else if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_SKYBOX_PANEL))
	{
		//open the skybox panel
		skyboxpanel->Close();
		skyboxpanel->Open(this);
	}
#endif
	else if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_WEATHER_PANEL))
	{
		//open the weather panel
		weatherpanel->Close();
		weatherpanel->Open(this);
	}
	else if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_EFFECTS_PANEL))
	{
		//toggle the effects panel
		engine->ClientCmd("toggleeffectspanel");
	}
	else
	{
		BaseClass::OnCommand(pcCommand);
	}
}

//------------------------------------------------------------------------------------------
// Purpose: Destructor for options panel
//------------------------------------------------------------------------------------------
COptionsPanel::~COptionsPanel()
{
	//close all the sub panels
	skyboxpanel->Close();
	creditspanel->Close();
	weatherpanel->Close();

	//delete the options file if needed
	if (kvMain)
		kvMain->deleteThis();
}


//i dont know if this command is needed but i wont remove it just in case
CON_COMMAND_F(amod_outrotest, "", FCVAR_HIDDEN)
{
	if (amod_new_ending.GetBool())
		engine->ClientCmd_Unrestricted("map credits_d");
}


//----------------------------------------------------------------------------------------------
// Interfaces
//----------------------------------------------------------------------------------------------

//options panel interface
class COptionsPanelInterface : public OptionsPanel
{
public:
	void Create(vgui::VPANEL parent)
	{
		//create the options
		m_OptionsPanel = new COptionsPanel(parent);
	}

	void Destroy()
	{
		//delete the options panel if needed
		if (m_OptionsPanel)
		{
			m_OptionsPanel->SetParent((vgui::Panel*)NULL);
			delete m_OptionsPanel;
		}

		//clear the panel
		m_OptionsPanel = nullptr;
	}

	//activates this panel
	void Activate(void)
	{
		if (m_OptionsPanel)
			m_OptionsPanel->Activate();
	}

	//options panel
	COptionsPanel* m_OptionsPanel = nullptr;
};
static COptionsPanelInterface g_OptionsPanel;
OptionsPanel* optionspanel = (OptionsPanel*)&g_OptionsPanel;


//skybox panel interface
class CAmodSkyboxPanelInterface : public AmodSkyboxPanel
{
public:
	//opens up the panel
	void Open(Panel* pan)
	{
		//if no panel then create it
		if (!m_SkyboxPanel)
			m_SkyboxPanel = new CAModSkyboxPanel(pan);

		//activate the panel
		m_SkyboxPanel->Activate();
	}

	//clears the m_SkyboxPanel
	void ClearPanel()
	{
		m_SkyboxPanel = nullptr;
	}

	//closes the panel
	void Close()
	{
		//delete and clear the skybox panel
		if (m_SkyboxPanel)
			delete m_SkyboxPanel;

		//set panel to nullptr
		ClearPanel();
	}
private:
	CAModSkyboxPanel* m_SkyboxPanel;

};
static CAmodSkyboxPanelInterface g_AmodSkyboxPanelInterface;
AmodSkyboxPanel* skyboxpanel = &g_AmodSkyboxPanelInterface;


//credits panel interface
class CAmodCreditsPanelInterface : public AmodCreditsPanel
{
public:
	void Open(Panel* parent)
	{
		//open the panel if it isnt already active
		if (!m_CreditsPanel)
			m_CreditsPanel = new CAModCreditsPanel(parent);

		//activate it (bring it to front)
		m_CreditsPanel->Activate();
	}

	void ClearPanel()
	{
		//set the m_CreditsPanel to nullptr
		m_CreditsPanel = nullptr;
	}

	void Close()
	{
		//close the credits panel if needed
		if (m_CreditsPanel)
			delete m_CreditsPanel;

		ClearPanel();
	}

private:
	CAModCreditsPanel* m_CreditsPanel;
};
static CAmodCreditsPanelInterface g_AmodCreditsPanelInterface;
AmodCreditsPanel* creditspanel = &g_AmodCreditsPanelInterface;


//weather panel interface
class CAmodWeatherPanelInterface : public AmodWeatherPanel
{
public:
	void Open(Panel* parent)
	{
		//open the panel if it isnt already active
		if (!m_WeatherPanel)
			m_WeatherPanel = new CAModWeatherPanel(parent);

		//activate it (bring it to front)
		m_WeatherPanel->Activate();
	}

	void ClearPanel()
	{
		//set the m_WeatherPanel to nullptr
		m_WeatherPanel = nullptr;
	}

	void Close()
	{
		//close the weather panel if needed
		if (m_WeatherPanel)
			delete m_WeatherPanel;

		ClearPanel();
	}

private:
	CAModWeatherPanel* m_WeatherPanel;
};
static CAmodWeatherPanelInterface g_AmodWeatherPanelInterface;
AmodWeatherPanel* weatherpanel = &g_AmodWeatherPanelInterface;


//debug command to reset the options panel
CON_COMMAND(amod_opanel_reset, "For Making The Panel")
{
	//destroy the panel if needed
	if (g_OptionsPanel.m_OptionsPanel)
		g_OptionsPanel.Destroy();

	//re-create the options panel
	g_OptionsPanel.Create(enginevgui->GetPanel(PANEL_GAMEUIDLL));
}

//command to toggle the options panel
CON_COMMAND_F(ToggleOptionsPanel, "Toggles The Alone Mod Options Panel", FCVAR_HIDDEN)
{
	cl_drawoptionspanel.SetValue(!cl_drawoptionspanel.GetBool());
	optionspanel->Activate();
};