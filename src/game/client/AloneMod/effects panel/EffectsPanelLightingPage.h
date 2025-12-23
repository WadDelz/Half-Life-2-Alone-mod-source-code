#ifndef __EFFECTSPANELLIGHTINGPAGE_H
#define __EFFECTSPANELLIGHTINGPAGE_H

#include "EffectsPanel.h"
#include "EffectsPanelConvarPage.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/ComboBox.h"
#include "dlight.h"
#include "flashlighteffect.h"

#ifdef _WIN32
#pragma once
#endif

//all the lighting modes
enum class LightingMode_t
{
	Mode_DynamicLight,
	Mode_DynamicEnvironmentalLight,
	Mode_Flashlight,
};

static const char* gsz_LightingModeStrings[] = {
	"Dynamic Light",
	"Dynamic Environmental Light",
	"Projected Flashlight (can be buggy)",
};

//all the lighting movement modes
enum class LightingMovementMode_t
{
	Mode_Static,
	Mode_PlayersEyes,
	Mode_PlayersWeaponMuzzle,
	Mode_ParentToClassnameEntity,
	Mode_ParentToTargetnameEntity,
	Mode_ParentToModelEntity,
	Mode_ParentToAllClassnameEntity,
	Mode_ParentToAllTargetnameEntity,
	Mode_ParentToAllModelEntity
};

static const char* gsz_LightingMovementModeStrings[] = {
	"Static",
	"Follow Players Eyes",
	"Parent to player's weapon muzzle (if possible)",
	"Parent to entity with classname",
	"Parent to entity with targetname",
	"Parent to entity with model name",
	"Parent to all entities with classname",
	"Parent to all entities with targetname",
	"Parent to all entities with model name",
};


//lighting active type
enum class LightingType_e
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

//lighting active type string's for combo box
static const char* g_LightingActiveType[] =
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

//text entry

//lighting page text entry
#define EFFECTS_LIGHTING_PANEL_BG_DISABLED_COLOR Color(0, 0, 0, 200)

class CLightingPageTextEntry : public vgui::TextEntry
{
	DECLARE_CLASS_SIMPLE(CLightingPageTextEntry, vgui::TextEntry);
public:
	//constructor
	CLightingPageTextEntry(vgui::Panel* parent, const char* name);

	//paint functions
	virtual void PaintBackground();

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme);

private:
	//default colors
	static Color m_DefaultBgColor;
};


//lighting button data
struct LightingButtonData_t
{
	//name
	char name[64];

	//modes
	LightingMode_t mode = LightingMode_t::Mode_DynamicLight;
	LightingMovementMode_t movementmode = LightingMovementMode_t::Mode_Static;

	//lighting active type
	LightingType_e ActiveType = LightingType_e::Active_Always;

	//list of flashlight effects and dynamic lights
	CUtlVector<dlight_t*> m_DynamicLights;
	CUtlVector<CFlashlightEffect*> m_DynamicFlashlights;

	//entity name
	char EntityName[128];
	char EntityAttachment[64];

	//offset and origin stuff
	Vector offset = vec3_origin;
	QAngle angoffset = vec3_angle;
	Vector origin = vec3_origin;
	QAngle angles = vec3_angle;

	//flashlight color
	Color color = Color(255, 255, 255, 100);

	//flashlight effect only
	int fov = 45;
	int distance = 750;
};

//lighting list button
class CLightingPageButton : public CConvarPageConvarButtonBase
{
	DECLARE_CLASS_SIMPLE(CLightingPageButton, CConvarPageConvarButtonBase);
public:
	//constructor
	CLightingPageButton(vgui::Panel* parent, const char* name, const char* text) :
		BaseClass(parent, name, text)
	{
		m_LightingData.name[0] = '\0';
		m_LightingData.EntityName[0] = '\0';
		m_LightingData.EntityAttachment[0] = '\0';
	}
	
	//Deletes all the lights
	void DeleteLights()
	{
		//delete all of the flashlights
		for (int i = 0; i < m_LightingData.m_DynamicFlashlights.Count(); i++)
		{
			if (m_LightingData.m_DynamicFlashlights[i])
				delete m_LightingData.m_DynamicFlashlights[i];
		}
		
		//delete all of the dynamic lights
		for (int i = 0; i < m_LightingData.m_DynamicLights.Count(); i++)
		{
			//die now
			if (m_LightingData.m_DynamicLights[i])
			{
				m_LightingData.m_DynamicLights[i]->die = gpGlobals->curtime;
				m_LightingData.m_DynamicLights[i]->radius = 0.0f;
			}
		}

		//clear the arrays
		m_LightingData.m_DynamicFlashlights.RemoveAll();
		m_LightingData.m_DynamicLights.RemoveAll();
	}

	//destructor
	~CLightingPageButton()
	{
		DeleteLights();
	}
private:
	//list data
	LightingButtonData_t m_LightingData;

	friend class CLightingPageList;
};




//lighting page lighting list
#define LIGHTING_LIST_PANEL_TITLE_HEIGHT 20
#define LIGHTING_LIST_PANEL_SCROLL_BAR_WIDTH 20
#define LIGHTING_LIST_PANEL_BUTTON_X_OFFSET 5
#define LIGHTING_LIST_PANEL_BUTTON_HEIGHT 22
#define LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX "LightingButton:"
#define LIGHTING_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL 3
#define LIGHTING_PAGE_ACTIVE_TYPE_PREFIX "LightingType:"

