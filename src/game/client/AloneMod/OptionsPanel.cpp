//HOLY FUCK IM FINALLY DONE (i fucking hope so) I HAVBE BEEN DOING THIS FOR THE LAST 8 HOURS AND NOW IM FINALLY DONE
//IT HAS TAKEN OVER 3 DAYS YIPPE


//This comment above was from over a year ago. Turned out i wasnt done. No more yippe
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
#include "AloneMod/geo guesser/GG_MiniMap.h"
#include "AloneMod/ColorPicker.h"

using namespace vgui;

//convar declirations
void AmodDayChangeCallback(IConVar* var, const char*, float);

//writes the alone mod config
void Amod_WriteConfig();

#if !AMOD_DAYTIME_EDITION
//day time convars
ConVar amod_day("amod_day", "0", 0, "", AmodDayChangeCallback);
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
	
	//called on game shutdown
	void Shutdown()
	{
		Amod_WriteConfig();
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
	MoveToCenterOfScreen();

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
#define WEATHER_PANEL_COMMAND_CLOUDS_COLOR "SetCloudsColor"
#define WEATHER_PANEL_COMMAND_CLOUDS_OVERRIDE "OverrideCloudsColor"

class CAModWeatherPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CAModWeatherPanel, vgui::Frame);
public:

	//constructor
	CAModWeatherPanel();

	//panel functions
	void OnTick();
	void OnCommand(const char* command);
	void OnKeyCodePressed(KeyCode code);

	void Paint();

	//called when the color picker picks a color
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);

	//destructor
	~CAModWeatherPanel();
private:

	//panel items
	CheckButton* m_cbClouds;
	CheckButton* m_cbCloudsOverrideColor;
	Button* m_ButtonCloudsColor;
	CheckButton* m_cbBreaths;
	CheckButton* m_cbEnable;
	CheckButton* m_cbThunder;
	CheckButton* m_cbSplashes;
	Label* m_RainWaitTimeMinLabel;
	Slider* m_RainWaitTimeMinSlider;
	Label* m_RainWaitTimeMaxLabel;
	Slider* m_RainWaitTimeMaxSlider;
	ComboBox* m_cbType;
	ComboBox* m_cbMode;

	//clouds color
	CColorPicker* m_CloudsColorPicker;
	Color m_CurrentCloudsColor;
};

//rain densities
#define RAIN_DENSITY_VERY_LIGHT "0.0002"
#define RAIN_DENSITY_LIGHT "0.0006"
#define RAIN_DENSITY_MEDIUM "0.001"
#define RAIN_DENSITY_HARD "0.002"
#define RAIN_DENSITY_VERY_HARD "0.003"

//rain types
const char* sg_RainTypes[] = {
	RAIN_DENSITY_VERY_LIGHT,
	RAIN_DENSITY_LIGHT,
	RAIN_DENSITY_MEDIUM,
	RAIN_DENSITY_HARD,
	RAIN_DENSITY_VERY_HARD,
};

