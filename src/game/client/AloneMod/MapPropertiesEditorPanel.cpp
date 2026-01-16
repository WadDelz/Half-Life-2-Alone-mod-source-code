#include "cbase.h"
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui/IVGui.h>
#include <vgui/IPanel.h>
#include "AloneMod/Amod_SharedDefs.h"
#include <vgui_controls/PropertyDialog.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/AnimationController.h>
#include "geo guesser/GG_MiniMap.h"
#include "effects panel/EffectsPanel.h"
#include "IOptionsPanel.h"
#include "ColorPicker.h"
#include "filesystem.h"
#include "ienginevgui.h"
#include "fmtstr.h"

using namespace vgui;

//------------------------------------------------------------------------------------------
// Purpose: Returns the proportionate scaled value
//------------------------------------------------------------------------------------------
int GetProportionateScaledValue(int value, int currentDimension, int baseDimension)
{
	if (baseDimension <= 0)
		return value;

	double scale = (double)currentDimension / (double)baseDimension;
	return (int)((double)value * scale + 0.5); // round to nearest int
}

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the fog info array
//----------------------------------------------------------------------------------------------------
static const char* FindFogInfoFromArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* def = "")
{
	for (int i = 0; i < info.Count(); i++)
	{
		if (!Q_stricmp(StringFromMapTimeStringTableIndex(info[i].convar), key))
			return StringFromMapTimeStringTableIndex(info[i].value);
	}

	return def;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the sun info array
//----------------------------------------------------------------------------------------------------
static const char* FindSunInfoFromArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* def = "")
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
static void AddOrUpdateSunInfoInArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* value)
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


//----------------------------------------------------------------------------------------------------
// Purpose: finds the map path from the input (looks inside directories)
//----------------------------------------------------------------------------------------------------
bool FindMapPath(const char* base, const char* find, char* output, int outputsize)
{
	//get the correct path
	FileFindHandle_t handle;
	const char* curr = filesystem->FindFirst(CFmtStr("%s/*", base), &handle);

	while (curr)
	{
		//dont do . or ..
		if (!Q_stricmp(curr, ".") || !Q_stricmp(curr, ".."))
		{
			curr = filesystem->FindNext(handle);
			continue;
		}

		//check for folder
		if (filesystem->FindIsDirectory(handle))
		{
			if (FindMapPath(CFmtStr("%s/%s", base, curr), find, output, outputsize))
			{
				filesystem->FindClose(handle);
				return true;
			}
		}
		else
		{
			//look for the file
			char bsp[512];
			Q_snprintf(bsp, sizeof(bsp), "%s.bsp", find);
			if (!Q_stricmp(curr, bsp))
			{
				filesystem->FindClose(handle);
				Q_strncpy(output, CFmtStr("%s/%s", base, find), outputsize);
				return true;
			}
		}

		curr = filesystem->FindNext(handle);
	}

	//close the handle
	filesystem->FindClose(handle);
	return false;
}









//right click button
class CMapPropertiesPanelButton : public Button
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelButton, Button);
public:
	CMapPropertiesPanelButton(Panel* parent, const char* name, const char* text) : BaseClass(parent, name, text) {}

	virtual void OnMouseReleased(MouseCode code) 
	{ 
		if (code == MouseCode::MOUSE_RIGHT) 
			PostActionSignal(new KeyValues("ChildRightClicked"));

		BaseClass::OnMouseReleased(code);
	}
};

//right click slider
class CMapPropertiesPanelSlider : public WheelSlider
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSlider, WheelSlider);
public:
	CMapPropertiesPanelSlider(Panel* parent, const char* name, int wheeldelta = 1) : BaseClass(parent, name, wheeldelta) {}

	virtual void OnMouseReleased(MouseCode code) 
	{ 
		if (code == MouseCode::MOUSE_RIGHT) 
		{
			PostActionSignal(new KeyValues("ChildRightClicked"));
			return;
		}

		BaseClass::OnMouseReleased(code);
	}

	virtual int GetMin()
	{
		int min, _;
		GetRange(min, _);
		return min;
	}

	virtual int GetMax()
	{
		int _, max;
		GetRange(_, max);
		return max;
	}
	
	virtual void OnMousePressed(MouseCode code) 
	{ 
		if (code != MouseCode::MOUSE_RIGHT) 
			BaseClass::OnMousePressed(code);
	}
};









//map properties panel
class CMapPropertiesPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanel, Frame)
public:
	CMapPropertiesPanel(Panel* parent);
	~CMapPropertiesPanel();

	//paint/tick
	void Paint();
	void OnThink();
	void OnClose();
	void OnCommand(const char* pszCommand);
	
	//loads the settings into the panel
	void PerformLayout();
	void ApplySettingsToPanel(KeyValues* subkey, Panel* panel);

	//keyboard/mouse
	void OnKeyCodePressed(KeyCode code);
	void OnKeyCodeReleased(KeyCode code);
	void OnMousePressed(KeyCode code);

	//called when the color picker picks a color
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PARAMS(OnSliderMoved, "SliderMoved", data);

	//init funcs
	void Init(MapTimeInfo_t& info, bool IsNightPage);
	void GetData(MapTimeInfo_t& info);
	void FormatImage(const char* input, char* output, int outside);
private:
	//original panel size for scaling
	int m_OriginalWidth, m_OriginalHeight;

	//keyvalues file
	KeyValues* m_KeyValuesFile = nullptr;

	//current color selector mode
	enum ColorSelectorMode {Color_Fog, Color_SkyboxFog, Color_Clouds, Color_Sun} m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;

	//list of sliders + dividers
	CUtlVector<Label*> m_Labels;
	CUtlVector<Divider*> m_Dividers;

	//fog settings
private:
	//override fog
	CheckButton* m_OverrideFogButton;

	//pixel fog
	CheckButton* m_EnablePixelFogButton;

	//enable fog
	CheckButton* m_EnableFogCheckButton;
	CheckButton* m_OverrideEnableFogCheckButton;

	//enable skybox fog
	CheckButton* m_EnableSkyboxFogCheckButton;
	CheckButton* m_OverrideEnableSkyboxFogCheckButton;

	//fog color
	CMapPropertiesPanelButton* m_FogColorButton;
	CheckButton* m_FogColorOverride;
	Color m_FogColor;
	Rect_t m_FogColorDrawRect;

	//fog skybox color
	CMapPropertiesPanelButton* m_FogSkyboxColorButton;
	CheckButton* m_FogSkyboxColorOverride;
	Color m_FogSkyboxColor;
	Rect_t m_FogSkyboxColorDrawRect;

	//fog start
	CMapPropertiesPanelSlider* m_FogStartSlider;
	Label* m_FogStartValueLabel;
	CheckButton* m_FogStartOverride;

	//fog end
	CMapPropertiesPanelSlider* m_FogEndSlider;
	Label* m_FogEndValueLabel;
	CheckButton* m_FogEndOverride;

	//fog start skybox
	CMapPropertiesPanelSlider* m_FogStartSkyboxSlider;
	Label* m_FogStartSkyboxValueLabel;
	CheckButton* m_FogStartSkyboxOverride;

	//fog end skybox
	CMapPropertiesPanelSlider* m_FogEndSkyboxSlider;
	Label* m_FogEndSkyboxValueLabel;
	CheckButton* m_FogEndSkyboxOverride;

	//fog density
	CMapPropertiesPanelSlider* m_FogDensitySlider;
	Label* m_FogDensityValueLabel;
	CheckButton* m_FogDensityOverride;

	//fog skybox density
	CMapPropertiesPanelSlider* m_FogSkyboxDensitySlider;
	Label* m_FogSkyboxDensityValueLabel;
	CheckButton* m_FogSkyboxDensityOverride;

	//skybox
private:
	//skybox background brush
	ImagePanel* m_SkyboxBackground;

	//skybox foreground brush
	CStretchingImage* m_SkyboxForeground;

	//skybox text entries
	ComboBox* m_SkyboxNames;

	//post processing filter
private:
	//filter list
	ComboBox* m_FilterComboBox;

	//filter intensity text
	Label* m_FilterIntensityText;

	//filter intensity slider
	CMapPropertiesPanelSlider* m_FilterIntensitySlider;

	//clouds
private:
	//clouds colors buttons
	CMapPropertiesPanelButton* m_CloudButton;
	
	//clouds colors
	Color m_CloudColor;
	Rect_t m_CloudColorRect;

	//bloom
private:
	//enable bloom
	CheckButton* m_EnableBloomCheckButton;

	//bloom slider
	CMapPropertiesPanelSlider* m_BloomScaleSlider;
	Label* m_BloomScaleText;

	//bloom scalar slider
	CMapPropertiesPanelSlider* m_BloomScalarSlider;
	Label* m_BloomScalarText;

	//sun
private:
	//enable sun
	CheckButton* m_EnableSunButton;

	//sun pitch
	Slider* m_SunPitchSlider;
	Label* m_SunPitchLabel;

	//sun yaw
	Slider* m_SunYawSlider;
	Label* m_SunYawLabel;

	//sun size
	Slider* m_SunSizeSlider;
	Label* m_SunSizeLabel;

	//sun color
	CMapPropertiesPanelButton* m_SunColorButton;
	Color m_SunColor;
	Rect_t m_SunColorDrawRect;

	//sun material text entry
	TextEntry* m_SunMaterialTextEntry;

	//set the pitch to the players eyes pitch button
	CMapPropertiesPanelButton* m_PitchToEyesButton;

	//apply button
private:
	CMapPropertiesPanelButton* m_ApplyButton;

	//other vars
private:
	//day or night?
	bool m_bNightTimeMode;

	//previous convar values to re-set when we close
	char m_PreviousFilterConvarValue[256];
	char m_PreviousFilterIntensityConvarValue[16];
	char m_PreviousCloudsColorValue[16];
	char m_PreviousCloudsShowValue[16];
	char m_PreviousCloudsOverrideValue[4];
	char m_PreviousGodConvarValue[4];
};

//static map properties panel
static CMapPropertiesPanel* g_MapPropertiesPanel;


#define COMMAND_CHANGE_FOG_COLOR "SetFogColor"
#define COMMAND_CHANGE_FOG_SKYBOX_COLOR "SetFogSkyboxColor"
#define COMMAND_CHANGE_CLOUDS_COLOR "ChangeCloudColors"
#define COMMAND_CHANGE_SUN_COLOR "ChangeSunColors"
#define COMMAND_APPLY_PAGE_SETTINGS "ApplySettings"

//sun commands
#define COMMAND_SUN_ACTIVATE "ActivateSun"
#define COMMAND_PITCH_TO_EYES "PitchToEyes"

//divisor for bloom scalar
#define BLOOM_SCALAR_DIVISOR 100

