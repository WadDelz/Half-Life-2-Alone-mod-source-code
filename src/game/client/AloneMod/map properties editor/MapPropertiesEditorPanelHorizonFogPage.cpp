#include "cbase.h"
#include "MapPropertiesEditorPanelHorizonFogPage.h"

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties fog page
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelHorizonFogPage::CMapPropertiesPanelHorizonFogPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/HorizonPage.res")
{
	//override button
	m_EnableHorizonButton = new CheckButton(this, "EnableHorizonCheckButton", "");

	//clip through 3d sky button
	m_ShouldHorizonClip3dSkybox = new CheckButton(this, "ShouldHorizonClip3dSkybox", "");

	//pitch/yaw sliders
	m_HorizonPitchSlider = new CMapPropertiesPanelSlider(this, "HorizonPitchSlider");
	m_HorizonPitchText = new Label(this, "HorizonPitchText", "");

	m_HorizonYawSlider = new CMapPropertiesPanelSlider(this, "HorizonPitchSlider");
	m_HorizonYawText = new Label(this, "HorizonYawText", "");

	//wide + tall slider
	m_HorizonWideSlider = new CMapPropertiesPanelSlider(this, "HorizonWideSlider", 10);
	m_HorizonWideText = new Label(this, "HorizonWideText", "");

	m_HorizonTallSlider = new CMapPropertiesPanelSlider(this, "HorizonTallSlider");
	m_HorizonTallText = new Label(this, "HorizonTallText", "");
	
	//offset sliders
	m_HorizonOffsetXSlider = new CMapPropertiesPanelSlider(this, "HorizonOffsetXSlider", 25);
	m_HorizonOffsetXText = new Label(this, "HorizonOffsetXText", "");
	
	m_HorizonOffsetYSlider = new CMapPropertiesPanelSlider(this, "HorizonOffsetYSlider", 25);
	m_HorizonOffsetYText = new Label(this, "HorizonOffsetYText", "");

	m_HorizonOffsetZSlider = new CMapPropertiesPanelSlider(this, "HorizonOffsetZSlider", 25);
	m_HorizonOffsetZText = new Label(this, "HorizonOffsetZText", "");

	//scale slider
	m_HorizonScaleSlider = new CMapPropertiesPanelSlider(this, "HorizonScaleSlider", 10);
	m_HorizonScaleText = new Label(this, "HorizonScaleText", "");

	//set horizon fog top color
	m_SetHorizonTopColorButton = new CMapPropertiesPanelButton(this, "SetHorizonTopColorBUtton", "");
	m_SetHorizonTopColorButton->SetCommand(COMMAND_HORIZON_SET_TOP_COLOR);
	m_SetHorizonTopColorButton->SetAttatchedColor(&m_HorizonTopColor);
	m_HorizonTopColor.SetColor(128, 179, 255, 255);

	//set horizon fog middle color
	m_SetHorizonMiddleColorButton = new CMapPropertiesPanelButton(this, "SetHorizonMiddleColorButton", "");
	m_SetHorizonMiddleColorButton->SetCommand(COMMAND_HORIZON_SET_MIDDLE_COLOR);
	m_SetHorizonMiddleColorButton->SetAttatchedColor(&m_HorizonMiddleColor);
	m_HorizonMiddleColor.SetColor(255, 128, 51, 255);

	//set horizon fog bottom color
	m_SetHorizonBottomColorButton = new CMapPropertiesPanelButton(this, "SetHorizonBottomColorButton", "");
	m_SetHorizonBottomColorButton->SetCommand(COMMAND_HORIZON_SET_BOTTOM_COLOR);
	m_SetHorizonBottomColorButton->SetAttatchedColor(&m_HorizonBottomColor);
	m_HorizonBottomColor.SetColor(26, 26, 13, 255);

	//perform layout to set the range sliders and such
	PerformLayout();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs layout for this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::PerformLayout()
{
	BaseClass::PerformLayout();

	//override button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("EnableHorizonCheckButton"), m_EnableHorizonButton);

	//should clip through 3d sky button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("ShouldHorizonClip3dSkybox"), m_ShouldHorizonClip3dSkybox);

	//pitch/yaw sliders
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonPitchSlider"), m_HorizonPitchSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonPitchText"), m_HorizonPitchText);

	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonYawSlider"), m_HorizonYawSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonYawText"), m_HorizonYawText);

	//wide + tall slider
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonWideSlider"), m_HorizonWideSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonWideText"), m_HorizonWideText);
	
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonTallSlider"), m_HorizonTallSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonTallText"), m_HorizonTallText);

	//offset sliders
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetXSlider"), m_HorizonOffsetXSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetXText"), m_HorizonOffsetXText);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetYSlider"), m_HorizonOffsetYSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetYText"), m_HorizonOffsetYText);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetZSlider"), m_HorizonOffsetZSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonOffsetZText"), m_HorizonOffsetZText);

	//horizon scale
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonScaleSlider"), m_HorizonScaleSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonScaleText"), m_HorizonScaleText);

	//horizon top color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonTopColorButton"), m_SetHorizonTopColorButton);
	sscanf(m_KeyValuesFile->GetString("HorizonTopColorRect"), "%d %d %d %d", &m_HorizonTopColorRect.x, &m_HorizonTopColorRect.y, &m_HorizonTopColorRect.width, &m_HorizonTopColorRect.height);

	//horizon middle color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonMiddleColorButton"), m_SetHorizonMiddleColorButton);
	sscanf(m_KeyValuesFile->GetString("HorizonMiddleColorRect"), "%d %d %d %d", &m_HorizonMiddleColorRect.x, &m_HorizonMiddleColorRect.y, &m_HorizonMiddleColorRect.width, &m_HorizonMiddleColorRect.height);
	
	//horizon bottom color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("HorizonBottomColorButton"), m_SetHorizonBottomColorButton);
	sscanf(m_KeyValuesFile->GetString("HorizonBottomColorRect"), "%d %d %d %d", &m_HorizonBottomColorRect.x, &m_HorizonBottomColorRect.y, &m_HorizonBottomColorRect.width, &m_HorizonBottomColorRect.height);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::Update()
{
	//get the convars
	static ConVar* r_horizonfog = cvar->FindVar("r_horizonfog");
	static ConVar* r_horizonfog_no3dskyclip = cvar->FindVar("r_horizonfog_no3dskyclip");
	static ConVar* r_horizonfog_pitch = cvar->FindVar("r_horizonfog_pitch");
	static ConVar* r_horizonfog_yaw = cvar->FindVar("r_horizonfog_yaw");
	static ConVar* r_horizonfog_height = cvar->FindVar("r_horizonfog_height");
	static ConVar* r_horizonfog_width = cvar->FindVar("r_horizonfog_width");
	static ConVar* r_horizonfog_offset_x = cvar->FindVar("r_horizonfog_offset_x");
	static ConVar* r_horizonfog_offset_y = cvar->FindVar("r_horizonfog_offset_y");
	static ConVar* r_horizonfog_offset_z = cvar->FindVar("r_horizonfog_offset_z");
	static ConVar* r_horizonfog_scale = cvar->FindVar("r_horizonfog_scale");
	static ConVar* r_horizonfog_top_r = cvar->FindVar("r_horizonfog_top_r");
	static ConVar* r_horizonfog_top_g = cvar->FindVar("r_horizonfog_top_g");
	static ConVar* r_horizonfog_top_b = cvar->FindVar("r_horizonfog_top_b");
	static ConVar* r_horizonfog_mid_r = cvar->FindVar("r_horizonfog_mid_r");
	static ConVar* r_horizonfog_mid_g = cvar->FindVar("r_horizonfog_mid_g");
	static ConVar* r_horizonfog_mid_b = cvar->FindVar("r_horizonfog_mid_b");
	static ConVar* r_horizonfog_bot_r = cvar->FindVar("r_horizonfog_bot_r");
	static ConVar* r_horizonfog_bot_g = cvar->FindVar("r_horizonfog_bot_g");
	static ConVar* r_horizonfog_bot_b = cvar->FindVar("r_horizonfog_bot_b");

	//set the convars
	r_horizonfog->SetValue(m_EnableHorizonButton->IsSelected());
	r_horizonfog_no3dskyclip->SetValue(!m_ShouldHorizonClip3dSkybox->IsSelected());
	r_horizonfog_pitch->SetValue(m_HorizonPitchSlider->GetValue());
	r_horizonfog_yaw->SetValue(m_HorizonYawSlider->GetValue());
	r_horizonfog_width->SetValue(m_HorizonWideSlider->GetValue());
	r_horizonfog_height->SetValue((float)m_HorizonTallSlider->GetValue() / HORIZON_TALL_DIVISOR);
	r_horizonfog_offset_x->SetValue(m_HorizonOffsetXSlider->GetValue());
	r_horizonfog_offset_y->SetValue(m_HorizonOffsetYSlider->GetValue());
	r_horizonfog_offset_z->SetValue(m_HorizonOffsetZSlider->GetValue());
	r_horizonfog_scale->SetValue((float)m_HorizonScaleSlider->GetValue() / HORIZON_SCALE_DIVISOR);
	r_horizonfog_top_r->SetValue((float)m_HorizonTopColor.r() / 255);
	r_horizonfog_top_g->SetValue((float)m_HorizonTopColor.g() / 255);
	r_horizonfog_top_b->SetValue((float)m_HorizonTopColor.b() / 255);
	r_horizonfog_mid_r->SetValue((float)m_HorizonMiddleColor.r() / 255);
	r_horizonfog_mid_g->SetValue((float)m_HorizonMiddleColor.g() / 255);
	r_horizonfog_mid_b->SetValue((float)m_HorizonMiddleColor.b() / 255);
	r_horizonfog_bot_r->SetValue((float)m_HorizonBottomColor.r() / 255);
	r_horizonfog_bot_g->SetValue((float)m_HorizonBottomColor.g() / 255);
	r_horizonfog_bot_b->SetValue((float)m_HorizonBottomColor.b() / 255);
		
	//set the enabled states
	m_ShouldHorizonClip3dSkybox->SetEnabled(r_horizonfog->GetBool());
	m_HorizonPitchSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonYawSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonWideSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonTallSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonOffsetXSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonOffsetYSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonOffsetZSlider->SetEnabled(r_horizonfog->GetBool());
	m_HorizonScaleSlider->SetEnabled(r_horizonfog->GetBool());
	m_SetHorizonTopColorButton->SetEnabled(r_horizonfog->GetBool());
	m_SetHorizonMiddleColorButton->SetEnabled(r_horizonfog->GetBool());
	m_SetHorizonBottomColorButton->SetEnabled(r_horizonfog->GetBool());

	
	//make the horizon pitch text
	wchar_t text[128];
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_PitchTextFormat"), m_HorizonPitchSlider->GetValue());
	m_HorizonPitchText->SetText(text);
	
	//make the horizon yaw text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_YawTextFormat"), m_HorizonYawSlider->GetValue());
	m_HorizonYawText->SetText(text);

	//make the horizon tall text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_TallTextFormat"), r_horizonfog_height->GetFloat());
	m_HorizonTallText->SetText(text);

	//make the horizon wide text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_WideTextFormat"), r_horizonfog_width->GetInt());
	m_HorizonWideText->SetText(text);

	//make the horizon x offset text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_OffsetXFormat"), r_horizonfog_offset_x->GetInt());
	m_HorizonOffsetXText->SetText(text);

	//make the horizon y offset text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_OffsetYFormat"), r_horizonfog_offset_y->GetInt());
	m_HorizonOffsetYText->SetText(text);

	//make the horizon z offset text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_OffsetZFormat"), r_horizonfog_offset_z->GetInt());
	m_HorizonOffsetZText->SetText(text);

	//make the horizon scale text
	memset(text, 0, sizeof(text));
	swprintf(text, SIZE_OF_ARRAY(text), g_pVGuiLocalize->Find("#MapProperties_HorizonPage_ScaleFormat"), r_horizonfog_scale->GetFloat());
	m_HorizonScaleText->SetText(text);

	BaseClass::Update();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Paints this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_HorizonTopColor);
	surface()->DrawFilledRect(m_HorizonTopColorRect.x, m_HorizonTopColorRect.y, m_HorizonTopColorRect.x + m_HorizonTopColorRect.width, m_HorizonTopColorRect.y + m_HorizonTopColorRect.height);

	//draw the colors
	surface()->DrawSetColor(m_HorizonMiddleColor);
	surface()->DrawFilledRect(m_HorizonMiddleColorRect.x, m_HorizonMiddleColorRect.y, m_HorizonMiddleColorRect.x + m_HorizonMiddleColorRect.width, m_HorizonMiddleColorRect.y + m_HorizonMiddleColorRect.height);

	//draw the colors
	surface()->DrawSetColor(m_HorizonBottomColor);
	surface()->DrawFilledRect(m_HorizonBottomColorRect.x, m_HorizonBottomColorRect.y, m_HorizonBottomColorRect.x + m_HorizonBottomColorRect.width, m_HorizonBottomColorRect.y + m_HorizonBottomColorRect.height);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::OnCommand(const char* pszCommand)
{
	//check for horizon top color command
	if (!Q_stricmp(pszCommand, COMMAND_HORIZON_SET_TOP_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_HorizonPage_ColorPicker_Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_HorizonTopColor);
		m_ColorPicker->SetDefaultColor(128, 179, 255, 255);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Top;
	}
	
	//check for horizon middle color command
	else if (!Q_stricmp(pszCommand, COMMAND_HORIZON_SET_MIDDLE_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_HorizonPage_ColorPicker_Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_HorizonMiddleColor);
		m_ColorPicker->SetDefaultColor(255, 128, 51, 255);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Middle;
	}

	//check for horizon bottom color command
	else if (!Q_stricmp(pszCommand, COMMAND_HORIZON_SET_BOTTOM_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_HorizonPage_ColorPicker_Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_HorizonBottomColor);
		m_ColorPicker->SetDefaultColor(26, 26, 13, 255);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Bottom;
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::OnColorSelected(KeyValues* data)
{
	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));
	switch (m_ColorSelectorMode)
	{
	case ColorSelectorMode::Color_Top:
		//add an undo step
		AddUndo_SetColor(&m_HorizonTopColor, m_HorizonTopColor._color);

		m_HorizonTopColor = color;
		break;
	case ColorSelectorMode::Color_Middle:
		//add an undo step
		AddUndo_SetColor(&m_HorizonMiddleColor, m_HorizonMiddleColor._color);

		m_HorizonMiddleColor = color;
		break;
	case ColorSelectorMode::Color_Bottom:
		//add an undo step
		AddUndo_SetColor(&m_HorizonBottomColor, m_HorizonBottomColor._color);

		m_HorizonBottomColor = color;
		break;
	}

	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	//call base func
	BaseClass::OnColorSelected(data);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the horizon info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::InitHorizonInfo(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

	//get the array
	CUtlVector<MapTimeInfo_t::FogInfo_t>& horizon_array = IsNightPage ? info.NightInfo.HorizonInfo : info.DayInfo.HorizonInfo;

	//set the enable button
	m_EnableHorizonButton->SetSelected(IsNightPage ? info.NightInfo.HorizonEnabled : info.DayInfo.HorizonEnabled);
	
	//set the sliders
	{
		//get the vars
		bool horizon_shouldclip = !atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_no3dskyclip", "1"));
		int horizon_pitch = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_pitch", "0"));
		int horizon_yaw = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_yaw", "0"));
		int horizon_width = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_width", "360"));
		int horizon_height = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_height", "1")) * HORIZON_TALL_DIVISOR);
		int horizon_offset_x = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_offset_x", "0"));
		int horizon_offset_y = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_offset_y", "0"));
		int horizon_offset_z = atoi(FindFogInfoFromArray(horizon_array, "r_horizonfog_offset_z", "0"));
		int horizon_scale = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_scale", "0.5")) * HORIZON_SCALE_DIVISOR);
		int horizon_color_top_r = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_top_r", "0.5")) * 255);
		int horizon_color_top_g = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_top_g", "0.7")) * 255);
		int horizon_color_top_b = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_top_b", "1.0")) * 255);
		int horizon_color_middle_r = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_mid_r", "1.0")) * 255);
		int horizon_color_middle_g = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_mid_g", "0.5")) * 255);
		int horizon_color_middle_b = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_mid_b", "0.2")) * 255);
		int horizon_color_bottom_r = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_bot_r", "0.1")) * 255);
		int horizon_color_bottom_g = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_bot_g", "0.1")) * 255);
		int horizon_color_bottom_b = (int)(atof(FindFogInfoFromArray(horizon_array, "r_horizonfog_bot_b", "0.05")) * 255);

		//clip through 3d world
		m_ShouldHorizonClip3dSkybox->SetSelected(horizon_shouldclip);

		//set pitch/yaw
		m_HorizonPitchSlider->SetValue(horizon_pitch);
		m_HorizonYawSlider->SetValue(horizon_yaw);

		//set the width + height
		m_HorizonWideSlider->SetValue(horizon_width);
		m_HorizonTallSlider->SetValue(horizon_height);

		//set the offsets
		m_HorizonOffsetXSlider->SetValue(horizon_offset_x);
		m_HorizonOffsetYSlider->SetValue(horizon_offset_y);
		m_HorizonOffsetZSlider->SetValue(horizon_offset_z);

		//set the scale
		m_HorizonScaleSlider->SetValue(horizon_scale);

		//set the colors
		m_HorizonTopColor.SetColor(horizon_color_top_r, horizon_color_top_g, horizon_color_top_b, 255);
		m_HorizonMiddleColor.SetColor(horizon_color_middle_r, horizon_color_middle_g, horizon_color_middle_b, 255);
		m_HorizonBottomColor.SetColor(horizon_color_bottom_r, horizon_color_bottom_g, horizon_color_bottom_b, 255);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the horizon info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelHorizonFogPage::GetHorizonInfo(MapTimeInfo_t& info)
{
	//set the horizon info
	do
	{
		//get and always clear the fog info
		CUtlVector<MapTimeInfo_t::FogInfo_t>& horizon_array = m_bNightTimeMode ? info.NightInfo.HorizonInfo : info.DayInfo.HorizonInfo;

		//set the enabled state
		(m_bNightTimeMode ? info.NightInfo.HorizonEnabled : info.DayInfo.HorizonEnabled) = m_EnableHorizonButton->IsSelected();

		//if the fog isnt enabled then just return
		if (!m_EnableHorizonButton->IsSelected())
		{
			AddOrUpdateFogInfoInArray(horizon_array, "r_horizonfog", "0");
			break;
		}

		//set the info
		MapTimeInfo_t::FogInfo_t HorizonInfos[18] = {
			//pitch/yaw
			{	StringToMapTimeStringTableIndex("r_horizonfog_no3dskyclip"),		StringToMapTimeStringTableIndex(CFmtStr("%d", !m_ShouldHorizonClip3dSkybox->IsSelected()))},

			//should clip
			{	StringToMapTimeStringTableIndex("r_horizonfog_pitch"),				StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonPitchSlider->GetValue()))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_yaw"),				StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonYawSlider->GetValue()))},

			//width/height
			{	StringToMapTimeStringTableIndex("r_horizonfog_width"),				StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonWideSlider->GetValue()))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_height"),				StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_HorizonTallSlider->GetValue() / HORIZON_TALL_DIVISOR))},

			//offsets
			{	StringToMapTimeStringTableIndex("r_horizonfog_offset_x"),			StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonOffsetXSlider->GetValue()))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_offset_y"),			StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonOffsetYSlider->GetValue()))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_offset_z"),			StringToMapTimeStringTableIndex(CFmtStr("%d", m_HorizonOffsetZSlider->GetValue()))},

			//scale
			{	StringToMapTimeStringTableIndex("r_horizonfog_scale"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonScaleSlider->GetValue() / HORIZON_SCALE_DIVISOR))},

			//colors
			{	StringToMapTimeStringTableIndex("r_horizonfog_top_r"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonTopColor.r() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_top_g"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonTopColor.g() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_top_b"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonTopColor.b() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_mid_r"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonMiddleColor.r() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_mid_g"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonMiddleColor.g() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_mid_b"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonMiddleColor.b() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_bot_r"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonBottomColor.r() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_bot_g"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonBottomColor.g() / 255))},
			{	StringToMapTimeStringTableIndex("r_horizonfog_bot_b"),				StringToMapTimeStringTableIndex(CFmtStr("%f", (float)m_HorizonBottomColor.b() / 255))},
		};

		//add to the array
		for (int i = 0; i < SIZE_OF_ARRAY(HorizonInfos); i++)
		{
			//i used to have it so i would clear the fog array then add all the HorizonInfos[i] to them. BUT i realized
			//you could possibly add any other convar into this array. These convars wouldnt show up in the panel BUT
			//would still be in the info. So what we are going to do is not clear the fog infos but instead modify
			//the already existing ones.
			AddOrUpdateFogInfoInArray(horizon_array, HorizonInfos[i].convar, HorizonInfos[i].value);
		}
	} while (false);
}