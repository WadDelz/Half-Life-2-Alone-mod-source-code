#include "cbase.h"
#include "MapPropertiesEditorPanelBasePage.h"

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the fog info array
//----------------------------------------------------------------------------------------------------
const char* FindFogInfoFromArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* def)
{
	for (int i = 0; i < info.Count(); i++)
	{
		if (!Q_stricmp(StringFromMapTimeStringTableIndex(info[i].convar), key))
			return StringFromMapTimeStringTableIndex(info[i].value);
	}

	return def;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a key/value to the fog info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, UtlSymId_t value)
{
	//search for existing key
	for (int i = 0; i < info.Count(); i++)
	{
		if (info[i].convar == key)
		{
			//update existing value
			info[i].value = value;
			return;
		}
	}

	//key not found, add new entry
	MapTimeInfo_t::FogInfo_t& newInfo = info[info.AddToTail()];
	newInfo.convar = key;
	newInfo.value = value;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Adds a key/value to the fog info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* value)
{
	AddOrUpdateFogInfoInArray(info, StringToMapTimeStringTableIndex(key), StringToMapTimeStringTableIndex(value));
}

//----------------------------------------------------------------------------------------------------
// Purpose: Finds the string from the sun info array
//----------------------------------------------------------------------------------------------------
const char* FindSunInfoFromArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* def)
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
void AddOrUpdateSunInfoInArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* value)
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

//undo stuff
UndoStep_t s_UndoSteps[MAX_UNDO_STEPS];
int s_CurrentUndoStep = 0; // next position for undo
int s_UndoStepsCount = 0;   // total valid steps
bool s_NeedSave = false;

//-----------------------------------------------------------------------------
// Purpose: internal step set func
//-----------------------------------------------------------------------------
static void AddUndoStep(UndoStep_t::StepType_e type, const UndoStep_t& data)
{
	// truncate redo steps if we added after undo
	if (s_CurrentUndoStep < s_UndoStepsCount)
		s_UndoStepsCount = s_CurrentUndoStep;

	// shift if full
	if (s_CurrentUndoStep >= MAX_UNDO_STEPS)
	{
		memmove(s_UndoSteps, s_UndoSteps + 1, sizeof(s_UndoSteps) - sizeof(UndoStep_t));
		s_CurrentUndoStep = MAX_UNDO_STEPS - 1;
		s_UndoStepsCount = MAX_UNDO_STEPS - 1;
	}

	UndoStep_t& step = s_UndoSteps[s_CurrentUndoStep];
	step.m_CurrentStepType = type;

	switch (type)
	{
	case UndoStep_t::StepType_e::Step_SetSlider:
		step.m_Data.SliderData = data.m_Data.SliderData;
		break;
	case UndoStep_t::StepType_e::Step_SetColor:
		step.m_Data.ColorData = data.m_Data.ColorData;
		break;
	case UndoStep_t::StepType_e::Step_SetCheckButton:
		step.m_Data.ButtonData = data.m_Data.ButtonData;
		break;
	}

	s_CurrentUndoStep++;
	if (s_UndoStepsCount < s_CurrentUndoStep)
		s_UndoStepsCount = s_CurrentUndoStep;

	//set our s_NeedSave
	s_NeedSave = true;
}

//-----------------------------------------------------------------------------
// Purpose: add an undo step for a slider value
//-----------------------------------------------------------------------------
void AddUndo_SetSlider(Slider* slider, int previousValue)
{
	UndoStep_t data;
	data.m_Data.SliderData.m_SetSlider = slider;
	data.m_Data.SliderData.m_GetValue = previousValue;
	AddUndoStep(UndoStep_t::StepType_e::Step_SetSlider, data);
}

//-----------------------------------------------------------------------------
// Purpose: add an undo step for a color change
//-----------------------------------------------------------------------------
void AddUndo_SetColor(Color* setColor, const unsigned char previouscolor[4])
{
	UndoStep_t data;
	data.m_Data.ColorData.m_SetColor = setColor;
	memcpy(data.m_Data.ColorData.m_GetColor, previouscolor, 4);
	memset(data.m_Data.ColorData.m_PreviousColor, 0, sizeof(unsigned char) * 4);
	AddUndoStep(UndoStep_t::StepType_e::Step_SetColor, data);
}

//HACK HACK VERY EVIL HACK (not really): 
//when we set the check buttons value for the UndoStep_Apply function. That calls the 'CheckButtonChecked' action signal.
//That then adds another check button undo step onto the undo steps. So prevent this
static bool g_bCurrentlyInSuperEvilCheckButtonHack = false;

