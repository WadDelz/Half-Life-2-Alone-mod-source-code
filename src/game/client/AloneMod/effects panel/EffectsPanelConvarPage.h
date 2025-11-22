#ifndef __EFFECTSPANELCONVARSPAGE_H
#define __EFFECTSPANELCONVARSPAGE_H

#include "EffectsPanel.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Divider.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/MenuItem.h"
#include "hl2_player_shared.h"

#ifdef _WIN32
#pragma once
#endif



//convar active type
enum class ConvarActiveType_e
{
	Active_Always = (1 << 0),
	Active_WhenPlayerFlashlightOn = (1 << 1),
	Active_WhenPlayerFlashlightOff = (1 << 2),
	Active_WhenWalking = (1 << 3),
	Active_WhenSprinting = (1 << 4),
	Active_WhenCrouching = (1 << 5),
	Active_WhenOnGround = (1 << 6),
	Active_WhenInAir = (1 << 7),
	Active_WhenUnderWater = (1 << 8),
	Active_WhenHealthLow = (1 << 9),
	Active_WhenHoldingObject = (1 << 10),
	Active_WhenUsingSuitZoom = (1 << 11),
};

//convar active type string's for combo box
static const char* g_ConvarActiveType[] =
{
	"Always Active",
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



//custom filtered text entry
class CFilteredTextEntry : public vgui::TextEntry
{
	DECLARE_CLASS_SIMPLE(CFilteredTextEntry, TextEntry);
public:

	//constructor
	CFilteredTextEntry(vgui::Panel* parent, const char* name, const char* filter = nullptr);

	//keyboard funcs
	virtual void OnKeyTyped(wchar_t unichar);
	virtual void SetFilter(const char* filter);

private:
	//filter
	const char* m_FilterOut = "";
};






//convar page convar button
#define EFFECTS_PANEL_BG_SELECTED_COLOR Color(255, 235, 200, 200)
#define EFFECTS_PANEL_FG_SELECTED_COLOR Color(0, 0, 0, 255)

class CConvarPageConvarButtonBase : public vgui::Button
{
	DECLARE_CLASS_SIMPLE(CConvarPageConvarButtonBase, vgui::Button);
public:
	//constructor
	CConvarPageConvarButtonBase(vgui::Panel* parent, const char* name, const char* text);

	//selected functions
	virtual void SetButtonSelected(bool selected);
	virtual const bool IsButtonSelected();

	//paint functions
	virtual void PaintBackground();
	virtual void Paint();

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	//is this selected
	bool m_bIsSelected = false;

	//default colors
	static Color m_DefaultBgColor;
	static Color m_DefaultFgColor;
};

//convar struct
struct ConvarButtonData_t
{
	char name[56];
	char value[202];
	char default[200];
	ConVar* convar = nullptr;
	ConvarActiveType_e type = ConvarActiveType_e::Active_Always;
};

class CConvarPageConvarButton : public CConvarPageConvarButtonBase
{
	DECLARE_CLASS_SIMPLE(CConvarPageConvarButton, CConvarPageConvarButtonBase);
public:
	CConvarPageConvarButton(vgui::Panel* parent, const char* name, const char* text) :
		BaseClass(parent, name, text) 
	{
		//clear m_Data
		m_ConvarData.name[0] = '\0';
		m_ConvarData.value[0] = '\0';
		m_ConvarData.default[0] = '\0';
	}

private:
	//convar data
	ConvarButtonData_t m_ConvarData;

	friend class CConvarPageConvarList;
};





//convar page convar list
#define CONVAR_LIST_PANEL_TITLE_HEIGHT 20
#define CONVAR_LIST_PANEL_SCROLL_BAR_WIDTH 20
#define CONVAR_LIST_PANEL_BUTTON_X_OFFSET 5
#define CONVAR_LIST_PANEL_BUTTON_HEIGHT 22
#define CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX "ConvarButton:"
#define CONVAR_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL 10
#define CONVAR_PAGE_OVERLAY_TYPE_PREFIX "ConvarType:"

class CConvarPageConvarList : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(CConvarPageConvarList, vgui::Divider);
public:
	//constructor
	CConvarPageConvarList(vgui::Panel* parent, const char* name, const char* title);

	//scroll bar moved and slider
	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

	//adds a convar
	bool AddConvar(const char* ConvarName, const char* ConvarValue, ConvarActiveType_e type);
	void ChangeConvarValue(const char* ConvarName, const char* newvalue, ConvarActiveType_e type);
	bool ChangeSelectedConvar(const char* newvalue, ConvarActiveType_e type);
	bool HasConvar(const char* ConvarName);
	void GetConvarData(CUtlVector<ConvarButtonData_t*>& data);
	void ClearConvars();
	bool RemoveSelectedConvar();
	void ApplyConvarValues();

	//panel functions
	void OnCommand(const char* pszCommand);

	bool ShouldConvarBeActive(CHL2_Player* pPlayer, ConvarActiveType_e type);
	void OnTick();

private:
	//array of buttons (convars)
	CUtlVector<CConvarPageConvarButton*> m_ButtonList;

	//title text
	vgui::Label* m_Title;

	//scroll wheel
	vgui::ScrollBar* m_ScrollBar;

	//convar page
	class CEffectsPanelConvarPage* m_ConvarPage;
};