//------------------------------------------------------------------------------------------
// Purpose: Constructor for the alone mod weather panel
//------------------------------------------------------------------------------------------
CAModWeatherPanel::CAModWeatherPanel() : vgui::Frame(nullptr, "CAModWeatherPanel")
{
	//set the parent
	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

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
	SetBounds(100, 100, 200, 384);
	SetTitle("Weather Settings", false);
	MoveToCenterOfScreen();
		
	//load the convars for the settings
	ConVarRef amod_do_breathing("amod_do_breathing");
	ConVarRef amod_rain_type("amod_rain_type");
	ConVarRef amod_rain_enable("amod_rain_enable");
	ConVarRef amod_rain_thunder("amod_rain_thunder");
	ConVarRef amod_rain_splashes("amod_rain_splashes");
	ConVarRef amod_rain_wait_min("amod_rain_wait_min");
	ConVarRef amod_rain_wait_max("amod_rain_wait_max");
	ConVarRef amod_clouds("amod_clouds");
	ConVarRef amod_clouds_color("amod_clouds_color");
	ConVarRef amod_clouds_color_override("amod_clouds_color_override");
	ConVarRef r_raindensity("r_raindensity");

	//set the clouds color
	int cloudscolor[4];
	UTIL_StringToIntArray(cloudscolor, 4, amod_clouds_color.GetString());
	m_CurrentCloudsColor.SetColor(cloudscolor[0], cloudscolor[1], cloudscolor[2], cloudscolor[3]);

	//create the 'Show weather info' check button
	m_cbClouds = new CheckButton(this, "ShowClouds", "Show Moving Clouds");
	m_cbClouds->SetSelected(amod_clouds.GetBool());
	m_cbClouds->SetBounds(2, 21, 150, 25);
	m_cbClouds->GetTooltip()->SetText("Shows moving clouds on maps that allow for clouds in them!");
	m_cbClouds->GetTooltip()->SetTooltipDelay(100);
	m_cbClouds->GetTooltip()->SetTooltipFormatToMultiLine();

	m_cbCloudsOverrideColor = new vgui::CheckButton(this, "OverrideCloudsColor", "Override Cloud Colors");
	m_cbCloudsOverrideColor->SetBounds(2, 46, 163, 20);
	m_cbCloudsOverrideColor->SetSelected(amod_clouds_color_override.GetBool());
	m_cbCloudsOverrideColor->SetCommand(WEATHER_PANEL_COMMAND_CLOUDS_OVERRIDE);

	//create the clouds color button
	m_ButtonCloudsColor = new vgui::Button(this, "ButtonCloudsColor", "Set Clouds Color", this, WEATHER_PANEL_COMMAND_CLOUDS_COLOR);
	m_ButtonCloudsColor->SetBounds(7, 68, 163, 20);
	m_ButtonCloudsColor->SetEnabled(amod_clouds_color_override.GetBool());

	//create the clouds divider
	vgui::Divider* CloudsDivider = new Divider(this, "CloudsDivider");
	CloudsDivider->SetBounds(-2, 93, 200, 1);

	//create the 'show player's breath' check button
	m_cbBreaths = new CheckButton(this, "ShowPlayersBreath", "Show Players Breath");
	m_cbBreaths->SetSelected(amod_do_breathing.GetBool());
	m_cbBreaths->SetBounds(2, 95, 150, 25);
	m_cbBreaths->GetTooltip()->SetText("Show the players breaths to show that it is currently cold.\nNote: you have to stand still for the breaths to show");
	m_cbBreaths->GetTooltip()->SetTooltipDelay(100);
	m_cbBreaths->GetTooltip()->SetTooltipFormatToMultiLine();

	//create the 'enable rain' check button
	m_cbEnable = new CheckButton(this, "CheckButtonEnableRain", "Enable Rain");
	m_cbEnable->SetSelected(amod_rain_enable.GetBool());
	m_cbEnable->SetBounds(2, 117, 150, 25);

	//create the 'enable thunder sounds' check button
	m_cbThunder = new CheckButton(this, "CheckButtonEnableThunder", "Enable Thunder Sounds");
	m_cbThunder->SetSelected(amod_rain_thunder.GetBool());
	m_cbThunder->SetBounds(2, 138, 175, 25);

	//create the splashes check button
	m_cbSplashes = new CheckButton(this, "CheckButtonEnableSplashes", "Show Rain Splash Particles");
	m_cbSplashes->SetSelected(amod_rain_splashes.GetBool());
	m_cbSplashes->SetBounds(2, 159, 175, 25);
	m_cbSplashes->SetEnabled(amod_rain_enable.GetBool());

	//create 'rain Options' label
	Label* RainType = new Label(this, "RainTypeLabel", "Rain Options");
	RainType->SetBounds(55, 186, 160, 20);

	//create the type of rain rain button
	m_cbType = new ComboBox(this, "RainTypeComboBox", 10, false);
	m_cbType->SetBounds(7, 206, 187, 22);
	m_cbType->AddItem("Rain Intensity: Very Light", nullptr);
	m_cbType->AddItem("Rain Intensity: Light", nullptr);
	m_cbType->AddItem("Rain Intensity: Medium", nullptr);
	m_cbType->AddItem("Rain Intensity: Heavy", nullptr);
	m_cbType->AddItem("Rain Intensity: Very Heavy", nullptr);
	m_cbType->SetEnabled(amod_rain_enable.GetBool());
	m_cbType->ActivateItem(2);
	m_cbType->GetTooltip()->SetText("Note that 'Very Heavy' and 'Heavy' rain CAN (rarely) crash the game!");
	m_cbType->GetTooltip()->SetTooltipDelay(100);
	m_cbType->GetTooltip()->SetTooltipFormatToMultiLine();

	//select the current rain
	for (int i = 0; i < SIZE_OF_ARRAY(sg_RainTypes); i++)
	{
		if (!Q_strncmp(r_raindensity.GetString(), sg_RainTypes[i], Q_strlen(sg_RainTypes[i])))
		{
			m_cbType->ActivateItem(i);
			break;
		}
	}

	//create the rain mode button
	m_cbMode = new ComboBox(this, "RainModeComboBox", 2, false);
	m_cbMode->SetBounds(7, 234, 187, 22);
	m_cbMode->AddItem("Always Rain", nullptr);
	m_cbMode->AddItem("Rain In Random Intervals", nullptr);
	m_cbMode->ActivateItem(amod_rain_type.GetInt() ? amod_rain_type.GetInt() - 1 : 0);
	m_cbMode->SetEnabled(amod_rain_enable.GetBool());

	//create 'rain wait time min' label
	m_RainWaitTimeMinLabel = new Label(this, "RainWaitTimeMinLabel", "Rain Wait Time Min: 600s");
	m_RainWaitTimeMinLabel->SetBounds(5, 261, 200, 20);

	//rain wait time min slider
	m_RainWaitTimeMinSlider = new vgui::Slider(this, "RainWaitTimeMinSlider");
	m_RainWaitTimeMinSlider->SetBounds(5, 281, 190, 20);
	m_RainWaitTimeMinSlider->SetRange(10, 600);
	m_RainWaitTimeMinSlider->SetValue(amod_rain_wait_min.GetInt());

	//create 'rain wait time max' label
	m_RainWaitTimeMaxLabel = new Label(this, "RainWaitTimeMaxLabel", "Rain Wait Time Max: 1200s");
	m_RainWaitTimeMaxLabel->SetBounds(10, 306, 200, 20);

	//rain wait time max slider
	m_RainWaitTimeMaxSlider = new vgui::Slider(this, "RainWaitTimeMaxSlider");
	m_RainWaitTimeMaxSlider->SetBounds(5, 326, 190, 20);
	m_RainWaitTimeMaxSlider->SetRange(30, 1200);
	m_RainWaitTimeMaxSlider->SetValue(amod_rain_wait_max.GetInt());

	//make the apply button
	Button* ApplyButton = new Button(this, "_ApplyButtonWeatherPanel", "Apply Settings");
	ApplyButton->SetBounds(5, 354, 187, 20);
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
	m_cbSplashes->SetEnabled(m_cbEnable->IsSelected());
	m_cbType->SetEnabled(m_cbEnable->IsSelected());
	m_cbMode->SetEnabled(m_cbEnable->IsSelected());

	//set the texts
	m_RainWaitTimeMinLabel->SetText(CFmtStr("Rain Wait Time Min: %ds", m_RainWaitTimeMinSlider->GetValue()));
	m_RainWaitTimeMaxLabel->SetText(CFmtStr("Rain Wait Time Max: %ds", m_RainWaitTimeMaxSlider->GetValue()));

	//enable/disable sliders
	m_RainWaitTimeMinSlider->SetEnabled(m_cbEnable->IsSelected() && m_cbMode->GetActiveItem() == 1);
	m_RainWaitTimeMaxSlider->SetEnabled(m_cbEnable->IsSelected() && m_cbMode->GetActiveItem() == 1);

	//call the base function
	BaseClass::OnTick();
}

