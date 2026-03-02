#include "cbase.h"
#include "MapPropertiesEditorPanelFogPage.h"

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties fog page
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelFogPage::CMapPropertiesPanelFogPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/FogPage.res")
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
	m_FogColorButton->SetAttatchedColor(&m_FogColor);
	m_FogColor.SetColor(255, 255, 255, 255);

	//fog skybox color
	m_FogSkyboxColorButton = new CMapPropertiesPanelButton(this, "FogSkyboxColorButton", "");
	m_FogSkyboxColorOverride = new CheckButton(this, "FogSkyboxColorOverrideButton", "");
	m_FogSkyboxColorButton->SetCommand(COMMAND_CHANGE_FOG_SKYBOX_COLOR);
	m_FogSkyboxColorButton->SetAttatchedColor(&m_FogSkyboxColor);
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

	//farz
	m_FarzClippingPlaneSlider = new CMapPropertiesPanelSlider(this, "FarzSlider");
	m_FarzClippingPlaneLabel = new Label(this, "FarzLabel", "");
	m_FarzClippingPlaneOverride = new CheckButton(this, "FarzOverride", "");

	//enable fog blend
	m_EnableFogBlendCheckButton = new CheckButton(this, "EnableFogBlendCheckButton", "");
	m_OverrideFogBlendCheckButton = new CheckButton(this, "OverrideFogBlendCheckButton", "");

	//fog blend color
	m_FogBlendColorButton = new CMapPropertiesPanelButton(this, "FogBlendColorButton", "");
	m_FogBlendColorOverride = new CheckButton(this, "FogBlendColorOverrideButton", "");
	m_FogBlendColorButton->SetCommand(COMMAND_CHANGE_FOG_BLEND_COLOR);
	m_FogBlendColorButton->SetAttatchedColor(&m_FogBlendColor);
	m_FogBlendColor.SetColor(255, 255, 255, 255);

	//fog blend angle
	m_FogBlendAngleSlider = new CMapPropertiesPanelSlider(this, "FogBlendAngleSlider");
	m_FogBlendAngleLabel = new Label(this, "FogBlendAngleLabel", "");
	m_FogBlendAngleOverride = new CheckButton(this, "FogBlendAngleOverride", "");


	//enable fog skybox blend
	m_EnableFogBlendSkyboxCheckButton = new CheckButton(this, "EnableFogBlendSkyboxCheckButton", "");
	m_OverrideFogBlendSkyboxCheckButton = new CheckButton(this, "OverrideFogBlendSkyboxCheckButton", "");

	//fog blend skybox color
	m_FogBlendSkyboxColorButton = new CMapPropertiesPanelButton(this, "FogBlendSkyboxColorButton", "");
	m_FogBlendSkyboxColorOverride = new CheckButton(this, "FogBlendSkyboxColorOverrideButton", "");
	m_FogBlendSkyboxColorButton->SetCommand(COMMAND_CHANGE_FOG_SKYBOX_BLEND_COLOR);
	m_FogBlendSkyboxColorButton->SetAttatchedColor(&m_FogBlendSkyboxColor);
	m_FogBlendSkyboxColor.SetColor(255, 255, 255, 255);

	//fog blend skybox angle
	m_FogBlendSkyboxAngleSlider = new CMapPropertiesPanelSlider(this, "FogBlendSkyboxAngleSlider");
	m_FogBlendSkyboxAngleLabel = new Label(this, "FogBlendSkyboxAngleLabel", "");
	m_FogBlendSkyboxAngleOverride = new CheckButton(this, "FogBlendSkyboxAngleOverride", "");

	//perform layout to set the range sliders and such
	PerformLayout();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs layout for this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::PerformLayout()
{
	BaseClass::PerformLayout();

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
	sscanf(m_KeyValuesFile->GetString("FogColorRect"), "%d %d %d %d", &m_FogColorDrawRect.x, &m_FogColorDrawRect.y, &m_FogColorDrawRect.width, &m_FogColorDrawRect.height);

	//fog skybox color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxColorButton"), m_FogSkyboxColorButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxColorOverrideButton"), m_FogSkyboxColorOverride);
	sscanf(m_KeyValuesFile->GetString("FogSkyboxColorRect"), "%d %d %d %d", &m_FogSkyboxColorDrawRect.x, &m_FogSkyboxColorDrawRect.y, &m_FogSkyboxColorDrawRect.width, &m_FogSkyboxColorDrawRect.height);

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

	//farz clipping plane
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FarzSlider"), m_FarzClippingPlaneSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FarzValueLabel"), m_FarzClippingPlaneLabel);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FarzOverride"), m_FarzClippingPlaneOverride);

	//fog blend
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendEnabled"), m_EnableFogBlendCheckButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("OverrideFogBlendEnabled"), m_OverrideFogBlendCheckButton);

	//fog blend color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendColorButton"), m_FogBlendColorButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendColorOverrideButton"), m_FogBlendColorOverride);
	sscanf(m_KeyValuesFile->GetString("FogBlendColorRect"), "%d %d %d %d", &m_FogBlendColorDrawRect.x, &m_FogBlendColorDrawRect.y, &m_FogBlendColorDrawRect.width, &m_FogBlendColorDrawRect.height);

	//fog blend angle
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendAngleSlider"), m_FogBlendAngleSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendAngleLabel"), m_FogBlendAngleLabel);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendAngleOverrideButton"), m_FogBlendAngleOverride);


	//fog skybox blend
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogSkyboxBlendEnabled"), m_EnableFogBlendSkyboxCheckButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("OverrideFogSkyboxBlendEnabled"), m_OverrideFogBlendSkyboxCheckButton);

	//fog skybox blend color
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendSkyboxColorButton"), m_FogBlendSkyboxColorButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendSkyboxColorOverrideButton"), m_FogBlendSkyboxColorOverride);
	sscanf(m_KeyValuesFile->GetString("FogSkyboxBlendColorRect"), "%d %d %d %d", &m_FogBlendSkyboxColorDrawRect.x, &m_FogBlendSkyboxColorDrawRect.y, &m_FogBlendSkyboxColorDrawRect.width, &m_FogBlendSkyboxColorDrawRect.height);

	//fog skybox blend angle
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendSkyboxAngleSlider"), m_FogBlendSkyboxAngleSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendSkyboxAngleLabel"), m_FogBlendSkyboxAngleLabel);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("FogBlendSkyboxAngleOverrideButton"), m_FogBlendSkyboxAngleOverride);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::Update()
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
	static ConVar* r_farz = cvar->FindVar("r_farz");
	static ConVar* fog_blend = cvar->FindVar("fog_blend");
	static ConVar* fog_blendcolor = cvar->FindVar("fog_blendcolor");
	static ConVar* fog_blendangle = cvar->FindVar("fog_blendangle");
	static ConVar* fog_blendskybox = cvar->FindVar("fog_blendskybox");
	static ConVar* fog_blendcolorskybox = cvar->FindVar("fog_blendcolorskybox");
	static ConVar* fog_blendangleskybox = cvar->FindVar("fog_blendangleskybox");

	//do we override
	bool _override = m_OverrideFogButton->IsSelected();

	//do we allow for overriding the blend?
	bool _overrideblend = _override && m_OverrideFogBlendCheckButton->IsSelected();

	//do we allow for overriding the skybox blend?
	bool _overrideskyboxblend = _override && m_OverrideFogBlendSkyboxCheckButton->IsSelected();

	//convars
	{
		//fog override
		fog_override->SetValue(_override);

		//fog enabled
		fog_enable->SetValue(!_override || !m_OverrideEnableFogCheckButton->IsSelected() ? -1 : m_EnableFogCheckButton->IsSelected());
		fog_enableskybox->SetValue(!_override || !m_OverrideEnableSkyboxFogCheckButton->IsSelected() ? -1 : m_EnableSkyboxFogCheckButton->IsSelected());

		//pixel fog
		r_pixelfog->SetValue(_override && !m_EnablePixelFogButton->IsSelected() ? false : true);

		//fog colors
		fog_color->SetValue(!_override || !m_FogColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b()));
		fog_colorskybox->SetValue(!_override || !m_FogSkyboxColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b()));


		//fog distances
		fog_start->SetValue(!_override || !m_FogStartOverride->IsSelected() ? -1 : m_FogStartSlider->GetValue());
		fog_end->SetValue(!_override || !m_FogEndOverride->IsSelected() ? -1 : m_FogEndSlider->GetValue());

		//fog skybox distances
		fog_startskybox->SetValue(!_override || !m_FogStartSkyboxOverride->IsSelected() ? -1 : m_FogStartSkyboxSlider->GetValue());
		fog_endskybox->SetValue(!_override || !m_FogEndSkyboxOverride->IsSelected() ? -1 : m_FogEndSkyboxSlider->GetValue());

		//fog density
		fog_maxdensity->SetValue(!_override || !m_FogDensityOverride->IsSelected() ? -1.0f : ((float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax()));
		fog_maxdensityskybox->SetValue(!_override || !m_FogSkyboxDensityOverride->IsSelected() ? -1.0f : ((float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax()));

		//farz
		r_farz->SetValue(!_override || !m_FarzClippingPlaneOverride->IsSelected() ? -1 : m_FarzClippingPlaneSlider->GetValue());

		//fog blend
		fog_blend->SetValue(!_overrideblend ? -1 : m_EnableFogBlendCheckButton->IsSelected());

		//fog blend colors
		fog_blendcolor->SetValue(!_overrideblend || !m_FogBlendColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogBlendColor.r(), m_FogBlendColor.g(), m_FogBlendColor.b()));

		//fog blend angle
		fog_blendangle->SetValue(!_overrideblend || !m_FogBlendAngleOverride->IsSelected() ? -1 : m_FogBlendAngleSlider->GetValue());

		//fog skybox blend
		fog_blendskybox->SetValue(!_overrideskyboxblend ? -1 : m_EnableFogBlendSkyboxCheckButton->IsSelected());

		//fog skybox blend colors
		fog_blendcolorskybox->SetValue(!_overrideskyboxblend || !m_FogBlendSkyboxColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogBlendSkyboxColor.r(), m_FogBlendSkyboxColor.g(), m_FogBlendSkyboxColor.b()));

		//fog skybox blend angle
		fog_blendangleskybox->SetValue(!_overrideskyboxblend || !m_FogBlendSkyboxAngleOverride->IsSelected() ? -1 : m_FogBlendSkyboxAngleSlider->GetValue());
	}

	//enable the items
	{
		//fog enable
		m_OverrideEnableFogCheckButton->SetEnabled(_override);
		m_OverrideEnableSkyboxFogCheckButton->SetEnabled(_override);
		m_EnableFogCheckButton->SetEnabled(_override && m_OverrideEnableFogCheckButton->IsSelected());
		m_EnableSkyboxFogCheckButton->SetEnabled(_override && m_OverrideEnableSkyboxFogCheckButton->IsSelected());

		//pixel fog
		m_EnablePixelFogButton->SetEnabled(_override);

		//enable the color buttons
		m_FogColorButton->SetEnabled(_override && m_FogColorOverride->IsSelected());
		m_FogSkyboxColorButton->SetEnabled(_override && m_FogSkyboxColorOverride->IsSelected());
		m_FogSkyboxColorOverride->SetEnabled(_override);
		m_FogColorOverride->SetEnabled(_override);

		//fog distances
		m_FogStartSlider->SetEnabled(_override && m_FogStartOverride->IsSelected());
		m_FogEndSlider->SetEnabled(_override && m_FogEndOverride->IsSelected());
		m_FogStartOverride->SetEnabled(_override);
		m_FogEndOverride->SetEnabled(_override);

		//fog skybox distances
		m_FogStartSkyboxSlider->SetEnabled(_override && m_FogStartSkyboxOverride->IsSelected());
		m_FogEndSkyboxSlider->SetEnabled(_override && m_FogEndSkyboxOverride->IsSelected());
		m_FogStartSkyboxOverride->SetEnabled(_override);
		m_FogEndSkyboxOverride->SetEnabled(_override);

		//density
		m_FogDensityOverride->SetEnabled(_override);
		m_FogSkyboxDensityOverride->SetEnabled(_override);
		m_FogDensitySlider->SetEnabled(_override && m_FogDensityOverride->IsSelected());
		m_FogSkyboxDensitySlider->SetEnabled(_override && m_FogSkyboxDensityOverride->IsSelected());

		//farz
		m_FarzClippingPlaneOverride->SetEnabled(_override);
		m_FarzClippingPlaneSlider->SetEnabled(_override && m_FarzClippingPlaneOverride->IsSelected());

		//fog blend
		m_OverrideFogBlendCheckButton->SetEnabled(_override);
		m_EnableFogBlendCheckButton->SetEnabled(_override && m_OverrideFogBlendCheckButton->IsSelected());

		//fog blend colors
		m_FogBlendColorOverride->SetEnabled(_overrideblend);
		m_FogBlendColorButton->SetEnabled(_overrideblend && m_FogBlendColorOverride->IsSelected());

		//fog blend angle
		m_FogBlendAngleOverride->SetEnabled(_overrideblend);
		m_FogBlendAngleSlider->SetEnabled(_overrideblend && m_FogBlendAngleOverride->IsSelected());


		//fog skybox blend
		m_OverrideFogBlendSkyboxCheckButton->SetEnabled(_override);
		m_EnableFogBlendSkyboxCheckButton->SetEnabled(_override && m_OverrideFogBlendSkyboxCheckButton->IsSelected());

		//fog blend colors
		m_FogBlendSkyboxColorOverride->SetEnabled(_overrideskyboxblend);
		m_FogBlendSkyboxColorButton->SetEnabled(_overrideskyboxblend && m_FogBlendSkyboxColorOverride->IsSelected());

		//fog blend angle
		m_FogBlendSkyboxAngleOverride->SetEnabled(_overrideskyboxblend);
		m_FogBlendSkyboxAngleSlider->SetEnabled(_overrideskyboxblend && m_FogBlendSkyboxAngleOverride->IsSelected());
	}

	//set the texts
	{
		//distances
		m_FogStartValueLabel->SetText(CFmtStr("%d", fog_start->GetInt()));
		m_FogEndValueLabel->SetText(CFmtStr("%d", fog_end->GetInt()));

		//skybox distances
		m_FogStartSkyboxValueLabel->SetText(CFmtStr("%d", fog_startskybox->GetInt()));
		m_FogEndSkyboxValueLabel->SetText(CFmtStr("%d", fog_endskybox->GetInt()));

		//densities
		m_FogDensityValueLabel->SetText(CFmtStr("%f", fog_maxdensity->GetFloat()));
		m_FogSkyboxDensityValueLabel->SetText(CFmtStr("%f", fog_maxdensityskybox->GetFloat()));

		//clipping planes
		m_FarzClippingPlaneLabel->SetText(CFmtStr("%d", r_farz->GetInt()));

		//fog blend angle
		m_FogBlendAngleLabel->SetText(CFmtStr("%d", fog_blendangle->GetInt()));

		//fog skybox angle
		m_FogBlendSkyboxAngleLabel->SetText(CFmtStr("%d", fog_blendangleskybox->GetInt()));
	}

	BaseClass::Update();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Paints this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_FogColor);
	surface()->DrawFilledRect(m_FogColorDrawRect.x, m_FogColorDrawRect.y, m_FogColorDrawRect.x + m_FogColorDrawRect.width, m_FogColorDrawRect.y + m_FogColorDrawRect.height);

	//draw the colors
	surface()->DrawSetColor(m_FogSkyboxColor);
	surface()->DrawFilledRect(m_FogSkyboxColorDrawRect.x, m_FogSkyboxColorDrawRect.y, m_FogSkyboxColorDrawRect.x + m_FogSkyboxColorDrawRect.width, m_FogSkyboxColorDrawRect.y + m_FogSkyboxColorDrawRect.height);

	//draw the colors
	surface()->DrawSetColor(m_FogBlendColor);
	surface()->DrawFilledRect(m_FogBlendColorDrawRect.x, m_FogBlendColorDrawRect.y, m_FogBlendColorDrawRect.x + m_FogBlendColorDrawRect.width, m_FogBlendColorDrawRect.y + m_FogBlendColorDrawRect.height);

	//draw the colors
	surface()->DrawSetColor(m_FogBlendSkyboxColor);
	surface()->DrawFilledRect(m_FogBlendSkyboxColorDrawRect.x, m_FogBlendSkyboxColorDrawRect.y, m_FogBlendSkyboxColorDrawRect.x + m_FogBlendSkyboxColorDrawRect.width, m_FogBlendSkyboxColorDrawRect.y + m_FogBlendSkyboxColorDrawRect.height);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::OnCommand(const char* pszCommand)
{
	//check for fog color command
	if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_FogPage_ColorPicker_Color", true);
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
		m_ColorPicker->SetTitle("#MapProperties_FogPage_ColorPicker_SkyboxColor", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogSkyboxColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_SkyboxFog;
	}

	//check for fog blend color command
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_BLEND_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_FogPage_ColorPicker_BlendTitle", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogBlendColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_BlendFog;
	}

	//check for fog blend skybox color command
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_SKYBOX_BLEND_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_FogPage_ColorPicker_SkyboxBlendTitle", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_FogBlendColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_BlendSkyboxFog;
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::OnColorSelected(KeyValues* data)
{
	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));;
	switch (m_ColorSelectorMode)
	{
	case ColorSelectorMode::Color_Fog:
		//add an undo step
		AddUndo_SetColor(&m_FogColor, m_FogColor._color);

		m_FogColor = color;
		break;
	case ColorSelectorMode::Color_SkyboxFog:
		//add an undo step
		AddUndo_SetColor(&m_FogSkyboxColor, m_FogSkyboxColor._color);

		m_FogSkyboxColor = color;
		break;
	case ColorSelectorMode::Color_BlendFog:
		//add an undo step
		AddUndo_SetColor(&m_FogBlendColor, m_FogBlendColor._color);

		m_FogBlendColor = color;
		break;
	case ColorSelectorMode::Color_BlendSkyboxFog:
		//add an undo step
		AddUndo_SetColor(&m_FogBlendSkyboxColor, m_FogBlendSkyboxColor._color);

		m_FogBlendSkyboxColor = color;
		break;
	}

	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	//call base func
	BaseClass::OnColorSelected(data);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::InitFogInfo(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

	//get the array
	CUtlVector<MapTimeInfo_t::FogInfo_t>& fog_array = IsNightPage ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;

	//init the fog check buttons
	{
		//get the enabled states
		int enabled = atoi(FindFogInfoFromArray(fog_array, "fog_enable", "-1"));
		int enabled_skybox = atoi(FindFogInfoFromArray(fog_array, "fog_enableskybox", "-1"));

		//override btton
		m_OverrideFogButton->SetSelected(IsNightPage ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled);

		//pixel fog
		m_EnablePixelFogButton->SetSelected(atoi(FindFogInfoFromArray(fog_array, "r_pixelfog", "1")) != 0);

		//enabled fog
		m_OverrideEnableFogCheckButton->SetSelected(enabled >= 0);
		m_EnableFogCheckButton->SetSelected(enabled > 0);

		//enabled skybox fog
		m_OverrideEnableSkyboxFogCheckButton->SetSelected(enabled_skybox >= 0);
		m_EnableSkyboxFogCheckButton->SetSelected(enabled_skybox > 0);
	}

	//init the fog sliders
	{
		//get the vars
		int fog_start = atoi(FindFogInfoFromArray(fog_array, "fog_start", "-1"));
		int fog_end = atoi(FindFogInfoFromArray(fog_array, "fog_end", "-1"));
		int fog_startskybox = atoi(FindFogInfoFromArray(fog_array, "fog_startskybox", "-1"));
		int fog_endskybox = atoi(FindFogInfoFromArray(fog_array, "fog_endskybox", "-1"));
		float fog_maxdensity = atof(FindFogInfoFromArray(fog_array, "fog_maxdensity", "-1"));
		float fog_maxdensityskybox = atof(FindFogInfoFromArray(fog_array, "fog_maxdensityskybox", "-1"));
		int r_farz = atoi(FindFogInfoFromArray(fog_array, "r_farz", "-1"));
		int fog_blend = atoi(FindFogInfoFromArray(fog_array, "fog_blend", "-1"));
		int fog_blendangle = atoi(FindFogInfoFromArray(fog_array, "fog_blendangle", "-1"));
		int fog_blendskybox = atoi(FindFogInfoFromArray(fog_array, "fog_blendskybox", "-1"));
		int fog_blendangleskybox = atoi(FindFogInfoFromArray(fog_array, "fog_blendangleskybox", "-1"));

		//distances
		m_FogStartOverride->SetSelected(fog_start != -1);
		m_FogEndOverride->SetSelected(fog_end != -1);
		m_FogStartSlider->SetValue(fog_start);
		m_FogEndSlider->SetValue(fog_end);

		//skybox distances
		m_FogStartSkyboxOverride->SetSelected(fog_startskybox != -1);
		m_FogEndSkyboxOverride->SetSelected(fog_endskybox != -1);
		m_FogStartSkyboxSlider->SetValue(fog_startskybox);
		m_FogEndSkyboxSlider->SetValue(fog_endskybox);

		//fog densities
		m_FogDensityOverride->SetSelected(fog_maxdensity >= 0);
		m_FogSkyboxDensityOverride->SetSelected(fog_maxdensityskybox >= 0);
		m_FogDensitySlider->SetValue((float)fog_maxdensity * m_FogDensitySlider->GetMax());
		m_FogSkyboxDensitySlider->SetValue((float)fog_maxdensityskybox * m_FogSkyboxDensitySlider->GetMax());

		//fog clipping plane
		m_FarzClippingPlaneOverride->SetSelected(r_farz != -1);
		m_FarzClippingPlaneSlider->SetValue(r_farz);

		//fog blend
		m_EnableFogBlendCheckButton->SetSelected(fog_blend > 0);
		m_OverrideFogBlendCheckButton->SetSelected(fog_blend != -1);

		//fog blend angle
		m_FogBlendAngleOverride->SetSelected(fog_blendangle != -1);
		m_FogBlendAngleSlider->SetValue(fog_blendangle);


		//fog skybox blend
		m_EnableFogBlendSkyboxCheckButton->SetSelected(fog_blendskybox > 0);
		m_OverrideFogBlendSkyboxCheckButton->SetSelected(fog_blendskybox != -1);

		//fog blend angle
		m_FogBlendSkyboxAngleOverride->SetSelected(fog_blendangleskybox != -1);
		m_FogBlendSkyboxAngleSlider->SetValue(fog_blendangleskybox);
	}

	//init the fog color buttons
	{
		//get the rgbs
		int r, g, b;

		//get the fog color
		sscanf(FindFogInfoFromArray(fog_array, "fog_color", "-1 -1 -1"), "%d %d %d", &r, &g, &b);
		m_FogColorOverride->SetSelected(r != -1 && g != -1 && b != -1);
		m_FogColor.SetColor(r, g, b, 255);

		//get the skybox fog color
		sscanf(FindFogInfoFromArray(fog_array, "fog_colorskybox", "-1 -1 -1"), "%d %d %d", &r, &g, &b);
		m_FogSkyboxColorOverride->SetSelected(r != -1 && g != -1 && b != -1);
		m_FogSkyboxColor.SetColor(r, g, b, 255);

		//get the fog blend color
		sscanf(FindFogInfoFromArray(fog_array, "fog_blendcolor", "-1 -1 -1"), "%d %d %d", &r, &g, &b);
		m_FogBlendColorOverride->SetSelected(r != -1 && g != -1 && b != -1);
		m_FogBlendColor.SetColor(r, g, b, 255);

		//get the fog skybox blend color
		sscanf(FindFogInfoFromArray(fog_array, "fog_blendcolorskybox", "-1 -1 -1"), "%d %d %d", &r, &g, &b);
		m_FogBlendSkyboxColorOverride->SetSelected(r != -1 && g != -1 && b != -1);
		m_FogBlendSkyboxColor.SetColor(r, g, b, 255);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogPage::GetFogInfo(MapTimeInfo_t& info)
{
	//set the fog info
	do
	{
		//get and always clear the fog info
		CUtlVector<MapTimeInfo_t::FogInfo_t>& fog_array = m_bNightTimeMode ? info.NightInfo.FogInfo : info.DayInfo.FogInfo;

		//set the enabled state
		(m_bNightTimeMode ? info.NightInfo.FogEnabled : info.DayInfo.FogEnabled) = m_OverrideFogButton->IsSelected();

		//if the fog isnt enabled then just return
		if (!m_OverrideFogButton->IsSelected())
		{
			AddOrUpdateFogInfoInArray(fog_array, "fog_override", "0");
			break;
		}

		//set the info
		MapTimeInfo_t::FogInfo_t FogInfos[19] = {
			//pixel fog
			{	StringToMapTimeStringTableIndex("r_pixelfog"),				StringToMapTimeStringTableIndex(CFmtStr("%d", m_OverrideFogButton->IsSelected() ? m_EnablePixelFogButton->IsSelected() : 1))},

			//fog override
			{	StringToMapTimeStringTableIndex("fog_override"),			StringToMapTimeStringTableIndex(CFmtStr("%d", m_OverrideFogButton->IsSelected())) },

			//fog enabled states
			{	StringToMapTimeStringTableIndex("fog_enable"),				StringToMapTimeStringTableIndex(!m_OverrideEnableFogCheckButton->IsSelected() ? "-1" : CFmtStr("%d", m_EnableFogCheckButton->IsSelected())) },
			{	StringToMapTimeStringTableIndex("fog_enableskybox"),		StringToMapTimeStringTableIndex(!m_OverrideEnableSkyboxFogCheckButton->IsSelected() ? "-1" : CFmtStr("%d", m_EnableSkyboxFogCheckButton->IsSelected())) },

			//fog distanes
			{	StringToMapTimeStringTableIndex("fog_start"),				StringToMapTimeStringTableIndex(!m_FogStartOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogStartSlider->GetValue())) },
			{	StringToMapTimeStringTableIndex("fog_end"),					StringToMapTimeStringTableIndex(!m_FogEndOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogEndSlider->GetValue())) },

			//fog skybox distances
			{	StringToMapTimeStringTableIndex("fog_startskybox"),			StringToMapTimeStringTableIndex(!m_FogStartSkyboxOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogStartSkyboxSlider->GetValue())) },
			{	StringToMapTimeStringTableIndex("fog_endskybox"),			StringToMapTimeStringTableIndex(!m_FogEndSkyboxOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogEndSkyboxSlider->GetValue())) },

			//fog densities
			{	StringToMapTimeStringTableIndex("fog_maxdensity"),			StringToMapTimeStringTableIndex(!m_FogDensityOverride->IsSelected() ? "-1" : CFmtStr("%.3f", (float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax())) },
			{	StringToMapTimeStringTableIndex("fog_maxdensityskybox"),	StringToMapTimeStringTableIndex(!m_FogSkyboxDensityOverride->IsSelected() ? "-1" : CFmtStr("%.3f", (float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax())) },

			//fog colors
			{	StringToMapTimeStringTableIndex("fog_color"),				StringToMapTimeStringTableIndex(!m_FogColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b())) },
			{	StringToMapTimeStringTableIndex("fog_colorskybox"),			StringToMapTimeStringTableIndex(!m_FogSkyboxColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b())) },

			//farz clipping plane
			{	StringToMapTimeStringTableIndex("r_farz"),					StringToMapTimeStringTableIndex(!m_FarzClippingPlaneOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FarzClippingPlaneSlider->GetValue())) },

			//fog blend
			{	StringToMapTimeStringTableIndex("fog_blend"),				StringToMapTimeStringTableIndex(!m_OverrideFogBlendCheckButton->IsSelected() ? "-1" : CFmtStr("%d", m_EnableFogBlendCheckButton->IsSelected())) },

			//fog blend color
			{	StringToMapTimeStringTableIndex("fog_blendcolor"),			StringToMapTimeStringTableIndex(!m_FogBlendColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogBlendColor.r(), m_FogBlendColor.g(), m_FogBlendColor.b())) },

			//fog blend angle
			{	StringToMapTimeStringTableIndex("fog_blendangle"),			StringToMapTimeStringTableIndex(!m_FogBlendAngleOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogBlendAngleSlider->GetValue())) },

			//fog skybox blend
			{	StringToMapTimeStringTableIndex("fog_blendskybox"),			StringToMapTimeStringTableIndex(!m_OverrideFogBlendSkyboxCheckButton->IsSelected() ? "-1" : CFmtStr("%d", m_EnableFogBlendSkyboxCheckButton->IsSelected())) },

			//fog skybox blend color
			{	StringToMapTimeStringTableIndex("fog_blendcolorskybox"),	StringToMapTimeStringTableIndex(!m_FogBlendSkyboxColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogBlendSkyboxColor.r(), m_FogBlendSkyboxColor.g(), m_FogBlendSkyboxColor.b())) },

			//fog skybox blend angle
			{	StringToMapTimeStringTableIndex("fog_blendangleskybox"),	StringToMapTimeStringTableIndex(!m_FogBlendSkyboxAngleOverride->IsSelected() ? "-1" : CFmtStr("%d", m_FogBlendSkyboxAngleSlider->GetValue())) },
		};

		//add to the array
		for (int i = 0; i < SIZE_OF_ARRAY(FogInfos); i++)
		{
			//i used to have it so i would clear the fog array then add all the FogInfos[i] to them. BUT i realized
			//you could possibly add any other convar into this array. These convars wouldnt show up in the panel BUT
			//would still be in the info. So what we are going to do is not clear the fog infos but instead modify
			//the already existing ones.
			AddOrUpdateFogInfoInArray(fog_array, FogInfos[i].convar, FogInfos[i].value);
		}
	} while (false);
}

