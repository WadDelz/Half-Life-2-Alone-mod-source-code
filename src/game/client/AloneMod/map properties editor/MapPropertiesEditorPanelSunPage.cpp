#include "cbase.h"
#include "MapPropertiesEditorPanelSunPage.h"


//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the sun page
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelSunPage::CMapPropertiesPanelSunPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/SunPage.res")
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
	m_SunColorButton->SetAttatchedColor(&m_SunColor);
	m_SunColorButton->SetPasteCommand("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d");
	m_SunColor.SetColor(255, 255, 255, 255);

	//sun material text entry
	m_SunMaterialTextEntry = new TextEntry(this, "MaterialTextEntry");
	m_SunMaterialTextEntry->AddActionSignalTarget(this);
	m_SunMaterialTextEntry->SetMaximumCharCount(255);

	//sun overlay size
	m_SunOverlaySizeSlider = new CMapPropertiesPanelSlider(this, "SunOverlaySizeSlider", 1);
	m_SunOverlaySizeLabel = new Label(this, "SunOverlaySizeLabel", "");

	//sun overlay color
	m_SunOverlayColorButton = new CMapPropertiesPanelButton(this, "SunOverlayColorButton", "");
	m_SunOverlayColorButton->SetCommand(COMMAND_CHANGE_SUN_OVERLAY_COLOR);
	m_SunOverlayColorButton->SetAttatchedColor(&m_SunOverlayColor);
	m_SunOverlayColorButton->SetPasteCommand("_amod_mapedit_server_sun_keyvalue overlaycolor \"%d %d %d");
	m_SunOverlayColor.SetColor(255, 255, 255, 255);

	//sun overlay material text entry
	m_SunOverlayMaterialTextEntry = new TextEntry(this, "OverlayMaterialTextEntry");
	m_SunOverlayMaterialTextEntry->AddActionSignalTarget(this);
	m_SunOverlayMaterialTextEntry->SetMaximumCharCount(255);

	//pitch to eyes button
	m_PitchToEyesButton = new CMapPropertiesPanelButton(this, "PitchToEyesButton", "");
	m_PitchToEyesButton->SetCommand(COMMAND_PITCH_TO_EYES);

	//perform layout to set the range sliders and such
	PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::Update()
{
	m_SunPitchLabel->SetText(CFmtStr("%d", m_SunPitchSlider->GetValue()));
	m_SunYawLabel->SetText(CFmtStr("%d", m_SunYawSlider->GetValue()));
	m_SunSizeLabel->SetText(CFmtStr("%d", m_SunSizeSlider->GetValue()));
	m_SunOverlaySizeLabel->SetText(CFmtStr("%d", m_SunOverlaySizeSlider->GetValue()));

	//set enabled states
	m_SunPitchSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunYawSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_PitchToEyesButton->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunSizeSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunColorButton->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunMaterialTextEntry->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunOverlaySizeSlider->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunOverlayColorButton->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_SunOverlayMaterialTextEntry->SetEnabled(!m_bNightTimeMode && m_EnableSunButton->IsSelected());
	m_EnableSunButton->SetEnabled(!m_bNightTimeMode);

	BaseClass::Update();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel paint
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_SunColor);
	surface()->DrawFilledRect(m_SunColorDrawRect.x, m_SunColorDrawRect.y, m_SunColorDrawRect.x + m_SunColorDrawRect.width, m_SunColorDrawRect.y + m_SunColorDrawRect.height);

	//draw the colors
	surface()->DrawSetColor(m_SunOverlayColor);
	surface()->DrawFilledRect(m_SunOverlayColorDrawRect.x, m_SunOverlayColorDrawRect.y, m_SunOverlayColorDrawRect.x + m_SunOverlayColorDrawRect.width, m_SunOverlayColorDrawRect.y + m_SunOverlayColorDrawRect.height);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel layout set
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::PerformLayout()
{
	BaseClass::PerformLayout();

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

	//get the value for the sun color draw rect
	sscanf(m_KeyValuesFile->GetString("SunColorRect"), "%d %d %d %d", &m_SunColorDrawRect.x, &m_SunColorDrawRect.y, &m_SunColorDrawRect.width, &m_SunColorDrawRect.height);

	//sun overlay size
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunOverlaySizeSlider"), m_SunOverlaySizeSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunOverlaySizeText"), m_SunOverlaySizeLabel);

	//sun overlay color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunOverlayColorButton"), m_SunOverlayColorButton);

	//sun overlay material
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SunOverlayMaterialTextEntry"), m_SunOverlayMaterialTextEntry);

	//get the value for the sun overlay color draw rect
	sscanf(m_KeyValuesFile->GetString("SunOverlayColorRect"), "%d %d %d %d", &m_SunOverlayColorDrawRect.x, &m_SunOverlayColorDrawRect.y, &m_SunOverlayColorDrawRect.width, &m_SunOverlayColorDrawRect.height);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a slider is moved
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::OnSliderMoved(KeyValues* data)
{
	//check for sun slider
	if (data->GetPtr("panel") == m_SunPitchSlider || data->GetPtr("panel") == m_SunYawSlider || data->GetPtr("panel") == m_SunSizeSlider || data->GetPtr("panel") == m_SunOverlaySizeSlider)
	{
		//call _amod_mapedit_server_sun_keyvalue
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue pitch %d", m_SunPitchSlider->GetValue()));
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue angle %d", m_SunYawSlider->GetValue()));
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue size %d", m_SunSizeSlider->GetValue()));
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaysize %d", m_SunOverlaySizeSlider->GetValue()));
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
void CMapPropertiesPanelSunPage::OnCommand(const char* pszCommand)
{
	//check for sun commands
	if (!Q_stricmp(pszCommand, COMMAND_CHANGE_SUN_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_SunPage_ColorPicker_SunColor", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_SunColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Sun;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_SUN_OVERLAY_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_SunPage_ColorPicker_SunOverlayColor", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_SunOverlayColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_SunOverlay;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_SUN_ACTIVATE))
	{
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun %d", m_EnableSunButton->IsSelected()));

		if (m_EnableSunButton->IsSelected())
		{
			//reset the keyvalues
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue pitch %d", m_SunPitchSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue angle %d", m_SunYawSlider->GetValue()));


			//size
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue size %d", m_SunSizeSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));

			//get the material
			char text[256];
			m_SunMaterialTextEntry->GetText(text, sizeof(text));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue material %s", text));


			//overlay size
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaysize %d", m_SunSizeSlider->GetValue()));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaycolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));

			//get the overlay material
			text[256];
			m_SunMaterialTextEntry->GetText(text, sizeof(text));
			engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaymaterial %s", text));
		}
		return;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_PITCH_TO_EYES))
	{
		//set the values of the sliders
		CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		//add undo steps
		AddUndo_SetSlider(m_SunPitchSlider, m_SunPitchSlider->GetValue());
		AddUndo_SetSlider(m_SunYawSlider, m_SunYawSlider->GetValue());

		//get the angle
		QAngle ang = pPlayer->GetAbsAngles();
		m_SunPitchSlider->SetValue(ang.x);
		m_SunYawSlider->SetValue(OppositeAngle(ang.y));			//these should change the values
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::OnTextChanged(KeyValues* data)
{
	//check for sun material
	if (data->GetPtr("panel") == m_SunMaterialTextEntry)
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

	//check for sun overlay material
	else if (data->GetPtr("panel") == m_SunOverlayMaterialTextEntry)
	{
		//get the text
		char text[256];
		m_SunOverlayMaterialTextEntry->GetText(text, sizeof(text));

		//check for the material first
		if (!CMaterialReference(text).IsValid() || CMaterialReference(text)->IsErrorMaterial())
		{
			//copy the default texture in
			Q_snprintf(text, sizeof(text), "sprites/light_glow02_add_noz.spr");
		}

		//send to server
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaymaterial %s", text));
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::OnColorSelected(KeyValues* data)
{
	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));
	switch (m_ColorSelectorMode)
	{
	case ColorSelectorMode::Color_Sun:
		//add an undo step
		AddUndo_SetColor(&m_SunColor, m_SunColor._color, "_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d");

		m_SunColor = color;
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));
		break;
	case ColorSelectorMode::Color_SunOverlay:
		//add an undo step
		AddUndo_SetColor(&m_SunOverlayColor, m_SunOverlayColor._color, "_amod_mapedit_server_sun_keyvalue overlaycolor \"%d %d %d");

		m_SunOverlayColor = color;
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue overlaycolor \"%d %d %d", m_SunOverlayColor.r(), m_SunOverlayColor.g(), m_SunOverlayColor.b()));
		break;
	}

	//call base func
	BaseClass::OnColorSelected(data);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the data
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::InitSunInfo(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

	//sun
	if (!IsNightPage)
	{
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun %d", info.DayInfo.SunInfoEnabled));
		m_EnableSunButton->SetSelected(info.DayInfo.SunInfoEnabled);

		//check for an 'angles' key. If not found then we will have to use the 'angle' and 'pitch' keys
		const char* angles = FindSunInfoFromArray(info.DayInfo.SunInfo, "angles", nullptr);
		if (angles)
		{
			//suck out the value
			int _, y;
			sscanf(angles, "%d %d", &_, &y);

			m_SunPitchSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "pitch", "90")));
			m_SunYawSlider->SetValue(y);

		}
		else
		{
			m_SunYawSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "angle", "0")));
			m_SunPitchSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "pitch", "90")));
		}

		//set the size
		m_SunSizeSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "size", "10")));

		//set color
		int r, g, b;
		sscanf(FindSunInfoFromArray(info.DayInfo.SunInfo, "rendercolor", "255 255 255"), "%d %d %d", &r, &g, &b);
		m_SunColor.SetColor(r, g, b, 255);

		//set the material
		m_SunMaterialTextEntry->SetText(FindSunInfoFromArray(info.DayInfo.SunInfo, "material", "sprites/light_glow02_add_noz.spr"));


		//set the overlay size
		m_SunOverlaySizeSlider->SetValue(atoi(FindSunInfoFromArray(info.DayInfo.SunInfo, "overlaysize", "-1")));

		//set overlay color
		int or , og, ob;
		sscanf(FindSunInfoFromArray(info.DayInfo.SunInfo, "overlaycolor", "255 255 255"), "%d %d %d", &or , &og, &ob);

		//check for -1
		m_SunOverlayColor.SetColor(or == -1 ? r : or , og == -1 ? g : og, ob == -1 ? b : ob, 255);

		//set the material
		m_SunOverlayMaterialTextEntry->SetText(FindSunInfoFromArray(info.DayInfo.SunInfo, "overlaymaterial", "sprites/light_glow02_add_noz.spr"));
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the sun info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSunPage::GetSunInfo(MapTimeInfo_t& info)
{
	//set the sun info
	do
	{
		//is this currently the night panel
		if (m_bNightTimeMode)
			break;

		//set info.DayInfo.SunInfoEnabled
		info.DayInfo.SunInfoEnabled = m_EnableSunButton->IsSelected();

		//set our angle
		const char* angles = FindSunInfoFromArray(info.DayInfo.SunInfo, "angles", nullptr);
		if (angles)
		{
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "angles", CFmtStr("0 %d 0", m_SunYawSlider->GetValue()));
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "pitch", CFmtStr("%d", m_SunPitchSlider->GetValue()));
		}
		else
		{
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "pitch", CFmtStr("%d", m_SunPitchSlider->GetValue()));
			AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "angle", CFmtStr("%d", m_SunYawSlider->GetValue()));
		}

		//set yaw

		//set size
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "size", CFmtStr("%d", m_SunSizeSlider->GetValue()));

		//set color
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "rendercolor", CFmtStr("%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));

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


		//set overlay size
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "overlaysize", CFmtStr("%d", m_SunOverlaySizeSlider->GetValue()));

		//set overlay color
		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "overlaycolor", CFmtStr("%d %d %d", m_SunOverlayColor.r(), m_SunOverlayColor.g(), m_SunOverlayColor.b()));

		//set material
		text[256];
		m_SunOverlayMaterialTextEntry->GetText(text, sizeof(text));

		//check for the material first
		if (!CMaterialReference(text).IsValid() || CMaterialReference(text)->IsErrorMaterial())
		{
			//copy the default texture in
			Q_snprintf(text, sizeof(text), "sprites/light_glow02_add_noz.spr");
		}

		AddOrUpdateSunInfoInArray(info.DayInfo.SunInfo, "overlaymaterial", text);

	} while (false);
}