//------------------------------------------------------------------------------------------
// Purpose: Called when a command for the panel is ran
//------------------------------------------------------------------------------------------
void CAModWeatherPanel::OnCommand(const char* command)
{
	//check for the 'apply' command
	if (!Q_strcmp(command, WEATHER_PANEL_COMMAND_APPLY))
	{
		//enable/disable breaths
		engine->ClientCmd_Unrestricted(CFmtStr("amod_do_breathing %d", m_cbBreaths->IsSelected()));

		//enable/disable the weather info
		engine->ClientCmd_Unrestricted(CFmtStr("amod_clouds %d", m_cbClouds->IsSelected()));
		
		//enable/disable clouds overriding
		engine->ClientCmd_Unrestricted(CFmtStr("amod_clouds_color_override %d", m_cbCloudsOverrideColor->IsSelected()));

		//set the clouds color
		engine->ClientCmd_Unrestricted(CFmtStr("amod_clouds_color %d %d %d %d", m_CurrentCloudsColor.r(), m_CurrentCloudsColor.g(), m_CurrentCloudsColor.b(), m_CurrentCloudsColor.a()));

		//get the convars
		static ConVarRef amod_rain_enable("amod_rain_enable");
		static ConVarRef amod_rain_type("amod_rain_type");
		static ConVarRef r_raindensity("r_raindensity");

		//set the rain wait times
		engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_wait_min %d", m_RainWaitTimeMinSlider->GetValue()));
		engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_wait_max %d", m_RainWaitTimeMaxSlider->GetValue()));

		//enable/disable the rain
		if (m_cbEnable->IsSelected() != amod_rain_enable.GetBool())
			engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_enable %d", m_cbEnable->IsSelected()));

		//set the rain mode
		if (amod_rain_type.GetInt() != m_cbMode->GetActiveItem() + 1)
			engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_type %d", m_cbMode->GetActiveItem() + 1));
		
		//set the rain type
		engine->ClientCmd_Unrestricted(CFmtStr("r_raindensity %s", sg_RainTypes[m_cbType->GetActiveItem()]));

		//enable/disable splashes
		engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_splashes %d", m_cbSplashes->IsSelected()));

		//enable/disable the thunder
		engine->ClientCmd_Unrestricted(CFmtStr("amod_rain_thunder %d", m_cbThunder->IsSelected()));

		//after the convars have been set write the config
		engine->ClientCmd_Unrestricted("amod_write_rain_config");

		//bounds checking
		if (m_RainWaitTimeMinSlider->GetValue() > m_RainWaitTimeMaxSlider->GetValue())
		{
			int tmp = m_RainWaitTimeMinSlider->GetValue();
			m_RainWaitTimeMinSlider->SetValue(m_RainWaitTimeMaxSlider->GetValue());
			m_RainWaitTimeMaxSlider->SetValue(tmp);
		}

		//set g_NeedSoundscapeEnable
		g_NeedSoundscapeEnable = m_cbEnable->IsSelected();
		return;
	}
	else if (!Q_strcmp(command, WEATHER_PANEL_COMMAND_CLOUDS_COLOR))
	{
		//create the color picker window
		m_CloudsColorPicker = new CColorPicker(GetVPanel());
		m_CloudsColorPicker->SetTitle("Set Clouds Color (255 255 255 255 = default clouds color)", true);
		m_CloudsColorPicker->SetColor(m_CurrentCloudsColor);
		m_CloudsColorPicker->DoModal();
		return;
	}
	else if (!Q_strcmp(command, WEATHER_PANEL_COMMAND_CLOUDS_OVERRIDE))
	{
		m_ButtonCloudsColor->SetEnabled(m_cbCloudsOverrideColor->IsSelected());
	}

	BaseClass::OnCommand(command);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: writes the rain info into the config
//-------------------------------------------------------------------------------------------------------
void Amod_WriteRainConfig(KeyValues* file)
{
	//write the weather stuff
	ConVarRef amod_rain_type("amod_rain_type");
	ConVarRef amod_rain_enable("amod_rain_enable");
	ConVarRef amod_rain_splashes("amod_rain_splashes");
	ConVarRef amod_rain_thunder("amod_rain_thunder");
	ConVarRef amod_rain_wait_min("amod_rain_wait_min");
	ConVarRef amod_rain_wait_max("amod_rain_wait_max");
	ConVarRef amod_do_breathing("amod_do_breathing");
	ConVarRef amod_clouds("amod_clouds");
	ConVarRef amod_clouds_color("amod_clouds_color");
	ConVarRef amod_clouds_color_override("amod_clouds_color_override");
	ConVarRef r_raindensity("r_raindensity");

	file->SetBool("amod_do_breathing", amod_do_breathing.GetBool());
	file->SetBool("amod_clouds", amod_clouds.GetBool());
	file->SetString("amod_clouds_color", amod_clouds_color.GetString());
	file->SetBool("amod_clouds_color_override", amod_clouds_color_override.GetBool());
	file->SetInt("amod_rain_type", amod_rain_type.GetInt());
	file->SetBool("amod_rain_enable", amod_rain_enable.GetBool());
	file->SetBool("amod_rain_thunder", amod_rain_thunder.GetBool());
	file->SetBool("amod_rain_splashes", amod_rain_splashes.GetBool());
	file->SetInt("amod_rain_wait_min", amod_rain_wait_min.GetInt());
	file->SetInt("amod_rain_wait_max", amod_rain_wait_max.GetInt());
	file->SetString("r_raindensity", r_raindensity.GetString());
}

//-------------------------------------------------------------------------------------------------------
// Purpose: writes the rain info into the config
//-------------------------------------------------------------------------------------------------------
CON_COMMAND(amod_write_rain_config, "writes the rain info into the config")
{
	//load the config
	KeyValues* file = new KeyValues("AloneModConfig");
	if (!file->LoadFromFile(filesystem, "cfg/AloneMod_Config.txt", "MOD"))
	{
		//delete the keyvalues
		file->deleteThis();
		return;
	}

	//write
	Amod_WriteRainConfig(file);

	//save the file
	file->SaveToFile(filesystem, "cfg/AloneMod_Config.txt", "MOD");

	//delete the keyvalues
	file->deleteThis();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Paints the panel
//-------------------------------------------------------------------------------------------------------
void CAModWeatherPanel::Paint()
{
	BaseClass::Paint();

	//paint the colored square
	Color color = m_CurrentCloudsColor;
	if (color.r() == 255 && color.g() == 255 && color.b() == 255 && color.a() == 255)
		color = Color(51, 103, 153, 255);

	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(175, 68, 175 + 20, 68 + 20);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on key press
//-------------------------------------------------------------------------------------------------------
void CAModWeatherPanel::OnKeyCodePressed(KeyCode code)
{
	//call the base function
	BaseClass::OnKeyCodePressed(code);

	//get the bind to toggle the song panel
	const char* bind = engine->Key_LookupBinding("ToggleWeatherPanel");
	if (!bind)
		return;

	//get the string of the key that was pressed
	const char* DisplayCode = (const char*)KeyCodeToDisplayString(code);

	//compare the 2 strings and if they match. close the panel
	if (!Q_stricmp(bind, DisplayCode))
		delete this;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CAModWeatherPanel::OnColorSelected(KeyValues* data)
{
	//set the clouds color
	m_CurrentCloudsColor = Color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));

	//close the modal if needed
	delete m_CloudsColorPicker;
	m_CloudsColorPicker = nullptr;
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
	void OnCommand(const char* pcCommand);
	void OnTick();

	//destructor
	~CAModSkyboxPanel();

private:

	//variables
	KeyValues* m_SkyboxFile;
	CUtlVector<const char*> m_Skyboxes;

	//panel items
	ComboBox* m_skyComboBox;
	ComboBox* m_filterComboBox;
	ComboBox* m_intensitiesComboBox;
	CStretchingImage* m_skyImage;
	CheckButton* m_FogDisable;
	CheckButton* m_SunDisableButton;

	//is it daytime
	bool m_bDaytime;

	//other variables
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
	SetTitle("Skybox Panel", false);
	SetSize(250, 432);
	MoveToCenterOfScreen();

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

	//check if its daytime
	m_bDaytime = IsDaytimeEnabled();

	//look for "day" or "night" key
	KeyValues* timekey = m_SkyboxFile->FindKey(m_bDaytime ? "day" : "night");
	if (!timekey)
	{
		m_SkyboxFile->deleteThis();
		m_SkyboxFile = nullptr;

		//send warning
		ConWarning("Failed To Find \"%s\" key in resource/panels/SkyPanel.txt\n", m_bDaytime ? "day" : "night");
		return;
	}

	//get the current sky name
	ConVarRef amod_sky(m_bDaytime ? "amod_day_sky" : "amod_night_sky");

	{
		int index = 0, current = 1;

		//before anything make the skybox's combo box so i can add the skybox names to it
		m_skyComboBox = new ComboBox(this, "m_skyComboBox", 14, false);
		m_skyComboBox->SetBounds(10, 282, 230, 20);

		//always add "" as the starting item
		m_skyComboBox->AddItem("use the maps default skybox", nullptr);
		m_Skyboxes.AddToTail(m_bDaytime ? "sky_day02_09" : "sky_borealis01");

		//go through each sky and add it
		const char* amod_sky_value = amod_sky.GetString();
		FOR_EACH_VALUE(timekey, sub)
		{
			//add the name to the m_skyComboBox
			m_skyComboBox->AddItem(sub->GetName(), nullptr);

			//add the skybox name
			m_Skyboxes.AddToTail(sub->GetString());

			//if the skyname == amod_sky then set index to current
			if (amod_sky_value[0] && !Q_strnicmp(amod_sky.GetString(), sub->GetString(), Q_strlen(amod_sky_value)))
				index = current;

			//increment current
			current++;
		}

		//active the item
		m_skyComboBox->ActivateItem(index);

		//divider under this
		Divider* div = new Divider(this, "DividerUnderSkybox");
		div->SetBounds(-2, 310, 300, 1);
	}

	ConVarRef amod_filter_filename(m_bDaytime ? "amod_epic_filter_day_filename" : "amod_epic_filter_night_filename");

	{
		//filter text above this
		Label* FilterText = new Label(this, "FilterText", "");
		FilterText->SetBounds(5, 318, 240, 50);
		FilterText->SetText("Post Processing Filter Settings.\nRequires post processing filter enabled");
		FilterText->SetContentAlignment(Label::Alignment::a_north);
		FilterText->SetCenterWrap(true);

		//make the custom filter combo box
		m_filterComboBox = new ComboBox(this, "m_filterComboBox", 14, false);
		m_filterComboBox->SetBounds(10, 361, 230, 20);

		//always 
		m_filterComboBox->AddItem("Use the maps default filter", new KeyValues(""));

		//get the stuff
		int index = 0, current = 1;

		//go through each filter in the scripts/colorcorrection/* directory
		FileFindHandle_t find;
		const char* first = filesystem->FindFirst("scripts/colorcorrection/*.raw", &find);
		while (first)
		{
			//check for . and ..
			if (!Q_stricmp(first, ".") || !Q_stricmp(first, ".."))
			{
				first = filesystem->FindNext(find);
				continue;
			}

			//check the extention
			const char* ext = Q_GetFileExtension(first);
			if (Q_stricmp(ext, "raw"))
			{
				first = filesystem->FindNext(find);
				continue;
			}

			//add the item
			const char* cc_string = CFmtStr("scripts/colorcorrection/%s", first);
			m_filterComboBox->AddItem(first, new KeyValues(cc_string));
			 
			//add to combo box
			if (!Q_stricmp(cc_string, amod_filter_filename.GetString()))
				index = current;

			first = filesystem->FindNext(find);

			current++;
		}

		//active the current index
		m_filterComboBox->ActivateItem(index);

		filesystem->FindClose(find);
	}
	
	ConVarRef amod_filter_intensity(m_bDaytime ? "amod_epic_filter_day_intensity" : "amod_epic_filter_night_intensity");

	{
		//make the filter intensities combo box
		m_intensitiesComboBox = new ComboBox(this, "m_intensitiesComboBox", 14, false);
		m_intensitiesComboBox->SetBounds(10, 383, 230, 20);

		//always 
		m_intensitiesComboBox->AddItem("Use the maps default intensity", new KeyValues(""));
		
		int index = 0, current = 1;

		//start from 0.05. Go up to 1
		float curr = 0.05;
		while (curr < 1.05)
		{
			//due to float inprecision i have to do a string comparison
			const char* curr_s = CFmtStr("%.2f", curr);
			if (!Q_strnicmp(amod_filter_intensity.GetString(), curr_s, Q_strlen(curr_s)))
				index = current;
			
			m_intensitiesComboBox->AddItem(curr_s, new KeyValues(curr_s));

			//add 0.05
			curr += 0.05;
			current++;
		}

		//active the current index
		m_intensitiesComboBox->ActivateItem(index);
	}

	//make the fog disable button
	m_FogDisable = new CheckButton(this, "", "Disable Fog");
	m_FogDisable->SetBounds(5, 260, 105, 20);
	m_FogDisable->SetSelected(amod_fog_disabled.GetBool());
	
	//make the sun disable button
	ConVarRef amod_sun_disable("amod_sun_disable");
	m_SunDisableButton = new CheckButton(this, "", "Disable The Sun");
	m_SunDisableButton->SetBounds(115, 260, 135, 20);
	m_SunDisableButton->SetEnabled(m_bDaytime);
	m_SunDisableButton->SetSelected(amod_sun_disable.GetBool());

	{
		//check for % to change the direction the skybox displays
		const char* tmp = m_Skyboxes[m_skyComboBox->GetActiveItem()];
		char curr[128];
		Q_strncpy(curr, tmp, sizeof(curr));

		char* percent = Q_strstr(curr, "%");
		if (percent)
		{
			*percent = '\0';
			percent++;
		}
		else
			percent = "up";

		//get the sky name
		char buf[218];
		Q_snprintf(buf, sizeof(buf), "../skybox/%s%s", curr, percent);

		m_skyImage = new CStretchingImage(this, "m_SkyImage");
		m_skyImage->SetImage(buf);
		m_skyImage->SetBounds(10, 30, 230, 230);
	}

	//make the apply button
	Button* SetButton = new Button(this, "SetButton", "Update Settings");
	SetButton->SetBounds(10, 407, 230, 20);
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
		{
			//disable the sun
			ConVar* var = cvar->FindVar("amod_sun_disable");
			if (!var)
				return;

			var->SetValue(m_SunDisableButton->IsSelected());
		}
		
		{
			//disable the fog
			amod_fog_disabled.SetValue(m_FogDisable->IsSelected());
		}

		{
			//set the filters filename
			ConVar* var = cvar->FindVar(m_bDaytime ? "amod_epic_filter_day_filename" : "amod_epic_filter_night_filename");
			if (!var)
				return;

			var->SetValue(m_filterComboBox->GetActiveItemUserData()->GetName());
		}

		{
			//set the filters intensities
			ConVar* var = cvar->FindVar(m_bDaytime ? "amod_epic_filter_day_intensity" : "amod_epic_filter_night_intensity");
			if (!var)
				return;

			var->SetValue(m_intensitiesComboBox->GetActiveItemUserData()->GetName());
		}

		{
			ConVar* var = cvar->FindVar(m_bDaytime ? "amod_day_sky" : "amod_night_sky");
			if (!var)
				return;

			//get the string
			int index = m_skyComboBox->GetActiveItem();
			const char* set = index == 0 ? "" : m_Skyboxes[index];

			//get 
			char curr[128];
			Q_strncpy(curr, set, sizeof(curr));

			//check for '%'
			char* percent = Q_strstr(curr, "%");
			if (percent)
				*percent = '\0';

			//set amod_sky to the current sky
			var->SetValue(curr);
		}

		//write the settings
		Amod_WriteConfig();

		//close this panel
		//skyboxpanel->Close();
		return;
	}
	else if (!Q_stricmp(pcCommand, "close"))
	{
		skyboxpanel->Close();
		return;
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

	//check for % to change the direction the skybox displays
	const char* tmp = m_Skyboxes[m_skyComboBox->GetActiveItem()];
	char curr[128];
	Q_strncpy(curr, tmp, sizeof(curr));

	char* percent = Q_strstr(curr, "%");
	if (percent)
	{
		*percent = '\0';
		percent++;
	}
	else
		percent = "up";

	//get the sky name
	char buf[218];
	Q_snprintf(buf, sizeof(buf), "../skybox/%s%s", curr, percent);

	//set the skybox image
	m_skyImage->SetImage(buf);
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
	friend class COptionsPanelInterface;

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

	//other settings
	CheckButton* m_EnableGodCheckButton;
	CheckButton* m_NoSoundscapesCheckButton;
	CheckButton* m_NoMusicCheckButton;
	CheckButton* m_MusicTransitionCheckButton;
	CheckButton* m_DayTimeCB;
	CheckButton* m_CoreTimerCheckButton;
	CheckButton* m_CitadelTimerCheckButton;

	//---------- bottom right -----------//
	ComboBox* m_EndingComboBox;

#if !AMOD_DAYTIME_EDITION
	Button* m_SkyboxButton;
#endif

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

	SetRoundedCorners(0);

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
	float FilterOnValue = GetFilterOnValue(m_FilterOnValue);
	float FilterExponentValue = GetFilterOnExponentValue(m_FilterOnExponent);
	float FilterOffValue = GetFilterOffValue(m_FilterOffValue);

	//set the monitor gamma values for the filter.
	engine->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma %f", FilterOffValue));
	engine->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma_tv_exp %f", FilterExponentValue));

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
		if (!Q_strcmp(varname, "amod_filter_brightness_on"))
		{
			amod_filter_brightness_on.SetValue(m_FilterOnValue = atoi(varvalue));
		}
		else if (!Q_strcmp(varname, "amod_filter_brightness_on_exp"))
		{
			amod_filter_brightness_on_exp.SetValue(m_FilterOnExponent = atoi(varvalue));
		}
		else if (!Q_strcmp(varname, "amod_filter_brightness_off"))
		{
			amod_filter_brightness_off.SetValue(m_FilterOffValue = atoi(varvalue));
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
			engine->ClientCmd_Unrestricted(CFmtStr("%s %s", varname, varvalue));

			//set the convar value
			var->SetValue(varvalue);
		}
	}
}

//macro for finding keyvalues
#define OPTIONS_KEYVALUES_FIND(name, string)																\
KeyValues* name = kvMain->FindKey(string);																	\
if (!name)																									\
{																											\
	kvMain->deleteThis();																					\
	kvMain = nullptr;																						\
	ConWarning("Got NULL Subkey: \"" string "\"In File: \"OptionsPanel.txt\" Aborting Panel Load!\n");		\
	return;																									\
}

//macto for adding tool tips
#define OPTIONS_ELEMENT_ADD_TOOLTIP(element, keyvalues)								\
if (element && element->GetTooltip() && keyvalues->GetBool("ToolTipEnabled"))		\
{																					\
	element->GetTooltip()->SetTooltipDelay(keyvalues->GetInt("TooltipDelay"));		\
																					\
	if (keyvalues->GetBool("ToolTipMultiline"))										\
		element->GetTooltip()->SetTooltipFormatToMultiLine();						\
	else																			\
		element->GetTooltip()->SetTooltipFormatToSingleLine();						\
																					\
	element->GetTooltip()->SetText(keyvalues->GetString("TooltipText"));			\
}

//------------------------------------------------------------------------------------------
// Purpose: Initalizes all the panel items
//------------------------------------------------------------------------------------------
void COptionsPanel::Init()
{
	//find all the panel item keyvalues

	//bottom right
	OPTIONS_KEYVALUES_FIND(EndingButtonKV, "EndingComboBox");			//ending combo box
	//OPTIONS_KEYVALUES_FIND(WeatherButtonKv, "WeatherButton");			//weather button

#if !AMOD_DAYTIME_EDITION
	OPTIONS_KEYVALUES_FIND(SkyboxButtonKv, "SkyboxButton");				//skybox button
#endif

	OPTIONS_KEYVALUES_FIND(CreditsButtonKv, "CreditsButton");			//credits button
	OPTIONS_KEYVALUES_FIND(ApplyButtonKv, "ApplyButton");				//apply button

	//bottom left
	OPTIONS_KEYVALUES_FIND(FilterSettingsLabelKv, "FilterSettingsLabel");		//filter settings label
	OPTIONS_KEYVALUES_FIND(FilterOnBrightnessLabelKv, "FilterBOnLabel");			//filter on brightness label
	OPTIONS_KEYVALUES_FIND(FilterOnBrightnessSliderKv, "FilterOnBSlider");			//filter on brightness
	OPTIONS_KEYVALUES_FIND(FilterOnExponentLabelKv, "FilterBOnExpLabel");		//filter exponent label
	OPTIONS_KEYVALUES_FIND(FilterOnExponentSliderKv, "FilterOnExpBSlider");		//filter exponent
	OPTIONS_KEYVALUES_FIND(FilterOffBrightnessLabelKv, "FilterBOffLabel");			//filter off brightness label
	OPTIONS_KEYVALUES_FIND(FilterOffBrightnessSliderKv, "FilterOffBSlider");		//filter off brightness
	OPTIONS_KEYVALUES_FIND(EpicFilterCheckButtonKV, "EpicFilterCB");			//epic filter check button
	OPTIONS_KEYVALUES_FIND(EffectsButtonKv, "EffectsButton");			//effects button

	//top left
	OPTIONS_KEYVALUES_FIND(ViewSettingsLabelKv, "ViewSettingsLabel");
	OPTIONS_KEYVALUES_FIND(DisableFootstepSoundsCheckButtonKV, "CBDFootstebSnds");
	OPTIONS_KEYVALUES_FIND(DisableHudCheckButtonKV, "CBDisableHud");
	OPTIONS_KEYVALUES_FIND(MirroredViewCheckButtonKv, "MirroredCB");
	OPTIONS_KEYVALUES_FIND(VignetteCheckButtonKv, "VignetteCB");
	OPTIONS_KEYVALUES_FIND(ViewBobCheckButtonKv, "ViewBobbingCB");
	OPTIONS_KEYVALUES_FIND(StandBobCheckButtonKv, "StandBobCB");
	OPTIONS_KEYVALUES_FIND(JumpViewpunchCheckButtonKv, "JumpViewpunchCB");
	OPTIONS_KEYVALUES_FIND(LandViewpunchCheckButtonKv, "LandViewpunchCB");
	OPTIONS_KEYVALUES_FIND(RollAngleCheckButtonKv, "RollAngleCB");
	OPTIONS_KEYVALUES_FIND(RollAngleSliderKv, "RollAngleSlider");
	OPTIONS_KEYVALUES_FIND(RollAngleLabelKv, "RollAngleLabel");

	//flashlight settings
	OPTIONS_KEYVALUES_FIND(FlashlightSettingsLabelKv, "FlashlightSettingsLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFarLabelKv, "FlashlightFarLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFovLabelKv, "FlashlightFovLabel");
	OPTIONS_KEYVALUES_FIND(FlashlightFarComboBoxKv, "FlashlightFarCombobox");
	OPTIONS_KEYVALUES_FIND(FlashlightFovComboBoxKv, "FlashlightFovCombobox");
	OPTIONS_KEYVALUES_FIND(FlashlightFlickerCheckButtonKv, "FFlickerCbox");
	OPTIONS_KEYVALUES_FIND(FlashlightLagCheckButtonKv, "FLagCBox");

	//get the far data
	KeyValues* FlashlightFarDataKv = FlashlightFarComboBoxKv->FindKey("Data");
	if (!FlashlightFarDataKv)
	{
		kvMain->deleteThis();
		kvMain = nullptr;
		ConWarning("Got NULL Subkey: \"Data\" In Subkey \"FlashlightFarCombobox\" File: \"OptionsPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//get the fov data
	KeyValues* FlashlightFovDataKv = FlashlightFovComboBoxKv->FindKey("Data");
	if (!FlashlightFovDataKv)
	{
		kvMain->deleteThis();
		kvMain = nullptr;
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

	//other settings
	OPTIONS_KEYVALUES_FIND(OtherSettingsLabelKv, "CheatSettings");
	OPTIONS_KEYVALUES_FIND(GodModeCheckButtonKv, "GodModeCB");
	OPTIONS_KEYVALUES_FIND(NoSoundscapesCheckButtonKv, "NoSoundscapesCB");
	OPTIONS_KEYVALUES_FIND(NoMusicCheckButtonKv, "NoMusicCB");
	OPTIONS_KEYVALUES_FIND(TransitionMusicCheckButtonKv, "TransitionMusicCB");
	OPTIONS_KEYVALUES_FIND(CitadelTimerCheckButtonKv, "CitadelTimerCB");
	OPTIONS_KEYVALUES_FIND(CoreTimerCheckButtonKv, "CoreTimerCB");
	OPTIONS_KEYVALUES_FIND(DayTimeCBoxKv, "DayTimeCBox");

	//dividers
	OPTIONS_KEYVALUES_FIND(Divider1Kv, "SettingsDivider1");
	OPTIONS_KEYVALUES_FIND(Divider2Kv, "SettingsDivider2");
	OPTIONS_KEYVALUES_FIND(Divider3Kv, "SettingsDivider3");
	OPTIONS_KEYVALUES_FIND(Divider4Kv, "SettingsDivider4");
	OPTIONS_KEYVALUES_FIND(Divider5Kv, "SettingsDivider5");

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
	ConVarRef amod_viewbob_enabled("amod_viewbob_enabled");
	m_ViewBobCheckButton = new CheckButton(this, "ViewBobCheckButton", ViewBobCheckButtonKv->GetString("Text", ""));
	m_ViewBobCheckButton->SetBounds(ViewBobCheckButtonKv->GetInt("XPos"), ViewBobCheckButtonKv->GetInt("YPos"), ViewBobCheckButtonKv->GetInt("Wide"), ViewBobCheckButtonKv->GetInt("Tall"));
	m_ViewBobCheckButton->SetSelected(amod_viewbob_enabled.GetInt());

	//stand bob
	ConVarRef amod_standbob_enabled("amod_standbob_enabled");
	m_StandBobCheckButton = new CheckButton(this, "StandBobCheckButton", StandBobCheckButtonKv->GetString("Text", ""));
	m_StandBobCheckButton->SetBounds(StandBobCheckButtonKv->GetInt("XPos"), StandBobCheckButtonKv->GetInt("YPos"), StandBobCheckButtonKv->GetInt("Wide"), StandBobCheckButtonKv->GetInt("Tall"));
	m_StandBobCheckButton->SetSelected(amod_standbob_enabled.GetBool());

	//jump viewpunch
	ConVarRef amod_jump_punch_enable("amod_jump_punch_enable");
	m_JumpViewpunchCheckButton = new CheckButton(this, "JumpViewpunch", JumpViewpunchCheckButtonKv->GetString("Text", ""));
	m_JumpViewpunchCheckButton->SetBounds(JumpViewpunchCheckButtonKv->GetInt("XPos"), JumpViewpunchCheckButtonKv->GetInt("YPos"), JumpViewpunchCheckButtonKv->GetInt("Wide"), JumpViewpunchCheckButtonKv->GetInt("Tall"));
	m_JumpViewpunchCheckButton->SetSelected(amod_jump_punch_enable.GetInt());

	//land viewpunch
	ConVarRef amod_land_punch_enable("amod_land_punch_enable");
	m_LandViewpunchCheckButton = new CheckButton(this, "LandViewpunch", LandViewpunchCheckButtonKv->GetString("Text", ""));
	m_LandViewpunchCheckButton->SetBounds(LandViewpunchCheckButtonKv->GetInt("XPos"), LandViewpunchCheckButtonKv->GetInt("YPos"), LandViewpunchCheckButtonKv->GetInt("Wide"), LandViewpunchCheckButtonKv->GetInt("Tall"));
	m_LandViewpunchCheckButton->SetSelected(amod_land_punch_enable.GetInt());

	//roll angle check button
	ConVarRef sv_rollangle("sv_rollangle");
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
	m_FilterOnBrightnessSlider->SetRange(0, 12);
	m_FilterOnBrightnessSlider->SetValue(m_FilterOnValue);

	//make 'filter on exponent' label
	Label* FilterOnExponentLabel = new Label(this, "FilterOnExponentLabel", FilterOnExponentLabelKv->GetString("text"));
	FilterOnExponentLabel->SetBounds(FilterOnExponentLabelKv->GetInt("XPos"), FilterOnExponentLabelKv->GetInt("YPos"), FilterOnExponentLabelKv->GetInt("Wide"), FilterOnExponentLabelKv->GetInt("Tall"));

	//make 'filter on exponent' slider
	m_FilterOnExponentSlider = new Slider(this, "FilterOnExponent");
	m_FilterOnExponentSlider->SetBounds(FilterOnExponentSliderKv->GetInt("XPos"), FilterOnExponentSliderKv->GetInt("YPos"), FilterOnExponentSliderKv->GetInt("Wide"), FilterOnExponentSliderKv->GetInt("Tall"));
	m_FilterOnExponentSlider->SetRange(0, 12);
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
	EffectsButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_EFFECTS_PANEL);

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

	//-------------------------------------- OTHER SETTINGS --------------------------------------//
	
	//other settings label
	Label* OtherSettingsLabel = new Label(this, "OtherSettings", OtherSettingsLabelKv->GetString("text"));
	OtherSettingsLabel->SetBounds(OtherSettingsLabelKv->GetInt("XPos"), OtherSettingsLabelKv->GetInt("YPos"), OtherSettingsLabelKv->GetInt("Wide"), OtherSettingsLabelKv->GetInt("Tall"));

	//god mode
	ConVarRef amod_god("amod_enable_god");
	m_EnableGodCheckButton = new CheckButton(this, "EnableGod", GodModeCheckButtonKv->GetString("Text", ""));
	m_EnableGodCheckButton->SetBounds(GodModeCheckButtonKv->GetInt("XPos"), GodModeCheckButtonKv->GetInt("YPos"), GodModeCheckButtonKv->GetInt("Wide"), GodModeCheckButtonKv->GetInt("Tall"));
	m_EnableGodCheckButton->SetSelected(amod_god.GetInt());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_EnableGodCheckButton, GodModeCheckButtonKv);

	//disable soundscapes
	ConVarRef amod_soundscapes_disable("amod_soundscapes_disable");
	m_NoSoundscapesCheckButton = new CheckButton(this, "DisableSoundscapes", NoSoundscapesCheckButtonKv->GetString("Text", ""));
	m_NoSoundscapesCheckButton->SetBounds(NoSoundscapesCheckButtonKv->GetInt("XPos"), NoSoundscapesCheckButtonKv->GetInt("YPos"), NoSoundscapesCheckButtonKv->GetInt("Wide"), NoSoundscapesCheckButtonKv->GetInt("Tall"));
	m_NoSoundscapesCheckButton->SetSelected(amod_soundscapes_disable.GetBool());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_NoSoundscapesCheckButton, NoSoundscapesCheckButtonKv);

	//disable map music
	ConVarRef amod_music_disable("amod_music_disable");
	m_NoMusicCheckButton = new CheckButton(this, "NoMusic", NoMusicCheckButtonKv->GetString("Text", ""));
	m_NoMusicCheckButton->SetBounds(NoMusicCheckButtonKv->GetInt("XPos"), NoMusicCheckButtonKv->GetInt("YPos"), NoMusicCheckButtonKv->GetInt("Wide"), NoMusicCheckButtonKv->GetInt("Tall"));
	m_NoMusicCheckButton->SetSelected(amod_music_disable.GetBool());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_NoMusicCheckButton, NoMusicCheckButtonKv);
	
	//transition music through levels
	ConVarRef amod_songs_transition_through_levels("amod_songs_transition_through_levels");
	m_MusicTransitionCheckButton = new CheckButton(this, "TransitionMusic", TransitionMusicCheckButtonKv->GetString("Text", ""));
	m_MusicTransitionCheckButton->SetBounds(TransitionMusicCheckButtonKv->GetInt("XPos"), TransitionMusicCheckButtonKv->GetInt("YPos"), TransitionMusicCheckButtonKv->GetInt("Wide"), TransitionMusicCheckButtonKv->GetInt("Tall"));
	m_MusicTransitionCheckButton->SetSelected(amod_songs_transition_through_levels.GetBool());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_MusicTransitionCheckButton, NoMusicCheckButtonKv);

	//day for nova prospekt
	m_DayTimeCB = new CheckButton(this, "DayAfterNovaProspekt", DayTimeCBoxKv->GetString("Text", ""));
	m_DayTimeCB->SetBounds(DayTimeCBoxKv->GetInt("XPos"), DayTimeCBoxKv->GetInt("YPos"), DayTimeCBoxKv->GetInt("Wide"), DayTimeCBoxKv->GetInt("Tall"));

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_DayTimeCB, DayTimeCBoxKv);