extern ConVar cl_mouselook;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties page
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanel::CMapPropertiesPanel(Panel* parent) : BaseClass(parent, "MapPropertiesPanel")
{
	//clear the previous convar values
	memset(m_PreviousCloudsOverrideValue, 0, sizeof(m_PreviousCloudsOverrideValue));
	memset(m_PreviousCloudsColorValue, 0, sizeof(m_PreviousCloudsColorValue));
	memset(m_PreviousCloudsShowValue, 0, sizeof(m_PreviousCloudsShowValue));
	memset(m_PreviousFilterConvarValue, 0, sizeof(m_PreviousFilterConvarValue));
	memset(m_PreviousFilterIntensityConvarValue, 0, sizeof(m_PreviousFilterIntensityConvarValue));
	memset(m_PreviousGodConvarValue, 0, sizeof(m_PreviousGodConvarValue));

	//set our settings
	g_MapPropertiesPanel = this;
	m_bNightTimeMode = true;

	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(true);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	Activate();

	//init our keyvalues and attempt to load our file
	m_KeyValuesFile = new KeyValues("PropertiesPanelDialog");
	if (!m_KeyValuesFile->LoadFromFile(filesystem, "resource/panels/MapPropertiesEditor.txt", "MOD"))
	{
		QueryBox* modal = new QueryBox("Error", "Failed to load: \"resource/panels/MapPropertiesEditor.txt\"!\nAborting panel load.", this);
		modal->MoveToCenterOfScreen();
		modal->Activate();
		modal->DoModal();
		modal->SetOKCommand(new KeyValues("Command", "command", "Close"));
		modal->SetCancelCommand(new KeyValues("Command", "command", "Close"));
		return;
	}

	//create our dividers
	KeyValues* dividers = m_KeyValuesFile->FindKey("Dividers");
	if (dividers)
	{
		FOR_EACH_TRUE_SUBKEY(dividers, divider)
		{
			//make the divider
			ApplySettingsToPanel(divider, (m_Dividers[m_Dividers.AddToTail(new Divider(this, divider->GetName()))]));
		}
	}

	//create our labels
	KeyValues* labels = m_KeyValuesFile->FindKey("Labels");
	if (labels)
	{
		FOR_EACH_TRUE_SUBKEY(labels, label)
		{
			//make the divider
			ApplySettingsToPanel(label, (m_Labels[m_Labels.AddToTail(new Label(this, label->GetName(), ""))]));
		}
	}

	//create our fog items
	{
		//override button
		m_OverrideFogButton = new CheckButton(this, "FogOverrideCheckButton", "");

		//enable pixel fog
		m_EnablePixelFogButton = new CheckButton(this, "EnablePixelFogButton", "");

		//enable fog
		m_EnableFogCheckButton = new CheckButton(this, "FogEnableCheckButton", "");
		m_OverrideEnableFogCheckButton = new CheckButton(this, "OverrideFogEnableCheckButton", "");

		//enable skybox fog
		m_EnableSkyboxFogCheckButton = new CheckButton(this, "FogEnableSkyboxCheckButton", "");
		m_OverrideEnableSkyboxFogCheckButton = new CheckButton(this, "OverrideFogEnableSkyboxCheckButton", "");

		//fog color
		m_FogColorButton = new CMapPropertiesPanelButton(this, "FogColorButton", "");
		m_FogColorOverride = new CheckButton(this, "FogColorOverrideButton", "");
		m_FogColorButton->SetCommand(COMMAND_CHANGE_FOG_COLOR);
		m_FogColor.SetColor(255, 255, 255, 255);

		//fog skybox color
		m_FogSkyboxColorButton = new CMapPropertiesPanelButton(this, "FogSkyboxColorButton", "");
		m_FogSkyboxColorOverride = new CheckButton(this, "FogSkyboxColorOverrideButton", "");
		m_FogSkyboxColorButton->SetCommand(COMMAND_CHANGE_FOG_SKYBOX_COLOR);
		m_FogSkyboxColor.SetColor(255, 255, 255, 255);

		//fog start
		m_FogStartSlider = new CMapPropertiesPanelSlider(this, "FogStartSlider", 150);
		m_FogStartValueLabel = new Label(this, "FogStartSlider", "");
		m_FogStartOverride = new CheckButton(this, "FogStartOverride", "");

		//fog end
		m_FogEndSlider = new CMapPropertiesPanelSlider(this, "FogEndSlider", 150);
		m_FogEndValueLabel = new Label(this, "FogEndSlider", "");
		m_FogEndOverride = new CheckButton(this, "FogEndOverride", "");

		//fog start skybox
		m_FogStartSkyboxSlider = new CMapPropertiesPanelSlider(this, "FogStartSkyboxSlider", 150);
		m_FogStartSkyboxValueLabel = new Label(this, "FogStartSkyboxSlider", "");
		m_FogStartSkyboxOverride = new CheckButton(this, "FogStartSkyboxOverride", "");

		//fog end skybox
		m_FogEndSkyboxSlider = new CMapPropertiesPanelSlider(this, "FogEndSkyboxSlider", 150);
		m_FogEndSkyboxValueLabel = new Label(this, "FogEndSkyboxSlider", "");
		m_FogEndSkyboxOverride = new CheckButton(this, "FogEndSkyboxOverride", "");

		//fog density
		m_FogDensitySlider = new CMapPropertiesPanelSlider(this, "FogDensitySlider");
		m_FogDensityValueLabel = new Label(this, "FogDensitySlider", "");
		m_FogDensityOverride = new CheckButton(this, "FogDensityOverride", "");

		//fog density skybox
		m_FogSkyboxDensitySlider = new CMapPropertiesPanelSlider(this, "FogSkyboxDensitySlider");
		m_FogSkyboxDensityValueLabel = new Label(this, "FogSkyboxDensitySlider", "");
		m_FogSkyboxDensityOverride = new CheckButton(this, "FogSkyboxDensityOverride", "");
	}

	//skybox
	{
		//make the background brush
		m_SkyboxBackground = new ImagePanel(this, "SkyboxBackground");

		//foreground brush
		m_SkyboxForeground = new CStretchingImage(this, "SkyboxForeground");

		//skies combo box
		m_SkyboxNames = new ComboBox(this, "SkyboxComboBox", 14, false);
	}

	//post processing
	{
		//intensity combo box
		m_FilterComboBox = new ComboBox(this, "FilterComboBox", 14, false);

		//intensity text
		m_FilterIntensityText = new Label(this, "FilterIntensitySlider", "");

		//intensity slider
		m_FilterIntensitySlider = new CMapPropertiesPanelSlider(this, "FilterIntensitySlider");
	}

	//cloud settings
	{
		m_CloudButton = new CMapPropertiesPanelButton(this, "CloudButton", "");
		m_CloudButton->SetCommand(COMMAND_CHANGE_CLOUDS_COLOR);
		
		m_CloudColor.SetColor(255, 255, 255, 255);
	}

	//bloom
	{
		//enable check button
		m_EnableBloomCheckButton = new CheckButton(this, "EnableBloom", "Enable Bloom");
		m_EnableBloomCheckButton->SetBounds(245, 488, 400, 18);

		//bloom amount slider
		m_BloomScaleSlider = new CMapPropertiesPanelSlider(this, "BloomScaleSlider");

		//bloom scale amount text
		m_BloomScaleText = new Label(this, "BloomScaleText", "");

		//bloom scalar slider
		m_BloomScalarSlider = new CMapPropertiesPanelSlider(this, "BloomSlider");

		//bloom scalar amount text
		m_BloomScalarText = new Label(this, "BloomScalarText", "");
	}

	//sun
	{
		//enable sun check button
		m_EnableSunButton = new CheckButton(this, "EnableSunButton", "");
		m_EnableSunButton->SetCommand(COMMAND_SUN_ACTIVATE);

		//sun pitch
		m_SunPitchSlider = new CMapPropertiesPanelSlider(this, "SunPitchSlider", 1);
		m_SunPitchLabel = new Label(this, "SunPitchLabel", "");

		//sun yaw
		m_SunYawSlider = new CMapPropertiesPanelSlider(this, "SunYawSlider", 1);
		m_SunYawLabel = new Label(this, "SunYawLabel", "");

		//sun size
		m_SunSizeSlider = new CMapPropertiesPanelSlider(this, "SunSizeSlider", 1);
		m_SunSizeLabel = new Label(this, "SunSizeLabel", "");

		//sun color
		m_SunColorButton = new CMapPropertiesPanelButton(this, "SunColorButton", "");
		m_SunColorButton->SetCommand(COMMAND_CHANGE_SUN_COLOR);
		m_SunColor.SetColor(255, 255, 255, 255);

		//sun material text entry
		m_SunMaterialTextEntry = new TextEntry(this, "MaterialTextEntry");
		m_SunMaterialTextEntry->AddActionSignalTarget(this);
		m_SunMaterialTextEntry->SetMaximumCharCount(255);

		//pitch to eyes button
		m_PitchToEyesButton = new CMapPropertiesPanelButton(this, "PitchToEyesButton", "");
		m_PitchToEyesButton->SetCommand(COMMAND_PITCH_TO_EYES);
	}

	//apply button
	m_ApplyButton = new CMapPropertiesPanelButton(this, "PropertiesButton", "");
	m_ApplyButton->SetCommand(COMMAND_APPLY_PAGE_SETTINGS);

	//set our size + pos then perform the panel layout
	SetBounds(m_KeyValuesFile->GetInt("x"), m_KeyValuesFile->GetInt("y"), m_OriginalWidth = m_KeyValuesFile->GetInt("w"), m_OriginalHeight = m_KeyValuesFile->GetInt("h"));
	SetMinimumSize(m_KeyValuesFile->GetInt("minwide"), m_KeyValuesFile->GetInt("mintall"));
	PerformLayout();

	//disable mouselook so keyboardlook can be used
	cl_mouselook.SetValue(false);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs the layout for our panel
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//set our divider + label positions
	KeyValues* dividers = m_KeyValuesFile->FindKey("Dividers");
	for (int i = 0; i < m_Dividers.Count(); i++)
	{
		ApplySettingsToPanel(dividers->FindKey(m_Dividers[i]->GetName()), m_Dividers[i]);
	}
	
	KeyValues* labels = m_KeyValuesFile->FindKey("Labels");
	for (int i = 0; i < m_Labels.Count(); i++)
	{
		ApplySettingsToPanel(labels->FindKey(m_Labels[i]->GetName()), m_Labels[i]);
	}

	//set our fog stuff
	{
		//override button
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogOverrideCheckButton"), m_OverrideFogButton);

		//enable pixel fog
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("EnablePixelFogButton"), m_EnablePixelFogButton);

		//enable fog
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEnableCheckButton"), m_EnableFogCheckButton);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("OverrideFogEnableCheckButton"), m_OverrideEnableFogCheckButton);

		//enable skybox fog
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEnableSkyboxCheckButton"), m_EnableSkyboxFogCheckButton);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("OverrideFogEnableSkyboxCheckButton"), m_OverrideEnableSkyboxFogCheckButton);

		//fog color
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogColorButton"), m_FogColorButton);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogColorOverrideButton"), m_FogColorOverride);

		//get the proportionate value for the fog color draw rect
		sscanf(m_KeyValuesFile->GetString("FogColorRect"), "%d %d %d %d", &m_FogColorDrawRect.x, &m_FogColorDrawRect.y, &m_FogColorDrawRect.width, &m_FogColorDrawRect.height);
		{
			m_FogColorDrawRect.x = GetProportionateScaledValue(m_FogColorDrawRect.x, GetWide(), m_OriginalWidth);
			m_FogColorDrawRect.y = GetProportionateScaledValue(m_FogColorDrawRect.y, GetTall(), m_OriginalHeight);
			m_FogColorDrawRect.width = GetProportionateScaledValue(m_FogColorDrawRect.width, GetWide(), m_OriginalWidth);
			m_FogColorDrawRect.height = GetProportionateScaledValue(m_FogColorDrawRect.height, GetTall(), m_OriginalHeight);
		}

		//fog skybox color
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxColorButton"), m_FogSkyboxColorButton);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxColorOverrideButton"), m_FogSkyboxColorOverride);

		//get the proportionate value for the skybox fog color draw rect
		sscanf(m_KeyValuesFile->GetString("FogSkyboxColorRect"), "%d %d %d %d", &m_FogSkyboxColorDrawRect.x, &m_FogSkyboxColorDrawRect.y, &m_FogSkyboxColorDrawRect.width, &m_FogSkyboxColorDrawRect.height);
		{
			m_FogSkyboxColorDrawRect.x = GetProportionateScaledValue(m_FogSkyboxColorDrawRect.x, GetWide(), m_OriginalWidth);
			m_FogSkyboxColorDrawRect.y = GetProportionateScaledValue(m_FogSkyboxColorDrawRect.y, GetTall(), m_OriginalHeight);
			m_FogSkyboxColorDrawRect.width = GetProportionateScaledValue(m_FogSkyboxColorDrawRect.width, GetWide(), m_OriginalWidth);
			m_FogSkyboxColorDrawRect.height = GetProportionateScaledValue(m_FogSkyboxColorDrawRect.height, GetTall(), m_OriginalHeight);
		}

		//fog start
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartSlider"), m_FogStartSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartValueLabel"), m_FogStartValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartOverride"), m_FogStartOverride);

		//fog end
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndSlider"), m_FogEndSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndValueLabel"), m_FogEndValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndOverride"), m_FogEndOverride);

		//fog start skybox
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartSkyboxSlider"), m_FogStartSkyboxSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartSkyboxValueLabel"), m_FogStartSkyboxValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogStartSkyboxOverride"), m_FogStartSkyboxOverride);

		//fog end skybox
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndSkyboxSlider"), m_FogEndSkyboxSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndSkyboxValueLabel"), m_FogEndSkyboxValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogEndSkyboxOverride"), m_FogEndSkyboxOverride);

		//fog density
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogDensitySlider"), m_FogDensitySlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogDensityValueLabel"), m_FogDensityValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogDensityOverride"), m_FogDensityOverride);

		//fog density skybox
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxDensitySlider"), m_FogSkyboxDensitySlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxDensityValueLabel"), m_FogSkyboxDensityValueLabel);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxDensityOverride"), m_FogSkyboxDensityOverride);
	}

	//set the skybox panel
	{
		//skybox background
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxBackground"), m_SkyboxBackground);

		//skybox foreground
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxForeground"), m_SkyboxForeground);

		//skybox combo box
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxNames"), m_SkyboxNames);
	}

	//post processing
	{
		//skybox background
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensityComboBox"), m_FilterComboBox);

		//skybox foreground
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensityText"), m_FilterIntensityText);

		//skybox combo box
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensitySlider"), m_FilterIntensitySlider);
	}

	//cloud
	{
		//cloud button
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("CloudButton"), m_CloudButton);

		//get the proportionate value for the cloud fog color draw rect
		sscanf(m_KeyValuesFile->GetString("CloudColorRect"), "%d %d %d %d", &m_CloudColorRect.x, &m_CloudColorRect.y, &m_CloudColorRect.width, &m_CloudColorRect.height);
		{
			m_CloudColorRect.x = GetProportionateScaledValue(m_CloudColorRect.x, GetWide(), m_OriginalWidth);
			m_CloudColorRect.y = GetProportionateScaledValue(m_CloudColorRect.y, GetTall(), m_OriginalHeight);
			m_CloudColorRect.width = GetProportionateScaledValue(m_CloudColorRect.width, GetWide(), m_OriginalWidth);
			m_CloudColorRect.height = GetProportionateScaledValue(m_CloudColorRect.height, GetTall(), m_OriginalHeight);
		}
	}

	//bloom
	{

		//bloom enabled
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomEnabledButton"), m_EnableBloomCheckButton);

		//bloom scale
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScaleSlider"), m_BloomScaleSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScaleText"), m_BloomScaleText);

		//bloom scalar
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScalarSlider"), m_BloomScalarSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScalarText"), m_BloomScalarText);
	}

	//sun
	{
		//sun enabled
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunEnabledCheckButton"), m_EnableSunButton);

		//sun pitch
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunPitchSlider"), m_SunPitchSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunPitchText"), m_SunPitchLabel);

		//sun yaw
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunYawSlider"), m_SunYawSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunYawText"), m_SunYawLabel);

		//pitch to eyes button
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("PitchToEyeAngleButton"), m_PitchToEyesButton);

		//sun size
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunSizeSlider"), m_SunSizeSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunSizeText"), m_SunSizeLabel);

		//sun color
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunColorButton"), m_SunColorButton);

		//sun material
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunMaterialTextEntry"), m_SunMaterialTextEntry);

		//get the proportionate value for the cloud fog color draw rect
		sscanf(m_KeyValuesFile->GetString("SunColorRect"), "%d %d %d %d", &m_SunColorDrawRect.x, &m_SunColorDrawRect.y, &m_SunColorDrawRect.width, &m_SunColorDrawRect.height);
		{
			m_SunColorDrawRect.x = GetProportionateScaledValue(m_SunColorDrawRect.x, GetWide(), m_OriginalWidth);
			m_SunColorDrawRect.y = GetProportionateScaledValue(m_SunColorDrawRect.y, GetTall(), m_OriginalHeight);
			m_SunColorDrawRect.width = GetProportionateScaledValue(m_SunColorDrawRect.width, GetWide(), m_OriginalWidth);
			m_SunColorDrawRect.height = GetProportionateScaledValue(m_SunColorDrawRect.height, GetTall(), m_OriginalHeight);
		}
	}

	//apply button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("ApplyButton"), m_ApplyButton);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Applies settings to the Panel* from the keyvalues*
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::ApplySettingsToPanel(KeyValues* subkey, Panel* panel)
{
	//check for subkey and panel
	if (!subkey || !panel)
		return;

	//set bounds
	int x = GetProportionateScaledValue(subkey->GetInt("x"), GetWide(), m_OriginalWidth);
	int y = GetProportionateScaledValue(subkey->GetInt("y"), GetTall(), m_OriginalHeight);
	int w = GetProportionateScaledValue(subkey->GetInt("w"), GetWide(), m_OriginalWidth);
	int h = GetProportionateScaledValue(subkey->GetInt("h"), GetTall(), m_OriginalHeight);

	if (subkey->GetBool("squaresize"))
	{
		if (h < w)
			w = h;
		else
			h = w;
	}

	panel->SetBounds(x, y, w, h);


	//check for zpos
	KeyValues* zpos = subkey->FindKey("zpos");
	if (zpos)
		panel->SetZPos(zpos->GetInt());

	//check for slider
	Slider* slider = dynamic_cast<Slider*>(panel);
	if (slider)
	{
		slider->SetRange(subkey->GetInt("min"), subkey->GetInt("max"));
	}

	//check for label
	Label* label = dynamic_cast<Label*>(panel);
	if (label)
	{
		label->SetText(subkey->GetString("text"));
		label->SetContentAlignment((Label::Alignment)AnimationController::LookupAlignment(subkey->GetString("alignment", "a_west")));
	}

	//check for image
	ImagePanel* Image = dynamic_cast<ImagePanel*>(panel);
	if (Image)
	{
		int r, g, b, a;
		sscanf(subkey->GetString("fillcolor", "255 255 255 255"), "%d %d %d %d", &r, &g, &b, &a);
		Image->SetFillColor(Color(r, g, b, a));
	}
}


