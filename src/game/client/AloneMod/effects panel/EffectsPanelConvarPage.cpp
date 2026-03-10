#include "cbase.h"
#include "EffectsPanelConvarPage.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "fmtstr.h"


//filtered text entry


//---------------------------------------------------------------------------------
// Purpose: Constructor for the filtered text entry
//---------------------------------------------------------------------------------
CFilteredTextEntry::CFilteredTextEntry(vgui::Panel* parent, const char* name, const char* filter)
	: BaseClass(parent, name)
{
	//set the filter
	SetFilter(filter);
}

//---------------------------------------------------------------------------------
// Purpose: Called when a character is typed
//---------------------------------------------------------------------------------
void CFilteredTextEntry::OnKeyTyped(wchar_t unichar)
{
	//check filter
	for (int i = 0; i < Q_strlen(m_FilterOut); i++)
	{
		if (unichar == m_FilterOut[i])
			return;
	}
	
	//passed filter check
	BaseClass::OnKeyTyped(unichar);
}

//---------------------------------------------------------------------------------
// Purpose: Called when a character is typed
//---------------------------------------------------------------------------------
void CFilteredTextEntry::SetFilter(const char* filter)
{
	m_FilterOut = filter ? filter : "";
}


//convar button


Color CConvarPageConvarButtonBase::m_DefaultBgColor = Color();
Color CConvarPageConvarButtonBase::m_DefaultFgColor = Color();