//-----------------------------------------------------------------------------
// Purpose: add an undo step for a checkbutton
//-----------------------------------------------------------------------------
void AddUndo_SetCheckButton(CheckButton* button, bool previousValue)
{
	if (g_bCurrentlyInSuperEvilCheckButtonHack)
	{
		g_bCurrentlyInSuperEvilCheckButtonHack = false;
		return;
	}

	UndoStep_t data;
	data.m_Data.ButtonData.m_SetCheckButton = button;
	data.m_Data.ButtonData.m_SetCheckButtonValue = previousValue;
	AddUndoStep(UndoStep_t::StepType_e::Step_SetCheckButton, data);
}

//-----------------------------------------------------------------------------
// Purpose: applies an undo or redo step
//-----------------------------------------------------------------------------
void UndoStep_Apply(bool undo)
{
	int pos = undo ? s_CurrentUndoStep - 1 : s_CurrentUndoStep;
	if (pos < 0 || pos >= s_UndoStepsCount)
		return;

	UndoStep_t& step = s_UndoSteps[pos];

	switch (step.m_CurrentStepType)
	{
	case UndoStep_t::StepType_e::Step_SetSlider:
	{
		Slider* slider = step.m_Data.SliderData.m_SetSlider;
		if (!slider) break;

		int temp = slider->GetValue();
		slider->SetValue(step.m_Data.SliderData.m_GetValue);
		step.m_Data.SliderData.m_GetValue = temp;
		break;
	}
	case UndoStep_t::StepType_e::Step_SetColor:
	{
		Color* target = step.m_Data.ColorData.m_SetColor;
		if (!target) break;

		unsigned char temp[4];

		if (undo)
		{
			// store current color for redo
			temp[0] = target->r();
			temp[1] = target->g();
			temp[2] = target->b();
			temp[3] = target->a();

			// apply undo color
			target->SetColor(step.m_Data.ColorData.m_GetColor[0],
				step.m_Data.ColorData.m_GetColor[1],
				step.m_Data.ColorData.m_GetColor[2],
				step.m_Data.ColorData.m_GetColor[3]);

			// store current color for redo
			memcpy(step.m_Data.ColorData.m_PreviousColor, temp, 4);
		}
		else
		{
			// store current color for undo
			temp[0] = target->r();
			temp[1] = target->g();
			temp[2] = target->b();
			temp[3] = target->a();

			// apply redo color
			target->SetColor(step.m_Data.ColorData.m_PreviousColor[0],
				step.m_Data.ColorData.m_PreviousColor[1],
				step.m_Data.ColorData.m_PreviousColor[2],
				step.m_Data.ColorData.m_PreviousColor[3]);

			// store current color for undo
			memcpy(step.m_Data.ColorData.m_GetColor, temp, 4);
		}
		break;
	}

	case UndoStep_t::StepType_e::Step_SetCheckButton:
	{
		CheckButton* button = step.m_Data.ButtonData.m_SetCheckButton;
		if (!button) break;

		bool temp = button->IsSelected();

		//HACK:
		g_bCurrentlyInSuperEvilCheckButtonHack = true;
		button->SetSelected(step.m_Data.ButtonData.m_SetCheckButtonValue);
		step.m_Data.ButtonData.m_SetCheckButtonValue = temp;
		break;
	}
	}

	if (undo)
		s_CurrentUndoStep--;
	else
		s_CurrentUndoStep++;

	//set our s_NeedSave
	s_NeedSave = true;
}











//----------------------------------------------------------------------------------------------------
// Purpose: Number only text entry
//----------------------------------------------------------------------------------------------------
class CNumberTextEntry : public TextEntry
{
	DECLARE_CLASS_SIMPLE(CNumberTextEntry, TextEntry)
public:
	CNumberTextEntry(Panel* parent, const char* name) : BaseClass(parent, name) {}

	//only allow a-z A-Z 0-9 _ - + :
	void OnKeyTyped(wchar_t code) override
	{
		if (code < '0' || code > '9')
		{
			surface()->PlaySound("resource/warning.wav");
			return;
		}

		BaseClass::OnKeyTyped(code);
	}
};







//----------------------------------------------------------------------------------------------------
// Purpose: Set the exact value of a slider panel
//----------------------------------------------------------------------------------------------------
class CSetSliderValuePanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CSetSliderValuePanel, Frame);
public:
	CSetSliderValuePanel(Panel* parent, const char* name, const char* text);
	~CSetSliderValuePanel();

	//command funcs
	void OnCommand(const char* pszCommand);
