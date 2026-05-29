#include "cbase.h"
#include "MapPropertiesEditorPanelFogTriggersPage.h"
#include "MapPropertiesEditorMenuPanel.h"
#include "debugoverlay_shared.h"

#if FOG_CUBE_TRIGGER_TEST

extern bool s_bUpdateTriggerValuesThisFrame;

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
	char convar[512];		//convar name
	char text[128];			//text to display in the listpanel + the label
	char tooltip[128];		//tooltip to display on the button/combo box

	//type. Used for the slider/button
	enum
	{
		TYPE_DIVIDER,
		TYPE_SLIDER,
		TYPE_COLORPICKER,
		TYPE_FILTER,
		TYPE_SKYBOX
	} type;

	//diviser to divide this value by
	float val_divisor;
	int sval_min;
	int sval_max;

};

//-------------------------------------------------------------------------------------------------------
// Purpose: Returns the type of the fog trigger data based on the inputted string
//-------------------------------------------------------------------------------------------------------
int GetFogTriggerDataTypeFromString(const char* string)
{
	if (!Q_stricmp(string, "TYPE_DIVIDER"))
	{
		return FogTriggerData_t::TYPE_DIVIDER;
	}
	if (!Q_stricmp(string, "TYPE_SLIDER"))
	{
		return FogTriggerData_t::TYPE_SLIDER;
	}
	else if (!Q_stricmp(string, "TYPE_COLORPICKER"))
	{
		return FogTriggerData_t::TYPE_COLORPICKER;
	}
	else if (!Q_strnicmp(string, "TYPE_COMBO_BOX", Q_strlen("TYPE_COMBO_BOX")))
	{
		string += Q_strlen("TYPE_COMBO_BOX");
		if (!Q_stricmp(string, ":SKYBOX"))
		{
			return FogTriggerData_t::TYPE_SKYBOX;
		}
		else if (!Q_stricmp(string, ":FILTERS"))
		{
			return FogTriggerData_t::TYPE_FILTER;
		}
	}

	return FogTriggerData_t::TYPE_SLIDER;
}


//const int MAX_FOG_VARIABLES = 24;
//
//static FogTriggerData_t FogDatas[MAX_FOG_VARIABLES] = {
//	{"fog_override",					"Override Fog",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"r_pixelfog",						"Enable Pixel Fog",				FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"fog_enable",						"Enable Fog",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"fog_enableskybox",				"Enable Skybox Fog",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"fog_color",						"Fog Color",					FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			0,				0},
//	{"fog_colorskybox",					"Fog Skybox Color",				FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			0,				0},
//	{"fog_start",						"Fog Start Pos",				FogTriggerData_t::TYPE_SLIDER,			1.0f,			-25000,			50000},
//	{"fog_end",							"Fog End Pos",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			-10000,			150000},
//	{"fog_startskybox",					"Fog Skybox Start Pos",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-25000,			50000},
//	{"fog_endskybox",					"Fog Skybox End Pos",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-10000,			150000},
//	{"fog_maxdensity",					"Fog Density",					FogTriggerData_t::TYPE_SLIDER,			1000.0f,		-1,				1000},
//	{"fog_maxdensityskybox",			"Fog Skybox Density",			FogTriggerData_t::TYPE_SLIDER,			1000.0f,		-1,				1000},
//	{"fog_blend",						"Enable Fog Blending",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"fog_blendskybox",					"Enable Fog Skybox Blending",	FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				1},
//	{"fog_blendangle",					"Fog Blending Angle",			FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				359},
//	{"fog_blendangleskybox",			"Skybox Fog Blending Angle",	FogTriggerData_t::TYPE_SLIDER,			1.0f,			-1,				359},
//	{"fog_blendcolor",					"Fog Blend Color",				FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			-1,				-1},
//	{"fog_blendcolorskybox",			"Skybox Fog Blend Color",		FogTriggerData_t::TYPE_COLORPICKER,		1.0f,			-1,				-1},
//	{"mat_force_bloom",					"Enable bloom",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			0,				1},
//	{"mat_bloomscale",					"Bloom Scale",					FogTriggerData_t::TYPE_SLIDER,			1.0f,			0,				500},
//	{"mat_bloom_scalefactor_scalar",	"Bloom scale factor",			FogTriggerData_t::TYPE_SLIDER,			100.0f,			0,				10000},
//	{"amod_trigger_filterintensity",	"Filter Intensity",				FogTriggerData_t::TYPE_SLIDER,			100.0f,			1,				100},
//	{"amod_trigger_filtername",			"Filter Name",					FogTriggerData_t::TYPE_FILTER,			1.0f,			-1,				-1},
//	{"sv_skyname",						"Skybox name",					FogTriggerData_t::TYPE_SKYBOX,			1.0f,			-1,				-1}
//};