//----------------------------------------------------------------------------------------------------
// Purpose: Called when a keycode is pressed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnKeyCodePressed(KeyCode code)
{
	//allow the user to move around
	if (code == KeyCode::KEY_W)
		engine->ClientCmd("+forward");
	else if (code == KeyCode::KEY_A)
		engine->ClientCmd("+moveleft");
	else if (code == KeyCode::KEY_S)
		engine->ClientCmd("+back");
	else if (code == KeyCode::KEY_D)
		engine->ClientCmd("+moveright");
	
	//allow the user to look around
	else if (code == KeyCode::KEY_UP)
		engine->ClientCmd("+lookup");
	else if (code == KeyCode::KEY_LEFT)
		engine->ClientCmd("+left");
	else if (code == KeyCode::KEY_DOWN)
		engine->ClientCmd("+lookdown");
	else if (code == KeyCode::KEY_RIGHT)
		engine->ClientCmd("+right");

	//use
	else if (code == KeyCode::KEY_E)
		engine->ClientCmd("+use");

	//enable noclip
	else if (code == KeyCode::KEY_V)
		engine->ClientCmd("noclip");

	//show the flashlight
	else if (code == KeyCode::KEY_F)
		engine->ClientCmd("impulse 100");

	BaseClass::OnKeyCodePressed(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a keycode is released
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnKeyCodeReleased(KeyCode code)
{
	//allow the user to move around
	if (code == KeyCode::KEY_W)
		engine->ClientCmd("-forward");
	else if (code == KeyCode::KEY_A)
		engine->ClientCmd("-moveleft");
	else if (code == KeyCode::KEY_S)
		engine->ClientCmd("-back");
	else if (code == KeyCode::KEY_D)
		engine->ClientCmd("-moveright");
	
	//allow the user to look around
	else if (code == KeyCode::KEY_UP)
		engine->ClientCmd("-lookup");
	else if (code == KeyCode::KEY_LEFT)
		engine->ClientCmd("-left");
	else if (code == KeyCode::KEY_DOWN)
		engine->ClientCmd("-lookdown");
	else if (code == KeyCode::KEY_RIGHT)
		engine->ClientCmd("-right");

	//use
	else if (code == KeyCode::KEY_E)
		engine->ClientCmd("-use");

	BaseClass::OnKeyCodeReleased(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a mouse is pressed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnMousePressed(MouseCode code)
{
	//regain focus by setting our focus to a button
	m_FogColorButton->RequestFocus();
	BaseClass::OnMousePressed(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties page.
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanel::~CMapPropertiesPanel()
{
	//delete our keyvalues
	if (m_KeyValuesFile)
		m_KeyValuesFile->deleteThis();
	m_KeyValuesFile = nullptr;

	//set the previous convar values
	if (m_PreviousCloudsOverrideValue[0]) ConVarRef("amod_clouds_color_override").SetValue(m_PreviousCloudsOverrideValue);
	if (m_PreviousCloudsColorValue[0]) ConVarRef("amod_clouds_color").SetValue(m_PreviousCloudsColorValue);
	if (m_PreviousCloudsShowValue[0]) ConVarRef("amod_clouds").SetValue(m_PreviousCloudsShowValue);
	if (m_PreviousGodConvarValue[0]) ConVarRef("amod_enable_god").SetValue(m_PreviousGodConvarValue);
	ConVarRef(m_bNightTimeMode ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename").SetValue(m_PreviousFilterConvarValue);
	ConVarRef(m_bNightTimeMode ? "amod_epic_filter_night_intensity" : "amod_epic_filter_day_intensity").SetValue(m_PreviousFilterIntensityConvarValue);

	void Amod_WriteConfig();
	Amod_WriteConfig();

	g_MapPropertiesPanel = nullptr;

	//enable mouselook so the mouse can be used
	cl_mouselook.SetValue(true);

	//stop all move commands
	engine->ClientCmd("-forward");
	engine->ClientCmd("-moveleft");
	engine->ClientCmd("-back");
	engine->ClientCmd("-moveright");
	engine->ClientCmd("-lookup");
	engine->ClientCmd("-left");
	engine->ClientCmd("-lookdown");
	engine->ClientCmd("-right");
	engine->ClientCmd("-use");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Paints the panel
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::Paint()
{
	//set our new bg color
	SetBgColor(Color(150, 150, 150, 150));
	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_FogColor);
	surface()->DrawFilledRect(m_FogColorDrawRect.x, m_FogColorDrawRect.y, m_FogColorDrawRect.x + m_FogColorDrawRect.width, m_FogColorDrawRect.y + m_FogColorDrawRect.height);

	//draw the colors
	surface()->DrawSetColor(m_FogSkyboxColor);
	surface()->DrawFilledRect(m_FogSkyboxColorDrawRect.x, m_FogSkyboxColorDrawRect.y, m_FogSkyboxColorDrawRect.x + m_FogSkyboxColorDrawRect.width, m_FogSkyboxColorDrawRect.y + m_FogSkyboxColorDrawRect.height);

	//draw the colors
	Color color = m_CloudColor;
	if (color.r() == 255 && color.g() == 255 && color.b() == 255 && color.a() == 255)
		color = Color(51, 103, 153, 255);

	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(m_CloudColorRect.x, m_CloudColorRect.y, m_CloudColorRect.x + m_CloudColorRect.width, m_CloudColorRect.y + m_CloudColorRect.height);

	//draw the colors
	surface()->DrawSetColor(m_SunColor);
	surface()->DrawFilledRect(m_SunColorDrawRect.x, m_SunColorDrawRect.y, m_SunColorDrawRect.x + m_SunColorDrawRect.width, m_SunColorDrawRect.y + m_SunColorDrawRect.height);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnThink()
{
	//set the fog
	{
		//get the convars
		static ConVar* r_pixelfog = cvar->FindVar("r_pixelfog");
		static ConVar* fog_override = cvar->FindVar("fog_override");
		static ConVar* fog_enable = cvar->FindVar("fog_enable");
		static ConVar* fog_enableskybox = cvar->FindVar("fog_enableskybox");
		static ConVar* fog_color = cvar->FindVar("fog_color");
		static ConVar* fog_colorskybox = cvar->FindVar("fog_colorskybox");
		static ConVar* fog_start = cvar->FindVar("fog_start");
		static ConVar* fog_end = cvar->FindVar("fog_end");
		static ConVar* fog_startskybox = cvar->FindVar("fog_startskybox");
		static ConVar* fog_endskybox = cvar->FindVar("fog_endskybox");
		static ConVar* fog_maxdensity = cvar->FindVar("fog_maxdensity");
		static ConVar* fog_maxdensityskybox = cvar->FindVar("fog_maxdensityskybox");

		//do we override
		bool _override = m_OverrideFogButton->IsSelected();

		//convars
		{
			//set the check button convar values
			fog_override->SetValue(_override);
			fog_enable->SetValue(!_override || !m_OverrideEnableFogCheckButton->IsSelected() ?					-1 :		m_EnableFogCheckButton->IsSelected());
			fog_enableskybox->SetValue(!_override || !m_OverrideEnableSkyboxFogCheckButton->IsSelected() ?		-1 :		m_EnableSkyboxFogCheckButton->IsSelected());
			r_pixelfog->SetValue(_override && !m_EnablePixelFogButton->IsSelected() ? false : true);

			//set the color convar values
			fog_color->SetValue(!_override || !m_FogColorOverride->IsSelected() ?						"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b()));
			fog_colorskybox->SetValue(!_override || !m_FogSkyboxColorOverride->IsSelected() ?			"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b()));

			//set the slider convar values
			fog_start->SetValue(!_override || !m_FogStartOverride->IsSelected() ?							-1 :			m_FogStartSlider->GetValue());
			fog_end->SetValue(!_override || !m_FogEndOverride->IsSelected() ?								-1 :			m_FogEndSlider->GetValue());
			fog_startskybox->SetValue(!_override || !m_FogStartSkyboxOverride->IsSelected() ?				-1 :			m_FogStartSkyboxSlider->GetValue());
			fog_endskybox->SetValue(!_override || !m_FogEndSkyboxOverride->IsSelected() ?					-1 :			m_FogEndSkyboxSlider->GetValue());
			fog_maxdensity->SetValue(!_override || !m_FogDensityOverride->IsSelected() ?					-1 :			(float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax());
			fog_maxdensityskybox->SetValue(!_override || !m_FogSkyboxDensityOverride->IsSelected() ?		-1 :			(float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax());
		}

		//enable the items
		{
			//enable the check buttons
			m_EnablePixelFogButton->SetEnabled(_override);
			m_EnableFogCheckButton->SetEnabled(_override && m_OverrideEnableFogCheckButton->IsSelected());
			m_EnableSkyboxFogCheckButton->SetEnabled(_override && m_OverrideEnableSkyboxFogCheckButton->IsSelected());
			m_OverrideEnableFogCheckButton->SetEnabled(_override);
			m_OverrideEnableSkyboxFogCheckButton->SetEnabled(_override);

			//enable the color buttons
			m_FogColorButton->SetEnabled(_override && m_FogColorOverride->IsSelected());
			m_FogSkyboxColorButton->SetEnabled(_override && m_FogSkyboxColorOverride->IsSelected());
			m_FogSkyboxColorOverride->SetEnabled(_override);
			m_FogColorOverride->SetEnabled(_override);

			//enable the sliders
			m_FogStartSlider->SetEnabled(_override && m_FogStartOverride->IsSelected());
			m_FogEndSlider->SetEnabled(_override && m_FogEndOverride->IsSelected());
			m_FogStartSkyboxSlider->SetEnabled(_override && m_FogStartSkyboxOverride->IsSelected());
			m_FogEndSkyboxSlider->SetEnabled(_override && m_FogEndSkyboxOverride->IsSelected());
			m_FogDensitySlider->SetEnabled(_override && m_FogDensityOverride->IsSelected());
			m_FogSkyboxDensitySlider->SetEnabled(_override && m_FogSkyboxDensityOverride->IsSelected());

			//enable the slider override buttons
			m_FogStartOverride->SetEnabled(_override);
			m_FogEndOverride->SetEnabled(_override);
			m_FogStartSkyboxOverride->SetEnabled(_override);
			m_FogEndSkyboxOverride->SetEnabled(_override);
			m_FogDensityOverride->SetEnabled(_override);
			m_FogSkyboxDensityOverride->SetEnabled(_override);
		}

		//set the texts
		{
			m_FogStartValueLabel->SetText(CFmtStr("%d", fog_start->GetInt()));
			m_FogEndValueLabel->SetText(CFmtStr("%d", fog_end->GetInt()));
			m_FogStartSkyboxValueLabel->SetText(CFmtStr("%d", fog_startskybox->GetInt()));
			m_FogEndSkyboxValueLabel->SetText(CFmtStr("%d", fog_endskybox->GetInt()));
			m_FogDensityValueLabel->SetText(CFmtStr("%f", fog_maxdensity->GetFloat()));
			m_FogSkyboxDensityValueLabel->SetText(CFmtStr("%f", fog_maxdensityskybox->GetFloat()));
		}
	}

	//color correction
	{
		//set intensity text
		m_FilterIntensityText->SetText(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));

		//convars
		static ConVar* intensity_day = cvar->FindVar("amod_epic_filter_day_intensity");
		static ConVar* intensity_night = cvar->FindVar("amod_epic_filter_night_intensity");

		//set the convar value
		float value = (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax();
		if (m_bNightTimeMode)
			intensity_night->SetValue(value);
		else
			intensity_day->SetValue(value);
	}

	//clouds color
	{
		static ConVar* amod_clouds_color = cvar->FindVar("amod_clouds_color");

		//check if the clouds color already matches our color. If so then dont set the value
		const char* color = CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a());
		if (Q_stricmp(amod_clouds_color->GetString(), color))
			amod_clouds_color->SetValue(color);
	}

	//bloom
	{
		static ConVar* mat_force_bloom = cvar->FindVar("mat_force_bloom");
		static ConVar* mat_bloomscale = cvar->FindVar("mat_bloomscale");
		static ConVar* mat_bloom_scalefactor_scalar = cvar->FindVar("mat_bloom_scalefactor_scalar");

		//set the convar values
		mat_force_bloom->SetValue(m_EnableBloomCheckButton->IsSelected());
		mat_bloomscale->SetValue(m_BloomScaleSlider->GetValue());
		mat_bloom_scalefactor_scalar->SetValue((float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR);

		//set text
		m_BloomScaleText->SetText(CFmtStr("%d", mat_bloomscale->GetInt()));
		m_BloomScalarText->SetText(CFmtStr("%.3f", mat_bloom_scalefactor_scalar->GetFloat()));

		//set enabled state
		m_BloomScaleSlider->SetEnabled(m_EnableBloomCheckButton->IsSelected());
		m_BloomScalarSlider->SetEnabled(m_EnableBloomCheckButton->IsSelected());
	}

	//sun
	{
		m_SunPitchLabel->SetText(CFmtStr("%d", m_SunPitchSlider->GetValue()));
		m_SunYawLabel->SetText(CFmtStr("%d", m_SunYawSlider->GetValue()));
		m_SunSizeLabel->SetText(CFmtStr("%d", m_SunSizeSlider->GetValue()));

		//set enabled states
		m_SunPitchSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_SunYawSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_PitchToEyesButton->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_SunSizeSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_SunColorButton->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_SunMaterialTextEntry->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
		m_EnableSunButton->SetEnabled(!m_bNightTimeMode);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Helper
//----------------------------------------------------------------------------------------------------
float OppositeAngle(float angle)
{
	float opposite = angle + 180.0f;
	if (opposite >= 360.0f)
		opposite -= 360.0f;
	else if (opposite < 0.0f)
		opposite += 360.0f;
	return opposite;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnCommand(const char* pszCommand)
{
	//check for fog color command
	if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Fog Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogColor);
		m_ColorPicker->DoModal();
	
		m_ColorSelectorMode = ColorSelectorMode::Color_Fog;
	}
	
	//check for fog color skybox command
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_SKYBOX_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Skybox Fog Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogSkyboxColor);
		m_ColorPicker->DoModal();
	
		m_ColorSelectorMode = ColorSelectorMode::Color_SkyboxFog;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_CLOUDS_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Clouds Color (255 255 255 255 = default)", true);
		m_ColorPicker->SetUsesAlpha(true);
		m_ColorPicker->SetColor(m_CloudColor);
		m_ColorPicker->DoModal();
	
		m_ColorSelectorMode = ColorSelectorMode::Color_Clouds;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_SUN_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Sun Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_SunColor);
		m_ColorPicker->DoModal();
	
		m_ColorSelectorMode = ColorSelectorMode::Color_Sun;
	}

	//check for apply command
	else if (!Q_stricmp(pszCommand, COMMAND_APPLY_PAGE_SETTINGS))
	{
		//post message
		PostActionSignal(new KeyValues("ApplyPageSetting"));
		return;
	}

	//check for sun commands
	else if (!Q_stricmp(pszCommand, COMMAND_SUN_ACTIVATE))
	{
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun %d", m_EnableSunButton->IsSelected()));

		if (m_EnableSunButton->IsSelected())
		{
			//reset the keyvalues
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue pitch %d", m_SunPitchSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue angle %d", m_SunYawSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue size %d", m_SunSizeSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));

			//get the text
			char text[256];
			m_SunMaterialTextEntry->GetText(text, sizeof(text));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue material %s", text));
		}
		return;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_PITCH_TO_EYES))
	{
		//set the values of the sliders
		CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		//get the angle
		QAngle ang = pPlayer->GetAbsAngles();
		m_SunPitchSlider->SetValue(ang.x);
		m_SunYawSlider->SetValue(OppositeAngle(ang.y));			//these should change the values
		return;
	}

	if (!Q_stricmp(pszCommand, "SetModal"))
	{
		int x, y;
		GetPos(x, y);
		DoModal();
		SetPos(x, y);
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the panel closes
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnClose()
{
	//reload server settings
	engine->ClientCmd("_amod_day_do");

	PostActionSignal(new KeyValues("MapPropertiesPanelClosed"));
	BaseClass::OnClose();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a slider is moved
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnSliderMoved(KeyValues* data)
{
	//check for sun slider
	if (data->GetPtr("panel") == m_SunPitchSlider || data->GetPtr("panel") == m_SunYawSlider || data->GetPtr("panel") == m_SunSizeSlider)
	{
		//call _amod_mapedit_server_sun_keyvalue
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue pitch %d", m_SunPitchSlider->GetValue()));
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue angle %d", m_SunYawSlider->GetValue()));
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue size %d", m_SunSizeSlider->GetValue()));
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnTextChanged(KeyValues* data)
{
	if (data->GetPtr("panel") == m_SkyboxNames)
	{
		//HACK: if you use the arrow keys to navigate, the active item will NOT get set. Fix this
		char string[128];
		m_SkyboxNames->GetText(string, sizeof(string));

		//go through all the items
		for (int i = 0; i < m_SkyboxNames->GetItemCount(); i++)
		{
			//compare with that items string
			char temp[128];
			m_SkyboxNames->GetItemText(i, temp, sizeof(temp));
			if (!Q_stricmp(string, temp))
			{
				m_SkyboxNames->ActivateItem(i);
				break;
			}
		}

		//get the image
		char image[512];
		FormatImage(m_SkyboxNames->GetActiveItemUserData()->GetName(), image, sizeof(image));

		//set the image
		m_SkyboxForeground->SetImage(image);

		//set the sv_skyname convar
		ConVarRef("sv_skyname").SetValue(m_SkyboxNames->GetActiveItemUserData()->GetName());
	}

	//check for epic filter combo box
	else if (data->GetPtr("panel") == m_FilterComboBox)
	{
		//HACK: if you use the arrow keys to navigate, the active item will NOT get set. Fix this
		char string[128];
		m_FilterComboBox->GetText(string, sizeof(string));

		//go through all the items
		for (int i = 0; i < m_FilterComboBox->GetItemCount(); i++)
		{
			//compare with that items string
			char temp[128];
			m_FilterComboBox->GetItemText(i, temp, sizeof(temp));
			if (!Q_stricmp(string, temp))
			{
				m_FilterComboBox->ActivateItem(i);
				break;
			}
		}

		//set amod_filter_filename
		ConVarRef amod_filter_filename(m_bNightTimeMode ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename");
		amod_filter_filename.SetValue(m_FilterComboBox->GetActiveItemUserData()->GetName());
	}
	
	//check for sun material
	else if (data->GetPtr("panel") == m_SunMaterialTextEntry)
	{
		//get the text
		char text[256];
		m_SunMaterialTextEntry->GetText(text, sizeof(text));

		//check for the material first
		if (!CMaterialReference(text).IsValid() || CMaterialReference(text)->IsErrorMaterial())
		{
			//copy the default texture in
			Q_snprintf(text, sizeof(text), "sprites/light_glow02_add_noz.spr");
		}

		//send to server
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue material %s", text));
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnColorSelected(KeyValues* data)
{
	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));
	switch (m_ColorSelectorMode)
	{
	case ColorSelectorMode::Color_Fog:
		m_FogColor = color;
		return;
	case ColorSelectorMode::Color_SkyboxFog:
		m_FogSkyboxColor = color;
		return;
	case ColorSelectorMode::Color_Clouds:
		m_CloudColor = color;
		return;
	case ColorSelectorMode::Color_Sun:
		m_SunColor = color;
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b(), m_SunColor.a()));
		return;
	}
}
 
//----------------------------------------------------------------------------------------------------
// Purpose: Formats the image filepath
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::FormatImage(const char* input, char* output, int outsize)
{
	//failsafe
	Q_strncpy(output, CFmtStr("../skybox/%sup", input), outsize);

	//load the panels/sky path
	KeyValuesAD keyvalues(new KeyValues("SkyPanel"));
	if (!keyvalues->LoadFromFile(filesystem, "resource/panels/skypanel.txt"))
		return;

	KeyValues* timekey = keyvalues->FindKey(m_bNightTimeMode ? "Night" : "Day");
	if (!timekey)
		return;

	//go through each key
	FOR_EACH_VALUE(timekey, value)
	{
		//check for % to change the direction the skybox displays
		char temp[512];
		Q_strncpy(temp, value->GetString(), sizeof(temp));
		const char* current = temp;

		//look for percent
		char* percent = Q_strstr(temp, "%");
		if (percent)
		{
			*percent = '\0';
			percent++;
		}
		else
			percent = "up";

		//check
		if (!Q_stricmp(current, input))
		{
			Q_strncpy(output, CFmtStr("../skybox/%s%s", current, percent), outsize);
			return;
		}
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the data
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::Init(MapTimeInfo_t& info, bool IsNightPage)
{	
	//store the convars current values
	Q_strncpy(m_PreviousCloudsOverrideValue,ConVarRef("amod_clouds_color_override").GetString(), sizeof(m_PreviousCloudsOverrideValue));
	Q_strncpy(m_PreviousCloudsColorValue, ConVarRef("amod_clouds_color").GetString(), sizeof(m_PreviousCloudsColorValue));
	Q_strncpy(m_PreviousCloudsShowValue, ConVarRef("amod_clouds").GetString(), sizeof(m_PreviousCloudsShowValue));
	Q_strncpy(m_PreviousFilterConvarValue, ConVarRef(IsNightPage ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename").GetString(), sizeof(m_PreviousFilterConvarValue));
	Q_strncpy(m_PreviousFilterIntensityConvarValue, ConVarRef(IsNightPage ? "amod_epic_filter_night_intensity" : "amod_epic_filter_day_intensity").GetString(), sizeof(m_PreviousFilterIntensityConvarValue));
	Q_strncpy(m_PreviousGodConvarValue, ConVarRef("amod_enable_god").GetString(), sizeof(m_PreviousGodConvarValue));

	//set these convars ALWAYS
	engine->ClientCmd("amod_enable_god 1");
	engine->ClientCmd("amod_clouds 1");
	engine->ClientCmd("amod_clouds_color_override 1");

	//set m_bNightTimeMode
	m_bNightTimeMode = IsNightPage;

	//fog
	{
		//get the array
		CUtlVector<MapTimeInfo_t::FogInfo_t>& fog_array = IsNightPage ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;

		//init the fog check buttons
		{
			//get
			int enabled = atoi(FindFogInfoFromArray(fog_array, "fog_enable", "-1"));
			int enabled_skybox = atoi(FindFogInfoFromArray(fog_array, "fog_enableskybox", "-1"));

			//set
			m_OverrideFogButton->SetSelected(IsNightPage ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled);
			m_EnablePixelFogButton->SetSelected(atoi(FindFogInfoFromArray(fog_array, "r_pixelfog", "1")) != 0);

			m_OverrideEnableFogCheckButton->SetSelected(enabled >= 0);
			m_EnableFogCheckButton->SetSelected(enabled > 0);

			m_OverrideEnableSkyboxFogCheckButton->SetSelected(enabled_skybox >= 0);
			m_EnableSkyboxFogCheckButton->SetSelected(enabled_skybox > 0);
		}

		//init the fog sliders
		{
			//get
			int fog_start = atoi(FindFogInfoFromArray(fog_array, "fog_start", "256"));
			int fog_end = atoi(FindFogInfoFromArray(fog_array, "fog_end", "2000"));
			int fog_startskybox = atoi(FindFogInfoFromArray(fog_array, "fog_startskybox", "2000"));
			int fog_endskybox = atoi(FindFogInfoFromArray(fog_array, "fog_endskybox", "10000"));
			float fog_maxdensity = atof(FindFogInfoFromArray(fog_array, "fog_maxdensity", "1.0"));
			float fog_maxdensityskybox = atof(FindFogInfoFromArray(fog_array, "fog_maxdensityskybox", "1.0"));

			//set
			m_FogStartOverride->SetSelected(fog_start != -1);
			m_FogEndOverride->SetSelected(fog_end != -1);
			m_FogStartSkyboxOverride->SetSelected(fog_startskybox != -1);
			m_FogEndSkyboxOverride->SetSelected(fog_endskybox != -1);
			m_FogDensityOverride->SetSelected(fog_maxdensity >= 0);
			m_FogSkyboxDensityOverride->SetSelected(fog_maxdensityskybox >= 0);

			m_FogStartSlider->SetValue(fog_start);
			m_FogEndSlider->SetValue(fog_end);
			m_FogStartSkyboxSlider->SetValue(fog_startskybox);
			m_FogEndSkyboxSlider->SetValue(fog_endskybox);
			m_FogDensitySlider->SetValue((float)fog_maxdensity * m_FogDensitySlider->GetMax());
			m_FogSkyboxDensitySlider->SetValue((float)fog_maxdensityskybox * m_FogSkyboxDensitySlider->GetMax());
		}

		//init the fog color buttons
		{
			//get
			int r, g, b;
			int skyboxr, skyboxg, skyboxb;

			sscanf(FindFogInfoFromArray(fog_array, "fog_color", "255 255 255"), "%d %d %d", &r, &g, &b);
			sscanf(FindFogInfoFromArray(fog_array, "fog_colorskybox", "255 255 255"), "%d %d %d", &skyboxr, &skyboxg, &skyboxb);

			//set the fog color override check buttons
			m_FogColorOverride->SetSelected(r != -1 && g != -1 && b != -1);
			m_FogSkyboxColorOverride->SetSelected(skyboxr != -1 && skyboxg != -1 && skyboxb != -1);

			//set the fog color buttons
			m_FogColor.SetColor(r, g, b, 255);
			m_FogSkyboxColor.SetColor(skyboxr, skyboxg, skyboxb, 255);
		}
	}

	//skybox
	{
		const char* skybox = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.DefaultNightSky) : StringFromMapTimeStringTableIndex(info.DayInfo.DefaultDaySky);

		//get the image
		char image[512];
		FormatImage(skybox, image, sizeof(image));

		//set the image
		m_SkyboxForeground->SetImage(image);

		//set the skybox combo box
		m_SkyboxNames->RemoveAll();
		do
		{
			//load keyvalues file
			KeyValuesAD keyvalues(new KeyValues("SkyPanel"));
			if (!keyvalues->LoadFromFile(filesystem, "resource/panels/skypanel.txt"))
				break;

			KeyValues* timekey = keyvalues->FindKey(IsNightPage ? "Night" : "Day");
			if (!timekey)
				break;

			//go through each key
			int index = 0, current = 0;
			FOR_EACH_VALUE(timekey, value)
			{
				//remove percent sigh
				char temp[512];
				Q_strncpy(temp, value->GetString(), sizeof(temp));
				char* percent = Q_strstr(temp, "%");
				if (percent)
					*percent = '\0';

				//add the item
				m_SkyboxNames->AddItem(value->GetName(), new KeyValues(temp));

				//check skybox
				if (!Q_strnicmp(skybox, value->GetString(), Q_strlen(skybox)))
					index = current;

				current++;
			}

			//active the current index
			m_SkyboxNames->ActivateItem(index);

		} while (false);
	}

	//post processing
	{
		const char* filtername = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.FilterName) : StringFromMapTimeStringTableIndex(info.DayInfo.FilterName);

		//get the stuff
		int index = 0, current = 0;

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
			m_FilterComboBox->AddItem(first, new KeyValues(cc_string));

			//add to combo box
			if (!Q_stricmp(cc_string, filtername))
				index = current;

			first = filesystem->FindNext(find);

			current++;
		}

		//active the current index
		m_FilterComboBox->ActivateItem(index);

		filesystem->FindClose(find);

		//set the slider
		float filtervalue = atof(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.FilterIntensity : info.DayInfo.FilterIntensity));
		m_FilterIntensitySlider->SetValue(int(filtervalue * m_FilterIntensitySlider->GetMax()));
	}

	//clouds
	{
		int cloudcolor[4] = { 0,0,0,0 };
		UTIL_StringToIntArray(cloudcolor, 4, StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.CloudsColor : info.DayInfo.CloudsColor));
		m_CloudColor.SetColor(cloudcolor[0], cloudcolor[1], cloudcolor[2], cloudcolor[3]);
	}


	//bloom
	{
		m_EnableBloomCheckButton->SetSelected(atoi(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomEnabled : info.DayInfo.BloomEnabled)));
		m_BloomScaleSlider->SetValue(atoi(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomScale : info.DayInfo.BloomScale)));
		m_BloomScalarSlider->SetValue(atof(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomScalarFactor : info.DayInfo.BloomScalarFactor)) * BLOOM_SCALAR_DIVISOR);
	}

	//sun
	//disable or enable the sun
	if (!IsNightPage)
	{
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun %d", info.DayInfo.SunInfoEnabled));
		m_EnableSunButton->SetSelected(info.DayInfo.SunInfoEnabled);

		//pitch
		m_SunPitchSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "pitch", "90")));

		//check for an 'angle' key. If not found then we will have to use the yaw value of the 'angles' %d %d %d
		const char* angle = FindSunInfoFromArray(info.DayInfo.SunInfo, "angle", nullptr);
		if (angle)
		{
			m_SunPitchSlider->SetValue(atoi(angle));
		}
		else
		{
			angle = FindSunInfoFromArray(info.DayInfo.SunInfo, "angles", "0 0 0");

			//suck out the value
			int p, y, r;
			sscanf(angle, "%d %d %d", &p, &y, &r);

			m_SunYawSlider->SetValue(y);
		}

		//set the size
		m_SunSizeSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "size", "10")));

		//set color
		int r, g, b;
		sscanf(FindSunInfoFromArray(info.DayInfo.SunInfo, "rendercolor", "255 255 255"), "%d %d %d", &r, &g, &b);
		m_SunColor.SetColor(r, g, b, 255);

		//set the material
		m_SunMaterialTextEntry->SetText(FindSunInfoFromArray(info.DayInfo.SunInfo, "material", "sprites/light_glow02_add_noz.spr"));
	}
}


//----------------------------------------------------------------------------------------------------
// Purpose: Sets the data from the panel
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::GetData(MapTimeInfo_t& info)
{
	//set the fog info
	do
	{
		//get and always clear the fog info
		CUtlVector<MapTimeInfo_t::FogInfo_t>& fog_array = m_bNightTimeMode ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;
		fog_array.RemoveAll();

		//set the enabled state
		if (m_OverrideFogButton->IsSelected())
			(m_bNightTimeMode ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled) = true;
		else
			(m_bNightTimeMode ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled) = false;

		//if the fog isnt enabled then just return
		if (!m_OverrideFogButton->IsSelected())
			break;

		//set the info
		MapTimeInfo_t::FogInfo_t r_pixelfog{			StringToMapTimeStringTableIndex("r_pixelfog"),				StringToMapTimeStringTableIndex(CFmtStr("%d", m_OverrideFogButton->IsSelected()	? m_EnablePixelFogButton->IsSelected() : 1))};
		MapTimeInfo_t::FogInfo_t fog_override{			StringToMapTimeStringTableIndex("fog_override"),			StringToMapTimeStringTableIndex(CFmtStr("%d", m_OverrideFogButton->IsSelected())) };
		MapTimeInfo_t::FogInfo_t fog_enable{			StringToMapTimeStringTableIndex("fog_enable"),				StringToMapTimeStringTableIndex(!m_OverrideEnableFogCheckButton->IsSelected() ?			"-1" :			CFmtStr("%d", m_EnableFogCheckButton->IsSelected())) };
		MapTimeInfo_t::FogInfo_t fog_enableskybox{		StringToMapTimeStringTableIndex("fog_enableskybox"),		StringToMapTimeStringTableIndex(!m_OverrideEnableSkyboxFogCheckButton->IsSelected() ?	"-1" :			CFmtStr("%d", m_EnableSkyboxFogCheckButton->IsSelected())) };
		MapTimeInfo_t::FogInfo_t fog_start{				StringToMapTimeStringTableIndex("fog_start"),				StringToMapTimeStringTableIndex(!m_FogStartOverride->IsSelected() ?						"-1" :			CFmtStr("%d", m_FogStartSlider->GetValue())) };
		MapTimeInfo_t::FogInfo_t fog_end{				StringToMapTimeStringTableIndex("fog_end"),					StringToMapTimeStringTableIndex(!m_FogEndOverride->IsSelected() ?						"-1" :			CFmtStr("%d", m_FogEndSlider->GetValue())) };
		MapTimeInfo_t::FogInfo_t fog_startskybox{		StringToMapTimeStringTableIndex("fog_startskybox"),			StringToMapTimeStringTableIndex(!m_FogStartSkyboxOverride->IsSelected() ?				"-1" :			CFmtStr("%d", m_FogStartSkyboxSlider->GetValue())) };
		MapTimeInfo_t::FogInfo_t fog_endskybox{			StringToMapTimeStringTableIndex("fog_endskybox"),			StringToMapTimeStringTableIndex(!m_FogEndSkyboxOverride->IsSelected() ?					"-1" :			CFmtStr("%d", m_FogEndSkyboxSlider->GetValue())) };
		MapTimeInfo_t::FogInfo_t fog_maxdensity{		StringToMapTimeStringTableIndex("fog_maxdensity"),			StringToMapTimeStringTableIndex(!m_FogDensityOverride->IsSelected() ?					"-1" :			CFmtStr("%.2f", (float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax())) };
		MapTimeInfo_t::FogInfo_t fog_maxdensityskybox{	StringToMapTimeStringTableIndex("fog_maxdensityskybox"),	StringToMapTimeStringTableIndex(!m_FogSkyboxDensityOverride->IsSelected() ?				"-1" :			CFmtStr("%.2f", (float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax())) };
		MapTimeInfo_t::FogInfo_t fog_color{				StringToMapTimeStringTableIndex("fog_color"),				StringToMapTimeStringTableIndex(!m_FogColorOverride->IsSelected() ?						"-1 -1 -1" :	CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b())) };
		MapTimeInfo_t::FogInfo_t fog_colorskybox{		StringToMapTimeStringTableIndex("fog_colorskybox"),			StringToMapTimeStringTableIndex(!m_FogSkyboxColorOverride->IsSelected() ?				"-1 -1 -1" :	CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b())) };

		//add to the array
		fog_array.AddToTail(r_pixelfog);
		fog_array.AddToTail(fog_override);
		fog_array.AddToTail(fog_enable);
		fog_array.AddToTail(fog_enableskybox);
		fog_array.AddToTail(fog_start);
		fog_array.AddToTail(fog_end);
		fog_array.AddToTail(fog_startskybox);
		fog_array.AddToTail(fog_endskybox);
		fog_array.AddToTail(fog_maxdensity);
		fog_array.AddToTail(fog_maxdensityskybox);
		fog_array.AddToTail(fog_color);
		fog_array.AddToTail(fog_colorskybox);
	} while (false);

	//set the skybox
	do
	{
		//set the value
		if (m_bNightTimeMode)
			info.NightInfo.DefaultNightSky = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());
		else
			info.DayInfo.DefaultDaySky = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());

	} while (false);


	//set the color correction
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.FilterName = StringToMapTimeStringTableIndex(m_FilterComboBox->GetActiveItemUserData()->GetName());
			info.NightInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));
		}
		else
		{
			info.DayInfo.FilterName = StringToMapTimeStringTableIndex(m_FilterComboBox->GetActiveItemUserData()->GetName());
			info.DayInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));
		}

	} while (false);

	//set the clouds color
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a()));
		}
		else
		{
			info.DayInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a()));
		}

	} while (false);

	//set the bloom
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.BloomEnabled = StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableBloomCheckButton->IsSelected()));
			info.NightInfo.BloomScale = StringToMapTimeStringTableIndex(CFmtStr("%d", m_BloomScaleSlider->GetValue()));
			info.NightInfo.BloomScalarFactor = StringToMapTimeStringTableIndex(CFmtStr("%.3f", (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR));
		}
		else
		{
			info.DayInfo.BloomEnabled = StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableBloomCheckButton->IsSelected()));
			info.DayInfo.BloomScale = StringToMapTimeStringTableIndex(CFmtStr("%d", m_BloomScaleSlider->GetValue()));
			info.DayInfo.BloomScalarFactor = StringToMapTimeStringTableIndex(CFmtStr("%.3f", (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR));
		}

	} while (false);

	//set the sun info
	do
	{
		//is this currently the night panel
		if (m_bNightTimeMode)
			break;

		//set info.DayInfo.SunInfoEnabled
		info.DayInfo.SunInfoEnabled = m_EnableSunButton->IsSelected();

		//set our pitch
		const char* angle = FindSunInfoFromArray(info.DayInfo.SunInfo, "angles");
		if (!angle)
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "angle", CFmtStr("%d", m_SunYawSlider->GetValue()));
		else
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "angles", CFmtStr("0 %d 0", m_SunYawSlider->GetValue()));

		//set yaw
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "pitch", CFmtStr("%d", m_SunPitchSlider->GetValue()));

		//set size
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "size", CFmtStr("%d", m_SunSizeSlider->GetValue()));

		//set color
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "color", CFmtStr("%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));

		//set material
		char text[256];
		m_SunMaterialTextEntry->GetText(text, sizeof(text));

		//check for the material first
		if (!CMaterialReference(text).IsValid() || CMaterialReference(text)->IsErrorMaterial())
		{
			//copy the default texture in
			Q_snprintf(text, sizeof(text), "sprites/light_glow02_add_noz.spr");
		}

		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "material", text);

	} while (false);
}









