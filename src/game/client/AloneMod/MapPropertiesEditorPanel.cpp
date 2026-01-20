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
// Purpose: Adds a key/value to the fog info array, or updates the value if the key already exists
//----------------------------------------------------------------------------------------------------
static void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, UtlSymId_t value)
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
static void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* value)
{
	AddOrUpdateFogInfoInArray(info, StringToMapTimeStringTableIndex(key), StringToMapTimeStringTableIndex(value));
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








//max undo steps
#define MAX_UNDO_STEPS 32

//undo struct
struct UndoStep_t
{
	//step type
	enum class StepType_e { Step_SetCheckButton, Step_SetColor, Step_SetSlider };
	StepType_e m_CurrentStepType;

	//data
	union
	{
		//slider
		struct
		{
			Slider* m_SetSlider;
			int m_GetValue;
		} SliderData;

		//color
		struct
		{
			unsigned char m_GetColor[4];
			unsigned char m_PreviousColor[4];
			Color* m_SetColor;
		} ColorData;

		//check button
		struct
		{
			CheckButton* m_SetCheckButton;
			bool m_SetCheckButtonValue;
		} ButtonData;
	} m_Data;
};

//undo stuff
static UndoStep_t s_UndoSteps[MAX_UNDO_STEPS];
static int s_CurrentUndoStep = 0; // next position for undo
static int s_UndoStepsCount = 0;   // total valid steps
static bool s_NeedSave = false;

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

//right click slider
class CMapPropertiesPanelSlider : public WheelSlider
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSlider, WheelSlider);

public:
	CMapPropertiesPanelSlider(Panel* parent, const char* name, int wheeldelta = 1);

	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseWheeled(int delta);

	virtual void OnCommand(const char* pszCommand);

	virtual int GetMin();
	virtual int GetMax();

	//called when the exact slider value is set from the panel
	MESSAGE_FUNC_PARAMS(OnSetExactValue, "SetExactValue", kv);
private:
	//previous value before the mouse button was presseed down
	int m_PreviousValue = 0;
};

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

//right click button
class CMapPropertiesPanelButton : public Button
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelButton, Button);

public:
	CMapPropertiesPanelButton(Panel* parent, const char* name, const char* text);

	virtual void OnMouseReleased(MouseCode code);
	
	//called on command
	virtual void OnCommand(const char* pszCommand);

	//attach functions
	virtual void SetAttatchedColor(Color* color) { m_AttachedColor = color; };
private:
	//color attached to this
	Color* m_AttachedColor = nullptr;
};

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










//map properties panel page base
class CMapPropertiesPanelPageBase : public PropertyPage
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelPageBase, PropertyPage)
public:
	CMapPropertiesPanelPageBase(Panel* parent, const char* name, const char* keyvaluesfile);
	~CMapPropertiesPanelPageBase();

	//format functions
	virtual void ApplySettingsToPanel(KeyValues* subkey, Panel* panel);
	virtual void PerformLayout();
	virtual void Update() = 0 {};

	//color selected
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data) {};
	MESSAGE_FUNC_PARAMS(OnCheckButtonChecked, "CheckButtonChecked", data);
protected:
	//keyvalues file
	KeyValues* m_KeyValuesFile = nullptr;

	//list of sliders + dividers
	CUtlVector<Label*> m_Labels;
	CUtlVector<Divider*> m_Dividers;
};

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










#define COMMAND_CHANGE_FOG_COLOR "SetFogColor"
#define COMMAND_CHANGE_FOG_SKYBOX_COLOR "SetFogSkyboxColor"
#define COMMAND_CHANGE_FOG_BLEND_COLOR "SetFogBlendColor"
#define COMMAND_CHANGE_FOG_SKYBOX_BLEND_COLOR "SetFogSkyboxBlendColor"

