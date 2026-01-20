#ifndef __MAPPROPERTIESEDITORPANELBASEPAGE_H
#define __MAPPROPERTIESEDITORPANELBASEPAGE_H

#include "AloneMod/AloneModTimeInfo.h"
#include "AloneMod/effects panel/EffectsPanel.h"
#include "AloneMod/geo guesser/GG_MiniMap.h"
#include "AloneMod/ColorPicker.h"
#include "AloneMod/Amod_SharedDefs.h"
#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <filesystem.h>
#include <ienginevgui.h>
#include <fmtstr.h>
#include <utlvector.h>

using namespace vgui;

//fog array helpers
const char* FindFogInfoFromArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* def = "");
void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, UtlSymId_t key, UtlSymId_t value);
void AddOrUpdateFogInfoInArray(CUtlVector<MapTimeInfo_t::FogInfo_t>& info, const char* key, const char* value);

//sun array helpers
const char* FindSunInfoFromArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* def = "");
void AddOrUpdateSunInfoInArray(CUtlVector<MapTimeInfo_t::DayInfo_t::SunInfo_t>& info, const char* key, const char* value);

//finds the map path
bool FindMapPath(const char* base, const char* find, char* output, int outputsize);




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

extern UndoStep_t s_UndoSteps[MAX_UNDO_STEPS];
extern int s_CurrentUndoStep;
extern int s_UndoStepsCount;
extern bool s_NeedSave;

//undo funcs
void AddUndo_SetSlider(Slider* slider, int previousValue);
void AddUndo_SetColor(Color* setColor, const unsigned char previouscolor[4]);
void AddUndo_SetCheckButton(CheckButton* button, bool previousValue);
void UndoStep_Apply(bool undo);





//map properties editor slider
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





//map properties editor button
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

#endif //__MAPPROPERTIESEDITORPANELBASEPAGE_H