#include "cbase.h"
#include "EffectsPanelLightingPage.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/QueryBox.h"
#include "vgui/ISurface.h"
#include "fmtstr.h"
#include "iefx.h"
#include "debugoverlay_shared.h"


ConVar amod_lighting_debug("amod_lighting_debug", "0");


//list of lighting pages. This is so i can loop through each effect page and call page->UpdateLighting() function;
static CUtlVector<CEffectsPanelLightingPage*> g_EffectsLightingPages;

//updates all of the lights
void Amod_EffectsPanelUpdateLighting()
{
	for (int i = 0; i < g_EffectsLightingPages.Count(); i++)
		g_EffectsLightingPages[i]->UpdateLighting();
}



//text entry

Color CLightingPageTextEntry::m_DefaultBgColor = Color();

//---------------------------------------------------------------------------------
// Purpose: Constructor for the convar button
//---------------------------------------------------------------------------------
CLightingPageTextEntry::CLightingPageTextEntry(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
}

//---------------------------------------------------------------------------------
// Purpose: Paints the background
//---------------------------------------------------------------------------------
void CLightingPageTextEntry::PaintBackground()
{
	//if the button is selected then set the bg color
	if (!IsEnabled())
		SetBgColor(EFFECTS_LIGHTING_PANEL_BG_DISABLED_COLOR);
	else
		SetBgColor(m_DefaultBgColor);

	BaseClass::PaintBackground();
}

//---------------------------------------------------------------------------------
// Purpose: Applies the scheme settings
//---------------------------------------------------------------------------------
void CLightingPageTextEntry::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	//get the default colors
	m_DefaultBgColor = GetBgColor();
}


//lighting list



