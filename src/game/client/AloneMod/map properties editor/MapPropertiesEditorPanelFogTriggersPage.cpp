#include "cbase.h"
#include "MapPropertiesEditorPanelFogTriggersPage.h"
#include "debugoverlay_shared.h"

//-------------------------------------------------------------------------------------------------------
// Purpose: Draws a debug box
//-------------------------------------------------------------------------------------------------------
void DrawDebugBox(const Vector& mins_, const Vector& maxs_, int r, int g, int b, int a, bool depthtest, float duration)
{
	//debthtest
	bool noDepthTest = !depthtest;

	//swap mins + maxs if needed
	Vector mins = mins_;
	Vector maxs = maxs_;

	//swap the x
	if (mins.x > maxs.x)
	{
		float t = mins.x;
		mins.x = maxs.x;
		maxs.x = t;
	}

	//swap the y
	if (mins.y > maxs.y)
	{
		float t = mins.y;
		mins.y = maxs.y;
		maxs.y = t;
	}

	//swap the z
	if (mins.z > maxs.z)
	{
		float t = mins.z;
		mins.z = maxs.z;
		maxs.z = t;
	}

	//all the points
	Vector p[8];
	p[0] = Vector(mins.x, mins.y, mins.z);
	p[1] = Vector(maxs.x, mins.y, mins.z);
	p[2] = Vector(maxs.x, maxs.y, mins.z);
	p[3] = Vector(mins.x, maxs.y, mins.z);
	p[4] = Vector(mins.x, mins.y, maxs.z);
	p[5] = Vector(maxs.x, mins.y, maxs.z);
	p[6] = Vector(maxs.x, maxs.y, maxs.z);
	p[7] = Vector(mins.x, maxs.y, maxs.z);

	//bottom
	NDebugOverlay::Line(p[0], p[1], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[1], p[2], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[2], p[3], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[3], p[0], r, g, b, noDepthTest, duration);

	//top
	NDebugOverlay::Line(p[4], p[5], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[5], p[6], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[6], p[7], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[7], p[4], r, g, b, noDepthTest, duration);

	//vertical
	NDebugOverlay::Line(p[0], p[4], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[1], p[5], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[2], p[6], r, g, b, noDepthTest, duration);
	NDebugOverlay::Line(p[3], p[7], r, g, b, noDepthTest, duration);

	//bottom face
	NDebugOverlay::Triangle(p[0], p[1], p[2], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[0], p[2], p[3], r, g, b, a, noDepthTest, duration);

	//top face
	NDebugOverlay::Triangle(p[4], p[6], p[5], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[4], p[7], p[6], r, g, b, a, noDepthTest, duration);

	//front
	NDebugOverlay::Triangle(p[0], p[4], p[5], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[0], p[5], p[1], r, g, b, a, noDepthTest, duration);

	//back
	NDebugOverlay::Triangle(p[3], p[2], p[6], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[3], p[6], p[7], r, g, b, a, noDepthTest, duration);

	//left
	NDebugOverlay::Triangle(p[0], p[3], p[7], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[0], p[7], p[4], r, g, b, a, noDepthTest, duration);

	//right
	NDebugOverlay::Triangle(p[1], p[5], p[6], r, g, b, a, noDepthTest, duration);
	NDebugOverlay::Triangle(p[1], p[6], p[2], r, g, b, a, noDepthTest, duration);
}




