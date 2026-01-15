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
#include "geo guesser/GG_MiniMap.h"
#include "effects panel/EffectsPanel.h"
#include "IOptionsPanel.h"
#include "ColorPicker.h"
#include "filesystem.h"
#include "fmtstr.h"

using namespace vgui;

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
	
	virtual void OnMousePressed(MouseCode code) 
	{ 
		if (code != MouseCode::MOUSE_RIGHT) 
			BaseClass::OnMousePressed(code);
	}
};







#define COMMAND_COPY_DATA "CopyCommand"
#define COMMAND_PASTE_DATA "PasteCommand"
#define COMMAND_APPLY_PAGE_SETTINGS "ApplyPageSettings"
#define COMMAND_APPLY_PAGE_SETTINGS_CONFIRM "ApplyPageSettingsConfirm"

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

	void OnKeyCodePressed(KeyCode code);
	void OnKeyCodeReleased(KeyCode code);
	void OnMousePressed(KeyCode code);

	//called when the color picker picks a color
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PARAMS(OnChildRightClicked, "ChildRightClicked", data);
	MESSAGE_FUNC_PARAMS(OnSliderMoved, "SliderMoved", data);
	MESSAGE_FUNC(SliderDragStart, "SliderDragStart") { m_bCanChange = true; };
	MESSAGE_FUNC(SliderDragEnd, "SliderDragEnd") { m_bCanChange = true; };

	//init funcs
	void Init(MapTimeInfo_t& info, bool IsNightPage);
	void GetData(MapTimeInfo_t& info);
	void FormatImage(const char* input, char* output, int outside);
private:

#pragma region Fog
	CheckButton* m_OverrideFog;
	CheckButton* m_EnableFog;
	CheckButton* m_EnableSkyboxFog;
	CheckButton* m_PixelFog;

	//fog color buttons
	CMapPropertiesPanelButton* m_SelectFogColorButton;
	CMapPropertiesPanelButton* m_SelectSkyboxFogColorButton;

	//fog sliders + labels
	CMapPropertiesPanelSlider* m_FogStartSlider;
	CMapPropertiesPanelSlider* m_FogEndSlider;
	CMapPropertiesPanelSlider* m_SkyboxFogStartSlider;
	CMapPropertiesPanelSlider* m_SkyboxFogEndSlider;
	CMapPropertiesPanelSlider* m_FogDensitySlider;
	CMapPropertiesPanelSlider* m_SkyboxFogDensitySlider;

	Label* m_FogStartLabel;
	Label* m_FogEndLabel;
	Label* m_SkyboxFogStartLabel;
	Label* m_SkyboxFogEndLabel;
	Label* m_FogDensityLabel;
	Label* m_SkyboxFogDensityLabel;

	//fog colors
	Color m_FogColor;
	Color m_FogSkyboxColor;
#pragma endregion

#pragma region Skybox
	ImagePanel* m_SkyboxImageBackground;
	CStretchingImage* m_SkyboxImage;

	ComboBox* m_SkyboxNames;
#pragma endregion

#pragma region EpicFilter
	ComboBox* m_filterComboBox;
	Label* m_IntensityText;
	CMapPropertiesPanelSlider* m_IntensitySlider;
#pragma endregion

#pragma region CloudColor
	//clouds colors
	Color m_CloudColors;
	CMapPropertiesPanelButton* m_CloudButton;
#pragma endregion

#pragma region SunInfo
	CheckButton* m_EnableSunButton;
#pragma endregion

	Button* m_ApplyButton;

	//copy paste data
	struct CopyPasteData_t
	{
		Panel* copypanel = nullptr;
		Panel* parent = nullptr;
		union
		{
			color32 cvalue;
			int ivalue = 0;
		};
	};
	CopyPasteData_t CopyPasteData;

	//is it nighttime or not
	bool m_bNightTimeMode;

	//current color selector mode
	enum ColorSelectorMode {Color_Fog, Color_SkyboxFog, Color_Clouds} m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;


	//previous convar values to re-set when we close
	char m_PreviousFilterConvarValue[256];
	char m_PreviousFilterIntensityConvarValue[16];
	char m_PreviousCloudsColorValue[16];
	char m_PreviousCloudsShowValue[16];
	char m_PreviousCloudsOverrideValue[4];
	char m_PreviousGodConvarValue[4];

	//have we changed any settings (this is used for the save propmt)
	bool m_bChangedAnything;
	bool m_bCanChange;
};