//fog triggers data
static CUtlVector<FogTriggerData_t> FogDatas;
static int MAX_FOG_VARIABLES = 0;				//legacy support

//-------------------------------------------------------------------------------------------------------
// Purpose: Loads the fog datas for the fog panel
//-------------------------------------------------------------------------------------------------------
static bool ReloadFogData(KeyValues* data)
{
	//check the data
	if (!data)
		return false;

	//remove the fog datas if not already deleted
	FogDatas.RemoveAll();
	MAX_FOG_VARIABLES = 0;

	//load all the data
	FOR_EACH_TRUE_SUBKEY(data, fogdata)
	{
		//make the data ptr
		FogTriggerData_t& dataptr = FogDatas[FogDatas.AddToTail()];
		memset(&dataptr, 0, sizeof(dataptr));

		//load the type
		dataptr.type = decltype(dataptr.type)(GetFogTriggerDataTypeFromString(fogdata->GetString("Type")));

		//load the convar
		Q_strncpy(dataptr.convar, fogdata->GetName(), sizeof(dataptr.convar));
		if (!cvar->FindVar(dataptr.convar) && dataptr.type != FogTriggerData_t::TYPE_DIVIDER)
		{
			FogDatas.Remove(FogDatas.Count() - 1);
			continue;
		}

		//load the text and tooltip
		Q_strncpy(dataptr.text, fogdata->GetString("DisplayText"), sizeof(dataptr.text));
		Q_strncpy(dataptr.tooltip, fogdata->GetString("DisplayTooltip"), sizeof(dataptr.tooltip));

		//min, max and divisor
		dataptr.sval_min = fogdata->GetInt("Min", 0);
		dataptr.sval_max = fogdata->GetInt("Max", 1);
		dataptr.val_divisor = fogdata->GetFloat("Divisor", 1.0f);
	}

	//set and return
	MAX_FOG_VARIABLES = FogDatas.Count();
	return MAX_FOG_VARIABLES > 0;
}

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



ConVar amod_mappropertieseditor_triggers_snapsize("amod_mappropertieseditor_triggers_snapsize", "8", 0, "snapsize for the triggers for the triggers page for the time properties panel");

#define SIZE_EDITOR_SNAP_SIZE amod_mappropertieseditor_triggers_snapsize.GetFloat()

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

