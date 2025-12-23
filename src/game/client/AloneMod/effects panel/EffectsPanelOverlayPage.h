#ifndef __EFFECTSPANELOVERLAYPAGE_H
#define __EFFECTSPANELOVERLAYPAGE_H

#include "EffectsPanel.h"
#include "EffectsPanelConvarPage.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ComboBox.h"
#include "c_basehlplayer.h"

#ifdef _WIN32
#pragma once
#endif

void Amod_PerformScreenOverlay(CMaterialReference& ref, int x, int y, int w, int h, int r = -1, int g = -1, int b = -1, int a = -1);

//overlay draw type
enum class OverlayDrawType_e
{
	Draw_Always = (1 << 0),
	Draw_WhenPlayerFlashlightOn = (1 << 1),
	Draw_WhenPlayerFlashlightOff = (1 << 2),
	Draw_WhenWalking = (1 << 3),
	Draw_WhenSprinting = (1 << 4),
	Draw_WhenCrouching = (1 << 5),
	Draw_WhenOnGround = (1 << 6),
	Draw_WhenInAir = (1 << 7),
	Draw_WhenUnderWater = (1 << 8),
	Draw_WhenHealthLow = (1 << 9),
	Draw_WhenHoldingObject = (1 << 10),
	Draw_WhenUsingSuitZoom = (1 << 11),
};

//overlay draw type string's for combo box
static const char* g_OverlayDrawStrings[] =
{
	"Always Draw",
	"When flashlight on",
	"When flashlight off",
	"When walking",
	"When sprinting",
	"When crouching",
	"When on ground",
	"When in air",
	"When under water",
	"When health low",
	"When holding object",
	"When using suit zoom"
};

//overlay button data
struct OverlayButtonData_t
{
	//name
	char name[256];
	CMaterialReference material;

	//red/green/blue colors
	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;

	//draw type
	OverlayDrawType_e drawtype = OverlayDrawType_e::Draw_Always;
};

//overlay list button
class COverlayPageButton : public CConvarPageConvarButtonBase
{
	DECLARE_CLASS_SIMPLE(COverlayPageButton, CConvarPageConvarButtonBase);
public:
	//constructor
	COverlayPageButton(vgui::Panel* parent, const char* name, const char* text) :
		BaseClass(parent, name, text)
	{
	}

	//destructor
	~COverlayPageButton()
	{
		//decrement the material counter
		if ((IMaterial*)m_OverlayData.material)
			m_OverlayData.material->DecrementReferenceCount();
	}
private:
	//list data
	OverlayButtonData_t m_OverlayData;

	friend class COverlayPageList;
};



//overlay page convar list
#define OVERLAY_LIST_PANEL_TITLE_HEIGHT 20
#define OVERLAY_LIST_PANEL_SCROLL_BAR_WIDTH 20
#define OVERLAY_LIST_PANEL_BUTTON_X_OFFSET 5
#define OVERLAY_LIST_PANEL_BUTTON_HEIGHT 22
#define OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX "OverlayButton:"
#define OVERLAY_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL 10

class COverlayPageList : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(COverlayPageList, vgui::Divider);
public:
	//constructor
	COverlayPageList(vgui::Panel* parent, const char* name, const char* title);

	//scroll bar moved and slider
	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

	//overlay funcs
	bool AddOverlay(const char* OverlayName, int ROverride = -1, int GOverride = -1, int BOverride = -1, int AtOverride = -1, OverlayDrawType_e DrawType = OverlayDrawType_e::Draw_Always);
	bool HasOverlay(const char* ConvarName);
	bool ChangeOverlay(const char* OldOverlay, const char* NewOverlay, int r = -1, int g = -1, int b = -1, int a = -1, OverlayDrawType_e DrawType = OverlayDrawType_e::Draw_Always);
	bool ChangeSelectedOverlay(const char* NewOverlay, int r = -1, int g = -1, int b = -1, int a = -1, OverlayDrawType_e DrawType = OverlayDrawType_e::Draw_Always);
	void GetOverlayData(CUtlVector<OverlayButtonData_t*>& data);
	void ClearOverlays();
	bool RemoveSelectedOverlay();
	void MoveSelectedOverlay(bool up);

	//overlay paint funcs
	bool ShouldOverlayDraw(C_BaseHLPlayer* pPlayer, OverlayDrawType_e type);
	void PaintOverlays(int x, int y, int w, int h);

	//panel functions
	void OnCommand(const char* pszCommand);
	void OnKeyCodePressed(vgui::KeyCode code);

private:
	//array of buttons (overlays)
	CUtlVector<COverlayPageButton*> m_ButtonList;

	//title text
	vgui::Label* m_Title;

	//scroll wheel
	vgui::ScrollBar* m_ScrollBar;

	//convar page
	class CEffectsPanelOverlayPage* m_OverlayPage;
};


//quick mouse wheel slider
class OverlaySlider : public WheelSlider
{
	DECLARE_CLASS_SIMPLE(OverlaySlider, WheelSlider);
public:
	//constructor
	OverlaySlider(vgui::Panel* parent, const char* name) : BaseClass(parent, name)
	{
	}

	//called on mouse press
	void OnMousePressed(vgui::MouseCode code)
	{
		if (code == vgui::MouseCode::MOUSE_RIGHT)
			SetValue(255);
		else
			BaseClass::OnMousePressed(code);
	}
};


//overlay page
#define ADD_OVERLAY_BUTTON_COMMAWND "AddOverlay"
#define CHANGE_OVERLAY_BUTTON_COMMAWND "ChangeOverlay"
#define REMOVE_OVERLAY_BUTTON_COMMAWND "RemoveOverlay"
#define OVERLAY_PAGE_OVERLAY_TYPE_PREFIX "OverlayType:"

class CEffectsPanelOverlayPage : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelOverlayPage, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelOverlayPage(vgui::Panel* parent, const char* name);
	~CEffectsPanelOverlayPage();

	//population function
	void AddVMTEffectsRecursive(vgui::ComboBox* combo, const char* directory);

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues, bool reset = false);
	void WriteToFile(KeyValues* keyvalues);

	//sets overlay text entry text
	void SetOverlayText(OverlayButtonData_t& newtext);

	//other panel functions
	void OnCommand(const char* pszCommand);
	void OnTick();

	//sets all the bounds for each item
	void PerformLayout();

	//paints all the overlays
	void PerformScreenOverlay(int x, int y, int w, int h);
private:

	//overlay data list
	COverlayPageList* m_OverlayList;

	//overlay combo box. Allows for effect select or effect text input
	vgui::ComboBox* m_OverlayTextEntry;

	//type combo box
	vgui::ComboBox* m_TypeComboBox;

	//sliders and text's
	vgui::Label* m_RedText;
	vgui::Label* m_GreenText;
	vgui::Label* m_BlueText;
	vgui::Label* m_AlphaText;

	OverlaySlider* m_RedSlider;
	OverlaySlider* m_GreenSlider;
	OverlaySlider* m_BlueSlider;
	OverlaySlider* m_AlphaSlider;

	//buttons
	vgui::Button* m_AddButton;
	vgui::Button* m_ChangeOverlay;
	vgui::Button* m_RemoveOverlay;
};

#endif //__EFFECTSPANELOVERLAYPAGE_H