#define MODIFY_MAP_PREFIX "OpenModifyMapWindow"
#define COPY_MAP_STATE_PREFIX "CopyMapState"
#define PASTE_MAP_STATE_PREFIX "PasteMapState"

//scroll bar values
static int CurrentScrolledNightValue = 0;
static int CurrentScrolledDayValue = 0;

//main map properties editor night page
class CMapPropertiesEditorPageBase : public PropertyPage
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorPageBase, PropertyPage);
public:
	CMapPropertiesEditorPageBase(Panel* parent, const char* name, bool IsNightPage);

	//layout
	void PerformLayout();

	//scroll wheel
	void OnMouseWheeled(int delta);

	//clear/add functions
	void Populate(CUtlVector<MapTimeInfo_t>& base);
	void Clear();

	void OnCommand(const char* cmd);

	//message funcs
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", kv);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");
	MESSAGE_FUNC(OnApplyPageSetting, "ApplyPageSetting");
	MESSAGE_FUNC(OnMapPropertiesPanelClosed, "MapPropertiesPanelClosed");
private:
	//is this for the day or night page
	bool m_bIsNightPage;

	//top combo box
	ComboBox* m_FileList;

	//scroll bar
	ScrollBar* m_ScrollBar;

	//list of items
	CUtlVector<Divider*> m_MapList;

	//current selected page
	int m_CurrentPage;

	//text font
	static HFont m_MapTextFont;
};
HFont CMapPropertiesEditorPageBase::m_MapTextFont = INVALID_FONT;