extern ConVar amod_fog_cubeinfo_debug;

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties fog ptriggers age
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelFogTriggersPage::CMapPropertiesPanelFogTriggersPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/FogTriggersPage.res")
{
	//check for our keyvalues file
	if (!m_KeyValuesFile)
	{
		//abort
		ConWarning("WARNING! WARNING! WARNING!\nGot NO fog data for the map properties editor fog triggers page!! Add data to the ''Data'' keyvalues of the fog triggers .res file!!\n");
		m_KeyValuesFile->deleteThis();
		m_KeyValuesFile = nullptr;
		return;
	}

	//load the fog data
	if (!ReloadFogData(m_KeyValuesFile->FindKey("FogData")))
	{
		//abort
		ConWarning("WARNING! WARNING! WARNING!\nGot NO fog data for the map properties editor fog triggers page!! Add data to the ''Data'' keyvalues of the fog triggers .res file!!\n");
		m_KeyValuesFile->deleteThis();
		m_KeyValuesFile = nullptr;
		return;
	}

	//set our debug info
	amod_fog_cubeinfo_debug.SetValue(true);

	//sizes
	m_Sizes[0] = vec3_origin;
	m_Sizes[1] = vec3_origin;

	//apply button
	//m_ApplyButton = new Button(this, "ApplyButton", "", this, COMMAND_APPLY_SETTING);
	//m_ApplyButton->SetEnabled(false);

	//create our data stuff
	m_ShouldOverrideButton = new CheckButton(this, "ShouldOverrideButton", "");
	m_ShouldOverrideButton->SetEnabled(false);
	m_ShouldOverrideButton->SetCommand(COMMAND_SHOULD_OVERRIDE);

	m_DataLabel = new Label(this, "DataLabel", "");
	m_DataLabel->SetEnabled(false);

	m_DataSlider = new CMapPropertiesPanelSlider(this, "DataSlider", 250, false);
	m_DataSlider->SetEnabled(false);

	m_DataButton = new Button(this, "DataButton", "", this, COMMAND_SET_COLOR);
	m_DataButton->SetEnabled(false);
	m_DataButton->SetVisible(false);

	m_FilterBox = new CMapPropertiesEditorComboBox(this, "FilterBox", 20, false);
	m_FilterBox->SetEnabled(false);
	m_FilterBox->SetVisible(false);
	
	m_SkyboxBox = new CMapPropertiesEditorComboBox(this, "FilterBox", 20, false);
	m_SkyboxBox->SetEnabled(false);
	m_SkyboxBox->SetVisible(false);

	m_DataButtonRect = Rect_t{ 0, 0, 0, 0 };
	m_DataButtonColor.SetColor(0, 0, 0, 0);

	m_ShouldOverrideColorButton = new CheckButton(this, "DataButton", "");
	m_ShouldOverrideColorButton->SetEnabled(false);
	m_ShouldOverrideColorButton->SetVisible(false);
	m_ShouldOverrideColorButton->SetSelected(true);

	//create the set size button
	m_SetSizeButton = new Button(this, "RenameButton", "", this, COMMAND_SET_SIZE);
	m_SetSizeButton->SetEnabled(false);
	
	//create the mins/maxs text entries
	m_MinsTextEntry = new TextEntry(this, "MinsTextEntry");
	m_MinsTextEntry->AddActionSignalTarget(this);
	m_MinsTextEntry->SetEnabled(false);
	m_MinsTextEntry->SetText("0 0 0");
	
	m_MaxsTextEntry = new TextEntry(this, "MaxsTextEntry");
	m_MaxsTextEntry->AddActionSignalTarget(this);
	m_MaxsTextEntry->SetEnabled(false);
	m_MaxsTextEntry->SetText("0 0 0");

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	//make the fog lerp slider
	m_FogLerpSliderText = new Label(this, "LerpSliderText", "");

	m_FogLerpSlider = new CMapPropertiesPanelSlider(this, "LerpSlider");
	m_FogLerpSlider->SetEnabled(false);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

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

	//collect our color corrections. ALWAYS add an empty color correction first
	{
		m_FilterBox->AddItem("#Amod_SkyboxPanel_UseDefaultFilter", new KeyValues(""));

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
			m_FilterBox->AddItem(first, new KeyValues(cc_string));

			//next file
			first = filesystem->FindNext(find);
		}
		m_FilterBox->ActivateItem(0);
	}

	//collect our skyboxs. ALWAYS add an empty skybox first
	do
	{
		m_SkyboxBox->AddItem("#Amod_SkyboxPanel_UseDefaultSkybox", new KeyValues(""));

		//load keyvalues file
		KeyValuesAD keyvalues(new KeyValues("SkyPanel"));
		if (!keyvalues->LoadFromFile(filesystem, "resource/panels/skypanel.txt"))
			break;

		KeyValues* timekey = keyvalues->FindKey(m_bNightTimeMode ? "Night" : "Day");
		if (!timekey)
			break;

		//go through each key
		FOR_EACH_VALUE(timekey, value)
		{
			//remove percent sigh
			char temp[512];
			Q_strncpy(temp, value->GetString(), sizeof(temp));
			char* percent = Q_strstr(temp, "%");
			if (percent)
				*percent = '\0';

			//add the item
			m_SkyboxBox->AddItem(value->GetName(), new KeyValues(temp));
		}

	} while (false);

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
	//disable our debug var
	amod_fog_cubeinfo_debug.SetValue(false);

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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	BaseClass::PerformLayout();

	//apply button
	//ApplySettingsToPanel(m_KeyValuesFile->FindKey("ApplyButton"), m_ApplyButton);

	//data stuff
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("ShouldOverrideButton"), m_ShouldOverrideButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataLabel"), m_DataLabel);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataSlider"), m_DataSlider);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataFilter"), m_FilterBox);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataFilter"), m_SkyboxBox);	//HACK: set m_SkyboxBox to the bounds of the filter combo box
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataShouldOverrideColor"), m_ShouldOverrideColorButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("DataButton"), m_DataButton);
	sscanf(m_KeyValuesFile->GetString("DataColorRect"), "%d %d %d %d", &m_DataButtonRect.x, &m_DataButtonRect.y, &m_DataButtonRect.width, &m_DataButtonRect.height);

	//set size button
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("SetSizeButton"), m_SetSizeButton);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("MinsTextEntry"), m_MinsTextEntry);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("MaxsTextEntry"), m_MaxsTextEntry);

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	//lerp slider stuff
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TransitionSliderText"), m_FogLerpSliderText);
	ApplySettingsToPanel(m_KeyValuesFile->FindKey("TransitionSlider"), m_FogLerpSlider);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

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

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		trigger.lerptime = m_TriggerInfos[i].lerptime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//add to the trigger list
	int index = m_TriggerList->AddItem(new KeyValues("Trigger", "TriggersName", name), m_TriggerList->GetItemCount(), false, false);
	m_TriggerList->ClearSelectedItems();
	m_TriggerList->AddSelectedItem(index);

	if (!addtoarray)
		return;
	
	//add to m_TriggerInfos
	MapTimeInfo_t::FogCubeTrigger_t& trigger = m_TriggerInfos[m_TriggerInfos.AddToTail()];
	memset(&trigger, 0, sizeof(trigger));
	Q_strncpy(trigger.name, name, sizeof(trigger.name));

	//add the 'fog_lerp_system_lerp_time' var
	if (Data && Data->foginfo.Count() <= 0)
		AddOrUpdateFogInfoInArray(trigger.foginfo, "fog_lerp_system_lerp_time", "0");

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
#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		trigger.lerptime = Data->lerptime;
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		trigger.mins = Data->mins;
		trigger.maxs = Data->maxs;
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Renames a fog trigger
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::RenameFogTrigger(int index, const char* newname)
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

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

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		m_FogLerpSlider->SetEnabled(true);
		m_FogLerpSlider->SetValue(m_TriggerInfos[index].lerptime * LERP_SLIDER_DIVISOR);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		m_SetSizeButton->SetEnabled(true);
		m_MinsTextEntry->SetEnabled(true);
		m_MaxsTextEntry->SetEnabled(true);

		//set the mins/maxs text
		m_MinsTextEntry->SetText(CFmtStr("%.0f %.0f %.0f", m_TriggerInfos[index].mins.x, m_TriggerInfos[index].mins.y, m_TriggerInfos[index].mins.z));
		m_MaxsTextEntry->SetText(CFmtStr("%.0f %.0f %.0f", m_TriggerInfos[index].maxs.x, m_TriggerInfos[index].maxs.y, m_TriggerInfos[index].maxs.z));

		//add our vars
		for (int i = 0; i < MAX_FOG_VARIABLES; i++)
		{
			const char* text = CFmtStr("%s - Override = %d", FogDatas[i].text, FindFogInfoFromArray(m_TriggerInfos[index].foginfo, FogDatas[i].convar, nullptr) != nullptr);
			m_VarList->AddItem(new KeyValues("Data", "TriggersData", FogDatas[i].type == FogTriggerData_t::TYPE_DIVIDER ? "" : text), m_VarList->GetItemCount(), false, false);
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
		bool enabled = info != nullptr && FogDatas[index].type != FogTriggerData_t::TYPE_DIVIDER;

		//enable our items
		//m_ApplyButton->SetEnabled(true);
		m_ShouldOverrideButton->SetEnabled(FogDatas[index].type != FogTriggerData_t::TYPE_DIVIDER);
		m_ShouldOverrideButton->SetSelected(enabled);

		//set our data stuff
		m_DataLabel->SetEnabled(enabled);

		//enable the color buttons
		m_DataButton->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER);
		m_DataButton->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER && enabled);
		m_DataButton->GetTooltip()->SetText(FogDatas[index].tooltip);
		m_ShouldOverrideColorButton->SetVisible(m_DataButton->IsVisible());
		m_ShouldOverrideColorButton->SetEnabled(m_DataButton->IsEnabled());

		//enable the data sliders
		m_DataSlider->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER);
		m_DataSlider->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER && enabled);
		m_DataSlider->GetTooltip()->SetText(FogDatas[index].tooltip);

		//enable the filters
		m_FilterBox->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_FILTER);
		m_FilterBox->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_FILTER && enabled);
		m_FilterBox->GetTooltip()->SetText(FogDatas[index].tooltip);

		//enable the skybox
		m_SkyboxBox->SetVisible(FogDatas[index].type == FogTriggerData_t::TYPE_SKYBOX);
		m_SkyboxBox->SetEnabled(FogDatas[index].type == FogTriggerData_t::TYPE_SKYBOX && enabled);
		m_SkyboxBox->GetTooltip()->SetText(FogDatas[index].tooltip);

		//set our var
		if (FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER && enabled)
		{
			//set button color
			int r = 0, g = 0, b = 0;
			sscanf(info ? info : "-1 -1 -1", "%d %d %d", &r, &g, &b);
			m_DataButtonColor._color[0] = (unsigned char)r;
			m_DataButtonColor._color[1] = (unsigned char)g;
			m_DataButtonColor._color[2] = (unsigned char)b;

			//check for -1 -1 -1
			if (r == -1 && g == -1 && b == -1)
			{
				m_ShouldOverrideColorButton->SetSelected(false);
				m_DataButton->SetEnabled(false);
			}

			return;
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER)
		{
			//set info
			info = info ? info : "";

			//set the slider stuff
			m_DataSlider->SetRange(FogDatas[index].sval_min, FogDatas[index].sval_max);
			m_DataSlider->SetValue((int)(atof(info) * FogDatas[index].val_divisor));
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_FILTER)
		{
			//check for info
			if (!info)
			{
				m_FilterBox->ActivateItem(0);
				return;
			}

			//look for the item
			for (int i = 0; i < m_FilterBox->GetItemCount(); i++)
			{
				if (!Q_stricmp(m_FilterBox->GetItemUserData(i)->GetName(), info))
				{
					m_FilterBox->ActivateItem(i);
					break;
				}
			}
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_SKYBOX)
		{
			//check for info
			if (!info)
			{
				m_SkyboxBox->ActivateItem(0);
				return;
			}

			//look for the item
			for (int i = 0; i < m_SkyboxBox->GetItemCount(); i++)
			{
				if (!Q_stricmp(m_SkyboxBox->GetItemUserData(i)->GetName(), info))
				{
					m_SkyboxBox->ActivateItem(i);
					break;
				}
			}
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
		m_MinsTextEntry->SetEnabled(false);
		m_MaxsTextEntry->SetEnabled(false);

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
		m_FogLerpSlider->SetEnabled(false);
		m_FogLerpSlider->SetValue(0);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

		//remove all the items from m_VarList
		m_VarList->RemoveAll();

		//disables the size editor mode
		m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

		//tell our property sheet to regain keyboard input
		GetParent()->SetKeyBoardInputEnabled(true);
		GetParent()->SetMouseInputEnabled(true);
	}
	else
	{
		//set our data stuff
		m_ShouldOverrideButton->SetEnabled(false);
		//m_ApplyButton->SetEnabled(false);

		//hide the data label
		m_DataLabel->SetEnabled(false);
		m_DataLabel->SetText("#MapProperties_FogTriggersPage_NoItemSelected");

		//hide the data button
		m_DataButton->SetVisible(false);
		m_DataButton->SetEnabled(false);
		m_ShouldOverrideColorButton->SetVisible(false);
		m_ShouldOverrideColorButton->SetEnabled(false);
		
		//hide the filter box
		m_FilterBox->SetVisible(false);
		m_FilterBox->SetEnabled(false);
		
		//hide the skybox box
		m_SkyboxBox->SetVisible(false);
		m_SkyboxBox->SetEnabled(false);

		//show only the data slider
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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//check index
	int index = m_TriggerList->GetSelectedItem(0);
	if (index < 0 || index >= m_TriggerInfos.Count())
		return;

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	//check whos which slider
	if (data->GetPtr("Panel") == m_FogLerpSlider)
	{
		//set the lerp time
		m_TriggerInfos[index].lerptime = (float)m_FogLerpSlider->GetValue() / LERP_SLIDER_DIVISOR;
	}
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

	UpdateArrayValue();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when some text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnTextChanged(KeyValues* data)
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	Panel* panelptr = (Panel*)data->GetPtr("panel");

	//update the array value
	if (panelptr == m_FilterBox || panelptr == m_SkyboxBox)
		UpdateArrayValue();

	//check for the size text entries
	else if (panelptr == m_MinsTextEntry || panelptr == m_MaxsTextEntry)
	{
		//get the mins
		char mins[64];
		m_MinsTextEntry->GetText(mins, sizeof(mins));
		sscanf(mins, "%f %f %f", &m_Sizes[0].x, &m_Sizes[0].y, &m_Sizes[0].z);
			
		//get the maxs
		char maxs[64];
		m_MaxsTextEntry->GetText(maxs, sizeof(maxs));
		sscanf(maxs, "%f %f %f", &m_Sizes[1].x, &m_Sizes[1].y, &m_Sizes[1].z);

		//update the trigger bbox
		UpdateSizes();
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Re-populates the trigger list
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::RepopulateList()
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//remove + re-add
	m_TriggerList->RemoveAll();

	for (int i = 0; i < m_TriggerInfos.Count(); i++)
		AddFogTrigger(m_TriggerInfos[i].name, &m_TriggerInfos[i], false);
}

void UpdateFogTriggers(CUtlVector<MapTimeInfo_t::FogCubeTrigger_t>& CubeTriggers, bool UpdateFogTriggers);

//-------------------------------------------------------------------------------------------------------
// Purpose: Called whent the page is hidden
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnPageHide()
{
	//disables the size editor mode
	m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

	//tell our property sheet to regain keyboard input
	GetParent()->SetKeyBoardInputEnabled(true);
	GetParent()->SetMouseInputEnabled(true);

	BaseClass::OnPageHide();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::Update()
{
	Update(false);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on panel think
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::Update(bool ForceUpdate)
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

#if FOG_CUBE_TRIGGER_TEST_VERSION_1
	//set our m_FogLerpSliderText text
	wchar_t* lerptext = g_pVGuiLocalize->Find("MapProperties_FogTriggersPage_TransitionLabel");
	wchar_t text[128];
	swprintf(text, SIZE_OF_ARRAY(text), L"%ws %.2f", lerptext, (float)m_FogLerpSlider->GetValue() / LERP_SLIDER_DIVISOR);
	m_FogLerpSliderText->SetText(text);
#endif //FOG_CUBE_TRIGGER_TEST_VERSION_1

	//set s_bUpdateTriggerValuesThisFrame if this isnt the active page
	if (((PropertyDialog*)GetParent()->GetParent())->GetActivePage() != this || ForceUpdate)
		s_bUpdateTriggerValuesThisFrame = true;

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

	//set our label
	int value = m_DataSlider->GetValue();
	if (value == -1)
		m_DataLabel->SetText(CFmtStr("%s: -1", FogDatas[index].text));
	else
		m_DataLabel->SetText(CFmtStr("%s: %.2f", FogDatas[index].text, ((float)value / FogDatas[index].val_divisor)));
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::TeleportToTrigger(int num)
{
	//enable noclip
	engine->ClientCmd("noclip_set 1");

	//dont do the following code if both bounds are the same
	if (m_Sizes[0] == m_Sizes[1])
		return;

	//teleport to the first trigger
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	//teleport to the mins
	Vector forward;
	AngleVectors(pPlayer->EyeAngles(), &forward);

	Vector targetPos = m_Sizes[num] - (forward * 100.0f);

	Vector dir = m_Sizes[num] - targetPos;
	QAngle ang;
	VectorAngles(dir, ang);

	//HACK: use setpos + setang
	engine->ClientCmd(CFmtStr("setpos %f %f %f", targetPos.x, targetPos.y, targetPos.z - 64.0f));
	engine->ClientCmd(CFmtStr("setang %f %f %f", ang.x, ang.y, ang.z));
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
			modal->SetOKCommand(new KeyValues("Command", "command", "DoModal"));
			modal->SetCancelCommand(new KeyValues("Command", "command", "DoModal"));
			bDidPrompt = true;
		}

		//reset our mins/maxs
		m_Sizes[0] = m_TriggerInfos[index].mins;
		m_Sizes[1] = m_TriggerInfos[index].maxs;
		m_iInSizeEditor = SIZE_EDITOR_MODE_MINS;

		//disable the mins/maxs entries
		m_MinsTextEntry->SetEnabled(false);
		m_MaxsTextEntry->SetEnabled(false);

		//tell our property sheet to lose keyboard input
		GetParent()->SetKeyBoardInputEnabled(false);
		GetParent()->SetMouseInputEnabled(false);
	
		//teleport 
		TeleportToTrigger(0);
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
	//else if (!Q_stricmp(pszCommand, COMMAND_APPLY_SETTING))
	//{
	//	UpdateArrayValue();
	//}

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

	//check for DoModal
	else if (!Q_stricmp(pszCommand, "DoModal"))
	{
		//redirect to panel (this->property sheet->panel)
		GetParent()->GetParent()->OnCommand("DoModal");
	}

	BaseClass::OnCommand(pszCommand);
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

	//check for divider
	if (FogDatas[index].type == FogTriggerData_t::TYPE_DIVIDER)
		return;

	//check if the override button is selected
	if (!m_ShouldOverrideButton->IsSelected())
	{
		RemoveFogInfoInArray(m_TriggerInfos[trigger_index].foginfo, FogDatas[index].convar);
	}
	else
	{
		const char* string = "";

		//get our string to add to the data array
		if (FogDatas[index].type == FogTriggerData_t::TYPE_SLIDER)
		{
			//get the value
			int value = m_DataSlider->GetValue();

			//check for -1
			if (value == -1)
				string = "-1";
			else
				string = CFmtStr("%f", (float)m_DataSlider->GetValue() / FogDatas[index].val_divisor);
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_COLORPICKER)
		{
			//check m_ShouldOverrideColorButton
			if (m_ShouldOverrideColorButton->IsSelected())
				string = CFmtStr("%d %d %d", m_DataButtonColor.r(), m_DataButtonColor.g(), m_DataButtonColor.b());
			else
				string = "-1 -1 -1";
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_FILTER)
		{
			string = m_FilterBox->GetActiveItemUserData()->GetName();
		}
		else if (FogDatas[index].type == FogTriggerData_t::TYPE_SKYBOX)
		{
			string = m_SkyboxBox->GetActiveItemUserData()->GetName();
		}

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

	//set s_bUpdateTriggerValuesThisFrame to make sure we update the trigger values
	s_bUpdateTriggerValuesThisFrame = true;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the override button is pressed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnOverrideButtonEnabled()
{
	//enable our items
	m_DataLabel->SetEnabled(m_DataLabel->IsVisible());
	m_DataButton->SetEnabled(m_DataButton->IsVisible());
	m_ShouldOverrideColorButton->SetEnabled(m_ShouldOverrideColorButton->IsVisible());
	m_DataSlider->SetEnabled(m_DataSlider->IsVisible());
	m_FilterBox->SetEnabled(m_FilterBox->IsVisible());
	m_SkyboxBox->SetEnabled(m_SkyboxBox->IsVisible());

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
	m_ShouldOverrideColorButton->SetEnabled(false);
	m_DataSlider->SetEnabled(false);
	m_FilterBox->SetEnabled(false);
	m_SkyboxBox->SetEnabled(false);

	//update our value
	UpdateArrayValue();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Updates the sizes of the trigger
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::UpdateSizes()
{
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

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when the page is shown
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnPageShow()
{
	//hack: reset run the OnItemSelected function
	int selected = m_TriggerList->GetSelectedItem(0);
	m_TriggerList->ClearSelectedItems();
	m_TriggerList->AddSelectedItem(selected);
	BaseClass::OnPageShow();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a key code is pressed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnKeyCodePressed(KeyCode code)
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	if ((code == KeyCode::KEY_LSHIFT || code == KeyCode::KEY_RSHIFT) && m_iInSizeEditor != SIZE_EDITOR_MODE_NONE)
	{
		//check m_iInSizeEditor
		if (m_iInSizeEditor == SIZE_EDITOR_MODE_MINS)
		{
			//set
			m_iInSizeEditor = SIZE_EDITOR_MODE_MAXS;
			
			//set m_MinsTextEntry
			m_MinsTextEntry->SetText(CFmtStr("%.2f %.2f %.2f", m_Sizes[0].x, m_Sizes[0].y, m_Sizes[0].z));

			//teleport ONLY if the maxs isnt 0 0 0
			if (m_Sizes[1] != vec3_origin)
				TeleportToTrigger(1);
		}
		else
		{
			//enable the mins/maxs entries
			m_MinsTextEntry->SetEnabled(true);
			m_MaxsTextEntry->SetEnabled(true);

			//tell our property sheet to regain keyboard input
			GetParent()->SetKeyBoardInputEnabled(true);
			GetParent()->SetMouseInputEnabled(true);

			//set m_MaxsTextEntry
			m_MaxsTextEntry->SetText(CFmtStr("%.2f %.2f %.2f", m_Sizes[1].x, m_Sizes[1].y, m_Sizes[1].z));

			//set our mode
			m_iInSizeEditor = SIZE_EDITOR_MODE_NONE;

			//update the sizes
			UpdateSizes();
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
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

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

	//update the array
	UpdateArrayValue();

	//call base func
	BaseClass::OnColorSelected(data);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Override OnCheckButtonChecked
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelFogTriggersPage::OnCheckButtonChecked(KeyValues* subkey)
{
	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//check the panel
	if (subkey->GetPtr("panel") == m_ShouldOverrideColorButton)
	{
		m_DataButton->SetEnabled(m_ShouldOverrideColorButton->IsSelected());
	}

	UpdateArrayValue();
}

#endif //FOG_CUBE_TRIGGER_TEST