//---------------------------------------------------------------------------------
// Purpose: Constructor for the overlay list panel
//---------------------------------------------------------------------------------
CLightingPageList::CLightingPageList(vgui::Panel* parent, const char* name, const char* title)
	: BaseClass(parent, name)
{
	//create the title
	m_Title = new vgui::Label(this, "_Title", title);
	m_Title->SetBounds(0, 0, 520 - 20, LIGHTING_LIST_PANEL_TITLE_HEIGHT);
	m_Title->SetContentAlignment(vgui::Label::a_center);

	//make the scroll bar
	m_ScrollBar = new vgui::ScrollBar(this, "_ScrollBar", true);
	m_ScrollBar->SetBounds(520 - 20, 0, 20, 120);
	m_ScrollBar->SetRangeWindow(1);
	m_ScrollBar->AddActionSignalTarget(this);

	//set the overlay page
	m_LightingPage = dynamic_cast<CEffectsPanelLightingPage*>(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse wheel moved
//---------------------------------------------------------------------------------
void CLightingPageList::OnMouseWheeled(int delta)
{
	BaseClass::OnMouseWheeled(delta);

	//check scroll bar bounds
	int min, max;
	m_ScrollBar->GetRange(min, max);

	//handle scroll for scroll wheel
	m_ScrollBar->SetValue(m_ScrollBar->GetValue() - delta);
}

//---------------------------------------------------------------------------------
// Purpose: Called on scroll bar moved
//---------------------------------------------------------------------------------
void CLightingPageList::OnScrollBarSliderMoved()
{
	//get the pos
	int pos = m_ScrollBar->GetValue();

	//format the items
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//the item at pos - 1 could overlap with the "Overlays::" title text. To fix this put the item at a stupid pos
		if (i == pos - 1)
			m_ButtonList[i]->SetPos(-100, -100);
		else
			m_ButtonList[i]->SetPos(LIGHTING_LIST_PANEL_BUTTON_X_OFFSET, LIGHTING_LIST_PANEL_TITLE_HEIGHT + (LIGHTING_LIST_PANEL_BUTTON_HEIGHT * i) - (LIGHTING_LIST_PANEL_BUTTON_HEIGHT * pos));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Adds a lighting button
//---------------------------------------------------------------------------------
bool CLightingPageList::AddLighting(const char* LightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment, Vector origin, QAngle angle)
{
	//check for player
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return true;	//keep error message happy

	//see if we have a button with the same name
	if (HasLighting(LightName))
		return false;

	//get the bounds
	int x = LIGHTING_LIST_PANEL_BUTTON_X_OFFSET;
	int y = LIGHTING_LIST_PANEL_TITLE_HEIGHT + (LIGHTING_LIST_PANEL_BUTTON_HEIGHT * m_ButtonList.Count());
	int w = GetWide() - x - (LIGHTING_LIST_PANEL_BUTTON_X_OFFSET * 2) - LIGHTING_LIST_PANEL_SCROLL_BAR_WIDTH;
	int h = LIGHTING_LIST_PANEL_BUTTON_HEIGHT;

	//make a new button
	CLightingPageButton* button = new CLightingPageButton(this, "OverlayButton", LightName);
	button->SetBounds(x, y, w, h);
	button->SetCommand(CFmtStr(LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count()));

	//set the light data
	Q_strncpy(button->m_LightingData.name, LightName, SIZE_OF_ARRAY(button->m_LightingData.name));
	Q_strncpy(button->m_LightingData.EntityName, entity, sizeof(button->m_LightingData.EntityName));
	Q_strncpy(button->m_LightingData.EntityAttachment, attachment, sizeof(button->m_LightingData.EntityAttachment));
	button->m_LightingData.color = color;
	button->m_LightingData.distance = distance;
	button->m_LightingData.fov = fov;
	button->m_LightingData.offset = offset;
	button->m_LightingData.angoffset = angoffset;
	button->m_LightingData.origin = origin;
	button->m_LightingData.angles = angle;
	button->m_LightingData.mode = mode;
	button->m_LightingData.ActiveType = activetype;
	button->m_LightingData.movementmode = movemode;

	if (button->m_LightingData.origin == vec3_origin)
		button->m_LightingData.origin = pPlayer->EyePosition();
	if (button->m_LightingData.angles == vec3_angle)
		button->m_LightingData.angles = pPlayer->EyeAngles();

	//add the button to the list of buttons
	m_ButtonList.AddToTail(button);

	//select this
	OnCommand(CFmtStr(LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count() - 1));


	//check the list size
	if (m_ButtonList.Count() > LIGHTING_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL)
	{
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetRange(0, m_ButtonList.Count() - LIGHTING_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
		m_ScrollBar->SetValue(m_ButtonList.Count() - LIGHTING_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
	}
	else
	{
		m_ScrollBar->SetRange(0, 0);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Sets the selected lights pos to the player's eye position + angle
//---------------------------------------------------------------------------------
bool CLightingPageList::SetSelectedLightToPlayersEyes()
{
	//check for player
	CBasePlayer* pPlayer = CBasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		//set the pos
		button->m_LightingData.origin = pPlayer->EyePosition();
		button->m_LightingData.angles = pPlayer->EyeAngles();
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Sets the light level transition state
//---------------------------------------------------------------------------------
void CLightingPageList::SetLightTransitionState(bool transitioning)
{
	//see if we are transitioning. If so then delete all the lights
	if (transitioning)
	{
		for (int i = 0; i < m_ButtonList.Count(); i++)
			m_ButtonList[i]->DeleteLights();
	}

	m_bTransitioning = transitioning;
}

#define LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST false

//---------------------------------------------------------------------------------
// Purpose: Sets the light level transition state
//---------------------------------------------------------------------------------
void CLightingPageList::DrawDebugOverlays()
{
	//go through each button and see which one is selected
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//get the data and draw the lights
		LightingButtonData_t* data = &button->m_LightingData;

		//draw dlight debug
		for (int i = 0; i < data->m_DynamicLights.Count(); i++)
		{
			//check for dlight
			dlight_t* dlight = data->m_DynamicLights[i];
			if (!dlight)
				continue;

			NDebugOverlay::Box(dlight->origin, Vector(-6, -6, -6), Vector(6, 6, 6), dlight->color.r, dlight->color.g, dlight->color.b, 100, 0.03);
			NDebugOverlay::Sphere(dlight->origin, vec3_angle, dlight->radius * (dlight->color.exponent - 1.28), dlight->color.r, dlight->color.g, dlight->color.b, 25, LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03);
			NDebugOverlay::Text(data->origin, CFmtStr("Type = %s", data->mode == LightingMode_t::Mode_DynamicLight ? "dynamic light" : "environmental light"), false, 0.03);
			NDebugOverlay::Text(Vector(data->origin.x, data->origin.y, data->origin.z - 10), CFmtStr("Name = %s", data->name), false, 0.03);
		}
		
		//draw flashlight debug
		for (int i = 0; i < data->m_DynamicFlashlights.Count(); i++)
		{
			//get the flashlight
			CFlashlightEffect* flashlight = data->m_DynamicFlashlights[i];
			if (!flashlight)
				continue;

			//flashlight frustum debug
			Vector pos = flashlight->m_pos;
			Vector forward = flashlight->m_forward;
			Vector right = flashlight->m_right;
			Vector up = flashlight->m_up;

			//flashlight FOV and distance
			float fov = data->fov;
			float farZ = data->distance;

			//compute half-size of frustum at FarZ
			float tanHalfFOV = tanf(DEG2RAD(fov * 0.5f));
			float halfSize = tanHalfFOV * farZ;

			//near plane is basically at origin of flashlight
			Vector nearCenter = pos;

			//far plane center
			Vector farCenter = pos + forward * farZ;

			//far plane corners
			Vector fr = farCenter + (right * halfSize) + (up * halfSize);
			Vector fl = farCenter - (right * halfSize) + (up * halfSize);
			Vector br = farCenter + (right * halfSize) - (up * halfSize);
			Vector bl = farCenter - (right * halfSize) - (up * halfSize);

			QAngle boxangle;
			boxangle.y = atan2f(forward.y, forward.x) * (180.0f / M_PI);
			boxangle.x = atan2f(-forward.z, sqrtf(forward.x * forward.x + forward.y * forward.y)) * (180.0f / M_PI);
			boxangle.z = atan2f(right.z, up.z) * (180.0f / M_PI);

			//draw debug box
			NDebugOverlay::BoxAngles(pos, Vector(-4, -4, -4), Vector(4, 4, 4), boxangle, data->color.r(), data->color.g(), data->color.b(), 100, 0.03);

			//draw lines to corners
			NDebugOverlay::Line(nearCenter, fr, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(nearCenter, fl, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(nearCenter, br, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(nearCenter, bl, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);

			//outline far plane
			NDebugOverlay::Line(fr, fl, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(fl, bl, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(bl, br, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);
			NDebugOverlay::Line(br, fr, data->color.r(), data->color.g(), data->color.b(), LIGHTING_DEBUG_OVERLAYS_DEPTH_TEST, 0.03f);

			NDebugOverlay::Text(pos, "Type = flashlight", false, 0.03);
			NDebugOverlay::Text(Vector(pos.x, pos.y, pos.z - 10), CFmtStr("Name = %s", data->name), false, 0.03);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns if this list has a light with the same name
//---------------------------------------------------------------------------------
bool CLightingPageList::HasLighting(const char* LightName)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		if (!Q_stricmp(button->m_LightingData.name, LightName))
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Changes an overlay button's value
//---------------------------------------------------------------------------------
bool CLightingPageList::ChangeLighting(const char* LightName, const char* NewLightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment)
{
	bool bRet = false;

	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//remove the light so it gets updated next update
		button->DeleteLights();

		//reset the lights index
		m_DlightIndex = 1;
		m_FlashLightIndex = 1;

		if (!Q_stricmp(button->m_LightingData.name, LightName))
		{
			//set the lighting name data
			Q_strncpy(button->m_LightingData.name, NewLightName, sizeof(button->m_LightingData.name));
			Q_strncpy(button->m_LightingData.EntityName, entity, sizeof(button->m_LightingData.EntityName));
			Q_strncpy(button->m_LightingData.EntityAttachment, attachment, sizeof(button->m_LightingData.EntityAttachment));
			button->m_LightingData.color = color;
			button->m_LightingData.distance = distance;
			button->m_LightingData.fov = fov;
			button->m_LightingData.offset = offset;
			button->m_LightingData.angoffset = angoffset;
			button->m_LightingData.mode = mode;
			button->m_LightingData.ActiveType = activetype;
			button->m_LightingData.movementmode = movemode;

			//set the button's text
			button->SetText(NewLightName);

			bRet = true;
		}
	}

	return bRet;
}

//---------------------------------------------------------------------------------
// Purpose: Changes a selected overlay
//---------------------------------------------------------------------------------
bool CLightingPageList::ChangeSelectedLighting(const char* NewLightName, Color color, int distance, int fov, Vector offset, QAngle angoffset, LightingMode_t mode, LightingMovementMode_t movemode, LightingType_e activetype, const char* entity, const char* attachment)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		//see if the name matches. If not check for button with same name
		if (Q_strcmp(button->m_LightingData.name, NewLightName) && HasLighting(NewLightName))
			return false;

		//change the value
		return ChangeLighting(button->m_LightingData.name, NewLightName, color, distance, fov, offset, angoffset, mode, movemode, activetype, entity, attachment);
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Returns all of the lighting data
//---------------------------------------------------------------------------------
void CLightingPageList::GetLightingData(CUtlVector<LightingButtonData_t*>& data)
{
	//go through each button data
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		data.AddToTail(&button->m_LightingData);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Clears all of the lights
//---------------------------------------------------------------------------------
void CLightingPageList::ClearLights()
{
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
		
		//delete the button now
		delete button;
	}

	//clear the array
	m_ButtonList.RemoveAll();

	//set the scroll wheel
	m_ScrollBar->SetRange(0, 0);
	m_ScrollBar->SetValue(0);
}

//---------------------------------------------------------------------------------
// Purpose: Removes the selected light
//---------------------------------------------------------------------------------
bool CLightingPageList::RemoveSelectedLight()
{
	int SelectedIndex = -1;
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		SelectedIndex = i;

		//delete the button now
		delete button;
		m_ButtonList.Remove(i--);
		break;
	}

	if (SelectedIndex == -1)
		return false;

	//now reset all of the commands
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CLightingPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//reset the command
		button->SetCommand(CFmtStr(LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", i));

		//reset the bounds
		button->SetPos(LIGHTING_LIST_PANEL_BUTTON_X_OFFSET, LIGHTING_LIST_PANEL_TITLE_HEIGHT + (LIGHTING_LIST_PANEL_BUTTON_HEIGHT * i));

		//delete all the lights so the lights get reset to have the correct index
		button->DeleteLights();
	}

	//set the scroll bar range
	int min, max;
	m_ScrollBar->GetRange(min, max);
	m_ScrollBar->SetRange(min, max - 1);
	m_ScrollBar->SetValue(m_ScrollBar->GetValue());

	//bounds check
	if (SelectedIndex >= m_ButtonList.Count())
		SelectedIndex -= 1;

	//select the last button
	OnCommand(CFmtStr(LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX "%d", SelectedIndex));

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if an overlay should draw
//---------------------------------------------------------------------------------
bool CLightingPageList::ShouldLightBeActive(C_BaseHLPlayer* pPlayer, LightingType_e type)
{
	//check for player
	if (!pPlayer)
		return false;

	//check for always draw
	if (((int)type & (int)LightingType_e::Active_Always))
		return true;

	//check for flashlight on
	if (((int)type & (int)LightingType_e::Active_WhenPlayerFlashlightOn) && pPlayer->IsEffectActive(EF_DIMLIGHT))
		return true;

	//check for flashlight off
	if (((int)type & (int)LightingType_e::Active_WhenPlayerFlashlightOff) && !pPlayer->IsEffectActive(EF_DIMLIGHT))
		return true;

	//moving?
	Vector vel = pPlayer->GetAbsVelocity();
	vel.z = 0;
	bool moving = vel.Length() >= 25;

	//check for walking
	if (((int)type & (int)LightingType_e::Active_WhenWalking) && moving && !pPlayer->IsSprinting())
		return true;

	//check for sprinting
	if (((int)type & (int)LightingType_e::Active_WhenSprinting) && moving && pPlayer->IsSprinting())
		return true;

	//check for health
	if (((int)type & (int)LightingType_e::Active_WhenHealthLow) && pPlayer->GetHealth() <= 20)
		return true;

	//check for under water
	if (((int)type & (int)LightingType_e::Active_WhenUnderWater) && pPlayer->GetWaterLevel() < WL_Eyes)
		return true;

	//check for crouching
	if (((int)type & (int)LightingType_e::Active_WhenCrouching) && (pPlayer->GetFlags() & FL_DUCKING))
		return true;

	//check for on ground
	if (((int)type & (int)LightingType_e::Active_WhenOnGround) && (pPlayer->GetFlags() & FL_ONGROUND))
		return true;

	//check for holding object
	if (((int)type & (int)LightingType_e::Active_WhenHoldingObject) && pPlayer->GetUseEntity())
		return true;

	//check for zooming
	if (((int)type & (int)LightingType_e::Active_WhenUsingSuitZoom) && pPlayer->IsZoomed())
		return true;

	//check for in air
	if (((int)type & (int)LightingType_e::Active_WhenInAir) && !(pPlayer->GetFlags() & FL_ONGROUND))
		return true;

	//should draw
	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Quick helper func to see if type is of the flashlight type
//---------------------------------------------------------------------------------
static const bool IsFlashlightMode(const LightingMode_t& mode)
{
	return mode == LightingMode_t::Mode_Flashlight;
}

//---------------------------------------------------------------------------------
// Purpose: Quick helper func to see if type is of the dynamic light type
//---------------------------------------------------------------------------------
static const bool IsDynamicLight(const LightingMode_t& mode)
{
	return mode == LightingMode_t::Mode_DynamicLight;
}

//---------------------------------------------------------------------------------
// Purpose: Quick helper func to see if type is of the dynamic enviroment light type
//---------------------------------------------------------------------------------
static const bool IsDynamicEnvironmentLight(const LightingMode_t& mode)
{
	return mode == LightingMode_t::Mode_DynamicEnvironmentalLight;
}

//offset for light index
#define LIGHT_INDEX_OFFSET 0

//---------------------------------------------------------------------------------
// Purpose: string comparison func with wildcard
//---------------------------------------------------------------------------------
int Q_strcmpwildcard(const char* str, const char* pattern)
{
	const char* star = NULL;
	const char* backup = NULL;

	//check str
	if (!*str)
		return 1;

	while (*str)
	{
		char c1 = tolower((unsigned char)*str);
		char c2 = tolower((unsigned char)*pattern);

		//match single char using ? or exact char
		if (*pattern == '?' || c1 == c2)
		{
			str++;
			pattern++;
			continue;
		}

		// * means wildcard run
		if (*pattern == '*')
		{
			star = pattern;
			pattern++;
			backup = str;
			continue;
		}

		//mismatch
		if (star)
		{
			pattern = star + 1;
			backup++;
			str = backup;
			continue;
		}

		return 1;
	}
	//skip trailing stars
	while (*pattern == '*')
		pattern++;

	return (*pattern == '\0') ? 0 : 1;
}

//---------------------------------------------------------------------------------
// Purpose: Creates a dlight
//---------------------------------------------------------------------------------
void CLightingPageList::CreateDLight(LightingButtonData_t* data, int index)
{
	//create the light
	dlight_t* light = IsDynamicLight(data->mode) ? effects->CL_AllocDlight(m_DlightIndex++ + LIGHT_INDEX_OFFSET) 
												 : effects->CL_AllocElight(m_DlightIndex++ + LIGHT_INDEX_OFFSET);

	//set the stuff
	light->die = gpGlobals->curtime + 1e10;
	light->flags = 0;
	light->minlight = 0;
	light->radius = data->distance;
	light->color.r = data->color.r();
	light->color.g = data->color.g();
	light->color.b = data->color.b();
	light->color.exponent = (float)data->color.a() / 100;

	int oldCount = data->m_DynamicLights.Count();
	data->m_DynamicLights.EnsureCount(index + 1);

	for (int i = oldCount; i < data->m_DynamicLights.Count(); i++)
		data->m_DynamicLights[i] = NULL;

	data->m_DynamicLights[index] = light;
}

//---------------------------------------------------------------------------------
// Purpose: Creates a flashlight
//---------------------------------------------------------------------------------
void CLightingPageList::CreateFlashLight(LightingButtonData_t* data, int index)
{
	//create the light
	CFlashlightEffect* flashlight = new CFlashlightEffect(m_FlashLightIndex++ + LIGHT_INDEX_OFFSET);
	flashlight->TurnOn();

	int oldCount = data->m_DynamicFlashlights.Count();
	data->m_DynamicFlashlights.EnsureCount(index + 1);

	for (int i = oldCount; i < data->m_DynamicFlashlights.Count(); i++)
		data->m_DynamicFlashlights[i] = NULL;

	data->m_DynamicFlashlights[index] = flashlight;
}

//---------------------------------------------------------------------------------
// Purpose: Updates a static light
//---------------------------------------------------------------------------------
Vector CLightingPageList::GetPositionOffset(Vector position, QAngle angle, Vector offset, Vector* forward, Vector* right, Vector* up)
{
	//get the directions
	Vector fw, rt, dn;
	AngleVectors(angle, &fw, &rt, &dn);

	//set the forward, right and up input if not nullptr
	if (forward)
		forward->Init(fw.x, fw.y, fw.z);
	
	if (right)
		right->Init(rt.x, rt.y, rt.z);
	
	if (up)
		up->Init(dn.x, dn.y, dn.z);

	//return the offset
	return Vector(position + (fw * offset.x) + (rt * offset.y) + (dn * offset.z));
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light at the index
//---------------------------------------------------------------------------------
void CLightingPageList::UpdateLight(LightingButtonData_t* data, int index, const LightingMode_t& LightMode)
{
	//get vectors for flashlight
	Vector forward, right, up;

	//get the stuff
	QAngle angle = data->angles + data->angoffset;
	Vector origin = data->origin;
	Vector offset = data->offset;

	//get the position
	Vector position = GetPositionOffset(origin, angle, offset, &forward, &right, &up);

	//create the flashlight if null
	if (IsFlashlightMode(LightMode) && data->m_DynamicFlashlights.Count() <= index)
	{
		//create the flashlight
		CreateFlashLight(data, index);
	}

	//create the dlight if null
	else if ((IsDynamicLight(LightMode) || IsDynamicEnvironmentLight(LightMode)) && data->m_DynamicLights.Count() <= index)
	{
		//create the dlight or elight
		CreateDLight(data, index);
	}

	//if the flashlight mode then update the flashlight
	if (IsFlashlightMode(LightMode))
	{
		//update the light
		if (data->m_DynamicFlashlights[index])
			data->m_DynamicFlashlights[index]->UpdateLightEffectsPanel(position, forward, right, up, data->color, data->distance, data->fov);
	}
	else
	{
		//update the dlight
		if (data->m_DynamicLights[index])
			data->m_DynamicLights[index]->origin = position;
	}
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light at the index
//---------------------------------------------------------------------------------
void CLightingPageList::RemoveLight(LightingButtonData_t* data, int index, const LightingMode_t& LightMode)
{
	//if the flashlight mode then remove the flashlight, else remove the dlight
	if (IsFlashlightMode(LightMode))
	{
		//remove the flashlight if we can
		if (index < data->m_DynamicFlashlights.Count() && data->m_DynamicFlashlights[index])
		{
			delete data->m_DynamicFlashlights[index];
			data->m_DynamicFlashlights.Remove(index);
		}
	}
	else
	{
		//remove the light if we can
		if (index < data->m_DynamicLights.Count() && data->m_DynamicLights[index])
		{
			//you cant actually 'delete' dlight_t*'s because they are located in an array in the engine's code. So just
			//make the dlight die now so the engine removes the light
			data->m_DynamicLights[index]->die = gpGlobals->curtime + 0.0f;
			data->m_DynamicLights[index]->radius = 0.0f;
			data->m_DynamicLights.Remove(index);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light at the player's eye position
//---------------------------------------------------------------------------------
void CLightingPageList::UpdatePlayerEyesLight(LightingButtonData_t* data, C_BasePlayer* pPlayer, const LightingMode_t& LightMode)
{
	//set the data pos + angle
	data->origin = pPlayer->EyePosition();
	data->angles = pPlayer->EyeAngles();

	//update the light
	UpdateLight(data, 0, LightMode);
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light at the player's viewmodel's position
//---------------------------------------------------------------------------------
void CLightingPageList::UpdatePlayerViewmodelight(LightingButtonData_t* data, C_BasePlayer* pPlayer, const LightingMode_t& LightMode)
{
	//get the player's weapon
	CBaseEntity* pWeapon = pPlayer->GetActiveWeapon();
	if (!pWeapon)
	{
		RemoveLight(data, 0, LightMode);
		return;
	}

	//check for the muzzle attachment
	int muzzle = pWeapon->LookupAttachment("muzzle");
	if (muzzle == 0)
	{
		RemoveLight(data, 0, LightMode);
		return;
	}

	//get the origin and update the light
	pWeapon->GetAttachment(muzzle, data->origin, data->angles);
	UpdateLight(data, 0, LightMode);
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light for an entity
//---------------------------------------------------------------------------------
void CLightingPageList::UpdateEntityLight(LightingButtonData_t* data, const LightingMode_t& LightMode)
{
	if (!engine->IsConnected())
		return;

	//get the entity stuff
	C_BaseEntity* pEntity = nullptr;
	C_BaseEntityIterator itterator;

	bool bFound = false;

	//go through each entity
	while ((pEntity = itterator.Next()) != nullptr)
	{
		//check for the entity's targetname or classname
		if ((data->movementmode == LightingMovementMode_t::Mode_ParentToClassnameEntity && !Q_strcmpwildcard(pEntity->GetRealClassname(), data->EntityName)) ||
			(data->movementmode == LightingMovementMode_t::Mode_ParentToTargetnameEntity && !Q_strcmpwildcard(pEntity->GetEntityName(), data->EntityName)) || 
			(data->movementmode == LightingMovementMode_t::Mode_ParentToModelEntity && !Q_strcmpwildcard(pEntity->GetModelName(), data->EntityName)))
		{
			//check for the attachment
			if (data->EntityAttachment[0])
			{
				//look for the attachment
				int attachment = pEntity->LookupAttachment(data->EntityAttachment);
				if (attachment == 0)
				{
					//no attachment found so just use the abs origin + angles
					data->origin = pEntity->EyePosition();
					data->angles = pEntity->EyeAngles();
				}
				else
				{
					//attachment found. Use the attachment's position + angle
					pEntity->GetAttachment(attachment, data->origin, data->angles);
				}
			}
			else
			{
				//set the data pos + angle
				data->origin = pEntity->EyePosition();
				data->angles = pEntity->EyeAngles();
			}

			//break out of the loop (we found the entity)
			bFound = true;
			break;
		}
	}

	//update the light if the entity was found, Else remove it
	if (bFound)
		UpdateLight(data, 0, LightMode);
	else
		RemoveLight(data, 0, LightMode);
}

//---------------------------------------------------------------------------------
// Purpose: Updates a light for all entities with the same ...
//---------------------------------------------------------------------------------
void CLightingPageList::UpdateEntityAllLight(LightingButtonData_t* data, const LightingMode_t& LightMode)
{
	//get the entity stuff
	C_BaseEntity* pEntity = nullptr;
	C_BaseEntityIterator itterator;

	//array vars
	int count = IsFlashlightMode(LightMode) ? data->m_DynamicFlashlights.Count() : data->m_DynamicLights.Count();
	int amount = 0;

	//go through each entity
	while ((pEntity = itterator.Next()) != nullptr)
	{
		//check for the entity's targetname or classname
		if ((data->movementmode == LightingMovementMode_t::Mode_ParentToAllClassnameEntity && !Q_strcmpwildcard(pEntity->GetRealClassname(), data->EntityName)) ||
			(data->movementmode == LightingMovementMode_t::Mode_ParentToAllTargetnameEntity && !Q_strcmpwildcard(pEntity->GetEntityName(), data->EntityName)) ||
			(data->movementmode == LightingMovementMode_t::Mode_ParentToAllModelEntity && !Q_strcmpwildcard(pEntity->GetModelName(), data->EntityName)))
		{
			//check for the attachment
			if (data->EntityAttachment[0])
			{
				//look for the attachment
				int attachment = pEntity->LookupAttachment(data->EntityAttachment);
				if (attachment == 0)
				{
					//no attachment found so just use the abs origin + angles
					data->origin = pEntity->EyePosition();
					data->angles = pEntity->EyeAngles();
				}
				else
				{
					//attachment found. Use the attachment's position + angle
					pEntity->GetAttachment(attachment, data->origin, data->angles);
				}
			}
			else
			{
				//set the data pos + angle
				data->origin = pEntity->EyePosition();
				data->angles = pEntity->EyeAngles();
			}

			//update the light
			UpdateLight(data, amount++, LightMode);
		}
	}

	//remove all the unused lights
	for (int i = amount; i < count;)
		RemoveLight(data, --count, LightMode);
}

//---------------------------------------------------------------------------------
// Purpose: Updates all the lights
//---------------------------------------------------------------------------------
void CLightingPageList::UpdateLights()
{
	if (m_bTransitioning)
		return;

	//check for the player
	CHL2_Player* pPlayer = dynamic_cast<CHL2_Player*>(CBasePlayer::GetLocalPlayer());
	if (!pPlayer)
		return;

	//go through each button/light
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//get the data
		LightingButtonData_t* data = &m_ButtonList[i]->m_LightingData;

		//remove unactive lights and return
		if (!ShouldLightBeActive(pPlayer, data->ActiveType))
		{
			//go through each light
			for (int j = 0; j < data->m_DynamicLights.Count(); j++)
			{
				RemoveLight(data, j, LightingMode_t::Mode_DynamicLight);
				RemoveLight(data, j, LightingMode_t::Mode_DynamicEnvironmentalLight);
			}

			//go through each flashlight
			for (int j = 0; j < data->m_DynamicFlashlights.Count(); j++)
				RemoveLight(data, j, LightingMode_t::Mode_Flashlight);

			continue;
		}

		//check the mode
		if (data->movementmode == LightingMovementMode_t::Mode_Static)
			UpdateLight(data, 0, data->mode);

		else if (data->movementmode == LightingMovementMode_t::Mode_PlayersEyes)
			UpdatePlayerEyesLight(data, pPlayer, data->mode);
		
		else if (data->movementmode == LightingMovementMode_t::Mode_PlayersWeaponMuzzle)
			UpdatePlayerViewmodelight(data, pPlayer, data->mode);
		
		else if (data->movementmode == LightingMovementMode_t::Mode_ParentToClassnameEntity ||  data->movementmode == LightingMovementMode_t::Mode_ParentToTargetnameEntity ||
				data->movementmode == LightingMovementMode_t::Mode_ParentToModelEntity)
			UpdateEntityLight(data, data->mode);
		
		else if (data->movementmode == LightingMovementMode_t::Mode_ParentToAllClassnameEntity ||  data->movementmode == LightingMovementMode_t::Mode_ParentToAllTargetnameEntity ||
				data->movementmode == LightingMovementMode_t::Mode_ParentToAllModelEntity)
			UpdateEntityAllLight(data, data->mode);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CLightingPageList::OnCommand(const char* pszCommand)
{
	if (StringHasPrefix(pszCommand, LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX))
	{
		//get the index
		int index = Q_atoi(pszCommand + Q_strlen(LIGHTING_LIST_PANEL_BUTTON_COMMAND_PREFIX));

		//select this button and de-select every other button
		for (int i = 0; i < m_ButtonList.Count(); i++)
		{
			//check for the button
			CLightingPageButton* button = m_ButtonList[i];
			if (!button)
				continue;

			//select the button ONLY if i == index
			button->SetButtonSelected(i == index);

			//set the overlay page text if i == index
			if (i == index)
				m_LightingPage->SetLightingText(button->m_LightingData);
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//lighting page

//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel lighting page
//---------------------------------------------------------------------------------
CEffectsPanelLightingPage::CEffectsPanelLightingPage(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//make the name/type stuff
	m_LightNameLabel = new vgui::Label(this, "LightNameLabel", "Name:");
	m_TypeLabel = new vgui::Label(this, "LightTypeLabel", "Light Type:");

	m_LightNameTextEntry = new CLightingPageTextEntry(this, "NameTextEntry");
	m_LightNameTextEntry->SetText("Light 1");
	m_LightNameTextEntry->SetMaximumCharCount(63);

	//type combo box
	m_TypeComboBox = new vgui::ComboBox(this, "LightTypeComboBox", 3, false);
	
	for (int i = 0; i < SIZE_OF_ARRAY(gsz_LightingModeStrings); i++)
		m_TypeComboBox->AddItem(gsz_LightingModeStrings[i], nullptr);

	m_TypeComboBox->ActivateItem(0);

	//make the mode/lighting offset/attach to entity name/attach to entity attachment text's
	m_LightingMovementModeLabel = new vgui::Label(this, "LightingMovementModeLabel", "Lighting Movement Mode:");
	m_LightingOffsetLabel = new vgui::Label(this, "LightingOffsetLabel", "Origin Offset (x y z):");
	m_LightingAngleOffsetLabel = new vgui::Label(this, "LightingAngleOffsetLabel", "Angle Offset (x y z):");
	m_LightingEntityLabel = new vgui::Label(this, "AttachToEntityLabel", "Entity 'name':");
	m_LightingEntityAttachmentLabel = new vgui::Label(this, "AttachToEntityAttachmentLabel", "Opt Entity 'attachment':");

	//movement combo box
	m_LightMovementComboBox = new vgui::ComboBox(this, "LightTypeComboBox", 10, false);

	for (int i = 0; i < SIZE_OF_ARRAY(gsz_LightingMovementModeStrings); i++)
		m_LightMovementComboBox->AddItem(gsz_LightingMovementModeStrings[i], nullptr);

	m_LightMovementComboBox->ActivateItem(0);
	
	//text entries
	m_LightOffsetTextEntry = new CLightingPageTextEntry(this, "LightOffsetTextEntry");
	m_LightOffsetTextEntry->SetText("0 0 0");
	m_LightOffsetTextEntry->SetMaximumCharCount(32);
	
	m_LightAngleOffsetTextEntry = new CLightingPageTextEntry(this, "LightAngleOffsetTextEntry");
	m_LightAngleOffsetTextEntry->SetText("0 0 0");
	m_LightAngleOffsetTextEntry->SetMaximumCharCount(32);
	
	m_LightingEntityTextEntry = new CLightingPageTextEntry(this, "LightingEntityTextEntry");
	m_LightingEntityTextEntry->SetText("");
	m_LightingEntityTextEntry->SetMaximumCharCount(127);
	m_LightingEntityTextEntry->SetEnabled(false);
	
	m_LightingEntityAttachmentTextEntry = new CLightingPageTextEntry(this, "LightingEntityAttachmentTextEntry");
	m_LightingEntityAttachmentTextEntry->SetText("");
	m_LightingEntityAttachmentTextEntry->SetEnabled(false);
	m_LightingEntityAttachmentTextEntry->SetMaximumCharCount(63);

	//make the color/fov/far stuff
	m_ColorLabel = new vgui::Label(this, "ColorLabel", "Color (r g b a)");
	m_FarLabel = new vgui::Label(this, "FarLabel", "Distance = 750");
	m_FovLabel = new vgui::Label(this, "FovLabel", "Flashlight Fov = 45");
	
	m_ColorTextEntry = new CLightingPageTextEntry(this, "ColorTextEntry");
	m_ColorTextEntry->SetText("255 255 255 255");
	m_ColorTextEntry->SetMaximumCharCount(32);

	m_FarSlider = new WheelSlider(this, "FarSlider");
	m_FarSlider->SetRange(0, 3000);
	m_FarSlider->SetValue(750);
	
	m_FovSlider = new WheelSlider(this, "FovSlider");
	m_FovSlider->SetRange(0, 179);
	m_FovSlider->SetValue(45);

	//make buttons
	m_AddButton = new vgui::Button(this, "AddButton", "Add Light", this, ADD_LIGHT_BUTTON_COMMAND);
	m_ChangeLight = new vgui::Button(this, "ChangeButton", "Update Selected Light", this, CHANGE_LIGHT_BUTTON_COMMAND);
	m_RemoveLight = new vgui::Button(this, "RemoveOverlay", "Remove Selected Light", this, REMOVE_LIGHT_BUTTON_COMMAND);
	m_SetSelectedToPlayer = new vgui::Button(this, "SetSelectedToPlayerButton", "Set selected lights position and angle to players current position and angle + offset", this, LIGHT_SET_TO_PLAYERS_POS_BUTTON);

	//active type combo box
	m_ActiveModeComboBox = new vgui::ComboBox(this, "ActiveTypeComboBox", 14, false);
	m_ActiveModeComboBox->SetMaximumCharCount(254);

	//add types
	for (int i = 0; i < SIZE_OF_ARRAY(g_LightingActiveType); i++)
	{
		m_ActiveModeComboBox->GetMenu()->AddCheckableMenuItem(g_LightingActiveType[i], new KeyValues("SetText", "text", g_LightingActiveType[i]), this, nullptr);
		m_ActiveModeComboBox->GetMenu()->GetMenuItem(i)->SetCommand(CFmtStr(LIGHTING_PAGE_ACTIVE_TYPE_PREFIX "%d", i));
		m_ActiveModeComboBox->GetMenu()->GetMenuItem(i)->AddActionSignalTarget(this);
	}

	//set the text
	m_ActiveModeComboBox->SetText("Active Types:");

	//finally make the light list
	m_LightList = new CLightingPageList(this, "LightingList", "Lights:");

	//reset the things to the defaults
	ResetEffects();

	//set the bounds for each item
	PerformLayout();

	//add this to the lighting pages list
	g_EffectsLightingPages.AddToTail(this);
}

//---------------------------------------------------------------------------------
// Purpose: destructor
//---------------------------------------------------------------------------------
CEffectsPanelLightingPage::~CEffectsPanelLightingPage()
{
	//remove this from the lighting pages list
	g_EffectsLightingPages.FindAndRemove(this);
}

//---------------------------------------------------------------------------------
// Purpose: Resets all the lighting on this page
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::ResetEffects()
{
	//reset mode
	m_ActiveModeComboBox->SetText("Active Types:");

	//only select menu item 1 (Always active)
	for (int i = 0; i < m_ActiveModeComboBox->GetItemCount(); i++)
		m_ActiveModeComboBox->GetMenu()->SetMenuItemChecked(i, i == 0);

	//delete the lights
	m_LightList->ClearLights();

	//set thte text entry/slider's
	m_LightNameTextEntry->SetText("Light 1");
	m_TypeComboBox->ActivateItem(0);
	m_LightOffsetTextEntry->SetText("0 0 0");
	m_LightAngleOffsetTextEntry->SetText("0 0 0");
	
	m_LightingEntityTextEntry->SetText("");
	m_LightingEntityTextEntry->SetEnabled(false);

	m_LightingEntityAttachmentTextEntry->SetText("");
	m_LightingEntityAttachmentTextEntry->SetEnabled(false);

	m_ColorTextEntry->SetText("255 255 255 255");
	m_FarSlider->SetValue(750);
	m_FovSlider->SetValue(45);
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::ReadFromFile(KeyValues* keyvalues, bool reset)
{
	//clear everything
	if (reset)
		ResetEffects();

	//find the view KeyValues
	KeyValues* lighting = keyvalues->FindKey("Lighting");
	if (!lighting)
		return;

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(lighting, light)
	{
		//get offset
		Vector offset = vec3_origin;
		UTIL_StringToVector(offset.Base(), light->GetString("Offset"));
		
		//get angle offset
		QAngle angoffset = vec3_angle;
		UTIL_StringToVector(angoffset.Base(), light->GetString("AngleOffset"));
		
		//get origin
		Vector origin = vec3_origin;
		UTIL_StringToVector(origin.Base(), light->GetString("Origin"));
		
		//get angles
		QAngle angle = vec3_angle;
		UTIL_StringToVector(angle.Base(), light->GetString("Angle"));

		//add it to the list
		m_LightList->AddLighting(light->GetString("Name"), light->GetColor("Color"), light->GetInt("Distance"), light->GetInt("Fov"), offset, angoffset, (LightingMode_t)light->GetInt("Mode"), (LightingMovementMode_t)light->GetInt("MovementMode"), (LightingType_e)light->GetInt("ActiveType"), light->GetString("Entity"), light->GetString("EntityAttachment"), origin, angle);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::WriteToFile(KeyValues* keyvalues)
{
	//create a new KeyValues*
	KeyValues* lighting = new KeyValues("Lighting");

	//get the data
	CUtlVector<LightingButtonData_t*> LightingData;
	m_LightList->GetLightingData(LightingData);

	for (int i = 0; i < LightingData.Count(); i++)
	{
		//create new light subkey
		KeyValues* lightsub = new KeyValues("Light");

		lightsub->SetString("Name", LightingData[i]->name);
		lightsub->SetInt("Mode", (int)LightingData[i]->mode);
		lightsub->SetInt("MovementMode", (int)LightingData[i]->movementmode);
		lightsub->SetString("Offset", CFmtStr("%.4f %.4f %.4f", LightingData[i]->offset.x, LightingData[i]->offset.y, LightingData[i]->offset.z));
		lightsub->SetString("AngleOffset", CFmtStr("%.4f %.4f %.4f", LightingData[i]->angoffset.x, LightingData[i]->angoffset.y, LightingData[i]->angoffset.z));
		lightsub->SetInt("ActiveType", (int)LightingData[i]->ActiveType);

		//only write the origin/angles if static
		if (LightingData[i]->movementmode == LightingMovementMode_t::Mode_Static)
		{
			lightsub->SetString("Origin", CFmtStr("%.4f %.4f %.4f", LightingData[i]->origin.x, LightingData[i]->origin.y, LightingData[i]->origin.z));
			lightsub->SetString("Angle", CFmtStr("%.4f %.4f %.4f", LightingData[i]->angles.x, LightingData[i]->angles.y, LightingData[i]->angles.z));
		}

		lightsub->SetString("Entity", LightingData[i]->EntityName);
		lightsub->SetString("EntityAttachment", LightingData[i]->EntityAttachment);
		lightsub->SetString("Color", CFmtStr("%d %d %d %d", LightingData[i]->color.r(), LightingData[i]->color.g(), LightingData[i]->color.b(), LightingData[i]->color.a()));
		lightsub->SetInt("Distance", LightingData[i]->distance);
		lightsub->SetInt("Fov", LightingData[i]->fov);

		//add to lighting subkey
		lighting->AddSubKey(lightsub);
	}

	//add the lighting to the base subkey
	keyvalues->AddSubKey(lighting);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the lighting text's
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::SetLightingText(LightingButtonData_t& data)
{
	m_LightNameTextEntry->SetText(data.name);
	m_TypeComboBox->ActivateItem((int)data.mode);
	m_LightMovementComboBox->ActivateItem((int)data.movementmode);
	m_LightOffsetTextEntry->SetText(CFmtStr("%.2f %.2f %.2f", data.offset.x, data.offset.y, data.offset.z));
	m_LightAngleOffsetTextEntry->SetText(CFmtStr("%.2f %.2f %.2f", data.angoffset.x, data.angoffset.y, data.angoffset.z));
	m_LightingEntityTextEntry->SetText(data.EntityName);
	m_LightingEntityAttachmentTextEntry->SetText(data.EntityAttachment);
	m_ColorTextEntry->SetText(CFmtStr("%d %d %d %d", data.color.r(), data.color.g(), data.color.b(), data.color.a()));
	m_FarSlider->SetValue(data.distance);
	m_FovSlider->SetValue(data.fov);

	//set the selected stuff
	for (int i = 0; i < m_ActiveModeComboBox->GetItemCount(); i++)
	{
		//see if that component is active
		if ((int)data.ActiveType & (1 << i))
			m_ActiveModeComboBox->GetMenu()->SetMenuItemChecked(i, true);
		else
			m_ActiveModeComboBox->GetMenu()->SetMenuItemChecked(i, false);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::OnCommand(const char* pszCommand)
{
	//check for add command
	if (!Q_strcmp(pszCommand, ADD_LIGHT_BUTTON_COMMAND))
	{
		//get the name
		char buf[255];
		m_LightNameTextEntry->GetText(buf, sizeof(buf));

		//see if buf is empty
		if (!buf[0])
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: Cant have empty light name");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}

		//get the color
		char colorstring[64];
		m_ColorTextEntry->GetText(colorstring, sizeof(colorstring));

		int r = 0, g = 0, b = 0, a = 255;
		sscanf(colorstring, "%d %d %d %d", &r, &g, &b, &a);

		//get the offset
		char offsetstring[64];
		m_LightOffsetTextEntry->GetText(offsetstring, sizeof(offsetstring));

		Vector offset;
		UTIL_StringToVector(offset.Base(), offsetstring);
		
		//get the angle offset
		char angoffsetstring[64];
		m_LightAngleOffsetTextEntry->GetText(angoffsetstring, sizeof(angoffsetstring));

		QAngle angoffset;
		UTIL_StringToVector(angoffset.Base(), angoffsetstring);

		//get the entity name
		char entity[64];
		m_LightingEntityTextEntry->GetText(entity, sizeof(entity));
		
		//get the entity attachment
		char attachment[64];
		m_LightingEntityAttachmentTextEntry->GetText(attachment, sizeof(attachment));

		//get the flags
		int drawtype = NULL;
		for (int i = 0; i < m_ActiveModeComboBox->GetItemCount(); i++)
		{
			if (m_ActiveModeComboBox->GetMenu()->IsChecked(i))
				drawtype |= (1 << i);
		}

		//see if we add the lighting
		if (!m_LightList->AddLighting(buf, Color(r, g, b, a), m_FarSlider->GetValue(), m_FovSlider->GetValue(), offset, angoffset, (LightingMode_t)m_TypeComboBox->GetActiveItem(), (LightingMovementMode_t)m_LightMovementComboBox->GetActiveItem(), (LightingType_e)drawtype, entity, attachment))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr("Error: Light with the name \"%s\" already exists!!", buf));
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for change command
	else if (!Q_strcmp(pszCommand, CHANGE_LIGHT_BUTTON_COMMAND))
	{
		//get the name
		char buf[255];
		m_LightNameTextEntry->GetText(buf, sizeof(buf));

		//see if buf is empty
		if (!buf[0])
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: Cant have empty light name");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}

		//get the color
		char colorstring[64];
		m_ColorTextEntry->GetText(colorstring, sizeof(colorstring));

		int r = 0, g = 0, b = 0, a = 255;
		sscanf(colorstring, "%d %d %d %d", &r, &g, &b, &a);

		//get the offset
		char offsetstring[64];
		m_LightOffsetTextEntry->GetText(offsetstring, sizeof(offsetstring));

		Vector offset;
		UTIL_StringToVector(offset.Base(), offsetstring);

		//get the entity name
		char entity[64];
		m_LightingEntityTextEntry->GetText(entity, sizeof(entity));

		//get the angle offset
		char angoffsetstring[64];
		m_LightAngleOffsetTextEntry->GetText(angoffsetstring, sizeof(angoffsetstring));

		QAngle angoffset;
		UTIL_StringToVector(angoffset.Base(), angoffsetstring);

		//get the entity attachment
		char attachment[64];
		m_LightingEntityAttachmentTextEntry->GetText(attachment, sizeof(attachment));

		//get the flags
		int drawtype = NULL;
		for (int i = 0; i < m_ActiveModeComboBox->GetItemCount(); i++)
		{
			if (m_ActiveModeComboBox->GetMenu()->IsChecked(i))
				drawtype |= (1 << i);
		}

		//see if we can change the light
		if (!m_LightList->ChangeSelectedLighting(buf, Color(r, g, b, a), m_FarSlider->GetValue(), m_FovSlider->GetValue(), offset, angoffset, (LightingMode_t)m_TypeComboBox->GetActiveItem(), (LightingMovementMode_t)m_LightMovementComboBox->GetActiveItem(), (LightingType_e)drawtype, entity, attachment))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No light currently selected OR got a light with the same name as the text entry");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}
	
	//check for remove command
	else if (!Q_strcmp(pszCommand, REMOVE_LIGHT_BUTTON_COMMAND))
	{
		//see if we can remove the light
		if (!m_LightList->RemoveSelectedLight())
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No light currently selected");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for set light pos command
	else if (!Q_strcmp(pszCommand, LIGHT_SET_TO_PLAYERS_POS_BUTTON))
	{
		//see if we can remove the light
		if (!m_LightList->SetSelectedLightToPlayersEyes())
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No light currently selected");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for LIGHTING_PAGE_ACTIVE_TYPE_PREFIX
	else if (StringHasPrefix(pszCommand, LIGHTING_PAGE_ACTIVE_TYPE_PREFIX))
	{
		//get index
		//int index = Q_atoi(pszCommand + Q_strlen(LIGHTING_PAGE_ACTIVE_TYPE_PREFIX));

		//set selected state of menu item
		//m_ActiveModeComboBox->GetMenu()->SetMenuItemChecked(index, m_ActiveModeComboBox->GetMenu()->IsChecked(index));

		//set text
		//m_ActiveModeComboBox->SetText("Active Types:");
	}

	//call base function
	else
	{
		BaseClass::OnCommand(pszCommand);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called every 30ms
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::OnTick()
{
	//enable/disable items
	m_FovSlider->SetEnabled(m_TypeComboBox->GetActiveItem() == (int)LightingMode_t::Mode_Flashlight);

	m_LightingEntityTextEntry->SetEnabled((LightingMovementMode_t)m_LightMovementComboBox->GetActiveItem() >= LightingMovementMode_t::Mode_ParentToClassnameEntity);
	m_LightingEntityAttachmentTextEntry->SetEnabled((LightingMovementMode_t)m_LightMovementComboBox->GetActiveItem() >= LightingMovementMode_t::Mode_ParentToClassnameEntity);

	//set the text's for the sliders
	m_FarLabel->SetText(CFmtStr("Distance = %d", m_FarSlider->GetValue()));
	m_FovLabel->SetText(CFmtStr("Flashlight Fov = %d", m_FovSlider->GetValue()));

	//draw debug overlays for the selected light(s)
	if (amod_lighting_debug.GetBool())
		m_LightList->DrawDebugOverlays();
}

//---------------------------------------------------------------------------------
// Purpose: Updates the lights
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::UpdateLighting()
{
	m_LightList->UpdateLights();
}

//---------------------------------------------------------------------------------
// Purpose: Called on level load
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::OnMapLoad()
{
	//mark the level as transitioning
	if (m_LightList)
		m_LightList->SetLightTransitionState(false);
}

//---------------------------------------------------------------------------------
// Purpose: Called on level shutdown
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::OnMapShutdown()
{
	//mark the level as not transitioning
	if (m_LightList)
		m_LightList->SetLightTransitionState(true);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::PerformLayout()
{
	//make the light list
	m_LightList->SetBounds(5, 10, 520, 122);

	//selected to player button
	m_SetSelectedToPlayer->SetBounds(5, 136, 520, 20);

	//lighting active mode combo box
	m_ActiveModeComboBox->SetBounds(5, 160, 520, 20);

	//name and type stuff
	m_LightNameLabel->SetBounds(5, 186, 50, 22);
	m_TypeLabel->SetBounds(240, 186, 70, 22);
	
	m_LightNameTextEntry->SetBounds(55, 186, 165, 22);
	m_TypeComboBox->SetBounds(310, 186, 215, 22);

	//mode/lighting offset/attach to entity name/attach to entity attachment stuff
	m_LightingMovementModeLabel->SetBounds(5, 210, 150, 22);
	m_LightingOffsetLabel->SetBounds(5, 236, 150, 22);
	m_LightingAngleOffsetLabel->SetBounds(285, 236, 150, 22);
	m_LightingEntityLabel->SetBounds(5, 262, 150, 22);
	m_LightingEntityAttachmentLabel->SetBounds(5, 288, 150, 22);

	m_LightMovementComboBox->SetBounds(160, 212, 365, 22);
	m_LightOffsetTextEntry->SetBounds(160, 238, 110, 22);
	m_LightAngleOffsetTextEntry->SetBounds(415, 238, 110, 22);
	m_LightingEntityTextEntry->SetBounds(160, 264, 365, 22);
	m_LightingEntityAttachmentTextEntry->SetBounds(160, 290, 365, 22);

	//bottom color/far/fov stuff
	m_ColorLabel->SetBounds(5, 320, 170, 22);
	m_FarLabel->SetBounds(182, 320, 170, 22);
	m_FovLabel->SetBounds(359, 320, 170, 22);
	
	m_ColorTextEntry->SetBounds(5, 345, 170, 22);
	m_FarSlider->SetBounds(182, 345, 170, 22);
	m_FovSlider->SetBounds(359, 345, 170, 22);
	
	//bottom buttons
	m_AddButton->SetBounds(5, 370, 170, 22);
	m_ChangeLight->SetBounds(182, 370, 170, 22);
	m_RemoveLight->SetBounds(359, 370, 170, 22);

	BaseClass::PerformLayout();
}