//copied state
static bool gs_HasCopiedState[2];
static MapTimeInfo_t gs_CopiedState;

//Clears the copied state of the panel.
void ClearCopiedState()
{
	gs_HasCopiedState[0] = false;
	gs_HasCopiedState[1] = false;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties editor night page.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPageBase::CMapPropertiesEditorPageBase(Panel* parent, const char* name, bool IsNightPage) : BaseClass(parent, name)
{
	//set settings
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);

	//initalize gs_HasCopiedState once
	static bool s_Initgs_CopiedState = false;
	if (!s_Initgs_CopiedState)
	{
		gs_HasCopiedState[0] = false;
		gs_HasCopiedState[1] = false;
		s_Initgs_CopiedState = true;
	}

	//init our vars
	m_bIsNightPage = IsNightPage;
	if (!gs_HasCopiedState[IsNightPage])
	{
		memset(&gs_CopiedState, 0, sizeof(MapTimeInfo_t));		//sizeof(MapTimeInfo_t) looks cooler in editor then sizeof(gs_CopiedState)
	}

	//create our top combo box
	m_FileList = new ComboBox(this, "FileList", 10, false);
	m_FileList->AddActionSignalTarget(this);

	//make the scroll bar
	m_ScrollBar = new ScrollBar(this, "ScrollBar", true);
	m_ScrollBar->AddActionSignalTarget(this);
	m_ScrollBar->SetValue((m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue));
	
	//add each page
	CUtlVector<MapTimeInfoBase_t>& base = GetDayNightInfo();

	for (int i = 0; i < base.Count(); i++)
		m_FileList->AddItem(base[i].filename, nullptr);

	m_FileList->ActivateItem(0);

	//set m_MapTextFont
	if (m_MapTextFont == INVALID_FONT)
	{
		m_MapTextFont = surface()->CreateFont();
		surface()->SetFontGlyphSet(m_MapTextFont, "", 24, 0, 0, 0, vgui::ISurface::FONTFLAG_ANTIALIAS);
	}
}