//static panel
static CAddTriggerDialogPanel* s_AddTriggerDialogSingleton = nullptr;

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the add/rename trigger dialog
//-------------------------------------------------------------------------------------------------------
CAddTriggerDialogPanel::CAddTriggerDialogPanel(Panel* parent, bool add, const char* currenttext) : BaseClass(parent, "AddTriggerDialogPanel")
{
	s_AddTriggerDialogSingleton = this;

	SetParent(parent);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetVisible(true);
	SetEnabled(true);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);
	SetFadeEffectDisableOverride(true);
	SetMoveable(false);
	SetTitleBarVisible(true);
	SetCloseButtonVisible(false);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetSize(300, 85);
	MoveToCenterOfScreen();
	Activate();
	SetTitle(add ? "#MapProperties_FogTriggersPage_AddDialog_Title" : "#MapProperties_FogTriggersPage_RenameDialog_Title", true);

	//create the text entry
	m_TextEntry = new TextEntry(this, "AddTriggerTextEntry");
	m_TextEntry->SetBounds(5, 25, 290, 25);
	m_TextEntry->SetMaximumCharCount(64);
	m_TextEntry->SetText(currenttext);

	//create the Save and Cancel button
	Button* m_SaveButton = new Button(this, "SaveButton", "#MapProperties_Button_Confirm");
	m_SaveButton->SetBounds(5, 55, 142, 25);
	m_SaveButton->SetCommand("Confirm");

	Button* m_CancelButton = new Button(this, "CancelButton", "#MapProperties_Button_Cancel");
	m_CancelButton->SetBounds(152, 55, 142, 25);
	m_CancelButton->SetCommand("Close");

	m_AddPanel = add;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CAddTriggerDialogPanel::OnCommand(const char* pszCommand)
{
	if (!Q_stricmp(pszCommand, "Confirm"))
	{
		char text[65];
		m_TextEntry->GetText(text, sizeof(text));

		//send a message to our parent
		PostActionSignal(new KeyValues(m_AddPanel ? "OnAddTrigger" : "OnRenameTrigger", "Text", text));
		pszCommand = "Close";
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Destructor for the add/rename trigger dialog
//-------------------------------------------------------------------------------------------------------
CAddTriggerDialogPanel::~CAddTriggerDialogPanel()
{
	s_AddTriggerDialogSingleton = nullptr;
}



//fog trigger var struct
struct FogTriggerData_t
{
	const char* convar;			//convar name
	const char* text;			//text to display in the listpanel + the label

	//type. Used for the slider/button
	enum
	{
		TYPE_SLIDER,
		TYPE_COLORPICKER
	} type;

	//diviser to divide this value by
	float val_divisor;
	int sval_min;
	int sval_max;
};

static FogTriggerData_t FogDatas[MAX_FOG_VARIABLES] = {
	{"fog_override",			"Override Fog",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"r_pixelfog",				"Enable Pixel Fog",				FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_enable",				"Enable Fog",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_enableskybox",		"Enable Skybox Fog",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_color",				"Fog Color",					FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			0,				0},
	{"fog_colorskybox",			"Fog Skybox Color",				FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			0,				0},
	{"fog_start",				"Fog Start Pos",				FogTriggerData_t::TYPE_SLIDER,			1.0f,			-25000,			50000},
	{"fog_end",					"Fog End Pos",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-10000,			150000},
	{"fog_startskybox",			"Fog Skybox Start Pos",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-25000,			50000},
	{"fog_endskybox",			"Fog Skybox End Pos",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-10000,			150000},
	{"fog_maxdensity",			"Fog Density",					FogTriggerData_t::TYPE_SLIDER,			1000.0f,		-1,				1000},
	{"fog_maxdensityskybox",	"Fog Skybox Density",			FogTriggerData_t::TYPE_SLIDER,			1000.0f,		-1,				1000},
	{"fog_blend",				"Enable Fog Blending",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_blendskybox",			"Enable Fog Skybox Blending",	FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_blendangle",			"Fog Blending Angle",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_blendangleskybox",	"Skybox Fog Blending Angle",	FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
	{"fog_blendcolor",			"Fog Blend Color",				FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			-1,				359},
	{"fog_blendcolorskybox",	"Skybox Fog Blend Color",		FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			-1,				359}
};

//-------------------------------------------------------------------------------------------------------
// Purpose: Finds the convar and index from the input convar. -1 on failure
//-------------------------------------------------------------------------------------------------------
static int FindFogConvar(const char* convar)
{
	for (int i = 0; i < MAX_FOG_VARIABLES; i++)
	{
		if (!Q_stricmp(convar, FogDatas[i].convar))
			return i;
	}

	//FAIL
	return -1;
}



#define SIZE_EDITOR_SNAP_SIZE 32

//-------------------------------------------------------------------------------------------------------
// Purpose: helper func to snap a vector to a pos
//-------------------------------------------------------------------------------------------------------
static float SnapVector(float value)
{
	int step = (int)(value / SIZE_EDITOR_SNAP_SIZE);
	if (step == 0)
		return 0.0f;

	return (float)(step * SIZE_EDITOR_SNAP_SIZE);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: returns the forward position for the mins/maxs
//-------------------------------------------------------------------------------------------------------
static Vector GetForwardPosForSizeEditor()
{
	//get the player
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return vec3_origin;

	Vector vecDir;
	QAngle EyeDir = pPlayer->EyeAngles();
	AngleVectors(EyeDir, &vecDir);

	//get the end pos
	Vector vecAbsEnd = pPlayer->EyePosition() + (vecDir * 128);

	//snap to 32 units
	vecAbsEnd.x = SnapVector(vecAbsEnd.x);
	vecAbsEnd.y = SnapVector(vecAbsEnd.y);
	vecAbsEnd.z = SnapVector(vecAbsEnd.z);
	return vecAbsEnd;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties fog ptriggers age
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelFogTriggersPage::CMapPropertiesPanelFogTriggersPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/FogTriggersPage.res")
{
	m_Sizes[0] = vec3_origin;
	m_Sizes[1] = vec3_origin;

	//apply button
	m_ApplyButton = new Button(this, "ApplyButton", "", this, COMMAND_APPLY_SETTING);
	m_ApplyButton->SetEnabled(false);

	//create our data stuff
	m_ShouldOverrideButton = new CheckButton(this, "ShouldOverrideButton", "");
	m_ShouldOverrideButton->SetEnabled(false);
	m_ShouldOverrideButton->SetCommand(COMMAND_SHOULD_OVERRIDE);

	m_DataLabel = new Label(this, "DataLabel", "");
	m_DataLabel->SetEnabled(false);

	m_DataSlider = new CMapPropertiesPanelSlider(this, "DataSlider", 250);
	m_DataSlider->SetEnabled(false);

	m_DataButton = new Button(this, "DataButton", "", this, COMMAND_SET_COLOR);
	m_DataButton->SetEnabled(false);
	m_DataButton->SetVisible(false);

	m_DataButtonRect = Rect_t{ 0, 0, 0, 0 };
	m_DataButtonColor.SetColor(0, 0, 0, 0);

	//create the set size button
	m_SetSizeButton = new Button(this, "RenameButton", "", this, COMMAND_SET_SIZE);
	m_SetSizeButton->SetEnabled(false);

	//make the fog lerp slider
	m_FogLerpSliderText = new Label(this, "LerpSliderText", "");

	m_FogLerpSlider = new CMapPropertiesPanelSlider(this, "LerpSlider");
	m_FogLerpSlider->SetEnabled(false);

	//create the fog data list
	m_VarList = new ListPanel(this, "TriggersData");
	m_VarList->AddColumnHeader(0, "TriggersData", "#MapProperties_FogTriggersPage_TriggerData_Header", 1000, 0);
	m_VarList->SetMultiselectEnabled(false);
	m_VarList->SetColumnSortable(0, false);
	m_VarList->SetKeyBoardInputEnabled(false);
	m_VarList->SetEnabled(true);

	//create the trigger list
	m_TriggerList = new ListPanel(this, "TriggerList");
	m_TriggerList->AddColumnHeader(0, "TriggersName", "#MapProperties_FogTriggersPage_Triggers_Header", 1000, 0);
	m_TriggerList->SetMultiselectEnabled(false);
	m_TriggerList->SetColumnSortable(0, false);
	m_TriggerList->SetKeyBoardInputEnabled(false);

	//bottom trigger buttons
	m_AddButton = new Button(this, "AddButton", "", this, COMMAND_ADD_TRIGGER);

	m_RenameButton = new Button(this, "RenameButton", "", this, COMMAND_RENAME_TRIGGER);
	m_RenameButton->SetEnabled(false);

	m_RemoveButton = new Button(this, "RemoveButton", "", this, COMMAND_REMOVE_TRIGGER);
	m_RemoveButton->SetEnabled(false);

	//perform layout to set the range sliders and such
	PerformLayout();
}

#if FOG_CUBE_TRIGGER_TEST
void ResetCubeTriggerData();
#endif

//-------------------------------------------------------------------------------------------------------
// Purpose: Destructor for the map properties fog ptriggers age
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelFogTriggersPage::~CMapPropertiesPanelFogTriggersPage()
{
#if FOG_CUBE_TRIGGER_TEST
	//reset the cube trigger info so it doesnt lerp to the trigger we're inside
	ResetCubeTriggerData();
#endif
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs layout for this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::PerformLayout()
{
	BaseClass::PerformLayout();

	//apply button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("ApplyButton"), m_ApplyButton);

	//data stuff
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("ShouldOverrideButton"), m_ShouldOverrideButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataLabel"), m_DataLabel);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataSlider"), m_DataSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataButton"), m_DataButton);
	sscanf(m_KeyValuesFile->GetString("DataColorRect"), "%d %d %d %d", &m_DataButtonRect.x, &m_DataButtonRect.y, &m_DataButtonRect.width, &m_DataButtonRect.height);

	//set size button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SetSizeButton"), m_SetSizeButton);

	//lerp slider stuff
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TransitionSliderText"), m_FogLerpSliderText);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TransitionSlider"), m_FogLerpSlider);

	//triggers
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TriggerData"), m_VarList);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TriggerList"), m_TriggerList);

	//bottom buttons
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("AddButton"), m_AddButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("RenameButton"), m_RenameButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("RemoveButton"), m_RemoveButton);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::InitFogTriggerInfo(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

	//get each trigger
	CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& Triggers = IsNightPage ? info.NightInfo.FogCubeTriggers : info.DayInfo.FogCubeTriggers;
	for (int i = 0; i < Triggers.Count(); i++)
	{
		AddFogTrigger(Triggers[i].name, &Triggers[i]);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::GetFogTriggerInfo(MapTimeInfo_t& info)
{
	//array to add to
	CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeInfo = m_bNightTimeMode ? info.NightInfo.FogCubeTriggers : info.DayInfo.FogCubeTriggers;
	CubeInfo.RemoveAll();

	//go through all our triggers
	for (int i = 0; i < m_TriggerInfos.Count(); i++)
	{
		//add a new trigger
		MapTimeInfo_t::FogCubeTrigger_t& trigger = CubeInfo[CubeInfo.AddToTail()];

		//add vars
		for (int j = 0; j < m_TriggerInfos[i].foginfo.Count(); j++)
			trigger.foginfo.AddToTail({ m_TriggerInfos[i].foginfo[j].convar, m_TriggerInfos[i].foginfo[j].value });

		trigger.lerptime = m_TriggerInfos[i].lerptime;
		trigger.mins = m_TriggerInfos[i].mins;
		trigger.maxs = m_TriggerInfos[i].maxs;
		Q_strncpy(trigger.name, m_TriggerInfos[i].name, sizeof(trigger.name));
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a new fog trigger
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::AddFogTrigger(const char* name, MapTimeInfo_t::FogCubeTrigger_t* Data, bool addtoarray)
{
	//add to the trigger list
	int index = m_TriggerList->AddItem(new KeyValues("Trigger", "TriggersName", name), m_TriggerList->GetItemCount(), false, false);
	m_TriggerList->ClearSelectedItems();
	m_TriggerList->AddSelectedItem(index);

	//if (!addtoarray)
	//	return;

	//add to m_TriggerInfos
	MapTimeInfo_t::FogCubeTrigger_t& trigger = m_TriggerInfos[m_TriggerInfos.AddToTail()];
	memset(&trigger, 0, sizeof(trigger));
	Q_strncpy(trigger.name, name, sizeof(trigger.name));

	//set to Data if needed
	if (Data)
	{
		//copy fog info
		for (int i = 0; i < Data->foginfo.Count(); i++)
		{
			//look for the convar
			int var = FindFogConvar(StringFromMapTimeStringTableIndex(Data->foginfo[i].convar));
			if (var == -1)
				continue;

			//add to the list
			trigger.foginfo.AddToTail({Data->foginfo[i].convar, Data->foginfo[i].value});
		}

		//copy the other stuff
		trigger.lerptime = Data->lerptime;
		trigger.mins = Data->mins;
		trigger.maxs = Data->maxs;
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Renames a fog trigger
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::RenameFogTrigger(int index, const char* newname)
{
	//this SHOULDNT be false but just to be safe, Check m_TriggerInfos.Count()
	if (index < 0 || index >= m_TriggerInfos.Count() || !newname)
		return;

	//set the data text
	Q_strncpy(m_TriggerInfos[index].name, newname, sizeof(m_TriggerInfos[index].name));

	//set the item text
	ListPanelItem* item = m_TriggerList->GetItemData(index);
	if (!item)
		return;

	item->kv->SetString("TriggersName", newname);
	m_TriggerList->Repaint();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a trigger is added
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnAddTrigger(KeyValues* data)
{
	AddFogTrigger(data->GetString("Text"));
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a trigger is renamed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnRenameTrigger(KeyValues* data)
{
	RenameFogTrigger(m_TriggerList->GetSelectedItem(0), data->GetString("Text"));
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when an item from a ListPanel is selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnItemSelected(KeyValues* data)
{
	Panel* from = (Panel*)data->GetPtr("Panel");

	if (from == m_TriggerList)
	{
		//check index
		int index = m_TriggerList->GetSelectedItem(0);
		if (index < 0 || index >= m_TriggerInfos.Count())
			return;

		//enable our items
		m_RenameButton->SetEnabled(true);
		m_RemoveButton->SetEnabled(true);
		m_FogLerpSlider->SetEnabled(true);
		m_FogLerpSlider->SetValue(m_TriggerInfos[index].lerptime * LERP_SLIDER_DIVISOR);
		m_SetSizeButton->SetEnabled(true);

		//add our vars
		for (int i = 0; i < MAX_FOG_VARIABLES; i++)
		{
			const char* text = CFmtStr("%s - Override = %d", FogDatas[i].text, FindFogInfoFromArray(m_TriggerInfos[index].foginfo, FogDatas[i].convar, nullptr) != nullptr);
			m_VarList->AddItem(new KeyValues("Data", "TriggersData", text), m_VarList->GetItemCount(), false, false);
		}
	}
	else
	{
		//check index
		int trigger_index = m_TriggerList->GetSelectedItem(0);
		if (trigger_index < 0 || trigger_index >= m_TriggerInfos.Count())
			return;

		//enable either our button or slider
		int index = m_VarList->GetSelectedItem(0);
		if (index < 0 || index >= MAX_FOG_VARIABLES)
			return;

		//is our convar enabled?
		const char* info = FindFogInfoFromArray(m_TriggerInfos[trigger_index].foginfo, FogDatas[index].convar, nullptr);
		bool enabled = info != nullptr;

		//enable our items
		m_ApplyButton->SetEnabled(true);
		m_ShouldOverrideButton->SetEnabled(true);
		m_ShouldOverrideButton->SetSelected(enabled);

		//set our data stuff
		m_DataLabel->SetEnabled(enabled);

		//enable
		m_DataButton->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER);
		m_DataButton->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER && enabled);
		m_DataSlider->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER);
		m_DataSlider->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER && enabled);

		//set our var
		if (FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER && enabled)
		{
			//set button color
			int r = 0, g = 0, b = 0;
			sscanf(info, "%d %d %d", &r, &g, &b);
			m_DataButtonColor._color[0] = (unsigned char)r;
			m_DataButtonColor._color[1] = (unsigned char)g;
			m_DataButtonColor._color[2] = (unsigned char)b;
			return;
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER)
		{
			//set the slider stuff
			m_DataSlider->SetRange(FogDatas[index].sval_min, FogDatas[index].sval_max);

			if (enabled)
				m_DataSlider->SetValue((int)(atof(info) * FogDatas[index].val_divisor));
		}
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when an item from a ListPanel is de-selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnItemDeselected(KeyValues* data)
{
	Panel* from = (Panel*)data->GetPtr("Panel");

	if (from == m_TriggerList)
	{
		//disable our buttons
		m_RenameButton->SetEnabled(false);
		m_RemoveButton->SetEnabled(false);
		m_SetSizeButton->SetEnabled(false);
		m_FogLerpSlider->SetEnabled(false);
		m_FogLerpSlider->SetValue(0);

		//remove all the items from m_VarList
		m_VarList->RemoveAll();

		//disables the size editor mode
		m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

		//tell our property sheet to regain keyboard input
		GetParent()->SetKeyBoardInputEnabled(true);
	}
	else
	{
		//set our data stuff
		m_ShouldOverrideButton->SetEnabled(false);
		m_ApplyButton->SetEnabled(false);

		m_DataLabel->SetEnabled(false);
		m_DataLabel->SetText("#MapProperties_FogTriggersPage_NoItemSelected");

		m_DataButton->SetVisible(false);
		m_DataButton->SetEnabled(false);

		m_DataSlider->SetEnabled(false);
		m_DataSlider->SetVisible(true);
		m_DataSlider->SetValue(0);
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a slider is moved
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnSliderMoved(KeyValues* data)
{
	//check index
	int index = m_TriggerList->GetSelectedItem(0);
	if (index < 0 || index >= m_TriggerInfos.Count())
		return;

	//check whos which slider
	if (data->GetPtr("Panel") == m_FogLerpSlider)
	{
		//set the lerp time
		m_TriggerInfos[index].lerptime = (float)m_FogLerpSlider->GetValue() / LERP_SLIDER_DIVISOR;
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Re-populates the trigger list
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::RepopulateList()
{
	//remove + re-add
	m_TriggerList->RemoveAll();

	for (int i = 0; i < m_TriggerInfos.Count(); i++)
		AddFogTrigger(m_TriggerInfos[i].name, &m_TriggerInfos[i], false);
}

void UpdateFogTriggers(CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers, bool forcend);

//-------------------------------------------------------------------------------------------------------
// Purpose: Called whent the page is hidden
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnPageHide()
{
	//disables the size editor mode
	m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

	//tell our property sheet to regain keyboard input
	GetParent()->SetKeyBoardInputEnabled(true);

	BaseClass::OnPageHide();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::Update()
{
	//set our m_FogLerpSliderText text
	wchar_t* lerptext = g_pVGuiLocalize->Find("MapProperties_FogTriggersPage_TransitionLabel");
	wchar_t text[128];
	swprintf(text, SIZE_OF_ARRAY(text), L"%ws %.2f", lerptext, (float)m_FogLerpSlider->GetValue() / LERP_SLIDER_DIVISOR);

	m_FogLerpSliderText->SetText(text);

	//update the trigger infos
	UpdateFogTriggers(m_TriggerInfos, true);

	//update our size editor if needed
	if (m_iInSizeEditor != SIZE_EDITOR_MODE_NONE)
	{
		m_iInSizeEditor = Clamp(m_iInSizeEditor, SIZE_EDITOR_MODE_NONE, SIZE_EDITOR_MODE_MAXS);

		//set our vector
		m_Sizes[m_iInSizeEditor - 1] = GetForwardPosForSizeEditor();

		//draw the debug bounds
		DrawDebugBox(m_Sizes[0], m_Sizes[1], 255, 200, 0, 10, true, 0.01f);
	}


	//update the text for our var slider
	int index = m_VarList->GetSelectedItem(0);
	if (index < 0 || index >= MAX_FOG_VARIABLES)
		return;

	//check type
	if (FogDatas[index].type != FogTriggerData_t::TYPE_SLIDER)
		return;

	m_DataLabel->SetText(CFmtStr("%s: %d", FogDatas[index].text, (int)((float)m_DataSlider->GetValue() / FogDatas[index].val_divisor)));
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnCommand(const char* pszCommand)
{
	//check for COMMAND_ADD_TRIGGER or COMMAND_RENAME_TRIGGER
	if (!Q_stricmp(pszCommand, COMMAND_ADD_TRIGGER) || !Q_stricmp(pszCommand, COMMAND_RENAME_TRIGGER))
	{
		//get the current text
		const char* buf = "";
		int index = m_TriggerList->GetSelectedItem(0);
		if (index >= 0 && index < m_TriggerInfos.Count())
			buf = m_TriggerInfos[index].name;

		//create the add dialog
		if (s_AddTriggerDialogSingleton)
			delete s_AddTriggerDialogSingleton;

		s_AddTriggerDialogSingleton = new CAddTriggerDialogPanel(this, !Q_stricmp(pszCommand, COMMAND_ADD_TRIGGER), buf);
		s_AddTriggerDialogSingleton->MoveToCenterOfScreen();
		s_AddTriggerDialogSingleton->DoModal();
		s_AddTriggerDialogSingleton->AddActionSignalTarget(this);
	}

	//check for COMMAND_REMOVE_TRIGGER
	else if (!Q_stricmp(pszCommand, COMMAND_REMOVE_TRIGGER))
	{
		//check index
		int index = m_TriggerList->GetSelectedItem(0);
		if (index < 0 || index >= m_TriggerInfos.Count())
			return;
			
		//clear the data for the infos + trigger list
		m_TriggerInfos.Remove(index);
		RepopulateList();

		m_TriggerList->ClearSelectedItems();
		m_TriggerList->AddSelectedItem(max(index-1, 0));
	}

	//check for COMMAND_SET_SIZE
	else if (!Q_stricmp(pszCommand, COMMAND_SET_SIZE))
	{
		//check index
		int index = m_TriggerList->GetSelectedItem(0);
		if (index < 0 || index >= m_TriggerInfos.Count())
			return;

		//prompt
		static int bDidPrompt = false;
		if (!bDidPrompt)
		{
			QueryBox* modal = new QueryBox("#MapProperties_FogTriggersPage_SizeDialog_Title", "#MapProperties_FogTriggersPage_SizeDialog_Description", this);
			modal->MoveToCenterOfScreen();
			modal->Activate();
			modal->DoModal();
			modal->SetOKCommand(new KeyValues("Command", "command", "ConfirmClose"));
			modal->SetCancelCommand(new KeyValues("Command", "command", "DoModal"));

			bDidPrompt = true;
		}

		//reset our mins/maxs
		m_Sizes[0] = m_TriggerInfos[index].mins;
		m_Sizes[1] = m_TriggerInfos[index].maxs;
		m_iInSizeEditor = SIZE_EDITOR_MODE_MINS;

		//tell our property sheet to lose keyboard input
		GetParent()->SetKeyBoardInputEnabled(false);
	}

	//check for COMMAND_SHOULD_OVERRIDE
	else if (!Q_stricmp(pszCommand, COMMAND_SHOULD_OVERRIDE))
	{
		//remove trigger
		if (m_ShouldOverrideButton->IsSelected())
		{
			OnOverrideButtonEnabled();
		}
		else
		{
			OnOverrideButtonDisabled();
		}
	}

	//check for COMMAND_APPLY_SETTING
	else if (!Q_stricmp(pszCommand, COMMAND_APPLY_SETTING))
	{
		UpdateArrayValue();
	}

	//check for COMMAND_SET_COLOR
	else if (!Q_stricmp(pszCommand, COMMAND_SET_COLOR))
	{
		//create the fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#ColorPicker_SetColorButton", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_DataButtonColor);
		m_ColorPicker->DoModal();
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Updates our array value
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::UpdateArrayValue()
{
	//check index
	int trigger_index = m_TriggerList->GetSelectedItem(0);
	if (trigger_index < 0 || trigger_index >= m_TriggerInfos.Count())
		return;

	//check index 2
	int index = m_VarList->GetSelectedItem(0);
	if (index < 0 || index >= MAX_FOG_VARIABLES)
		return;

	//check if the override button is selected
	if (!m_ShouldOverrideButton->IsSelected())
	{
		RemoveFogInfoInArray(m_TriggerInfos[trigger_index].foginfo, FogDatas[index].convar);
	}
	else
	{
		const char* string = "";

		//get our string to add
		if (FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER)
			string = CFmtStr("%f", (float)m_DataSlider->GetValue() / FogDatas[index].val_divisor);
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER)
			string = CFmtStr("%d %d %d", m_DataButtonColor.r(), m_DataButtonColor.g(), m_DataButtonColor.b());

		//update
		AddOrUpdateFogInfoInArray(m_TriggerInfos[trigger_index].foginfo, FogDatas[index].convar, string);
	}

	//set the text of the item
	ListPanelItem* item = m_VarList->GetItemData(index);
	if (!item)
		return;

	const char* text = CFmtStr("%s - Override = %d", FogDatas[index].text, m_ShouldOverrideButton->IsSelected());
	item->kv->SetString("TriggersData", text);
	m_VarList->Repaint();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the override button is pressed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnOverrideButtonEnabled()
{
	//enable our items
	m_DataLabel->SetEnabled(m_DataLabel->IsVisible());
	m_DataButton->SetEnabled(m_DataButton->IsVisible());
	m_DataSlider->SetEnabled(true);

	//update our value
	UpdateArrayValue();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the override button is pressed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnOverrideButtonDisabled()
{
	//disable our items
	m_DataLabel->SetEnabled(false);
	m_DataButton->SetEnabled(false);
	m_DataSlider->SetEnabled(false);

	//update our value
	UpdateArrayValue();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a key code is pressed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnKeyCodePressed(KeyCode code)
{
	if ((code == KeyCode::KEY_LSHIFT || code == KeyCode::KEY_RSHIFT) && m_iInSizeEditor != SIZE_EDITOR_MODE_NONE)
	{
		//check m_iInSizeEditor
		if (m_iInSizeEditor == SIZE_EDITOR_MODE_MINS)
		{
			m_iInSizeEditor = SIZE_EDITOR_MODE_MAXS;
		}
		else
		{
			//tell our property sheet to regain keyboard input
			GetParent()->SetKeyBoardInputEnabled(true);

			//set our mode
			m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

			//check index
			int index = m_TriggerList->GetSelectedItem(0);
			if (index < 0 || index >= m_TriggerInfos.Count())
				return;

			//swap mins + maxs if needed
			Vector mins = m_Sizes[0];
			Vector maxs = m_Sizes[1];

			//swap the x
			if (mins.x > maxs.x)
			{
				float t = mins.x;
				mins.x = maxs.x;
				maxs.x = t;
			}

			//swap the y
			if (mins.y > maxs.y)
			{
				float t = mins.y;
				mins.y = maxs.y;
				maxs.y = t;
			}

			//swap the z
			if (mins.z > maxs.z)
			{
				float t = mins.z;
				mins.z = maxs.z;
				maxs.z = t;
			}

			//set the lerp time
			m_TriggerInfos[index].mins = mins;
			m_TriggerInfos[index].maxs = maxs;
		}

		return;
	}

	BaseClass::OnKeyCodePressed(code);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Paints this page
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::Paint()
{
	BaseClass::Paint();

	//check index
	int index = m_TriggerList->GetSelectedItem(0);
	if (index < 0 || index >= m_TriggerInfos.Count())
		return;

	index = m_VarList->GetSelectedItem(0);
	if (index < 0 || index >= MAX_FOG_VARIABLES)
		return;

	//check if we selected a color
	if (FogDatas[index].type != FogTriggerData_t::TYPE_COLORPICKER)
		return;

	//paint the color
	m_DataButtonColor._color[3] = 255;		//alpha is always 255
	surface()->DrawSetColor(m_DataButtonColor);
	surface()->DrawFilledRect(m_DataButtonRect.x, m_DataButtonRect.y, m_DataButtonRect.x + m_DataButtonRect.width, m_DataButtonRect.y + m_DataButtonRect.height);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnColorSelected(KeyValues* data)
{
	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	//set our color
	m_DataButtonColor = Color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), 255);

	//call base func
	BaseClass::OnColorSelected(data);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Override OnCheckButtonChecked
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnCheckButtonChecked(KeyValues* subkey)
{
}