#if !AMOD_DAYTIME_EDITION
	//set selected if not daytime edition
	m_DayTimeCB->SetSelected(amod_day.GetBool());
#endif

	//citadel timer
	ConVarRef amod_do_citadel_timer("amod_do_citadel_timer");
	m_CitadelTimerCheckButton = new CheckButton(this, "CitadelTimer", CitadelTimerCheckButtonKv->GetString("Text"));
	m_CitadelTimerCheckButton->SetBounds(CitadelTimerCheckButtonKv->GetInt("XPos"), CitadelTimerCheckButtonKv->GetInt("YPos"), CitadelTimerCheckButtonKv->GetInt("Wide"), CitadelTimerCheckButtonKv->GetInt("Tall"));
	m_CitadelTimerCheckButton->SetSelected(amod_do_citadel_timer.GetBool());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_CitadelTimerCheckButton, CitadelTimerCheckButtonKv);

	//core timer
	ConVarRef amod_do_core_timer("amod_do_core_timer");
	m_CoreTimerCheckButton = new CheckButton(this, "", CoreTimerCheckButtonKv->GetString("Text"));
	m_CoreTimerCheckButton->SetBounds(CoreTimerCheckButtonKv->GetInt("XPos"), CoreTimerCheckButtonKv->GetInt("YPos"), CoreTimerCheckButtonKv->GetInt("Wide"), CoreTimerCheckButtonKv->GetInt("Tall"));
	m_CoreTimerCheckButton->SetSelected(amod_do_core_timer.GetBool());

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_CoreTimerCheckButton, CoreTimerCheckButtonKv);

	//-------------------------------------- APPLY AND OTHER --------------------------------------//

	//make the ending combo box
	m_EndingComboBox = new ComboBox(this, "EndingComboBox", 2, false);
	m_EndingComboBox->AddItem("Mod Ending: Ending 1", nullptr);
	m_EndingComboBox->AddItem("Mod Ending: Ending 2", nullptr);
	m_EndingComboBox->ActivateItem(0);
	m_EndingComboBox->SetBounds(EndingButtonKV->GetInt("XPos"), EndingButtonKV->GetInt("YPos"), EndingButtonKV->GetInt("Wide"), EndingButtonKV->GetInt("Tall"));

	OPTIONS_ELEMENT_ADD_TOOLTIP(m_EndingComboBox, EndingButtonKV);

	//credits button
	vgui::Button* CreditButton = new Button(this, "CreditButton", CreditsButtonKv->GetString("Text", "Button"));
	CreditButton->SetBounds(CreditsButtonKv->GetInt("XPos"), CreditsButtonKv->GetInt("YPos"), CreditsButtonKv->GetInt("Wide"), CreditsButtonKv->GetInt("Tall"));
	CreditButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_CREDITS_PANEL);

	//weather button
	//vgui::Button* WeatherButton = new Button(this, "WeatherButton", WeatherButtonKv->GetString("Text", "Button"));
	//WeatherButton->SetBounds(WeatherButtonKv->GetInt("XPos"), WeatherButtonKv->GetInt("YPos"), WeatherButtonKv->GetInt("Wide"), WeatherButtonKv->GetInt("Tall"));
	//WeatherButton->SetCommand(AMOD_OPTIONS_PANEL_COMMAND_WEATHER_PANEL);

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