//find convar panel
#define CONVAR_LIST_PANEL_WIDTH 375
#define CONVAR_LIST_PANEL_HEIGHT 180
#define CONVAR_LIST_INSERT_COMMAND "Insert"
#define CONVAR_LIST_SEARCH_COMMAND "Search"

class CConvarListPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CConvarListPanel, vgui::Frame);
public:
	//constructor
	CConvarListPanel(vgui::VPANEL parent, const char* name, class CEffectsPanelConvarPage* ConvarPage);

	//initalizes convar combo box
	void InitalizeConvars();

	//sets the convar page
	void SetConvarPage(class CEffectsPanelConvarPage* ConvarPage);

	//other
	void OnCommand(const char* pszCommand);
	void OnClose();

private:
	vgui::ComboBox* m_ConvarList;
	vgui::TextEntry* m_SearchText;
	vgui::Button* m_SearchButton;
	vgui::Button* m_InsertButton;

	class CEffectsPanelConvarPage* m_ConvarPage;
};
extern CConvarListPanel* g_ConvarListPanel;






//convars page
#define CONVAR_PAGE_NAME_FILTER "(){}:'\" "
#define CONVAR_PAGE_ADD_COMMAND "AddConvar"
#define CONVAR_PAGE_FIND_COMMAND "FindConvar"
#define CONVAR_PAGE_CHANGE_COMMAND "ChangeConvar"
#define CONVAR_PAGE_REMOVE_COMMAND "RemoveConvar"
#define CONVAR_PAGE_SHOULD_CHANGE_COMMAND "ShouldChangeValue"

class CEffectsPanelConvarPage : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelConvarPage, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelConvarPage(vgui::Panel* parent, const char* name);

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues, bool reset = false);
	void WriteToFile(KeyValues* keyvalues);

	//sets the convar text entry
	void SetConvarText(const char* name, const char* value = nullptr);

	//other panel functions
	void OnMapLoad();
	void OnCommand(const char* pszCommand);
	void OnTick();

	//sets all the bounds for each item
	void PerformLayout();

private:
	//list of convars
	CConvarPageConvarList* m_ConvarListPanel;

	//type combo box
	vgui::ComboBox* m_TypeComboBox;

	//name stuff
	vgui::Label* m_ConvarNameText;
	CFilteredTextEntry* m_ConvarNameTextEntry;
	vgui::Button* m_FindConvarButton;

	//value stuff
	vgui::Label* m_ConvarValueText;
	CFilteredTextEntry* m_ConvarValueTextEntry;

	//add-remove-change button
	vgui::Button* m_AddButton;
	vgui::Button* m_ChangeButton;
	vgui::Button* m_RemoveButton;
};

#endif //__EFFECTSPANELCONVARSPAGE_H