class CLightingPageList : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(CLightingPageList, vgui::Divider);
public:
	//constructor
	CLightingPageList(vgui::Panel* parent, const char* name, const char* title);

	//scroll bar moved and slider
	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

	//lighting funcs
	bool AddLighting(const char* LightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment, Vector origin = vec3_origin, QAngle angle = vec3_angle);
	bool HasLighting(const char* LightName);
	bool ChangeLighting(const char* LightName, const char* NewLightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment);
	bool ChangeSelectedLighting(const char* NewLightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment);
	void GetLightingData(CUtlVector<LightingButtonData_t*>& data);
	void ClearLights();
	bool RemoveSelectedLight();
	bool SetSelectedLightToPlayersEyes();

	//returns if the light should be active
	bool ShouldLightBeActive(C_BaseHLPlayer* pPlayer, LightingType_e type);

	//sets the light state for level transitioning
	void SetLightTransitionState(bool transitioning);

	//debug
	void DrawDebugOverlays();

	//lighting functions
	void CreateDLight(LightingButtonData_t* data, int index);
	void CreateFlashLight(LightingButtonData_t* data, int index);

	//vector offset func
	Vector GetPositionOffset(Vector position, QAngle angle, Vector offset, Vector* forward = nullptr, Vector* right = nullptr, Vector* up = nullptr);
	QAngle GetAngleOffset(Vector position, QAngle angle, Vector offset, Vector* forward = nullptr, Vector* right = nullptr, Vector* up = nullptr);

	//updates/removes a light
	void UpdateLight(LightingButtonData_t* data, int index, const LightingMode_t& LightMode);
	void RemoveLight(LightingButtonData_t* data, int index, const LightingMode_t& LightMode);

	//update sub funcs
	void UpdatePlayerEyesLight(LightingButtonData_t* data, C_BasePlayer* pPlayer, const LightingMode_t& LightMode);
	void UpdatePlayerViewmodelight(LightingButtonData_t* data, C_BasePlayer* pPlayer, const LightingMode_t& LightMode);
	void UpdateEntityLight(LightingButtonData_t* data, const LightingMode_t& LightMode);
	void UpdateEntityAllLight(LightingButtonData_t* data, const LightingMode_t& LightMode);

	void UpdateLights();

	//panel functions
	void OnCommand(const char* pszCommand);

private:
	//array of buttons (lightings)
	CUtlVector<CLightingPageButton*> m_ButtonList;

	//title text
	vgui::Label* m_Title;

	//scroll wheel
	vgui::ScrollBar* m_ScrollBar;

	//convar page
	class CEffectsPanelLightingPage* m_LightingPage;

	//transition stuff
	bool m_bTransitioning = false;

	//indexs for flashlights and dlights
	int m_FlashLightIndex = 1;
	int m_DlightIndex = 1;
};



//lighting page
#define ADD_LIGHT_BUTTON_COMMAND "AddLight"
#define CHANGE_LIGHT_BUTTON_COMMAND "ChangeLight"
#define REMOVE_LIGHT_BUTTON_COMMAND "RemoveLight"
#define LIGHT_SET_TO_PLAYERS_POS_BUTTON "SetLightToPlayer"

class CEffectsPanelLightingPage : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelLightingPage, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelLightingPage(vgui::Panel* parent, const char* name);
	~CEffectsPanelLightingPage();

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues, bool reset = false);
	void WriteToFile(KeyValues* keyvalues);

	//child funcs
	void SetLightingText(LightingButtonData_t& data);

	//other panel functions
	void OnCommand(const char* pszCommand);
	void OnTick();
	
	//sets all the bounds for each item
	void PerformLayout();

	//updates the lights
	void UpdateLighting();

	//map funcs
	void OnMapLoad();
	void OnMapShutdown();
private:

	//lighting list
	CLightingPageList* m_LightList;

	//buttons
	vgui::Button* m_AddButton;
	vgui::Button* m_ChangeLight;
	vgui::Button* m_RemoveLight;

	//sets the selected light's position and angle to the player's eye pos and angle
	vgui::Button* m_SetSelectedToPlayer;

	//name/type stuff
	vgui::Label* m_LightNameLabel;
	CLightingPageTextEntry* m_LightNameTextEntry;

	//active combo box
	vgui::ComboBox* m_ActiveModeComboBox;

	vgui::Label* m_TypeLabel;
	vgui::ComboBox* m_TypeComboBox;

	//movement mode/lighting offset/attach to entity name/attach to entity attachment label/stuff
	vgui::Label* m_LightingMovementModeLabel;
	vgui::ComboBox* m_LightMovementComboBox;

	vgui::Label* m_LightingOffsetLabel;
	CLightingPageTextEntry* m_LightOffsetTextEntry;
	
	vgui::Label* m_LightingAngleOffsetLabel;
	CLightingPageTextEntry* m_LightAngleOffsetTextEntry;

	vgui::Label* m_LightingEntityLabel;
	CLightingPageTextEntry* m_LightingEntityTextEntry;

	vgui::Label* m_LightingEntityAttachmentLabel;
	CLightingPageTextEntry* m_LightingEntityAttachmentTextEntry;

	//bottom color/far/fov stuff
	vgui::Label* m_ColorLabel;
	CLightingPageTextEntry* m_ColorTextEntry;

	vgui::Label* m_FarLabel;
	WheelSlider* m_FarSlider;
	
	vgui::Label* m_FovLabel;
	WheelSlider* m_FovSlider;
};

#endif //__EFFECTSPANELLIGHTINGPAGE_H