#define MAP_CONTAINER_HEIGHT 150

//----------------------------------------------------------------------------------------------------
// Purpose: Lays out controls
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::PerformLayout()
{
	BaseClass::PerformLayout();

	int x = 5;
	int y = 5;
	int gap = 5;

	int wide = GetWide() - (x * 2);

	// file list
	m_FileList->SetBounds(x, y, wide, 20);
	y += 20 + gap;

	// scrollbar
	int scrollBarWidth = 20;
	m_ScrollBar->SetBounds(GetWide() - scrollBarWidth - 5, y, scrollBarWidth, GetTall() - y - 5);

	//This code below was coded by chatgpt. its 1am and i just want to go to sleep!
	{
		//available height for items
		int availableHeight = GetTall() - y - 5;
		int itemHeight = MAP_CONTAINER_HEIGHT + gap;

		int itemsPerPage = availableHeight / itemHeight;
		if (itemsPerPage < 1)
			itemsPerPage = 1;

		int totalItems = m_MapList.Count();
		int maxScroll = totalItems - itemsPerPage + 1;
		if (maxScroll < 0)
			maxScroll = 0;

		m_ScrollBar->SetRange(0, maxScroll);
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetButtonPressedScrollValue(1);

		//if ((m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue) > maxScroll)
		//	(m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue) = maxScroll;

		m_ScrollBar->SetValue((m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue));

		// layout dividers
		int contentWide = wide - scrollBarWidth - 5;

		for (int i = 0; i < totalItems; i++)
		{
			if (i < (m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue) || i >= (m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue) + itemsPerPage)
			{
				m_MapList[i]->SetVisible(false);
				continue;
			}

			m_MapList[i]->SetVisible(true);
			m_MapList[i]->SetBounds(x, y, contentWide, MAP_CONTAINER_HEIGHT);

			y += itemHeight;
		}
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the mouse wheel is scrollar
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnMouseWheeled(int delta)
{
	m_ScrollBar->SetValue(m_ScrollBar->GetValue() - delta);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the combo box changes
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnTextChanged(KeyValues* data)
{
	//get the pages
	CUtlVector<MapTimeInfoBase_t>& base = GetDayNightInfo();

	//select the page
	int index = m_FileList->GetActiveItem();
	if (index < 0 || index >= base.Count())
		return;

	//clear then populate the list
	Clear();
	Populate(base[index].base);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the scroll bar is moved
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnScrollBarSliderMoved()
{
	(m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue) = m_ScrollBar->GetValue();
	PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Applies the page settings
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnApplyPageSetting()
{
	//check the index's
	int file = m_FileList->GetActiveItem();
	int map = m_CurrentPage;

	//get the time info
	CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
	if (map < 0 || map >= baseinfo[file].base.Count())
		return;

	//g_MapPropertiesPanel must be open
	if (!g_MapPropertiesPanel)
		return;

	//get the new data
	MapTimeInfo_t& info = baseinfo[file].base[m_CurrentPage];
	g_MapPropertiesPanel->GetData(info);

	//send to server
	{
		KeyValuesAD temp(new KeyValues("temp_file"));
		WriteTimeInfoToKeyvalues(info, temp);
		temp->SaveToFile(filesystem, "__temptime.txt", "MOD", false, true);

		//tell server to read the info
		engine->ClientCmd(CFmtStr("_amod_daytimeinfo_reset %d %d", file, map));
	}

	//clear then restore the panel to show the changes
	Clear();
	Populate(baseinfo[m_FileList->GetActiveItem()].base);
	return;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the map properties panel is closed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnMapPropertiesPanelClosed()
{
	GetParent()->GetParent()->SetAlpha(255);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Populates the list
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::Populate(CUtlVector<MapTimeInfo_t>& base)
{
	for (int i = 0; i < base.Count(); i++)
	{
		//create the divider
		Divider* divider = new Divider(this, "ButtonFrame");

		//create the mapname text
		Label* maptext = new Label(divider, "MapText", CFmtStr("Map: %s", base[i].mapname));
		maptext->SetBounds(10, 5, 365, 40);
		maptext->SetFont(m_MapTextFont);
		maptext->SetContentAlignment(Label::Alignment::a_northwest);

		const char* skyName = m_bIsNightPage ? StringFromMapTimeStringTableIndex(base[i].NightInfo.DefaultNightSky) : StringFromMapTimeStringTableIndex(base[i].DayInfo.DefaultDaySky);

		//create the 6 skybox faces
		struct SkyFace_t
		{
			const char* suffix;
			int col;
			int row;
		};

		SkyFace_t faces[] =
		{
			{ "up", 3, 0 },
			{ "lf", 1, 1 },
			{ "ft", 0, 1 },
			{ "rt", 3, 1 },
			{ "bk", 2, 1 },
			{ "dn", 3, 2 },
		};

		//get the bounds
		int baseX = 230;
		int baseY = 5;
		int baseW = 184;
		int baseH = 137;

		int cellW = baseW / 4;
		int cellH = baseH / 3;

		//create images
		for (int f = 0; f < 6; f++)
		{
			CStretchingImage* skybox = new CStretchingImage(divider, "SkyboxImage");
			skybox->SetImage(CFmtStr("../skybox/%s%s", skyName, faces[f].suffix));
			skybox->SetBounds(baseX + faces[f].col * cellW, baseY + faces[f].row * cellH, cellW, cellH);
		}

		//create the copy map settings button
		Button* copyButton = new Button(divider, "copyButton", "Copy Map State");
		copyButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 61, 105, 24);
		copyButton->SetCommand(CFmtStr(COPY_MAP_STATE_PREFIX "%d", i));
		copyButton->AddActionSignalTarget(this);
		
		//create the paste map settings button
		Button* pasteButton = new Button(divider, "pasteButton", "Paste Map State");
		pasteButton->SetBounds(114, MAP_CONTAINER_HEIGHT - 61, 105, 24);
		pasteButton->SetCommand(CFmtStr(PASTE_MAP_STATE_PREFIX"%d", i));
		pasteButton->AddActionSignalTarget(this);

		//create the modify map settings button
		Button* modifyButton = new Button(divider, "ModifyMapSettings", "Modify Map Settings");
		modifyButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 35, 355, 24);
		modifyButton->SetCommand(CFmtStr(MODIFY_MAP_PREFIX "%d", i));
		modifyButton->AddActionSignalTarget(this);

		m_MapList.AddToTail(divider);
	}

	//perform layout to set the bounds
	PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Clears the list
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::Clear()
{
	//remove all divider
	for (int i = 0; i < m_MapList.Count(); i++)
		delete m_MapList[i];

	m_MapList.RemoveAll();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::OnCommand(const char* cmd)
{
	//check for MODIFY_MAP_PREFIX
	if (Q_strstr(cmd, MODIFY_MAP_PREFIX))
	{
		//get the index
		int index = Q_atoi(cmd + Q_strlen(MODIFY_MAP_PREFIX));

		//get the time info
		CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
		if (index < 0 || index >= baseinfo[m_FileList->GetActiveItem()].base.Count())
			return;

		MapTimeInfo_t& info = baseinfo[m_FileList->GetActiveItem()].base[index];

		//look for the maps
		char realcurrpath[512];
		if (!FindMapPath("maps", szMapName, realcurrpath, sizeof(realcurrpath)))
			return;

		//look for the map
		char realpath[512];
		if (!FindMapPath("maps", info.mapname, realpath, sizeof(realpath)))
			return;

		//take away 5 for the maps/ part
		char* _realcurrpath = realcurrpath + 5;
		char* _realpath = realpath + 5;

		//load set the settings
		engine->ClientCmd(CFmtStr("amod_day %d; amod_day_sky \"\"; amod_night_sky \"\"", !m_bIsNightPage));

		//open the properties panel
		if (g_MapPropertiesPanel)
			delete g_MapPropertiesPanel;

		//set the data
		g_MapPropertiesPanel = new CMapPropertiesPanel(GetParent());
		g_MapPropertiesPanel->SetTitle(CFmtStr("%s Map Properties: %s", m_bIsNightPage ? "Night" : "Day", _realpath), true);
		g_MapPropertiesPanel->DoModal();
		g_MapPropertiesPanel->SetPos(0, 0);
		g_MapPropertiesPanel->AddActionSignalTarget(this);
		g_MapPropertiesPanel->Init(info, m_bIsNightPage);

		//hide us
		GetParent()->GetParent()->SetAlpha(0);

		//set the data
		m_CurrentPage = index;

		optionspanel->SetFilterButtonValue(true, true);

		//check to see if we are currently in the map
		if (!Q_stricmp(_realcurrpath, _realpath))
			return;

		engine->ClientCmd(CFmtStr("map \"%s", _realpath));
		return;
	}

	//check for copy state
	else if (Q_strstr(cmd, COPY_MAP_STATE_PREFIX))
	{
		//get the index
		int index = Q_atoi(cmd + Q_strlen(COPY_MAP_STATE_PREFIX));

		//get the time info
		CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
		if (index < 0 || index >= baseinfo[m_FileList->GetActiveItem()].base.Count())
			return;

		MapTimeInfo_t& info = baseinfo[m_FileList->GetActiveItem()].base[index];

		//copy to our map state
		gs_HasCopiedState[m_bIsNightPage] = true;
	
		//copy state
		CopyTimeInfoData(info, gs_CopiedState, m_bIsNightPage, !m_bIsNightPage);
	}

		//check for paste state
	else if (Q_strstr(cmd, PASTE_MAP_STATE_PREFIX))
	{
		//we must have a copied state
		if (!gs_HasCopiedState[m_bIsNightPage])
		{
			//show error
			QueryBox* modal = new QueryBox("No Copied State", "There is currently no state coppied!", this);
			modal->MoveToCenterOfScreen();
			modal->Activate();
			modal->DoModal();
			return;
		}

		//get the index
		int index = Q_atoi(cmd + Q_strlen(PASTE_MAP_STATE_PREFIX));

		//get the time info
		CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
		if (index < 0 || index >= baseinfo[m_FileList->GetActiveItem()].base.Count())
			return;

		MapTimeInfo_t& info = baseinfo[m_FileList->GetActiveItem()].base[index];

		//copy state
		CopyTimeInfoData(gs_CopiedState, info, m_bIsNightPage, !m_bIsNightPage);

		//clear then repopulate the list to show the changes
		Clear();
		Populate(baseinfo[m_FileList->GetActiveItem()].base);

		//send to server
		{
			KeyValuesAD temp(new KeyValues("temp_file"));
			WriteTimeInfoToKeyvalues(info, temp);
			temp->SaveToFile(filesystem, "__temptime.txt", "MOD", true, true);

			//tell server to read the info
			engine->ClientCmd(CFmtStr("_amod_daytimeinfo_reset %d %d", m_FileList->GetActiveItem(), index));

			//reload server settings. This could have been the map that was changed.
			engine->ClientCmd("_amod_day_do");
		}
	}

	return BaseClass::OnCommand(cmd);
}









//main map properties editor night page
class CMapPropertiesEditorNightPage : public CMapPropertiesEditorPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorNightPage, CMapPropertiesEditorPageBase);
public:
	CMapPropertiesEditorNightPage(Panel* parent) : BaseClass(parent, "MapPropertiesDayPage", true) {}
};

//main map properties editor day page
class CMapPropertiesEditorDayPage : public CMapPropertiesEditorPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorDayPage, CMapPropertiesEditorPageBase);
public:
	CMapPropertiesEditorDayPage(Panel* parent) : BaseClass(parent, "MapPropertiesDayPage", false) {}
};










#define COMMAND_SAVE_TO_FILE "SaveToFile"
#define COMMAND_RELOAD_SCRIPTS "ReloadScripts"
#define COMMAND_RELOAD_SCRIPTS_CONFIRM "ReloadScripts_C"

//main map properties editor panel!!
class CMapPropertiesEditorPanel : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorPanel, PropertyDialog);
public:
	CMapPropertiesEditorPanel(VPANEL parent);
	~CMapPropertiesEditorPanel();

	//called on command
	void OnCommand(const char* pszCommand);
private:
	//each page
	CMapPropertiesEditorNightPage* m_NightPage;
	CMapPropertiesEditorDayPage* m_DayPage;
};

//static panel
static CMapPropertiesEditorPanel* s_MapPropertiesEditorPanel;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPanel::CMapPropertiesEditorPanel(VPANEL parent) : BaseClass(nullptr, "MapPropertiesEditor")
{
	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSize(480, 600);
	SetDeleteSelfOnClose(true);
	MoveToCenterOfScreen();
	SetTitle("Map Properties Editor", false);

	//add the pages
	AddPage(m_NightPage = new CMapPropertiesEditorNightPage(this), "Night Map Properties");
	AddPage(m_DayPage = new CMapPropertiesEditorDayPage(this), "Day Map Properties");
	GetPropertySheet()->SetActivePage(ConVarRef("amod_day").GetBool() ? (Panel*)m_DayPage : (Panel*)m_NightPage);
	GetPropertySheet()->SetKeyBoardInputEnabled(false);

	//set our save button
	SetOKButtonText("Save");
	_okButton->SetCommand(COMMAND_SAVE_TO_FILE);
	SetCancelButtonText("Reload Scripts");
	_cancelButton->SetCommand(COMMAND_RELOAD_SCRIPTS);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnCommand(const char* pszCommand)
{
	//check for COMMAND_SAVE_TO_FILE
	if (!Q_stricmp(pszCommand, COMMAND_SAVE_TO_FILE))
	{
		WriteAllTimeInfosToFiles();
		return;
	}

	//check for COMMAND_RELOAD_SCRIPTS
	else if (!Q_stricmp(pszCommand, COMMAND_RELOAD_SCRIPTS))
	{
		QueryBox* modal = new QueryBox("Are you sure?", "Are you sure you want to reload the resource/time_info/*.txt script files?\nAny unsaved data will be lost!", this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		modal->SetOKCommand(new KeyValues("Command", "command", COMMAND_RELOAD_SCRIPTS_CONFIRM));
	}

	//check for COMMAND_RELOAD_SCRIPTS_CONFIRM
	else if (!Q_stricmp(pszCommand, COMMAND_RELOAD_SCRIPTS_CONFIRM))
	{
		//reload the script file
		engine->ClientCmd("amod_timeinfo_reset");

		//reload the pages
		int activepage = GetPropertySheet()->GetActivePageNum();

		//delete our pages
		GetPropertySheet()->DeletePage(m_NightPage);
		GetPropertySheet()->DeletePage(m_DayPage);

		delete m_DayPage;

		//ALWAYS delete g_MapPropertiesPanel  if its open
		if (g_MapPropertiesPanel)
			delete g_MapPropertiesPanel;

		//add the pages
		AddPage(m_NightPage = new CMapPropertiesEditorNightPage(this), "Night Map Properties");
		AddPage(m_DayPage = new CMapPropertiesEditorDayPage(this), "Day Map Properties");
		GetPropertySheet()->SetActivePage(activepage ? (Panel*)m_DayPage : (Panel*)m_NightPage);
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPanel::~CMapPropertiesEditorPanel()
{
	s_MapPropertiesEditorPanel = nullptr;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Deletes and clears s_MapPropertiesEditorPanel if its open/active
//----------------------------------------------------------------------------------------------------
void DeleteMapPropertiesPanel()
{
	if (s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel->DeletePanel();

	s_MapPropertiesEditorPanel = nullptr;
}

//command to open the map time properties editor
CON_COMMAND(open_map_time_properties_editor, "")
{
	//delete if needed
	if (atoi(args.Arg(1)) != 0)
	{
		bool open = s_MapPropertiesEditorPanel != nullptr;;

		if (s_MapPropertiesEditorPanel)
		{
			delete s_MapPropertiesEditorPanel;
			s_MapPropertiesEditorPanel = nullptr;
		}
		
		//do we open
		if (!open)
			return;
	}


	//create the panel if not already here
	if (!s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel = new CMapPropertiesEditorPanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	s_MapPropertiesEditorPanel->Activate();
}