//fog page
class CMapPropertiesPanelFogPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelFogPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelFogPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void InitFogInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetFogInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
private:
	bool m_bNightTimeMode = false;

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
	
	//farz clipping plane
	CMapPropertiesPanelSlider* m_FarzClippingPlaneSlider;
	Label* m_FarzClippingPlaneLabel;
	CheckButton* m_FarzClippingPlaneOverride;



	//enable fog blend
	CheckButton* m_EnableFogBlendCheckButton;
	CheckButton* m_OverrideFogBlendCheckButton;

	//fog blend color
	CMapPropertiesPanelButton* m_FogBlendColorButton;
	CheckButton* m_FogBlendColorOverride;
	Color m_FogBlendColor;
	Rect_t m_FogBlendColorDrawRect;

	//fog blend angle
	CMapPropertiesPanelSlider* m_FogBlendAngleSlider;
	Label* m_FogBlendAngleLabel;
	CheckButton* m_FogBlendAngleOverride;



	//enable skybox fog blend
	CheckButton* m_EnableFogBlendSkyboxCheckButton;
	CheckButton* m_OverrideFogBlendSkyboxCheckButton;

	//fog blend color
	CMapPropertiesPanelButton* m_FogBlendSkyboxColorButton;
	CheckButton* m_FogBlendSkyboxColorOverride;
	Color m_FogBlendSkyboxColor;
	Rect_t m_FogBlendSkyboxColorDrawRect;

	//fog blend angle
	CMapPropertiesPanelSlider* m_FogBlendSkyboxAngleSlider;
	Label* m_FogBlendSkyboxAngleLabel;
	CheckButton* m_FogBlendSkyboxAngleOverride;

	//current color selector mode
	enum ColorSelectorMode { Color_Fog, Color_SkyboxFog, Color_BlendFog, Color_BlendSkyboxFog } m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;
};

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
		fog_maxdensity->SetValue(!_override || !m_FogDensityOverride->IsSelected() ? -1 : (float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax());
		fog_maxdensityskybox->SetValue(!_override || !m_FogSkyboxDensityOverride->IsSelected() ? -1 : (float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax());

		//farz
		r_farz->SetValue(!_override || !m_FarzClippingPlaneOverride->IsSelected() ? -1 : m_FarzClippingPlaneSlider->GetValue());

		//fog blend
		fog_blend->SetValue(_overrideblend && m_EnableFogBlendCheckButton->IsSelected());

		//fog blend colors
		fog_blendcolor->SetValue(!_overrideblend || !m_FogBlendColorOverride->IsSelected() ? "-1 -1 -1" : CFmtStr("%d %d %d", m_FogBlendColor.r(), m_FogBlendColor.g(), m_FogBlendColor.b()));

		//fog blend angle
		fog_blendangle->SetValue(!_overrideblend || !m_FogBlendAngleOverride->IsSelected() ? -1 : m_FogBlendAngleSlider->GetValue());


		//fog skybox blend
		fog_blendskybox->SetValue(_overrideskyboxblend && m_EnableFogBlendSkyboxCheckButton->IsSelected());

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
		m_FogBlendSkyboxColorButton->SetEnabled(_overrideskyboxblend&& m_FogBlendSkyboxColorOverride->IsSelected());

		//fog blend angle
		m_FogBlendSkyboxAngleOverride->SetEnabled(_overrideskyboxblend);
		m_FogBlendSkyboxAngleSlider->SetEnabled(_overrideskyboxblend&& m_FogBlendSkyboxAngleOverride->IsSelected());
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
	
	//check for fog blend color command
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_FOG_BLEND_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Fog Blend Color", true);
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
		m_ColorPicker->SetTitle("Set Fog Blend Color", true);
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
		AddUndo_SetColor(&m_FogColor, m_FogSkyboxColor._color);

		m_FogSkyboxColor = color;
		break;
	case ColorSelectorMode::Color_BlendFog:
		//add an undo step
		AddUndo_SetColor(&m_FogColor, m_FogBlendColor._color);

		m_FogBlendColor = color;
		break;
	case ColorSelectorMode::Color_BlendSkyboxFog:
		//add an undo step
		AddUndo_SetColor(&m_FogColor, m_FogBlendSkyboxColor._color);

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
			{	StringToMapTimeStringTableIndex("fog_enable"),				StringToMapTimeStringTableIndex(!m_OverrideEnableFogCheckButton->IsSelected() ?											"-1" :				CFmtStr("%d", m_EnableFogCheckButton->IsSelected())) },
			{	StringToMapTimeStringTableIndex("fog_enableskybox"),		StringToMapTimeStringTableIndex(!m_OverrideEnableSkyboxFogCheckButton->IsSelected() ?									"-1" :				CFmtStr("%d", m_EnableSkyboxFogCheckButton->IsSelected())) },

			//fog distanes
			{	StringToMapTimeStringTableIndex("fog_start"),				StringToMapTimeStringTableIndex(!m_FogStartOverride->IsSelected() ?														"-1" :				CFmtStr("%d", m_FogStartSlider->GetValue())) },
			{	StringToMapTimeStringTableIndex("fog_end"),					StringToMapTimeStringTableIndex(!m_FogEndOverride->IsSelected() ?														"-1" :				CFmtStr("%d", m_FogEndSlider->GetValue())) },

			//fog skybox distances
			{	StringToMapTimeStringTableIndex("fog_startskybox"),			StringToMapTimeStringTableIndex(!m_FogStartSkyboxOverride->IsSelected() ?												"-1" :				CFmtStr("%d", m_FogStartSkyboxSlider->GetValue())) },
			{	StringToMapTimeStringTableIndex("fog_endskybox"),			StringToMapTimeStringTableIndex(!m_FogEndSkyboxOverride->IsSelected() ?													"-1" :				CFmtStr("%d", m_FogEndSkyboxSlider->GetValue())) },

			//fog densities
			{	StringToMapTimeStringTableIndex("fog_maxdensity"),			StringToMapTimeStringTableIndex(!m_FogDensityOverride->IsSelected() ?													"-1" :				CFmtStr("%.2f", (float)m_FogDensitySlider->GetValue() / m_FogDensitySlider->GetMax())) },
			{	StringToMapTimeStringTableIndex("fog_maxdensityskybox"),	StringToMapTimeStringTableIndex(!m_FogSkyboxDensityOverride->IsSelected() ?												"-1" :				CFmtStr("%.2f", (float)m_FogSkyboxDensitySlider->GetValue() / m_FogSkyboxDensitySlider->GetMax())) },
			
			//fog colors
			{	StringToMapTimeStringTableIndex("fog_color"),				StringToMapTimeStringTableIndex(!m_FogColorOverride->IsSelected() ?														"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogColor.r(), m_FogColor.g(), m_FogColor.b())) },
			{	StringToMapTimeStringTableIndex("fog_colorskybox"),			StringToMapTimeStringTableIndex(!m_FogSkyboxColorOverride->IsSelected() ?												"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogSkyboxColor.r(), m_FogSkyboxColor.g(), m_FogSkyboxColor.b())) },

			//farz clipping plane
			{	StringToMapTimeStringTableIndex("r_farz"),					StringToMapTimeStringTableIndex(!m_FarzClippingPlaneOverride->IsSelected() ?											"-1" :				CFmtStr("%d", m_FarzClippingPlaneSlider->GetValue())) },

			//fog blend
			{	StringToMapTimeStringTableIndex("fog_blend"),				StringToMapTimeStringTableIndex(!m_OverrideFogBlendCheckButton->IsSelected() ?											"-1" :				CFmtStr("%d", m_EnableFogBlendCheckButton->IsSelected())) },

			//fog blend color
			{	StringToMapTimeStringTableIndex("fog_blendcolor"),			StringToMapTimeStringTableIndex(!m_FogBlendColorOverride->IsSelected() ?												"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogBlendColor.r(), m_FogBlendColor.g(), m_FogBlendColor.b())) },

			//fog blend angle
			{	StringToMapTimeStringTableIndex("fog_blendangle"),			StringToMapTimeStringTableIndex(!m_FogBlendAngleOverride->IsSelected() ?												"-1" :				CFmtStr("%d", m_FogBlendAngleSlider->GetValue())) },

			//fog skybox blend
			{	StringToMapTimeStringTableIndex("fog_blendskybox"),			StringToMapTimeStringTableIndex(!m_OverrideFogBlendSkyboxCheckButton->IsSelected() ?									"-1" :				CFmtStr("%d", m_EnableFogBlendSkyboxCheckButton->IsSelected())) },

			//fog skybox blend color
			{	StringToMapTimeStringTableIndex("fog_blendcolorskybox"),	StringToMapTimeStringTableIndex(!m_FogBlendSkyboxColorOverride->IsSelected() ?											"-1 -1 -1" :		CFmtStr("%d %d %d", m_FogBlendSkyboxColor.r(), m_FogBlendSkyboxColor.g(), m_FogBlendSkyboxColor.b())) },

			//fog skybox blend angle
			{	StringToMapTimeStringTableIndex("fog_blendangleskybox"),	StringToMapTimeStringTableIndex(!m_FogBlendSkyboxAngleOverride->IsSelected() ?											"-1" :				CFmtStr("%d", m_FogBlendSkyboxAngleSlider->GetValue())) },
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







//divisor for bloom scalar
#define BLOOM_SCALAR_DIVISOR 100
#define COMMAND_CHANGE_CLOUDS_COLOR "ChangeCloudColors"

//skybox + filters page
class CMapPropertiesPanelSkyboxFiltersPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSkyboxFiltersPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelSkyboxFiltersPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void FormatImage(const char* input, char* output, int outsize);
	virtual void InitSkyboxAndFilter(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetSkyboxFilterInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
private:
	//night time mode
	bool m_bNightTimeMode = false;

private:
	//color picker
	CColorPicker* m_ColorPicker;

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
};


//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the skybox filter page
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelSkyboxFiltersPage::CMapPropertiesPanelSkyboxFiltersPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/SkyboxFilterPage.res")
{
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

	//perform layout to set the range sliders and such
	PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Formats the image filepath
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::FormatImage(const char* input, char* output, int outsize)
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
// Purpose: Called on think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::Update()
{
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

	BaseClass::Update();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel paint
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	Color color = m_CloudColor;
	if (color.r() == 255 && color.g() == 255 && color.b() == 255 && color.a() == 255)
		color = Color(51, 103, 153, 255);

	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(m_CloudColorRect.x, m_CloudColorRect.y, m_CloudColorRect.x + m_CloudColorRect.width, m_CloudColorRect.y + m_CloudColorRect.height);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel layout set
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::PerformLayout()
{
	BaseClass::PerformLayout();

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
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnCommand(const char* pszCommand)
{
	//called on command
	if (!Q_stricmp(pszCommand, COMMAND_CHANGE_CLOUDS_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Clouds Color (255 255 255 255 = default)", true);
		m_ColorPicker->SetUsesAlpha(true);
		m_ColorPicker->SetColor(m_CloudColor);
		m_ColorPicker->DoModal();
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnTextChanged(KeyValues* data)
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
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnColorSelected(KeyValues* data)
{
	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	//add an undo step
	AddUndo_SetColor(&m_CloudColor, m_CloudColor._color);

	//get the color
	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));
	m_CloudColor = color;

	//call base func
	BaseClass::OnColorSelected(data);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the skybox, filter and cloud color settings
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::InitSkyboxAndFilter(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

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
			int index = -1, current = 0;
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
				if (!Q_stricmp(skybox, temp) && index == -1)
					index = current;

				current++;
			}

			//active the current index
			m_SkyboxNames->ActivateItem(index == -1 ? 0 : index);

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
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::GetSkyboxFilterInfo(MapTimeInfo_t& info)
{
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
}









//divisor for bloom scalar
#define COMMAND_SUN_ACTIVATE "ActivateSun"
#define COMMAND_PITCH_TO_EYES "PitchToEyes"
#define COMMAND_CHANGE_SUN_COLOR "ChangeSunColors"
#define COMMAND_CHANGE_SUN_OVERLAY_COLOR "ChangeSunOverlayColors"

//skybox + filters page
class CMapPropertiesPanelSunPage : public CMapPropertiesPanelPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanelSunPage, CMapPropertiesPanelPageBase);
public:
	CMapPropertiesPanelSunPage(Panel* parent, const char* name);

	virtual void PerformLayout();
	virtual void Paint();
	virtual void Update();
	virtual void OnCommand(const char* pszCommand);

	//init funcs
	virtual void InitSunInfo(MapTimeInfo_t& info, bool IsNightPage);
	virtual void GetSunInfo(MapTimeInfo_t& info);

	MESSAGE_FUNC_PARAMS(OnColorSelected, "ColorSelected", data);
	MESSAGE_FUNC_PARAMS(OnSliderMoved, "SliderMoved", data);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", kv);
private:
	//night time mode
	bool m_bNightTimeMode = false;

	//color picker
	enum ColorSelectorMode { Color_Sun, Color_SunOverlay } m_ColorSelectorMode;
	CColorPicker* m_ColorPicker;

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

	//sun overlay size
	Slider* m_SunOverlaySizeSlider;
	Label* m_SunOverlaySizeLabel;

	//sun overlay color
	CMapPropertiesPanelButton* m_SunOverlayColorButton;
	Color m_SunOverlayColor;
	Rect_t m_SunOverlayColorDrawRect;

	//sun overlay material text entry
	TextEntry* m_SunOverlayMaterialTextEntry;

	//set the pitch to the players eyes pitch button
	CMapPropertiesPanelButton* m_PitchToEyesButton;
};


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
		m_ColorPicker->SetTitle("Set Sun Color", true);
		m_ColorPicker->SetUsesAlpha(false);
		m_ColorPicker->SetColor(m_SunColor);
		m_ColorPicker->DoModal();

		m_ColorSelectorMode = ColorSelectorMode::Color_Sun;
	}
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_SUN_OVERLAY_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("Set Sun Overlay Color", true);
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
		AddUndo_SetColor(&m_SunColor, m_SunColor._color);

		m_SunColor = color;
		engine->ClientCmd(CFmtStr("_amod_mapedit_server_sun_keyvalue rendercolor \"%d %d %d", m_SunColor.r(), m_SunColor.g(), m_SunColor.b()));
		break;
	case ColorSelectorMode::Color_SunOverlay:
		//add an undo step
		AddUndo_SetColor(&m_SunColor, m_SunColor._color);

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







//map properties panel
class CMapPropertiesPanel : public PropertyDialog
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesPanel, PropertyDialog)
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

	//keyboard/mouse
	void OnKeyCodePressed(KeyCode code);
	void OnKeyCodeReleased(KeyCode code);
	void OnMousePressed(KeyCode code);

	//init funcs
	void Init(MapTimeInfo_t& info, bool IsNightPage);

	void GetData(MapTimeInfo_t& info);
	void FormatImage(const char* input, char* output, int outside);

	MESSAGE_FUNC_PARAMS(OnResetUndoSteps, "ResetUndoSteps", data);
private:
	//pages
	CMapPropertiesPanelFogPage* m_FogPage;
	CMapPropertiesPanelSkyboxFiltersPage* m_SkyboxFilterPage;
	CMapPropertiesPanelSunPage* m_SunPage;

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

#define COMMAND_APPLY_PAGE_SETTINGS "ApplySettings"

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
	RemoveActionSignalTarget(this);
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

	//HACK: reset the undo steps
	AddActionSignalTarget(this);
	PostActionSignal(new KeyValues("ResetUndoSteps"));

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
	void ApplySchemeSettings(IScheme* settings);

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
	HFont m_MapTextFont = INVALID_FONT;
};

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
	//reset these
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
// Purpose: Applies the scheme settings for this
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPageBase::ApplySchemeSettings(IScheme* settings)
{
	//set m_MapTextFont
	BaseClass::ApplySchemeSettings(settings);

	m_MapTextFont = surface()->CreateFont();
	surface()->SetFontGlyphSet(m_MapTextFont, "", 24, 0, 0, 0, vgui::ISurface::FONTFLAG_ANTIALIAS);

	//clear then re-populate the list
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









//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder text entry
//----------------------------------------------------------------------------------------------------
class CSaveToFolderTextEntry : public TextEntry
{
	DECLARE_CLASS_SIMPLE(CSaveToFolderTextEntry, TextEntry)
public:
	CSaveToFolderTextEntry(Panel* parent, const char* name) : BaseClass(parent, name) {}

	//only allow a-z A-Z 0-9 _ - + :
	void OnKeyTyped(wchar_t code) override
	{
		//check
		struct range_t { char min, max; } AllowedKeys[8] = {
			{ 'a', 'z' },
			{ 'A', 'Z' },
			{ '0', '9' },
			{ '_', '_' },
			{ '-', '-' },
			{ '+', '+' },
			{ ':', ':' },
			{ ' ', ' ' }
		};
		
		//check for ctrl + a
		if ((input()->IsKeyDown(KeyCode::KEY_LSHIFT) || input()->IsKeyDown(KeyCode::KEY_RSHIFT)) && (code == 'a' || code == 'A'))
		{
			BaseClass::OnKeyTyped(code);
			return;
		}

		//check for backspace
		if (code == 8)
		{
			BaseClass::OnKeyTyped(code);
			return;
		}

		//check the key
		for (int i = 0; i < SIZE_OF_ARRAY(AllowedKeys); i++)
		{
			if (code >= AllowedKeys[i].min && code <= AllowedKeys[i].max)
			{
				BaseClass::OnKeyTyped(code);
				return;
			}
		}

		//play error sound
		surface()->PlaySound("resource/warning.wav");
	}
};







//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder panel
//----------------------------------------------------------------------------------------------------
class CSaveToFolderPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CSaveToFolderPanel, Frame);
public:
	CSaveToFolderPanel(Panel* parent, const char* name);
	~CSaveToFolderPanel();

	//command funcs
	void OnCommand(const char* pszCommand);
private:
	//save text entry
	CSaveToFolderTextEntry* m_TextEntry;
};

//singleton
static CSaveToFolderPanel* gs_SaveToFolderPanel = nullptr;

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder panel
//----------------------------------------------------------------------------------------------------
CSaveToFolderPanel::CSaveToFolderPanel(Panel* parent, const char* name) : BaseClass(parent, name)
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
	SetTitle("Save theme", true);
	SetSize(300, 85);
	MoveToCenterOfScreen();
	Activate();

	//create the text entry
	m_TextEntry = new CSaveToFolderTextEntry(this, "SaveToFolderTextEntry");
	m_TextEntry->SetBounds(5, 25, 290, 25);
	m_TextEntry->SetMaximumCharCount(64);

	//create the Save and Cancel button
	Button* m_SaveButton = new Button(this, "SaveButton", "Save");
	m_SaveButton->SetBounds(5, 55, 142, 25);
	m_SaveButton->SetCommand("Save");

	Button* m_CancelButton = new Button(this, "CancelButton", "Cancel");
	m_CancelButton->SetBounds(152, 55, 142, 25);
	m_CancelButton->SetCommand("Close");
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CSaveToFolderPanel::OnCommand(const char* pszCommand)
{
	//check for save
	if (!Q_stricmp(pszCommand, "Save"))
	{
		//get the text
		char buf[512];
		m_TextEntry->GetText(buf, sizeof(buf));

		//post the message
		PostActionSignal(new KeyValues("OnSavePanelSaved", "Folder", buf));

		//close this
		Close();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Save to folder destructor
//----------------------------------------------------------------------------------------------------
CSaveToFolderPanel::~CSaveToFolderPanel()
{
	gs_SaveToFolderPanel = nullptr;
}





//----------------------------------------------------------------------------------------------------
// Purpose: Main map properties panel
//----------------------------------------------------------------------------------------------------
#define COMMAND_SAVE_TO_FILE "SaveToFile"
#define COMMAND_SAVE_CONFIRM "ConfirmSave"
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
	void ApplySchemeSettings(IScheme* settings);

	//called when the save panel saves
	MESSAGE_FUNC_PARAMS(OnFileSaved, "OnSavePanelSaved", kv);
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
// Purpose: Called when the save panel saves to a folder
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnFileSaved(KeyValues* data)
{
	//get the filename
	const char* filename = data->GetString("Folder");
	
	//MUST not be empty
	if (!filename || !*filename)
	{
		surface()->PlaySound("resource/warning.wav");
		QueryBox* modal = new QueryBox("Error?", "The text entry was empty for the save dialog!", this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		return;
	}

	//check if the folder exists
	if (filesystem->IsDirectory(CFmtStr("resource/time_info/%s", filename), "MOD"))
	{
		QueryBox* modal = new QueryBox("Folder Already Exists?", CFmtStr("The folder %s already exists. Would you like to override the contents inside?", filename), this);
		modal->MoveToCenterOfScreen();
		modal->DoModal(this);
		modal->Activate();
		modal->SetOKCommand(new KeyValues("Command", "command", CFmtStr(COMMAND_SAVE_CONFIRM "%s", filename)));
		return;
	}

	//save now
	OnCommand(CFmtStr(COMMAND_SAVE_CONFIRM "%s", filename));
}

//----------------------------------------------------------------------------------------------------
// Purpose: Apply the scheme settings
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::ApplySchemeSettings(IScheme* settings)
{
	MoveToCenterOfScreen();
	BaseClass::ApplySchemeSettings(settings);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for map properties editor panel.
//----------------------------------------------------------------------------------------------------
void CMapPropertiesEditorPanel::OnCommand(const char* pszCommand)
{
	//check for COMMAND_SAVE_TO_FILE
	if (!Q_stricmp(pszCommand, COMMAND_SAVE_TO_FILE))
	{
		//open the save dialog
		if (gs_SaveToFolderPanel)
			gs_SaveToFolderPanel->DeletePanel();

		gs_SaveToFolderPanel = new CSaveToFolderPanel(this, "SaveToFolderPanel");
		gs_SaveToFolderPanel->DoModal();
		gs_SaveToFolderPanel->AddActionSignalTarget(this);
		gs_SaveToFolderPanel->Activate();
		return;
	}

	//check for COMMAND_SAVE_CONFIRM
	else if (!Q_strnicmp(pszCommand, COMMAND_SAVE_CONFIRM, Q_strlen(COMMAND_SAVE_CONFIRM)))
	{
		//get the filename
		const char* filename = pszCommand + Q_strlen(COMMAND_SAVE_CONFIRM);
		if (!filename || !*filename)
			return;

		//save
		WriteAllTimeInfosToFiles(filename);
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

		//ALWAYS delete g_MapPropertiesPanel if its open
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

	//check for the actuall editor
	if (g_MapPropertiesPanel)
		g_MapPropertiesPanel->DeletePanel();

	g_MapPropertiesPanel = nullptr;
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

	//HACK HACK:
	//we always want to modify the map properties in the "resource/time_info" directory. Incase 'amod_timeinfo_load_directory' isnt 
	//"resource/time_info". Set it.
	extern ConVar amod_timeinfo_load_directory;
	if (Q_stricmp(amod_timeinfo_load_directory.GetString(), "resource/time_info") && Q_stricmp(amod_timeinfo_load_directory.GetString(), "resource\\time_info"))
	{
		engine->ClientCmd("amod_timeinfo_load_directory resource/time_info; amod_timeinfo_reset");
	}

	//create the panel if not already here
	if (!s_MapPropertiesEditorPanel)
		s_MapPropertiesEditorPanel = new CMapPropertiesEditorPanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	s_MapPropertiesEditorPanel->Activate();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Returns if the map properties panel is open or not
//----------------------------------------------------------------------------------------------------
const bool IsMapPropertiesPanelOpen()
{
	return g_MapPropertiesPanel != nullptr;
}