#if AMOD_DAYTIME_EDITION
	//disable these if daytime edition
	FilterOnBrightness->SetEnabled(false);
	FilterOnExps->SetEnabled(false);
	FilterOffBrightness->SetEnabled(false);
	m_cbDayAfterNP->SetEnabled(false);
	m_cbDayAfterRav->SetEnabled(false);
#endif

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
	//weatherpanel->Close();
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
		m_NoSoundscapesCheckButton->SetEnabled(!g_NeedSoundscapeEnable);
	}
}

//these are the current values of the filter. When changing the filter settings it will check
//to see if the filter is on or off, Then set the monitor gamma to the on value or off value.
static bool gs_FilterIsOn = true;

//sets if the filter is on or off
CON_COMMAND_F(Amod_FilterSet, "", FCVAR_HIDDEN)
{
	gs_FilterIsOn = atoi(args.Arg(1));

	engine->ClientCmd(CFmtStr("gamepadui_filterset %d", gs_FilterIsOn));
}

//------------------------------------------------------------------------------------------
// Purpose: Initalizes all the panel items
//------------------------------------------------------------------------------------------
void COptionsPanel::OnCommand(const char* pcCommand)
{
	//not sure why this is needed but if i dont want to remove it
	static bool s_bWasSoundscapesChecked = m_NoSoundscapesCheckButton->IsSelected();

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
			{"sv_footsteps",							!m_DisableFootstepsCheckButton->IsSelected(),									nullptr},
			{"hidehud",									m_NoHudCheckButton->IsSelected() ? 8 : 0,										nullptr},
			{"amod_mirrored",							m_MirroredCheckButton->IsSelected(),											nullptr},
			{"amod_vignette",							m_VignetteCheckButton->IsSelected(),											nullptr},
			{"amod_viewbob_enabled",					m_ViewBobCheckButton->IsSelected(),												nullptr},
			{"amod_standbob_enabled",					m_StandBobCheckButton->IsSelected(),											nullptr},
			{"amod_jump_punch_enable",					m_JumpViewpunchCheckButton->IsSelected(),										nullptr},
			{"amod_land_punch_enable",					m_LandViewpunchCheckButton->IsSelected(),										nullptr},
			{"sv_rollangle",							m_RollAngleCheckButton->IsSelected() ? m_RollAngleSlider->GetValue() : 0,		nullptr},

			//effects and filter
			{"amod_filter_brightness_on",				m_FilterOnBrightnessSlider->GetValue(),											nullptr},
			{"amod_filter_brightness_on_exp",			m_FilterOnExponentSlider->GetValue(),											nullptr},
			{"amod_filter_brightness_off",				m_FilterOffBrightnessSlider->GetValue(),										nullptr},
			{"amod_epic_filter",						m_EpicFilterCheckButton->IsSelected(),											nullptr},		//12th index

			//flashlight settings
			{"r_flashlightfar",							m_FlashlightFarComboBox->GetActiveItem(),										&m_FlashlightFarData},
			{"r_flashlightfov",							m_FlashlightFovComboBox->GetActiveItem(),										&m_FlashlightFovData},
			{"amod_flashlightlag",						m_FlashlightLagCheckButton->IsSelected(),										nullptr},
			{"amod_flashlightflicker",					m_FlashlightFlickerCheckButton->IsSelected(),									nullptr},

			//other settings
			{"amod_enable_god",							m_EnableGodCheckButton->IsSelected(),											nullptr},
			{"amod_soundscapes_disable",				m_NoSoundscapesCheckButton->IsSelected(),										nullptr},
			{"amod_music_disable",						m_NoMusicCheckButton->IsSelected(),												nullptr},
			{"amod_songs_transition_through_levels",	m_MusicTransitionCheckButton->IsSelected(),										nullptr},
			{"amod_do_citadel_timer",					m_CitadelTimerCheckButton->IsSelected(),										nullptr},
			{"amod_do_core_timer",						m_CoreTimerCheckButton->IsSelected(),											nullptr},

			//ending
			{"amod_new_ending",							m_EndingComboBox->GetActiveItem(),												nullptr},

			//do this convar last because this can save/load the game
			{"amod_day",								m_DayTimeCB->IsSelected(),														nullptr}
		};

		//HACK: these require a hack to work
		if (amod_day.GetBool() ^ m_DayTimeCB->IsSelected() && m_DayTimeCB->IsSelected())
		{
			//if the daytime is enabled then make sure the user has the filter on. Without it the game looks to dark.
			m_EpicFilterCheckButton->SetSelected(true);

			//HACK set WriteConvars[12] (12 = amod_epic_filter index. Change this if you change the index)
			WriteConvars[12].Value = true;
		}

		//check the soundscapes to see if thet can be disabled or not
		if (s_bWasSoundscapesChecked ^ (m_NoSoundscapesCheckButton->IsSelected() && !g_NeedSoundscapeEnable))
			engine->ExecuteClientCmd("playsoundscape Nothing");

		s_bWasSoundscapesChecked = (m_NoSoundscapesCheckButton->IsSelected() && !g_NeedSoundscapeEnable);

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
		if (gs_FilterIsOn && !IsDaytimeEnabled())
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
	//else if (!Q_strcmp(pcCommand, AMOD_OPTIONS_PANEL_COMMAND_WEATHER_PANEL))
	//{
	//	//open the weather panel
	//	weatherpanel->Close();
	//	weatherpanel->Open(this);
	//}
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
	//weatherpanel->Close();

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
	void SetSkyboxButtonEnabled(bool enabled)
	{
		if (!m_OptionsPanel || !m_OptionsPanel->m_SkyboxButton)
			return;

		//set the button's enabled state
		m_OptionsPanel->m_SkyboxButton->SetEnabled(enabled);

		//close the skybox panel if needed
		if (!enabled)
			skyboxpanel->Close();
	}

	//activates this panel
	void Activate(void)
	{
		if (m_OptionsPanel)
			m_OptionsPanel->Activate();
	}
	
	//sets the epic filter button state
	void SetFilterButtonValue(bool state, bool setconvar = false)
	{
		if (m_OptionsPanel && m_OptionsPanel->m_EpicFilterCheckButton)
		{
			m_OptionsPanel->m_EpicFilterCheckButton->SetSelected(state);

			//set the convar value
			if (setconvar)
				ConVarRef("amod_epic_filter").SetValue(state);
		}
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

	//reset the m_SkyboxPanel
	void ResetPanel()
	{
		if (!m_SkyboxPanel)
			return;

		//get current stuff
		int x, y;
		m_SkyboxPanel->GetPos(x, y);

		vgui::Panel* currparent = m_SkyboxPanel->GetParent();

		//create new panel
		m_SkyboxPanel->DeletePanel();
		m_SkyboxPanel = new CAModSkyboxPanel(currparent);
		m_SkyboxPanel->SetPos(x, y);
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
	void Open()
	{
		//open the panel if it isnt already active
		if (!m_WeatherPanel)
			m_WeatherPanel = new CAModWeatherPanel();

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

//command to toggle the weather panel
CON_COMMAND_F(ToggleWeatherPanel, "Toggles The Alone Mod Options Panel", FCVAR_HIDDEN)
{
	//open the weather panel
	weatherpanel->Close();
	weatherpanel->Open();
};

//command to toggle the epic filter panel
CON_COMMAND_F(ToggleEpicFilter, "Toggles The Alone Mod Epic Filter", FCVAR_HIDDEN)
{
	//look for epic filter convar
	ConVar* amod_epic_filter = cvar->FindVar("amod_epic_filter");
	if (!amod_epic_filter)
		return;

	//set the value
	amod_epic_filter->SetValue(!amod_epic_filter->GetBool());

	//tell the options panel to set the button value
	g_OptionsPanel.SetFilterButtonValue(amod_epic_filter->GetBool());

	//write config
	Amod_WriteConfig();
};