private:
	//save text entry
	CNumberTextEntry* m_TextEntry;
};

//singleton
static CSetSliderValuePanel* gs_SetExactValueSliderPanel = nullptr;

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder panel
//----------------------------------------------------------------------------------------------------
CSetSliderValuePanel::CSetSliderValuePanel(Panel* parent, const char* name, const char* text) : BaseClass(parent, name)
{
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

	//create the text entry
	m_TextEntry = new CNumberTextEntry(this, "NumberTextEntry");
	m_TextEntry->SetBounds(5, 25, 290, 25);
	m_TextEntry->SetMaximumCharCount(6);
	m_TextEntry->SetText(text);

	//create the Save and Cancel button
	Button* m_SaveButton = new Button(this, "SetButton", "Set Value");
	m_SaveButton->SetBounds(5, 55, 142, 25);
	m_SaveButton->SetCommand("Set");

	Button* m_CancelButton = new Button(this, "CancelButton", "Cancel");
	m_CancelButton->SetBounds(152, 55, 142, 25);
	m_CancelButton->SetCommand("Close");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CSetSliderValuePanel::OnCommand(const char* pszCommand)
{
	//check for save
	if (!Q_stricmp(pszCommand, "Set"))
	{
		//get the text
		char buf[512];
		m_TextEntry->GetText(buf, sizeof(buf));

		//post the message
		PostActionSignal(new KeyValues("SetExactValue", "Value", atoi(buf)));

		//close this
		Close();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: destructor
//----------------------------------------------------------------------------------------------------
CSetSliderValuePanel::~CSetSliderValuePanel()
{
	gs_SetExactValueSliderPanel = nullptr;
}








//current coppied value
static int g_bHasValueCoppied = INT_MAX;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties panel slider
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanelSlider::CMapPropertiesPanelSlider(Panel* parent, const char* name, int wheeldelta)
	: BaseClass(parent, name, wheeldelta)
{
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when the mouse is wheeled
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSlider::OnMouseWheeled(int delta)
{
	if (!IsEnabled())
		return;

	//do the mouse wheel
	int previous = GetValue();
	BaseClass::OnMouseWheeled(delta);

	//add an undo step if needed
	if (previous != GetValue())
		AddUndo_SetSlider(this, previous);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a mouse button is pressed
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSlider::OnMousePressed(MouseCode code)
{
	//check for the right mouse button
	if (code == MouseCode::MOUSE_RIGHT)
		return;

	//check for left mouse button
	if (code == MouseCode::MOUSE_LEFT && IsEnabled())
		m_PreviousValue = GetValue();

	BaseClass::OnMousePressed(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a mouse button is released
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSlider::OnMouseReleased(MouseCode code)
{
	if (code == MouseCode::MOUSE_RIGHT && IsEnabled())
	{
		//get cursor pos
		int x, y;
		vgui::surface()->SurfaceGetCursorPos(x, y);

		//make a popup prompting the user to copy or paste OR set the exact value
		Menu* menu = new Menu(this, "CopyMenu");
		menu->AddMenuItem("Set", "Set exact value", "SetExactValue", this);
		menu->AddSeparator();
		menu->AddMenuItem("CopyToClipboard", "Copy", "Copy", this);

		//check if we have a value coppied
		if (g_bHasValueCoppied != INT_MAX)
			menu->AddMenuItem("PasteFromClipboard", "Paste", "Paste", this);

		menu->SetBounds(x, y, 200, 50);
		menu->SetVisible(true);
		return;
	}

	//check for left mouse button
	if (code == MouseCode::MOUSE_LEFT && IsEnabled())
	{
		AddUndo_SetSlider(this, m_PreviousValue);
	}

	BaseClass::OnMouseReleased(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSlider::OnCommand(const char* pszCommand)
{
	//check for copy or paste
	if (!Q_stricmp(pszCommand, "Copy"))
	{
		g_bHasValueCoppied = GetValue();
	}
	else if (!Q_stricmp(pszCommand, "Paste"))
	{
		SetValue(g_bHasValueCoppied);
	}
	else if (!Q_stricmp(pszCommand, "SetExactValue"))
	{
		//open the save dialog
		if (gs_SetExactValueSliderPanel)
			gs_SetExactValueSliderPanel->DeletePanel();

		gs_SetExactValueSliderPanel = new CSetSliderValuePanel(this, "SetValuePanel", CFmtStr("%d", GetValue()));
		gs_SetExactValueSliderPanel->DoModal();
		gs_SetExactValueSliderPanel->AddActionSignalTarget(this);
		gs_SetExactValueSliderPanel->Activate();
		gs_SetExactValueSliderPanel->SetTitle(CFmtStr("%s: Set Slider Value", GetName()), true);
		return;
	}

	//call the base func
	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Returns the minimum value of the slider
//----------------------------------------------------------------------------------------------------
int CMapPropertiesPanelSlider::GetMin()
{
	int min, _;
	GetRange(min, _);
	return min;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Returns the maximum value of the slider
//----------------------------------------------------------------------------------------------------
int CMapPropertiesPanelSlider::GetMax()
{
	int _, max;
	GetRange(_, max);
	return max;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Sets the value of the slider from the panel
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSlider::OnSetExactValue(KeyValues* data)
{
	SetValue(data->GetInt("Value"));
}







//current coppied color
static bool s_bHasCoppiedColor;
static Color s_CurrentCoppiedColor;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for the map properties panel button
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanelButton::CMapPropertiesPanelButton(Panel* parent, const char* name, const char* text)
	: BaseClass(parent, name, text)
{
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called when a mouse button is released
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelButton::OnMouseReleased(MouseCode code)
{
	if (code == MouseCode::MOUSE_RIGHT && IsEnabled())
	{
		//get cursor pos
		int x, y;
		vgui::surface()->SurfaceGetCursorPos(x, y);

		//make a popup prompting the user to copy or paste
		Menu* menu = new Menu(this, "CopyMenu");
		menu->AddMenuItem("CopyToClipboard", "Copy", "Copy", this);

		//check if we have a value coppied
		if (s_bHasCoppiedColor)
			menu->AddMenuItem("PasteFromClipboard", "Paste", "Paste", this);

		menu->SetBounds(x, y, 200, 50);
		menu->SetVisible(true);
		return;
	}

	BaseClass::OnMouseReleased(code);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelButton::OnCommand(const char* pszCommand)
{
	//check for copy or paste
	if (!Q_stricmp(pszCommand, "Copy"))
	{
		//check for our color
		if (m_AttachedColor)
			s_CurrentCoppiedColor = *m_AttachedColor;

		s_bHasCoppiedColor = true;
	}
	else if (!Q_stricmp(pszCommand, "Paste"))
	{
		//check for our color
		if (m_AttachedColor)
			*m_AttachedColor = s_CurrentCoppiedColor;

		s_bHasCoppiedColor = true;
	}

	//call the base func
	BaseClass::OnCommand(pszCommand);
}










//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for map properties main page
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanelPageBase::CMapPropertiesPanelPageBase(Panel* parent, const char* name, const char* keyvaluesfile) : BaseClass(parent, name)
{
	//init our keyvalues and attempt to load our file
	m_KeyValuesFile = new KeyValues("PropertiesPanelDialog");
	if (!m_KeyValuesFile->LoadFromFile(filesystem, keyvaluesfile, "MOD"))
	{
		ConWarning("Error: Failed to load page: \"%s\"\n", keyvaluesfile);
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
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties main page
//----------------------------------------------------------------------------------------------------
CMapPropertiesPanelPageBase::~CMapPropertiesPanelPageBase()
{
	//delete our keyvalues
	if (m_KeyValuesFile)
		m_KeyValuesFile->deleteThis();

	m_KeyValuesFile = nullptr;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Performs the layout for this panel
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelPageBase::PerformLayout()
{
	BaseClass::PerformLayout();

	//check for m_KeyValuesFile
	if (!m_KeyValuesFile)
		return;

	//set our parents parents size (property sheet -> property panel)
	Panel* parent = GetParent();
	if (parent && parent->GetParent())
	{
		parent->GetParent()->SetSize(m_KeyValuesFile->GetInt("wide", parent->GetWide()), m_KeyValuesFile->GetInt("tall", parent->GetTall()));
	}

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
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a check button is checked
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelPageBase::OnCheckButtonChecked(KeyValues* subkey)
{
	AddUndo_SetCheckButton((CheckButton*)subkey->GetPtr("panel"), !((CheckButton*)subkey->GetPtr("panel"))->IsSelected());
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Applies settings to the Panel* from the keyvalues*
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelPageBase::ApplySettingsToPanel(KeyValues* subkey, Panel* panel)
{
	//check for subkey and panel
	if (!subkey || !panel)
		return;

	//set bounds
	int x = subkey->GetInt("x");
	int y = subkey->GetInt("y");
	int w = subkey->GetInt("w");
	int h = subkey->GetInt("h");
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
