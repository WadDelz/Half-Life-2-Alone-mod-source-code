#include "cbase.h"
#include "MapPropertiesEditorPanel.h"
#include <vgui_controls/PropertySheet.h>

//static map properties panel
CMapPropertiesPanel* g_MapPropertiesPanel;

extern ConVar cl_mouselook;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties panel
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanel::CMapPropertiesPanel(Panel* parent) : BaseClass(nullptr, "MapPropertiesPanel")
{
	//clear the previous convar values
	memset(m_PreviousCloudsOverrideValue, 0, sizeof(m_PreviousCloudsOverrideValue));
	memset(m_PreviousCloudsColorValue, 0, sizeof(m_PreviousCloudsColorValue));
	memset(m_PreviousCloudsShowValue, 0, sizeof(m_PreviousCloudsShowValue));
	memset(m_PreviousFilterConvarValue, 0, sizeof(m_PreviousFilterConvarValue));
	memset(m_PreviousFilterIntensityConvarValue, 0, sizeof(m_PreviousFilterIntensityConvarValue));
	memset(m_PreviousGodConvarValue, 0, sizeof(m_PreviousGodConvarValue));

	//reset our coppied steps
	memset(s_UndoSteps, 0, sizeof(s_UndoSteps));
	s_NeedSave = false;
	s_UndoStepsCount = 0;
	s_CurrentUndoStep = 0;

	//set our settings
	g_MapPropertiesPanel = this;
	m_bNightTimeMode = true;

	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	Activate();

	//tell our property sheet to shut up. 
	//edit: nevermind. This also disables keyboard input to combo boxes and sliders :(
	//GetPropertySheet()->SetKeyBoardInputEnabled(false);

	//set our ok button command
	SetOKButtonText("Apply");
	_okButton->SetCommand(COMMAND_APPLY_PAGE_SETTINGS);

	//set our pos
	SetPos(0, 0);

	//disable mouselook so keyboard look can be used
	cl_mouselook.SetValue(false);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: HACK: When we create and set the check buttons, this actually creates an undo step. To counter this.
//			send a message to this (self) to reset the undo steps once all the values have been initalized.
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnResetUndoSteps(KeyValues* subkey)
{
	s_UndoStepsCount = 0;
	s_CurrentUndoStep = 0;
	s_NeedSave = false;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs the layout for our panel
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::PerformLayout()
{
	BaseClass::PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a keycode is pressed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnKeyCodePressed(KeyCode code)
{
	//check for r. This requests focus back to the panel (or a button so this panel will accept input)
	if (code == KeyCode::KEY_R)
	{
		_okButton->RequestFocus();
		return;
	}

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

	//handle ctrl + z
	if (code == KeyCode::KEY_Z && (input()->IsKeyDown(KEY_RCONTROL) || input()->IsKeyDown(KEY_LCONTROL)))
	{
		bool shiftdown = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
		UndoStep_Apply(shiftdown == false);
	}

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
	//regain focus by setting our focus to a button. This makes it so we can move around if
	//our input was previously being sent to the text entry
	_okButton->RequestFocus();
	BaseClass::OnMousePressed(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties page.
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanel::~CMapPropertiesPanel()
{
	//reset our coppied steps
	memset(s_UndoSteps, 0, sizeof(s_UndoSteps));
	s_UndoStepsCount = 0;
	s_CurrentUndoStep = 0;

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
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnThink()
{
	//call each pages Update function
	m_FogPage->Update();
	m_SkyboxFilterPage->Update();

	//only update the sun page if not using the daytime panel
	if (!m_bNightTimeMode)
		m_SunPage->Update();

	BaseClass::OnThink();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel close
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnClose()
{
	//check our s_NeedSave
	if (s_NeedSave)
	{
		//show confirm
		QueryBox* modal = new QueryBox("Close?", "Are you sure you would like to close this dialog without saving?", this);
		modal->MoveToCenterOfScreen();
		modal->Activate();
		modal->DoModal();
		modal->SetOKCommand(new KeyValues("Command", "command", "ConfirmClose"));
		modal->SetCancelCommand(new KeyValues("Command", "command", "DoModal"));
		return;
	}

	//call base func
	engine->ClientCmd("_amod_day_do");
	PostActionSignal(new KeyValues("MapPropertiesPanelClosed"));
	BaseClass::OnClose();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::OnCommand(const char* pszCommand)
{
	//check for apply command
	if (!Q_stricmp(pszCommand, COMMAND_APPLY_PAGE_SETTINGS))
	{
		s_NeedSave = false;

		//post message
		PostActionSignal(new KeyValues("ApplyPageSetting"));
		return;
	}

	//confirms the close command
	else if (!Q_stricmp(pszCommand, "ConfirmClose"))
	{
		s_NeedSave = false;
		BaseClass::OnCommand("Close");
		return;
	}

	//resets this panels modal state
	else if (!Q_stricmp(pszCommand, "DoModal"))
	{
		m_hPreviousModal = vgui::input()->GetAppModalSurface();
		vgui::input()->SetAppModalSurface(GetVPanel());
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the data
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::Init(MapTimeInfo_t& info, bool IsNightPage)
{
	//store the convars current values
	Q_strncpy(m_PreviousCloudsOverrideValue, ConVarRef("amod_clouds_color_override").GetString(), sizeof(m_PreviousCloudsOverrideValue));
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

	//create and set our pages
	AddPage(m_FogPage = new CMapPropertiesPanelFogPage(this, "FogPage"), "Fog Settings");
	m_FogPage->InitFogInfo(info, IsNightPage);

	AddPage(m_SkyboxFilterPage = new CMapPropertiesPanelSkyboxFiltersPage(this, "SkyboxFilterPage"), "Skybox + Filter Settings");
	m_SkyboxFilterPage->InitSkyboxAndFilter(info, IsNightPage);

	//only add the sun page if it isnt the daytime panel
	if (!IsNightPage)
	{
		AddPage(m_SunPage = new CMapPropertiesPanelSunPage(this, "SunPage"), "Sun Settings");
		m_SunPage->InitSunInfo(info, IsNightPage);
	}

	//HACK: reset the undo steps because setting the values (with like SetSelected or SetActiveItem can
	//		add an undo step)
	PostMessage(this, new KeyValues("ResetUndoSteps"), 0.05f);

	//set our active page
	_propertySheet->SetActivePage(m_FogPage);

}

//----------------------------------------------------------------------------------------------------
// Purpose: Sets the data from the panel
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanel::GetData(MapTimeInfo_t& info)
{
	m_FogPage->GetFogInfo(info);
	m_SkyboxFilterPage->GetSkyboxFilterInfo(info);

	//dont get the sun data if this is the night page
	if (!m_bNightTimeMode)
		m_SunPage->GetSunInfo(info);
}