//static map properties panel
static CMapPropertiesPanel* g_MapPropertiesPanel;

#define COMMAND_SET_FOG_COLOR "SetFogColor"
#define COMMAND_SET_SKYBOX_FOG_COLOR "SetSkyboxFogColor"
#define COMMAND_SET_CLOUDS_COLOR "SetCloudsColor"

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
	m_bChangedAnything = false;
	m_bCanChange = false;
	g_MapPropertiesPanel = this;
	m_CloudColors.SetColor(255, 255, 255, 255);

	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(false);
	SetPos(0, 0);
	SetSize(550, 800);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	Activate();
	
	//disable mouselook so keyboardlook can be used
	cl_mouselook.SetValue(false);

	//fog check buttons
	{
		m_OverrideFog = new CheckButton(this, "OverrideFogCB", "Override Fog");
		m_EnableFog = new CheckButton(this, "EnableFogCB", "Enable Fog");
		m_EnableSkyboxFog = new CheckButton(this, "EnableSkyboxFogCB", "Enable Skybox Fog");
		m_PixelFog = new CheckButton(this, "EnablePixelFog", "Enable Pixel Fog");

		//set the bounds
		m_OverrideFog->SetBounds(215, 30, 150, 20);
		m_EnableFog->SetBounds(220, 47, 150, 20);
		m_EnableSkyboxFog->SetBounds(197, 64, 150, 20);
		m_PixelFog->SetBounds(203, 81, 150, 20);
	}

	//create the 8 labels
	{
		Label* FogLabels[8];
		memset(FogLabels, 0, sizeof(FogLabels));

		FogLabels[0] = new Label(this, "Label", "Fog Color =");
		FogLabels[1] = new Label(this, "Label", "Fog Skybox Color =");
		FogLabels[2] = new Label(this, "Label", "Fog Start =");
		FogLabels[3] = new Label(this, "Label", "Fog End =");
		FogLabels[4] = new Label(this, "Label", "Fog Skybox Start =");
		FogLabels[5] = new Label(this, "Label", "Fog Skybox End =");
		FogLabels[6] = new Label(this, "Label", "Fog Max Density =");
		FogLabels[7] = new Label(this, "Label", "Fog Max Skybox Density =");

		//align the labels and set their positions
		int y = 100;

		for (int i = 0; i < SIZE_OF_ARRAY(FogLabels); i++)
		{
			if (!FogLabels[i])
				continue;

			FogLabels[i]->SetContentAlignment(Label::Alignment::a_east);
			FogLabels[i]->SetBounds(10, y, 245, 20);

			y += 24;
		}
	}

	//create the select color buttons for the fog
	{
		m_SelectFogColorButton = new CMapPropertiesPanelButton(this, "SelectFogButton", "Select Fog Color");
		m_SelectFogColorButton->SetBounds(280, 103, 160, 20);
		m_SelectFogColorButton->SetCommand(COMMAND_SET_FOG_COLOR);

		m_SelectSkyboxFogColorButton = new CMapPropertiesPanelButton(this, "SelectSkybox", "Select Skybox Fog Color");
		m_SelectSkyboxFogColorButton->SetBounds(280, 125, 160, 20);
		m_SelectSkyboxFogColorButton->SetCommand(COMMAND_SET_SKYBOX_FOG_COLOR);
	}

	//create the fog sliders
	{
		m_FogStartSlider = new CMapPropertiesPanelSlider(this, "FogStartSlider", 150);
		m_FogEndSlider = new CMapPropertiesPanelSlider(this, "FogEndSlider", 150);
		m_SkyboxFogStartSlider = new CMapPropertiesPanelSlider(this, "SkyboxFogStartSlider", 150);
		m_SkyboxFogEndSlider = new CMapPropertiesPanelSlider(this, "SkyboxFogEndSlider", 150);
		m_FogDensitySlider = new CMapPropertiesPanelSlider(this, "FogDensitySlider");
		m_SkyboxFogDensitySlider = new CMapPropertiesPanelSlider(this, "SkyboxFogDensitySlider");

		//create the labels
		m_FogStartLabel = new Label(this, "SkyboxFogDensityLabel", "0");
		m_FogEndLabel = new Label(this, "SkyboxFogDensityLabel", "0");
		m_SkyboxFogStartLabel = new Label(this, "SkyboxFogDensityLabel", "0");
		m_SkyboxFogEndLabel = new Label(this, "SkyboxFogDensityLabel", "0");
		m_FogDensityLabel = new Label(this, "SkyboxFogDensityLabel", "0");
		m_SkyboxFogDensityLabel = new Label(this, "SkyboxFogDensityLabel", "0");

		//set bounds
		m_FogStartSlider->SetBounds(260, 154, 250, 20);
		m_FogEndSlider->SetBounds(260, 176, 250, 20);
		m_SkyboxFogStartSlider->SetBounds(260, 198, 250, 20);
		m_SkyboxFogEndSlider->SetBounds(260, 220, 250, 20);
		m_FogDensitySlider->SetBounds(260, 242, 250, 20);
		m_SkyboxFogDensitySlider->SetBounds(260, 264, 250, 20);

		m_FogStartLabel->SetBounds(510, 154, 75, 20);
		m_FogEndLabel->SetBounds(510, 176, 75, 20);
		m_SkyboxFogStartLabel->SetBounds(510, 198, 75, 20);
		m_SkyboxFogEndLabel->SetBounds(510, 220, 75, 20);
		m_FogDensityLabel->SetBounds(510, 242, 75, 20);
		m_SkyboxFogDensityLabel->SetBounds(510, 264, 75, 20);

		//set range
		m_FogStartSlider->SetRange(-10000, 25000);
		m_FogEndSlider->SetRange(-10000, 35000);
		m_SkyboxFogStartSlider->SetRange(-10000, 75000);
		m_SkyboxFogEndSlider->SetRange(-10000, 150000);
		m_FogDensitySlider->SetRange(0, 100);
		m_SkyboxFogDensitySlider->SetRange(0, 100);
	}

	//create the divider for the fog
	Divider* fogdivider = new Divider(this, "FogDivider");
	fogdivider->SetBounds(-1, 295, 620, 1);

	//create the skybox image
	{
		//title
		Label* title = new Label(this, "SkyboxTitle", "Skybox Settings:");
		title->SetBounds(20, 295, 200, 20);
		title->SetContentAlignment(Label::Alignment::a_center);

		//background
		m_SkyboxImageBackground = new ImagePanel(this, "SkyboxBackgroundImage");
		m_SkyboxImageBackground->SetBounds(15, 316, 210, 210);
		m_SkyboxImageBackground->SetFillColor(Color(0, 0, 0, 255));
		m_SkyboxImageBackground->SetZPos(0);

		//image
		m_SkyboxImage = new CStretchingImage(this, "SkyboxImage");
		m_SkyboxImage->SetBounds(20, 321, 200, 200);
		m_SkyboxImage->SetZPos(10);

		//combo box
		m_SkyboxNames = new ComboBox(this, "SkyboxNames", 14, false);
		m_SkyboxNames->SetBounds(15, 529, 210, 22);
	}

	//create the divider for the epic filter
	Divider* filterdivider = new Divider(this, "EpicFilterDivider");
	filterdivider->SetBounds(245, 295, 1, 263);

	//post processing
	{
		//title
		Label* title = new Label(this, "FilterTitle", "Post Processing Filter Settings:");
		title->SetBounds(245, 295, 305, 20);
		title->SetContentAlignment(Label::Alignment::a_center);
	
		//make the custom filter combo box
		m_filterComboBox = new ComboBox(this, "filterComboBox", 14, false);
		m_filterComboBox->SetBounds(255, 320, 285, 22);

		//make the filter slider text
		m_IntensityText = new Label(this, "IntensitySlider", "Filter Intensity");
		m_IntensityText->SetBounds(260, 344, 270, 22);
		m_IntensityText->SetContentAlignment(Label::Alignment::a_center);

		//make the filter slider
		m_IntensitySlider = new CMapPropertiesPanelSlider(this, "IntensitySlider");
		m_IntensitySlider->SetBounds(260, 370, 270, 22);
		m_IntensitySlider->SetRange(0, 100);
	}

	//create the divider for the clouds
	Divider* cloudsdivider = new Divider(this, "cloudsdivider");
	cloudsdivider->SetBounds(245, 410, 400, 1);

	//clouds
	{
		//title
		Label* title = new Label(this, "FilterTitle", "Cloud Colors:");
		title->SetBounds(245, 415, 305, 20);
		title->SetContentAlignment(Label::Alignment::a_center);

		m_CloudButton = new CMapPropertiesPanelButton(this, "CloudButton", "Change Clouds Color");
		m_CloudButton->SetBounds(278, 440, 265, 20);
		m_CloudButton->SetCommand(COMMAND_SET_CLOUDS_COLOR);
	}

	//create the divider for the skybox + cloud color
	Divider* skyboxdivider = new Divider(this, "Skyboxdivider");
	skyboxdivider->SetBounds(-1, 558, 620, 1);

	//apply button
	m_ApplyButton = new Button(this, "ApplyButton", "Apply", this, COMMAND_APPLY_PAGE_SETTINGS);
	m_ApplyButton->SetBounds(20, 600, 580, 20);
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
	m_SelectFogColorButton->RequestFocus();
	BaseClass::OnMousePressed(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when an element is right clicked
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnChildRightClicked(KeyValues* keyvalues)
{
	int x, y;
	surface()->SurfaceGetCursorPos(x, y);

	//create the copy/paste menu
	Menu* CopyPasteMenu = new Menu(this, "CopyPasteMenu");
	CopyPasteMenu->SetBounds(x, y, 200, 50);
	CopyPasteMenu->AddMenuItem("CopyItem", "Copy Data", COMMAND_COPY_DATA, this);

	//only if we have a parent
	if (CopyPasteData.copypanel)
		CopyPasteMenu->AddMenuItem("PasteItem", "Paste Data", COMMAND_PASTE_DATA, this);

	CopyPasteMenu->SetVisible(true);

	//set our copy paste data
	CopyPasteData.parent = (Panel*)keyvalues->GetPtr("panel");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties page.
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanel::~CMapPropertiesPanel()
{
	//set the previous convar values
	if (m_PreviousCloudsOverrideValue[0]) ConVarRef("amod_clouds_color_override").SetValue(m_PreviousCloudsOverrideValue);
	if (m_PreviousCloudsColorValue[0]) ConVarRef("amod_clouds_color").SetValue(m_PreviousCloudsColorValue);
	if (m_PreviousCloudsShowValue[0]) ConVarRef("amod_clouds").SetValue(m_PreviousCloudsShowValue);
	if (m_PreviousGodConvarValue[0]) ConVarRef("amod_enable_god").SetValue(m_PreviousGodConvarValue);
	ConVarRef(m_bNightTimeMode ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename").SetValue(m_PreviousFilterConvarValue);
	ConVarRef(m_bNightTimeMode ? "amod_epic_filter_night_intensity" : "amod_epic_filter_day_intensity").SetValue(m_PreviousFilterIntensityConvarValue);

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
	SetBgColor(Color(150, 150, 150, 255));

	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_FogColor);
	surface()->DrawFilledRect(260, 104, 260 + 16, 104 + 16);

	//draw the colors
	surface()->DrawSetColor(m_FogSkyboxColor);
	surface()->DrawFilledRect(260, 128, 260 + 16, 128 + 16);
	
	//draw the colors
	Color color = m_CloudColors;
	if (color.r() == 255 && color.g() == 255 && color.b() == 255 && color.a() == 255)
		color = Color(51, 103, 153, 255);

	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(255, 440, 255 + 19, 440 + 19);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnThink()
{
	//set the fog values
	{
		static ConVar* fog_override = cvar->FindVar("fog_override");
		static ConVar* fog_enable = cvar->FindVar("fog_enable");
		static ConVar* fog_enableskybox = cvar->FindVar("fog_enableskybox");
		static ConVar* r_pixelfog = cvar->FindVar("r_pixelfog");
		static ConVar* fog_color = cvar->FindVar("fog_color");
		static ConVar* fog_colorskybox = cvar->FindVar("fog_colorskybox");
		static ConVar* fog_start = cvar->FindVar("fog_start");
		static ConVar* fog_end = cvar->FindVar("fog_end");
		static ConVar* fog_startskybox = cvar->FindVar("fog_startskybox");
		static ConVar* fog_endskybox = cvar->FindVar("fog_endskybox");
		static ConVar* fog_maxdensity = cvar->FindVar("fog_maxdensity");
		static ConVar* fog_maxdensityskybox = cvar->FindVar("fog_maxdensityskybox");

		//set values
		fog_override->SetValue(m_OverrideFog->IsSelected());
		fog_enable->SetValue(m_EnableFog->IsSelected());
		fog_enableskybox->SetValue(m_EnableSkyboxFog->IsSelected());
		r_pixelfog->SetValue(m_PixelFog->IsSelected());
		fog_color->SetValue(CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b()));
		fog_colorskybox->SetValue(CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b()));
		fog_start->SetValue(m_FogStartSlider->GetValue());
		fog_end->SetValue(m_FogEndSlider->GetValue());
		fog_startskybox->SetValue(m_SkyboxFogStartSlider->GetValue());
		fog_endskybox->SetValue(m_SkyboxFogEndSlider->GetValue());
		fog_maxdensity->SetValue((float)m_FogDensitySlider->GetValue() / 100);
		fog_maxdensityskybox->SetValue((float)m_SkyboxFogDensitySlider->GetValue() / 100);

		//set the label texts
		m_FogStartLabel->SetText(CFmtStr("%d", fog_start->GetInt()));
		m_FogEndLabel->SetText(CFmtStr("%d", fog_end->GetInt()));
		m_SkyboxFogStartLabel->SetText(CFmtStr("%d", fog_startskybox->GetInt()));
		m_SkyboxFogEndLabel->SetText(CFmtStr("%d", fog_endskybox->GetInt()));
		m_FogDensityLabel->SetText(CFmtStr("%.2f", fog_maxdensity->GetFloat()));
		m_SkyboxFogDensityLabel->SetText(CFmtStr("%.2f", fog_maxdensityskybox->GetFloat()));

		//set enabled states
		m_EnableFog->SetEnabled(m_OverrideFog->IsSelected());
		m_EnableSkyboxFog->SetEnabled(m_OverrideFog->IsSelected());
		m_PixelFog->SetEnabled(m_OverrideFog->IsSelected());
		m_SelectFogColorButton->SetEnabled(m_OverrideFog->IsSelected());
		m_SelectSkyboxFogColorButton->SetEnabled(m_OverrideFog->IsSelected());
		m_FogStartSlider->SetEnabled(m_OverrideFog->IsSelected());
		m_FogEndSlider->SetEnabled(m_OverrideFog->IsSelected());
		m_SkyboxFogStartSlider->SetEnabled(m_OverrideFog->IsSelected());
		m_SkyboxFogEndSlider->SetEnabled(m_OverrideFog->IsSelected());
		m_FogDensitySlider->SetEnabled(m_OverrideFog->IsSelected());
		m_SkyboxFogDensitySlider->SetEnabled(m_OverrideFog->IsSelected());
	}

	//color correction
	{
		//set text
		m_IntensityText->SetText(CFmtStr("%.2f", (float)m_IntensitySlider->GetValue() / 100));

		//convars
		static ConVar* intensity_day = cvar->FindVar("amod_epic_filter_day_intensity");
		static ConVar* intensity_night = cvar->FindVar("amod_epic_filter_night_intensity");

		//set the convar value
		float value = (float)m_IntensitySlider->GetValue() / 100;
		if (m_bNightTimeMode)
			intensity_night->SetValue(value);
		else
			intensity_day->SetValue(value);
	}

	//clouds color
	{
		static ConVar* amod_clouds_color = cvar->FindVar("amod_clouds_color");

		//check if the clouds color already matches our color. If so then dont set the value
		const char* color = CFmtStr("%d %d %d %d", m_CloudColors.r(), m_CloudColors.g(), m_CloudColors.b(), m_CloudColors.a());
		if (Q_stricmp(amod_clouds_color->GetString(), color))
			amod_clouds_color->SetValue(color);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnCommand(const char* pszCommand)
{
	if (!Q_stricmp(pszCommand, COMMAND_SET_FOG_COLOR))
	{
		m_bCanChange = true;

		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Fog Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Fog;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_SET_SKYBOX_FOG_COLOR))
	{
		m_bCanChange = true;

		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Skybox Fog Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogSkyboxColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_SkyboxFog;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_SET_CLOUDS_COLOR))
	{
		m_bCanChange = true;

		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Clouds Color (255 255 255 255 = default)", true);
		m_ColorPicker->SetUsesAlpha(true);
		m_ColorPicker->SetColor(m_CloudColors);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Clouds;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_APPLY_PAGE_SETTINGS))
	{
		//check for m_bChangedAnything
		if (m_bChangedAnything)
		{
			//get title
			char title[512];
			_title->GetText(title, sizeof(title));

			//remove *
			title[Q_strlen(title) - 1] = '\0';
			SetTitle(title, true);
		}

		m_bChangedAnything = false;

		//post message
		PostActionSignal(new KeyValues("ApplyPageSetting"));
		return;
	}

	//array of all color selector buttons
	vgui::Panel* colorPanels[] =
	{
		m_SelectFogColorButton,
		m_SelectSkyboxFogColorButton,
		m_CloudButton
	};

	//array of color values corresponding to each button
	Color* colorValues[] =
	{
		&m_FogColor,
		&m_FogSkyboxColor,
		&m_CloudColors
	};

	int colorCount = sizeof(colorPanels) / sizeof(colorPanels[0]);

	//check for copy paste command
	if (!Q_stricmp(pszCommand, COMMAND_COPY_DATA))
	{
		//store which panel the copy came from
		CopyPasteData.copypanel = CopyPasteData.parent;

		//check if the copied panel is a slider
		if (dynamic_cast<Slider*>(CopyPasteData.parent) != nullptr)
		{
			//copy the slider value
			CopyPasteData.ivalue = dynamic_cast<Slider*>(CopyPasteData.parent)->GetValue();
			return;
		}

		//check if the copied panel is a color button
		for (int i = 0; i < colorCount; i++)
		{
			if (CopyPasteData.parent == colorPanels[i])
			{
				//copy RGB values from the associated color
				CopyPasteData.cvalue.r = colorValues[i]->r();
				CopyPasteData.cvalue.g = colorValues[i]->g();
				CopyPasteData.cvalue.b = colorValues[i]->b();
				return;
			}
		}
	}
	else if (!Q_stricmp(pszCommand, COMMAND_PASTE_DATA))
	{
		//if both source and destination are sliders, paste slider value
		if (dynamic_cast<Slider*>(CopyPasteData.parent) != nullptr && dynamic_cast<Slider*>(CopyPasteData.copypanel))
		{
			//apply the copied slider value
			dynamic_cast<Slider*>(CopyPasteData.parent)->SetValue(CopyPasteData.ivalue);
			return;
		}

		int dst = -1;
		int src = -1;

		//find destination and source color buttons
		for (int i = 0; i < colorCount; i++)
		{
			if (CopyPasteData.parent == colorPanels[i])
				dst = i;

			if (CopyPasteData.copypanel == colorPanels[i])
				src = i;
		}

		//if both source and destination are valid color buttons
		if (dst != -1 && src != -1)
		{
			//apply copied RGB values to destination color
			colorValues[dst]->SetColor(
				CopyPasteData.cvalue.r,
				CopyPasteData.cvalue.g,
				CopyPasteData.cvalue.b,
				255
			);
		}
	}
	else if (!Q_stricmp(pszCommand, "Close"))
	{
		if (!m_bChangedAnything)
		{
			BaseClass::OnCommand(pszCommand);
			return;
		}
		
		//show the modal
		QueryBox* modal = new QueryBox("Are you sure?", "Are you sure you would like to close this dialog without saving.\nAny unsaved data will be lost!", this);
		modal->MoveToCenterOfScreen();
		modal->Activate();
		modal->DoModal();
		modal->SetOKCommand(new KeyValues("Command", "command", COMMAND_APPLY_PAGE_SETTINGS_CONFIRM));
		modal->SetCancelCommand(new KeyValues("Command", "command", "SetModal"));
		return;
	}
	else if (!Q_stricmp(pszCommand, "SetModal"))
	{
		int x, y;
		GetPos(x, y);
		DoModal();
		SetPos(x, y);
		return;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_APPLY_PAGE_SETTINGS_CONFIRM))
	{
		//post message
		BaseClass::OnCommand("Close");
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the panel closes
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnClose()
{
	PostActionSignal(new KeyValues("MapPropertiesPanelClosed"));
	BaseClass::OnClose();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnTextChanged(KeyValues* data)
{
	//check for skybox combo box
	if (data->GetPtr("panel") == m_SkyboxNames)
	{
		//get the image
		char image[512];
		FormatImage(m_SkyboxNames->GetActiveItemUserData()->GetName(), image, sizeof(image));

		//set the image
		m_SkyboxImage->SetImage(image);

		//set the sv_skyname convar
		ConVarRef("sv_skyname").SetValue(m_SkyboxNames->GetActiveItemUserData()->GetName());
	}

	//check for epic filter combo box
	else if (data->GetPtr("panel") == m_filterComboBox)
	{
		//set amod_filter_filename
		ConVarRef amod_filter_filename(m_bNightTimeMode ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename");
		amod_filter_filename.SetValue(m_filterComboBox->GetActiveItemUserData()->GetName());
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a slider gets moved
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnSliderMoved(KeyValues* data)
{
	//check to see if we havnt already changed anything
	if (!m_bChangedAnything && m_bCanChange)
	{
		m_bChangedAnything = true;

		//add * to the end of the title
		char title[512];
		_title->GetText(title, sizeof(title));

		//set
		int index = min(Q_strlen(title), 510);
		title[index] = '*';
		title[index + 1] = '\0';

		SetTitle(title, true);
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnColorSelected(KeyValues* data)
{
	//check to see if we havnt already changed anything
	if (!m_bChangedAnything && m_bCanChange)
	{
		m_bChangedAnything = true;

		//add * to the end of the title
		char title[512];
		_title->GetText(title, sizeof(title));

		//set
		int index = min(Q_strlen(title), 510);
		title[index] = '*';
		title[index+1] = '\0';

		SetTitle(title, true);
	}

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
		m_CloudColors = color;
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

	//set these convars
	engine->ClientCmd("amod_enable_god 1");
	engine->ClientCmd("amod_clouds 1");
	engine->ClientCmd("amod_clouds_color_override 1");

	m_bNightTimeMode = IsNightPage;

	//FOG
	{
		CUtlVector<MapTimeInfo_t::FogInfo_t>& FogInfo = IsNightPage ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;

		//check buttons
		m_OverrideFog->SetSelected(IsNightPage ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled);
		m_EnableFog->SetSelected(atoi(FindFogInfoFromArray(FogInfo, "fog_enable", "0")));
		m_EnableSkyboxFog->SetSelected(atoi(FindFogInfoFromArray(FogInfo, "fog_enableskybox", "0")));
		m_PixelFog->SetSelected(atoi(FindFogInfoFromArray(FogInfo, "r_pixelfog", "1")));

		//get the fog colors
		{
			int fogcolors[3] = { 0,0,0 };
			UTIL_StringToIntArray(fogcolors, 3, FindFogInfoFromArray(FogInfo, "fog_color"));
			m_FogColor.SetColor(fogcolors[0], fogcolors[1], fogcolors[2], 255);
		}
		
		{
			int fogcolors[3] = { 0,0,0 };
			UTIL_StringToIntArray(fogcolors, 3, FindFogInfoFromArray(FogInfo, "fog_color"));
			m_FogSkyboxColor.SetColor(fogcolors[0], fogcolors[1], fogcolors[2], 255);
		}

		//get the other fog values
		m_FogStartSlider->SetValue(atoi(FindFogInfoFromArray(FogInfo, "fog_start", "500")));
		m_FogEndSlider->SetValue(atoi(FindFogInfoFromArray(FogInfo, "fog_end", "2000")));
		m_SkyboxFogStartSlider->SetValue(atoi(FindFogInfoFromArray(FogInfo, "fog_startskybox", "10000")));
		m_SkyboxFogEndSlider->SetValue(atoi(FindFogInfoFromArray(FogInfo, "fog_endskybox", "35000")));
		m_FogDensitySlider->SetValue(atof(FindFogInfoFromArray(FogInfo, "fog_maxdensity", "1.0")) * 100);
		m_SkyboxFogDensitySlider->SetValue(atof(FindFogInfoFromArray(FogInfo, "fog_maxdensityskybox", "1.0")) * 100);
	}

	//SKYBOX
	{
		const char* skybox = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.DefaultNightSky) : StringFromMapTimeStringTableIndex(info.DayInfo.DefaultDaySky);

		//get the image
		char image[512];
		FormatImage(skybox, image, sizeof(image));

		//set the image
		m_SkyboxImage->SetImage(image);

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

	//POST PROCESSING
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
			m_filterComboBox->AddItem(first, new KeyValues(cc_string));

			//add to combo box
			if (!Q_stricmp(cc_string, filtername))
				index = current;

			first = filesystem->FindNext(find);

			current++;
		}

		//active the current index
		m_filterComboBox->ActivateItem(index);

		filesystem->FindClose(find);

		//set the slider
		float filtervalue = atof(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.FilterIntensity : info.DayInfo.FilterIntensity));
		m_IntensitySlider->SetValue(int(filtervalue * 100));
	}

	//CLOUDS

	{
		int fogcolors[3] = { 0,0,0 };
		UTIL_StringToIntArray(fogcolors, 3, StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.CloudsColor : info.DayInfo.CloudsColor));
		m_CloudColors.SetColor(fogcolors[0], fogcolors[1], fogcolors[2], 255);
	}

	m_bChangedAnything = false;
	m_bCanChange = false;
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
		CUtlVector<MapTimeInfo_t::FogInfo_t>& FogInfo = m_bNightTimeMode ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;
		FogInfo.RemoveAll();

		//set the enabled state
		if (!m_OverrideFog->IsSelected())
		{
			m_bNightTimeMode ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled = false;
		}
		else
		{
			m_bNightTimeMode ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled = true;
		}

		//if the fog isnt enabled then clear the fog array
		if (!m_OverrideFog->IsSelected())
		{
			FogInfo.RemoveAll();
			break;
		}

		//set the info
		MapTimeInfo_t::FogInfo_t fog_enable{ StringToMapTimeStringTableIndex("fog_enable"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableFog->IsSelected()))};
		MapTimeInfo_t::FogInfo_t fog_enableskybox{ StringToMapTimeStringTableIndex("fog_enableskybox"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableSkyboxFog->IsSelected()))};
		MapTimeInfo_t::FogInfo_t fog_start{ StringToMapTimeStringTableIndex("fog_start"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_FogStartSlider->GetValue()))};
		MapTimeInfo_t::FogInfo_t fog_end{ StringToMapTimeStringTableIndex("fog_end"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_FogEndSlider->GetValue()))};
		MapTimeInfo_t::FogInfo_t fog_startskybox{ StringToMapTimeStringTableIndex("fog_startskybox"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_FogStartSlider->GetValue()))};
		MapTimeInfo_t::FogInfo_t fog_endskybox{ StringToMapTimeStringTableIndex("fog_endskybox"),StringToMapTimeStringTableIndex(CFmtStr("%d", m_FogEndSlider->GetValue() / 100)) };
		MapTimeInfo_t::FogInfo_t fog_maxdensity{ StringToMapTimeStringTableIndex("fog_maxdensity"),StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_FogDensitySlider->GetValue() / 100))};
		MapTimeInfo_t::FogInfo_t fog_maxdensityskybox{ StringToMapTimeStringTableIndex("fog_maxdensityskybox"),StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_SkyboxFogDensitySlider->GetValue() / 100))};
		MapTimeInfo_t::FogInfo_t fog_color{ StringToMapTimeStringTableIndex("fog_color"),StringToMapTimeStringTableIndex(CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b()))};
		MapTimeInfo_t::FogInfo_t fog_colorskybox{ StringToMapTimeStringTableIndex("fog_colorskybox"),StringToMapTimeStringTableIndex(CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b()))};

		FogInfo.AddToTail(fog_enable);
		FogInfo.AddToTail(fog_enableskybox);
		FogInfo.AddToTail(fog_start);
		FogInfo.AddToTail(fog_end);
		FogInfo.AddToTail(fog_startskybox);
		FogInfo.AddToTail(fog_endskybox);
		FogInfo.AddToTail(fog_maxdensity);
		FogInfo.AddToTail(fog_maxdensityskybox);
		FogInfo.AddToTail(fog_color);
		FogInfo.AddToTail(fog_colorskybox);

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
			info.NightInfo.FilterName = StringToMapTimeStringTableIndex(m_filterComboBox->GetActiveItemUserData()->GetName());
			info.NightInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_IntensitySlider->GetValue() / 100));
		}
		else
		{
			info.DayInfo.FilterName = StringToMapTimeStringTableIndex(m_filterComboBox->GetActiveItemUserData()->GetName());
			info.DayInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_IntensitySlider->GetValue() / 100));
		}

	} while (false);

	//set the clouds color
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColors.r(), m_CloudColors.g(), m_CloudColors.b(), m_CloudColors.a()));
		}
		else
		{
			info.DayInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColors.r(), m_CloudColors.g(), m_CloudColors.b(), m_CloudColors.a()));
		}

	} while (false);
}







#define MODIFY_MAP_PREFIX "OpenModifyMapWindow"

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

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties editor night page.
//----------------------------------------------------------------------------------------------------
CMapPropertiesEditorPageBase::CMapPropertiesEditorPageBase(Panel* parent, const char* name, bool IsNightPage) : BaseClass(parent, name)
{
	//set settings
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);

	m_bIsNightPage = IsNightPage;

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

#include "ienginevgui.h"

//command to open the map time properties editor
CON_COMMAND(open_map_time_properties_editor, "")
{
	//create the panel if not already here
	if (!s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel = new CMapPropertiesEditorPanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	s_MapPropertiesEditorPanel->Activate();
}