//---------------------------------------------------------------------------------
// Purpose: Constructor for the convar button
//---------------------------------------------------------------------------------
CConvarPageConvarButtonBase::CConvarPageConvarButtonBase(vgui::Panel* parent, const char* name, const char* text)
	: BaseClass(parent, name, text)
{
	//add the parent as a target for the OnCommand
	AddActionSignalTarget(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Selects the button
//---------------------------------------------------------------------------------
void CConvarPageConvarButtonBase::SetButtonSelected(bool selected)
{
	m_bIsSelected = selected;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if the button is selected or not
//---------------------------------------------------------------------------------
const bool CConvarPageConvarButtonBase::IsButtonSelected()
{
	return m_bIsSelected;
}

//---------------------------------------------------------------------------------
// Purpose: Paints the background
//---------------------------------------------------------------------------------
void CConvarPageConvarButtonBase::PaintBackground()
{
	//if the button is selected then set the bg color
	if (m_bIsSelected)
		SetBgColor(EFFECTS_PANEL_BG_SELECTED_COLOR);
	else
		SetBgColor(m_DefaultBgColor);

	BaseClass::PaintBackground();
}

//---------------------------------------------------------------------------------
// Purpose: Paints the forground
//---------------------------------------------------------------------------------
void CConvarPageConvarButtonBase::Paint()
{
	//if the button is selected then set the fg color
	if (m_bIsSelected)
		SetFgColor(EFFECTS_PANEL_FG_SELECTED_COLOR);
	else
		SetFgColor(m_DefaultFgColor);

	BaseClass::Paint();
}

//---------------------------------------------------------------------------------
// Purpose: Applies the scheme settings
//---------------------------------------------------------------------------------
void CConvarPageConvarButtonBase::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	//get the default colors
	m_DefaultBgColor = GetButtonArmedBgColor();
	m_DefaultFgColor = GetButtonArmedFgColor();
}


//convar list


//---------------------------------------------------------------------------------
// Purpose: Constructor for the convar list panel
//---------------------------------------------------------------------------------
CConvarPageConvarList::CConvarPageConvarList(vgui::Panel* parent, const char* name, const char* title)
	: BaseClass(parent, name)
{
	//create the title
	m_Title = new vgui::Label(this, "_Title", title);
	m_Title->SetBounds(0, 0, EFFECTS_PAGE_WIDTH - 20, CONVAR_LIST_PANEL_TITLE_HEIGHT);
	m_Title->SetContentAlignment(vgui::Label::a_center);
	
	//make the scroll bar
	m_ScrollBar = new vgui::ScrollBar(this, "_ScrollBar", true);
	m_ScrollBar->SetBounds(EFFECTS_PAGE_WIDTH - 20, 0, 20, 278);
	m_ScrollBar->SetRangeWindow(1);
	m_ScrollBar->AddActionSignalTarget(this);

	//set the convar page
	m_ConvarPage = dynamic_cast<CEffectsPanelConvarPage*>(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse wheel moved
//---------------------------------------------------------------------------------
void CConvarPageConvarList::OnMouseWheeled(int delta)
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
void CConvarPageConvarList::OnScrollBarSliderMoved()
{
	//get the pos
	int pos = m_ScrollBar->GetValue();

	//format the items
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//the item at pos - 1 could overlap with the "Convars:" title text. To fix this put the item at a stupid pos
		if (i == pos - 1)
			m_ButtonList[i]->SetPos(-100, -100);
		else
			m_ButtonList[i]->SetPos(CONVAR_LIST_PANEL_BUTTON_X_OFFSET, CONVAR_LIST_PANEL_TITLE_HEIGHT + (CONVAR_LIST_PANEL_BUTTON_HEIGHT * i) - (CONVAR_LIST_PANEL_BUTTON_HEIGHT * pos));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Adds a convar button
//---------------------------------------------------------------------------------
bool CConvarPageConvarList::AddConvar(const char* ConvarName, const char* ConvarValue, ConvarActiveType_e type)
{
	//see if we have a button with the same name
	if (HasConvar(ConvarName))
		return false;

	//check for the convar
	ConVar* var = cvar->FindVar(ConvarName);
	if (!var)
		return false;

	//get the bounds
	int x = CONVAR_LIST_PANEL_BUTTON_X_OFFSET;
	int y = CONVAR_LIST_PANEL_TITLE_HEIGHT + (CONVAR_LIST_PANEL_BUTTON_HEIGHT * m_ButtonList.Count());
	int w = GetWide() - x - (CONVAR_LIST_PANEL_BUTTON_X_OFFSET * 2) - CONVAR_LIST_PANEL_SCROLL_BAR_WIDTH;
	int h = CONVAR_LIST_PANEL_BUTTON_HEIGHT;

	//make a new button
	CConvarPageConvarButton* button = new CConvarPageConvarButton(this, "ConvarButton", CFmtStr("%s = %s", ConvarName, ConvarValue));
	button->SetBounds(x, y, w, h);
	button->SetCommand(CFmtStr(CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count()));

	//set the convar data
	Q_strncpy(button->m_ConvarData.name, ConvarName, sizeof(button->m_ConvarData.name));
	Q_strncpy(button->m_ConvarData.value, ConvarValue, sizeof(button->m_ConvarData.value));
	Q_strncpy(button->m_ConvarData.default, var->GetString(), sizeof(button->m_ConvarData.default));
	button->m_ConvarData.convar = var;
	button->m_ConvarData.type = type;

	//set the convar's value if we can
	if (ShouldConvarBeActive(dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer()), type))
		var->SetValue(ConvarValue);

	//select the button and de-select every other button
	button->SetButtonSelected(true);

	for (int i = 0; i < m_ButtonList.Count(); i++)
		m_ButtonList[i]->SetButtonSelected(false);

	//add the button to the list of buttons
	m_ButtonList.AddToTail(button);

	//select button
	OnCommand(CFmtStr(CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count() - 1));

	//check the list size
	if (m_ButtonList.Count() > CONVAR_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL)
	{
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetRange(0, m_ButtonList.Count() - CONVAR_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
		m_ScrollBar->SetValue(m_ButtonList.Count() - CONVAR_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
	}
	else
	{
		m_ScrollBar->SetRange(0, 0);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if this list has a convar
//---------------------------------------------------------------------------------
bool CConvarPageConvarList::HasConvar(const char* ConvarName)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		if (!Q_stricmp(button->m_ConvarData.name, ConvarName))
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Changes a convar's value
//---------------------------------------------------------------------------------
void CConvarPageConvarList::ChangeConvarValue(const char* ConvarName, const char* newvalue, ConvarActiveType_e type)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		if (!Q_stricmp(button->m_ConvarData.name, ConvarName))
		{
			//set the convar name data
			Q_strncpy(button->m_ConvarData.value, newvalue, sizeof(button->m_ConvarData.value));

			//set the convar's value
			button->m_ConvarData.type = type;

			//should we activate the convar?
			if (ShouldConvarBeActive(dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer()), button->m_ConvarData.type))
				button->m_ConvarData.convar->SetValue(newvalue);

			//set the button's text
			button->SetText(CFmtStr("%s = %s", ConvarName, newvalue));

			//select this button and de-select every other button
			for (int i = 0; i < m_ButtonList.Count(); i++)
			{
				//check for the button
				CConvarPageConvarButton* button = m_ButtonList[i];
				if (!button)
					continue;

				//select the button ONLY if the convar name matches the input
				button->SetButtonSelected(!Q_strcmp(button->m_ConvarData.name, ConvarName));
			}

			return;
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns if this list has a convar
//---------------------------------------------------------------------------------
bool CConvarPageConvarList::ChangeSelectedConvar(const char* newvalue, ConvarActiveType_e type)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		//change the value
		ChangeConvarValue(button->m_ConvarData.name, newvalue, type);
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Returns all of the convar data
//---------------------------------------------------------------------------------
void CConvarPageConvarList::GetConvarData(CUtlVector<ConvarButtonData_t*>& data)
{
	//go through each button data
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		data.AddToTail(&button->m_ConvarData);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Clears all of the convar data
//---------------------------------------------------------------------------------
void CConvarPageConvarList::ClearConvars()
{
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//revert the convar to it's default value
		button->m_ConvarData.convar->SetValue(button->m_ConvarData.default);
		
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
// Purpose: Removes the selected command
//---------------------------------------------------------------------------------
bool CConvarPageConvarList::RemoveSelectedConvar()
{
	int SelectedIndex = -1;
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		SelectedIndex = i;

		//revert the convar to it's default value
		button->m_ConvarData.convar->SetValue(button->m_ConvarData.default);

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
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//reset the command
		button->SetCommand(CFmtStr(CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", i));

		//reset the bounds
		button->SetPos(CONVAR_LIST_PANEL_BUTTON_X_OFFSET, CONVAR_LIST_PANEL_TITLE_HEIGHT + (CONVAR_LIST_PANEL_BUTTON_HEIGHT * i));
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
	OnCommand(CFmtStr(CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX "%d", SelectedIndex));

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Applies all the convar values to the convar
//---------------------------------------------------------------------------------
void CConvarPageConvarList::ApplyConvarValues()
{
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CConvarPageConvarButton* button = m_ButtonList[i];
		if (!button)
			continue;

		if (ShouldConvarBeActive(dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer()), button->m_ConvarData.type))
			button->m_ConvarData.convar->SetValue(button->m_ConvarData.value);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CConvarPageConvarList::OnCommand(const char* pszCommand)
{
	if (StringHasPrefix(pszCommand, CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX))
	{
		//get the index
		int index = Q_atoi(pszCommand + Q_strlen(CONVAR_LIST_PANEL_BUTTON_COMMAND_PREFIX));

		//select this button and de-select every other button
		for (int i = 0; i < m_ButtonList.Count(); i++)
		{
			//check for the button
			CConvarPageConvarButton* button = m_ButtonList[i];
			if (!button)
				continue;
			
			//select the button ONLY if i == index
			button->SetButtonSelected(i == index);

			//set the convar page text if i == index
			if (i == index)
			{
				m_ConvarPage->SetConvarText(button->m_ConvarData.name, button->m_ConvarData.value, button->m_ConvarData.type);

				//also set the convar value
				if (Q_strcmp(button->m_ConvarData.convar->GetString(), button->m_ConvarData.value) && ShouldConvarBeActive(dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer()), button->m_ConvarData.type))
				{
					button->m_ConvarData.convar->SetValue(button->m_ConvarData.value);
				}
			}
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//---------------------------------------------------------------------------------
// Purpose: Returns if the convar should be active
//---------------------------------------------------------------------------------
bool CConvarPageConvarList::ShouldConvarBeActive(CHL2_Player* pPlayer, ConvarActiveType_e type)
{
	//check for player
	if (!pPlayer)
		return false;

	//check for always draw
	if (((int)type & (int)ConvarActiveType_e::Active_Always))
		return true;

	//check for flashlight on
	if (((int)type & (int)ConvarActiveType_e::Active_WhenPlayerFlashlightOn) && pPlayer->IsEffectActive(EF_DIMLIGHT))
		return true;

	//check for flashlight off
	if (((int)type & (int)ConvarActiveType_e::Active_WhenPlayerFlashlightOff) && !pPlayer->IsEffectActive(EF_DIMLIGHT))
		return true;

	//moving?
	Vector vel = pPlayer->GetAbsVelocity();
	vel.z = 0;
	bool moving = vel.Length() >= 25;

	//check for walking
	if (((int)type & (int)ConvarActiveType_e::Active_WhenWalking) && moving && !pPlayer->IsSprinting())
		return true;

	//check for sprinting
	if (((int)type & (int)ConvarActiveType_e::Active_WhenSprinting) && moving && pPlayer->IsSprinting())
		return true;

	//check for health
	if (((int)type & (int)ConvarActiveType_e::Active_WhenHealthLow) && pPlayer->GetHealth() <= 20)
		return true;

	//check for under water
	if (((int)type & (int)ConvarActiveType_e::Active_WhenUnderWater) && pPlayer->GetWaterLevel() < WL_Eyes)
		return true;

	//check for crouching
	if (((int)type & (int)ConvarActiveType_e::Active_WhenCrouching) && (pPlayer->GetFlags() & FL_DUCKING))
		return true;

	//check for on ground
	if (((int)type & (int)ConvarActiveType_e::Active_WhenOnGround) && (pPlayer->GetFlags() & FL_ONGROUND))
		return true;

	//check for holding object
	if (((int)type & (int)ConvarActiveType_e::Active_WhenHoldingObject) && pPlayer->GetUseEntity())
		return true;

	//check for zooming
	if (((int)type & (int)ConvarActiveType_e::Active_WhenUsingSuitZoom) && pPlayer->IsZoomed())
		return true;

	//check for in air
	if (((int)type & (int)ConvarActiveType_e::Active_WhenInAir) && !(pPlayer->GetFlags() & FL_ONGROUND))
		return true;

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Called on think
//---------------------------------------------------------------------------------
void CConvarPageConvarList::OnTick()
{
	//get the player
	C_BaseHLPlayer* pPlayer = dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer());

	//paint all the overlays
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//get the data
		ConvarButtonData_t& data = m_ButtonList[i]->m_ConvarData;

		bool ConvarValueIsSet = !Q_strcmp(data.convar->GetString(), data.value);
		bool ShouldBeActive = ShouldConvarBeActive(pPlayer, data.type);

		//see if the overlay should draw
		if (ShouldBeActive && !ConvarValueIsSet)
			data.convar->SetValue(data.value);
		
		//see if the overlay shouldnt draw
		else if (!ShouldBeActive && ConvarValueIsSet)
			data.convar->SetValue(data.default);
	}
}

//convar list panel


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CConvarListPanel::CConvarListPanel(vgui::VPANEL parent, const char* name, CEffectsPanelConvarPage* ConvarPage)
	: BaseClass(nullptr, name), m_ConvarPage(ConvarPage)
{
	//MUST have m_ConvarPage
	if (!m_ConvarPage)
	{
		delete this;
		return;
	}

	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(false);

	//set the size, pos and title
	SetTitle("#Amod_EffectsPanel_ConvarPage_FindConvar_Title", true);
	SetSize(CONVAR_LIST_PANEL_WIDTH, CONVAR_LIST_PANEL_HEIGHT);
	MoveToCenterOfScreen();

	//create combo box's
	m_ConvarList = new vgui::ComboBox(this, "ConvarList", 20, true);
	m_ConvarList->SetBounds(5, 25, CONVAR_LIST_PANEL_WIDTH - 10, 20);
	m_ConvarList->AddActionSignalTarget(this);
	m_ConvarList->SetVisible(true);
	//make divider
	vgui::Divider* divider1 = new vgui::Divider(this, "Divider");
	divider1->SetBounds(-5, 48, CONVAR_LIST_PANEL_WIDTH + 10, 2);

	//create text
	vgui::Label* label1 = new vgui::Label(this, "FindConvar", "#Amod_EffectsPanel_ConvarPage_FindConvar_FindButton");
	label1->SetBounds(145, 51, 120, 20);

	//create text entry
	m_SearchText = new vgui::TextEntry(this, "SearchTextEntry");
	m_SearchText->SetBounds(5, 75, CONVAR_LIST_PANEL_WIDTH - 10, 20);
	m_SearchText->SetEnabled(true);
	m_SearchText->SetText("");

	//create search for button
	m_SearchButton = new vgui::Button(this, "SearchButton", "#Amod_EffectsPanel_ConvarPage_FindConvar_SearchForLabel");
	m_SearchButton->SetBounds(5, 100, CONVAR_LIST_PANEL_WIDTH - 10, 20);;
	m_SearchButton->SetEnabled(true);
	m_SearchButton->SetCommand(CONVAR_LIST_SEARCH_COMMAND);

	//make divider
	vgui::Divider* divider2 = new vgui::Divider(this, "Divider");
	divider2->SetBounds(-5, 124, CONVAR_LIST_PANEL_WIDTH + 10, 2);

	//create text
	vgui::Label* label2 = new vgui::Label(this, "ConvarButtons", "#Amod_EffectsPanel_ConvarPage_FindConvar_InsertLabel");
	label2->SetBounds(140, 127, 120, 20);

	//create convar insert button
	m_InsertButton = new vgui::Button(this, "InsertConvar", "#Amod_EffectsPanel_ConvarPage_FindConvar_InsertButton", this);
	m_InsertButton->SetBounds(5, 150, CONVAR_LIST_PANEL_WIDTH - 10, 20);
	m_InsertButton->SetCommand(CONVAR_LIST_INSERT_COMMAND);

	//initalize the convars combo box
	InitalizeConvars();
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for utl vector
//-----------------------------------------------------------------------------
static int VectorSortFunc(const char* const* p1, const char* const* p2)
{
	return Q_stricmp(*p1, *p2);
}

//-----------------------------------------------------------------------------
// Purpose: Initalizes the convar list
//-----------------------------------------------------------------------------
void CConvarListPanel::InitalizeConvars()
{
	//store the commands in this
	CUtlVector<const char*> commands;
	
	//get the first command
	ConCommandBase* first = cvar->GetCommands();

	//add all the convars
	while (first)
	{
		//if it is a convar then add it to commands
		if (!first->IsCommand())
			commands.AddToTail(first->GetName());

		//get the next command
		first = first->GetNext();
	}

	//sort the commands
	commands.Sort(VectorSortFunc);

	//add the commands
	for (int i = 0; i < commands.Count(); i++)
		m_ConvarList->AddItem(commands[i], nullptr);

	//activate item 0
	m_ConvarList->ActivateItem(0);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the convar page
//-----------------------------------------------------------------------------
void CConvarListPanel::SetConvarPage(CEffectsPanelConvarPage* ConvarPage)
{
	if (!ConvarPage)
		return;

	m_ConvarPage = ConvarPage;
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CConvarListPanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, CONVAR_LIST_SEARCH_COMMAND))
	{
		//get text
		char buf[512];
		m_SearchText->GetText(buf, sizeof(buf));
	
		//check for shift key
		bool shift = (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LSHIFT) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RSHIFT));
	
		//vector of texts
		CUtlVector<char*> ConvarNames;
	
		//get all the convars
		for (int i = 0; i < m_ConvarList->GetItemCount(); i++)
		{
			//insert
			char* tmpbuf = new char[202];
			m_ConvarList->GetItemText(i, tmpbuf, 202);
	
			ConvarNames.AddToTail(tmpbuf);
		}
	
		if (shift)
		{
			//start from current index - 1
			int start = m_ConvarList->GetMenu()->GetActiveItem() - 1;
	
			//look for convar with same name starting from the start first and going down
			for (int i = start; i >= 0; i--)
			{
				if (Q_stristr(ConvarNames[i], buf))
				{
					//select item
					m_ConvarList->GetMenu()->SetCurrentlyHighlightedItem(i);
					m_ConvarList->ActivateItem(i);
	
					//set text
					m_ConvarList->SetText(ConvarNames[i]);
	
					//delete all the convar names
					for (int i = 0; i < ConvarNames.Count(); i++)
						delete[] ConvarNames[i];
	
					return;
				}
			}
	
	
			//now cheeck from the ConvarNames to the start
			for (int i = ConvarNames.Count() - 1; i > start; i--)
			{
				if (Q_stristr(ConvarNames[i], buf))
				{
					//select item
					m_ConvarList->GetMenu()->SetCurrentlyHighlightedItem(i);
					m_ConvarList->ActivateItem(i);
	
					//set text
					m_ConvarList->SetText(ConvarNames[i]);
	
					//delete all the convar names
					for (int i = 0; i < ConvarNames.Count(); i++)
						delete[] ConvarNames[i];
	
					return;
				}
			}
		}
		else
		{
			//start from current index + 1
			int start = m_ConvarList->GetMenu()->GetActiveItem() + 1;
	
			//look for convar with same name starting from the start first
			for (int i = start; i < ConvarNames.Count(); i++)
			{
				if (Q_stristr(ConvarNames[i], buf))
				{
					//select item
					m_ConvarList->GetMenu()->SetCurrentlyHighlightedItem(i);
					m_ConvarList->ActivateItem(i);
	
					//set text
					m_ConvarList->SetText(ConvarNames[i]);
	
					//delete all the convar names
					for (int i = 0; i < ConvarNames.Count(); i++)
						delete[] ConvarNames[i];
	
					return;
				}
			}
	
	
			//now cheeck from 0 to the start
			for (int i = 0; i < start; i++)
			{
				if (Q_stristr(ConvarNames[i], buf))
				{
					//select item
					m_ConvarList->GetMenu()->SetCurrentlyHighlightedItem(i);
					m_ConvarList->ActivateItem(i);
	
					//set text
					m_ConvarList->SetText(ConvarNames[i]);
	
					//delete all the convar names
					for (int i = 0; i < ConvarNames.Count(); i++)
						delete[] ConvarNames[i];
	
					return;
				}
			}
		}
	
		//delete all the convar names
		for (int i = 0; i < ConvarNames.Count(); i++)
			delete[] ConvarNames[i];

		return;
	}
	else if (!Q_strcmp(pszCommand, CONVAR_LIST_INSERT_COMMAND))
	{
		//close this
		Close();
	
		//get the convar name
		char buf[512];
		m_ConvarList->GetText(buf, sizeof(buf));
	
		//call m_ConvarPage->SetConvarText
		m_ConvarPage->SetConvarText(buf);
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Called on close
//-----------------------------------------------------------------------------
void CConvarListPanel::OnClose()
{
	vgui::input()->SetAppModalSurface(0);
	BaseClass::OnClose();
}

//convar find panel singleton
CConvarListPanel* g_ConvarListPanel = nullptr;


//convar page


//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel convar page
//---------------------------------------------------------------------------------
CEffectsPanelConvarPage::CEffectsPanelConvarPage(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//create the convar list panel
	m_ConvarListPanel = new CConvarPageConvarList(this, "ConvarList", "#Amod_EffectsPanel_ConvarPage_ConvarListText");

	//make types combo box
	m_TypeComboBox = new vgui::ComboBox(this, "TypesComboBox", 14, false);
	m_TypeComboBox->SetMaximumCharCount(254);

	//add types
	for (int i = 0; i < SIZE_OF_ARRAY(g_ConvarActiveType); i++)
	{
		m_TypeComboBox->GetMenu()->AddCheckableMenuItem(g_ConvarActiveType[i], new KeyValues("SetText", "text", g_ConvarActiveType[i]), this, nullptr);
		m_TypeComboBox->GetMenu()->GetMenuItem(i)->SetCommand(CFmtStr(CONVAR_PAGE_OVERLAY_TYPE_PREFIX "%d", i));
		m_TypeComboBox->GetMenu()->GetMenuItem(i)->AddActionSignalTarget(this);
	}

	//set the text
	m_TypeComboBox->SetText("#Amod_EffectsPanel_ConvarPage_ActiveTypes");

	//create the name stuff
	m_ConvarNameText = new vgui::Label(this, "ConvarNameText", "#Amod_EffectsPanel_ConvarPage_ConvarName");
	m_ConvarNameTextEntry = new CFilteredTextEntry(this, "ConvarNameTextEntry", CONVAR_PAGE_NAME_FILTER);
	m_ConvarNameTextEntry->SetMaximumCharCount(55);
		
	//find convar button
	m_FindConvarButton = new vgui::Button(this, "FindConvar", "#Amod_EffectsPanel_ConvarPage_FindConvar", this, CONVAR_PAGE_FIND_COMMAND);

	//create the value stuff
	m_ConvarValueText = new vgui::Label(this, "ConvarValueText", "#Amod_EffectsPanel_ConvarPage_ConvarValue");
	m_ConvarValueTextEntry = new CFilteredTextEntry(this, "ConvarValueTextEntry", "");
	m_ConvarValueTextEntry->SetMaximumCharCount(200);

	//bottom buttons
	m_AddButton = new vgui::Button(this, "AddConvarButton", "#Amod_EffectsPanel_ConvarPage_AddConvar", this, CONVAR_PAGE_ADD_COMMAND);
	m_ChangeButton = new vgui::Button(this, "ChangeConvarButton", "#Amod_EffectsPanel_ConvarPage_UpdateConvar", this, CONVAR_PAGE_CHANGE_COMMAND);
	m_RemoveButton = new vgui::Button(this, "RemoveConvarButton", "#Amod_EffectsPanel_ConvarPage_RemoveConvar", this, CONVAR_PAGE_REMOVE_COMMAND);

	//add the tool tips
	ADD_TOOLTIP(m_ConvarListPanel, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_ConvarList", true);
	ADD_TOOLTIP(m_TypeComboBox, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_ActiveTypes", true);
	ADD_TOOLTIP(m_ConvarNameTextEntry, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_ConvarNameTextEntry", true);
	ADD_TOOLTIP(m_FindConvarButton, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_FindConvarButton", true);
	ADD_TOOLTIP(m_ConvarValueTextEntry, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_ConvarValueTextEntry", true);
	ADD_TOOLTIP(m_AddButton, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_AddButton", true);
	ADD_TOOLTIP(m_ChangeButton, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_UpdateButton", true);
	ADD_TOOLTIP(m_RemoveButton, 100, "#Amod_EffectsPanel_ConvarPage_Tooltip_RemoveButton", true);

	//reset the things to the defaults
	ResetEffects();

	//set the bounds for each item
	PerformLayout();
}

//---------------------------------------------------------------------------------
// Purpose: Resets all the effects on this page
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::ResetEffects()
{
	//clear the convars
	m_ConvarListPanel->ClearConvars();

	//reset mode
	m_TypeComboBox->SetText("#Amod_EffectsPanel_ConvarPage_ActiveTypes");

	//only select menu item 1 (Always active)
	for (int i = 0; i < m_TypeComboBox->GetItemCount(); i++)
		m_TypeComboBox->GetMenu()->SetMenuItemChecked(i, i == 0);

	//reset the text entries
	m_ConvarValueTextEntry->SetText("");
	m_ConvarNameTextEntry->SetText("");
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::ReadFromFile(KeyValues* keyvalues, bool reset)
{
	//reset everything
	if (reset)
		ResetEffects();

	//find the view KeyValues
	KeyValues* convars = keyvalues->FindKey("ConVars");
	if (!convars)
		return;

	//add each convar
	FOR_EACH_TRUE_SUBKEY(convars, subkey)
	{
		m_ConvarListPanel->AddConvar(subkey->GetString("Convar"), subkey->GetString("Value"), (ConvarActiveType_e)subkey->GetInt("Type"));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::WriteToFile(KeyValues* keyvalues)
{
	//create a new KeyValues*
	KeyValues* convars = new KeyValues("ConVars");

	//get the data
	CUtlVector<ConvarButtonData_t*> data;
	m_ConvarListPanel->GetConvarData(data);

	//add the keys
	for (int i = 0; i < data.Count(); i++)
	{
		//create a new subkey
		KeyValues* subkey = new KeyValues("Convar");

		subkey->SetString("Convar", data[i]->name);
		subkey->SetString("Value", data[i]->value);
		subkey->SetInt("Type", (int)data[i]->type);

		//add subkey
		convars->AddSubKey(subkey);
	}

	//add the convars settings
	keyvalues->AddSubKey(convars);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the convar text entrys
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::SetConvarText(const char* name, const char* value, ConvarActiveType_e type)
{
	m_ConvarNameTextEntry->SetText(name);
	
	if (value)
		m_ConvarValueTextEntry->SetText(value);

	//set the selected stuff
	for (int i = 0; i < m_TypeComboBox->GetItemCount(); i++)
	{
		//see if that component is active
		if ((int)type & (1 << i))
			m_TypeComboBox->GetMenu()->SetMenuItemChecked(i, true);
		else
			m_TypeComboBox->GetMenu()->SetMenuItemChecked(i, false);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called on map load
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::OnMapLoad()
{
	//sometimes when a map loads some convars get changed. Do this to make them not change
	m_ConvarListPanel->ApplyConvarValues();
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::OnCommand(const char* pszCommand)
{
	//check for CONVAR_PAGE_ADD_COMMAND
	if (!Q_strcmp(pszCommand, CONVAR_PAGE_ADD_COMMAND))
	{
		//get the texts
		char name[56];
		char value[202];

		m_ConvarNameTextEntry->GetText(name, sizeof(name));
		m_ConvarValueTextEntry->GetText(value, sizeof(value));
	
		//if the convar exists then prompt the user to see if they want to change
		//the current value
		if (m_ConvarListPanel->HasConvar(name))
		{
			//show queary box
			vgui::QueryBox* error = new vgui::QueryBox("Convar Already Exists", CFmtStr("Convar \"%s\" Already exists. Would you like to change the value?!", name));
			error->SetOKButtonText("Yes");
			error->SetOKCommand(new KeyValues("Command", "command", CONVAR_PAGE_SHOULD_CHANGE_COMMAND));
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}

		//get the type
		int drawtype = NULL;
		for (int i = 0; i < m_TypeComboBox->GetItemCount(); i++)
		{
			if (m_TypeComboBox->GetMenu()->IsChecked(i))
				drawtype |= (1 << i);
		}

		//add the convar
		if (!m_ConvarListPanel->AddConvar(name, value, (ConvarActiveType_e)drawtype))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr("Error: Convar \"%s\" Doesnt exist!", name));
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for replace command
	else if (!Q_strcmp(pszCommand, CONVAR_PAGE_REMOVE_COMMAND))
	{
		//remove the selected convar
		if (!m_ConvarListPanel->RemoveSelectedConvar())
		{
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Select a convar", "No convar selected!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for replace command
	else if (!Q_strcmp(pszCommand, CONVAR_PAGE_CHANGE_COMMAND))
	{
		//get the value
		char value[202];
		m_ConvarValueTextEntry->GetText(value, sizeof(value));

		//get the draw type
		int drawtype = NULL;
		for (int i = 0; i < m_TypeComboBox->GetItemCount(); i++)
		{
			if (m_TypeComboBox->GetMenu()->IsChecked(i))
				drawtype |= (1 << i);
		}

		//remove the selected convar
		if (!m_ConvarListPanel->ChangeSelectedConvar(value, (ConvarActiveType_e)drawtype))
		{
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Select a convar", "No convar selected!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}

	//check for replace command
	else if (!Q_strcmp(pszCommand, CONVAR_PAGE_SHOULD_CHANGE_COMMAND))
	{
		//get the texts
		char name[56];
		char value[202];

		m_ConvarNameTextEntry->GetText(name, sizeof(name));
		m_ConvarValueTextEntry->GetText(value, sizeof(value));

		//get the draw type
		int drawtype = NULL;
		for (int i = 0; i < m_TypeComboBox->GetItemCount(); i++)
		{
			if (m_TypeComboBox->GetMenu()->IsChecked(i))
				drawtype |= (1 << i);
		}

		//change the value
		m_ConvarListPanel->ChangeConvarValue(name, value, (ConvarActiveType_e)drawtype);
	}

	//check for OVERLAY_PAGE_OVERLAY_TYPE_PREFIX
	else if (StringHasPrefix(pszCommand, CONVAR_PAGE_OVERLAY_TYPE_PREFIX))
	{
		//get index
		//int index = Q_atoi(pszCommand + Q_strlen(CONVAR_PAGE_OVERLAY_TYPE_PREFIX));

		//set selected state of menu item
		//m_TypeComboBox->GetMenu()->SetMenuItemChecked(index, m_TypeComboBox->GetMenu()->IsChecked(index));

		//m_TypeComboBox->SetText("Active Types:");
	}

	//check for find convar button
	else if (!Q_strcmp(pszCommand, CONVAR_PAGE_FIND_COMMAND))
	{
		//check for the convar list panel
		if (!g_ConvarListPanel)
			g_ConvarListPanel = new CConvarListPanel(GetParent()->GetVPanel(), "ConvarListPanel", this);

		g_ConvarListPanel->SetConvarPage(this);
		g_ConvarListPanel->SetVisible(true);
		g_ConvarListPanel->MoveToFront();
		g_ConvarListPanel->MoveToCenterOfScreen();
		g_ConvarListPanel->RequestFocus();

		//set the surface modal to the panel
		vgui::input()->SetAppModalSurface(g_ConvarListPanel->GetVPanel());
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
void CEffectsPanelConvarPage::OnTick()
{
	//call m_ConvarListPanel on tick
	m_ConvarListPanel->OnTick();
}

//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelConvarPage::PerformLayout()
{
	//set the convar list
	m_ConvarListPanel->SetBounds(5, 10, EFFECTS_PAGE_WIDTH, 280);

	//set type combo box
	m_TypeComboBox->SetBounds(5, 294, EFFECTS_PAGE_WIDTH, 22);

	//name stuff
	m_ConvarNameText->SetBounds(8, 315, 256, 20);
	m_ConvarNameTextEntry->SetBounds(5, 335, 171, 22);
	m_FindConvarButton->SetBounds(180, 334, 80, 22);

	//value stuff
	m_ConvarValueText->SetBounds(275, 315, 256, 20);
	m_ConvarValueTextEntry->SetBounds(272, 335, 256, 22);

	//bottom buttons
	m_AddButton->SetBounds(5, 362, 171, 22);
	m_ChangeButton->SetBounds(180, 362, 171, 22);
	m_RemoveButton->SetBounds(355, 362, 171, 22);

	BaseClass::PerformLayout();
}
