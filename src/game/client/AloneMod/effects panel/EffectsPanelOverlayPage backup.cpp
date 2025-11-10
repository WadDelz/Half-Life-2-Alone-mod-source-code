#include "cbase.h"
#include "EffectsPanelOverlayPage.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/Tooltip.h"
#include "c_soundscape.h"
#include "filesystem.h"
#include "fmtstr.h"

//list of effect pages. This is so i can loop through each effect page and call page->PerformScreenOverlay();
static CUtlVector<CEffectsPanelOverlayPage*> g_EffectOverlayPages;

//draws all of the overlays
void Amod_EffectsPanelPaintOverlays(int x, int y, int w, int h)
{
	for (int i = 0; i < g_EffectOverlayPages.Count(); i++)
		g_EffectOverlayPages[i]->PerformScreenOverlay(x, y, w, h);
}


//overlay list


//---------------------------------------------------------------------------------
// Purpose: Constructor for the overlay list panel
//---------------------------------------------------------------------------------
COverlayPageList::COverlayPageList(vgui::Panel* parent, const char* name, const char* title)
	: BaseClass(parent, name)
{
	//create the title
	m_Title = new vgui::Label(this, "_Title", title);
	m_Title->SetBounds(0, 0, EFFECTS_PAGE_WIDTH - 20, OVERLAY_LIST_PANEL_TITLE_HEIGHT);
	m_Title->SetContentAlignment(vgui::Label::a_center);

	//make the scroll bar
	m_ScrollBar = new vgui::ScrollBar(this, "_ScrollBar", true);
	m_ScrollBar->SetBounds(EFFECTS_PAGE_WIDTH - 20, 0, 20, 298);
	m_ScrollBar->SetRangeWindow(1);
	m_ScrollBar->AddActionSignalTarget(this);

	//set the overlay page
	m_OverlayPage = dynamic_cast<CEffectsPanelOverlayPage*>(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse wheel moved
//---------------------------------------------------------------------------------
void COverlayPageList::OnMouseWheeled(int delta)
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
void COverlayPageList::OnScrollBarSliderMoved()
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
			m_ButtonList[i]->SetPos(OVERLAY_LIST_PANEL_BUTTON_X_OFFSET, OVERLAY_LIST_PANEL_TITLE_HEIGHT + (OVERLAY_LIST_PANEL_BUTTON_HEIGHT * i) - (OVERLAY_LIST_PANEL_BUTTON_HEIGHT * pos));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Adds an overlay button
//---------------------------------------------------------------------------------
bool COverlayPageList::AddOverlay(const char* OverlayName, int ROverride, int GOverride, int BOverride, int AOverride, OverlayDrawType_e DrawType)
{
	//see if we have a button with the same name
	if (HasOverlay(OverlayName))
		return false;

	//look for the material
	IMaterial* material = materials->FindMaterial(OverlayName, TEXTURE_GROUP_VGUI);
	if (!material || material->IsErrorMaterial())
		return false;

	//get the bounds
	int x = OVERLAY_LIST_PANEL_BUTTON_X_OFFSET;
	int y = OVERLAY_LIST_PANEL_TITLE_HEIGHT + (OVERLAY_LIST_PANEL_BUTTON_HEIGHT * m_ButtonList.Count());
	int w = GetWide() - x - (OVERLAY_LIST_PANEL_BUTTON_X_OFFSET * 2) - OVERLAY_LIST_PANEL_SCROLL_BAR_WIDTH;
	int h = OVERLAY_LIST_PANEL_BUTTON_HEIGHT;

	//make a new button
	COverlayPageButton* button = new COverlayPageButton(this, "OverlayButton", OverlayName);
	button->SetBounds(x, y, w, h);
	button->SetCommand(CFmtStr(OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count()));

	//set the overlay data
	Q_strncpy(button->m_OverlayData.name, OverlayName, SIZE_OF_ARRAY(button->m_OverlayData.name));
	button->m_OverlayData.material.Init(material);
	button->m_OverlayData.material->IncrementReferenceCount();
	button->m_OverlayData.r = ROverride;
	button->m_OverlayData.g = GOverride;
	button->m_OverlayData.b = BOverride;
	button->m_OverlayData.a = AOverride;
	button->m_OverlayData.drawtype = DrawType;

	//add the button to the list of buttons
	m_ButtonList.AddToTail(button);

	//select this
	OnCommand(CFmtStr(OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count()-1));

	//check the list size
	if (m_ButtonList.Count() > OVERLAY_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL)
	{
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetRange(0, m_ButtonList.Count() - OVERLAY_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
		m_ScrollBar->SetValue(m_ButtonList.Count() - OVERLAY_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
	}
	else
	{
		m_ScrollBar->SetRange(0, 0);
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if this list has an overlay
//---------------------------------------------------------------------------------
bool COverlayPageList::HasOverlay(const char* OverlayName)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		if (!Q_stricmp(button->m_OverlayData.name, OverlayName))
			return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Changes an overlay button's value
//---------------------------------------------------------------------------------
bool COverlayPageList::ChangeOverlay(const char* OldOverlayName, const char* NewOverlayName, int r, int g, int b, int a, OverlayDrawType_e type)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		if (!Q_stricmp(button->m_OverlayData.name, OldOverlayName))
		{
			//check for material
			IMaterial* material = materials->FindMaterial(NewOverlayName, TEXTURE_GROUP_VGUI);
			if (!material || material->IsErrorMaterial())
				return false;

			//set new material
			button->m_OverlayData.material.Shutdown();
			button->m_OverlayData.material.Init(material);
			button->m_OverlayData.material->IncrementReferenceCount();
			button->m_OverlayData.r = r;
			button->m_OverlayData.g = g;
			button->m_OverlayData.b = b;
			button->m_OverlayData.a = a;
			button->m_OverlayData.drawtype = type;

			//set the overlay name data
			Q_strncpy(button->m_OverlayData.name, NewOverlayName, sizeof(button->m_OverlayData.name));;
	
			//set the button's text
			button->SetText(NewOverlayName);
	
			//select this button and de-select every other button
			for (int i = 0; i < m_ButtonList.Count(); i++)
			{
				//check for the button
				COverlayPageButton* button = m_ButtonList[i];
				if (!button)
					continue;
	
				//select the button ONLY if the material name matches the new name
				button->SetButtonSelected(!Q_strcmp(button->m_OverlayData.name, NewOverlayName));
			}
	
			return true;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Moves the selected overlay up/down
//---------------------------------------------------------------------------------
void COverlayPageList::MoveSelectedOverlay(bool up)
{
	int selectedIndex = -1;

	//find the selected button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		if (button->IsButtonSelected())
		{
			selectedIndex = i;
			break;
		}
	}

	//no button selected
	if (selectedIndex == -1)
		return;

	//determine target index
	int targetIndex = up ? selectedIndex - 1 : selectedIndex + 1;

	//bounds check
	if (targetIndex < 0 || targetIndex >= m_ButtonList.Count())
		return;

	//swap buttons in the list
	COverlayPageButton* temp = m_ButtonList[selectedIndex];
	m_ButtonList[selectedIndex] = m_ButtonList[targetIndex];
	m_ButtonList[targetIndex] = temp;

	//hack to reposition buttons
	OnScrollBarSliderMoved();
}

//---------------------------------------------------------------------------------
// Purpose: Changes a selected overlay
//---------------------------------------------------------------------------------
bool COverlayPageList::ChangeSelectedOverlay(const char* newvalue, int r, int g, int b, int a, OverlayDrawType_e type)
{
	//check each button
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		//check the selected state
		if (!button->IsButtonSelected())
			continue;
	
		//see if the name matches. If not check for button with same name
		if (Q_strcmp(button->m_OverlayData.name, newvalue) && HasOverlay(newvalue))
			return false;

		//change the value
		return ChangeOverlay(button->m_OverlayData.name, newvalue, r, g, b, a, type);
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Returns all of the overlay data
//---------------------------------------------------------------------------------
void COverlayPageList::GetOverlayData(CUtlVector<OverlayButtonData_t*>& data)
{
	//go through each button data
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		data.AddToTail(&button->m_OverlayData);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Clears all of the overlays
//---------------------------------------------------------------------------------
void COverlayPageList::ClearOverlays()
{
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		//check the material
		//if (((IMaterial*)button->m_OverlayData.material))
		//	button->m_OverlayData.material->DecrementReferenceCount();

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
// Purpose: Removes the selected overlay
//---------------------------------------------------------------------------------
bool COverlayPageList::RemoveSelectedOverlay()
{
	int SelectedIndex = -1;
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		//check the selected state
		if (!button->IsButtonSelected())
			continue;
	
		SelectedIndex = i;
	
		//decrement ref count
		//if (((IMaterial*)button->m_OverlayData.material))
		//	button->m_OverlayData.material->DecrementReferenceCount();
	
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
		COverlayPageButton* button = m_ButtonList[i];
		if (!button)
			continue;
	
		//reset the command
		button->SetCommand(CFmtStr(OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", i));
	
		//reset the bounds
		button->SetPos(OVERLAY_LIST_PANEL_BUTTON_X_OFFSET, OVERLAY_LIST_PANEL_TITLE_HEIGHT + (OVERLAY_LIST_PANEL_BUTTON_HEIGHT * i));
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
	OnCommand(CFmtStr(OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX "%d", SelectedIndex));
	
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if an overlay should draw
//---------------------------------------------------------------------------------
bool COverlayPageList::ShouldOverlayDraw(C_BaseHLPlayer *pPlayer, OverlayDrawType_e type)
{
	//check for player
	if (!pPlayer)
		return true;
	
	//check for flashlight on
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenPlayerFlashlightOn) && !pPlayer->IsEffectActive(EF_DIMLIGHT))
		return false;

	//check for flashlight off
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenPlayerFlashlightOff) && pPlayer->IsEffectActive(EF_DIMLIGHT))
		return false;
	
	//moving?
	bool moving = pPlayer->GetAbsVelocity().Length() >= 35;

	//check for walking
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenWalking) && moving && !pPlayer->IsSprinting())
		return false;
	
	//check for sprinting
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenSprinting) && moving && pPlayer->IsSprinting())
		return false;
	
	//check for moving
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenMoving) && moving)
		return false;

	//check for health
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenHealthLow) && pPlayer->GetHealth() <= 20)
		return false;
	
	//check for under water
	if (((int)type == (int)OverlayDrawType_e::Draw_WhenHealthUnderWater) && pPlayer->GetWaterLevel() < WL_Eyes)
		return false;
	
	//should draw
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Paints all the overlays
//---------------------------------------------------------------------------------
void COverlayPageList::PaintOverlays(int x, int y, int w, int h)
{
	//get the player
	C_BaseHLPlayer* pPlayer = dynamic_cast<C_BaseHLPlayer*>(CBasePlayer::GetLocalPlayer());

	//paint all the overlays
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//get the data
		OverlayButtonData_t& data = m_ButtonList[i]->m_OverlayData;

		//see if the overlay should draw
		if (ShouldOverlayDraw(pPlayer, data.drawtype))
			Amod_PerformScreenOverlay(data.material, x, y, w, h, data.r, data.g, data.b, data.a);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void COverlayPageList::OnCommand(const char* pszCommand)
{
	if (StringHasPrefix(pszCommand, OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX))
	{
		//get the index
		int index = Q_atoi(pszCommand + Q_strlen(OVERLAY_LIST_PANEL_BUTTON_COMMAND_PREFIX));

		//select this button and de-select every other button
		for (int i = 0; i < m_ButtonList.Count(); i++)
		{
			//check for the button
			COverlayPageButton* button = m_ButtonList[i];
			if (!button)
				continue;

			//select the button ONLY if i == index
			button->SetButtonSelected(i == index);

			//set the overlay page text if i == index
			if (i == index)
				m_OverlayPage->SetOverlayText(button->m_OverlayData);
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//---------------------------------------------------------------------------------
// Purpose: Called on key press
//---------------------------------------------------------------------------------
void COverlayPageList::OnKeyCodePressed(vgui::KeyCode code)
{
	//check for shift and arrow key
	if (!vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LSHIFT) && !vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RSHIFT))
		return;

	//move selected button up/down
	if (code == vgui::KeyCode::KEY_DOWN)
		MoveSelectedOverlay(false);
	else if (code == vgui::KeyCode::KEY_UP)
		MoveSelectedOverlay(true);
}

//overlay page



//all directories to populate the overlay page combo box with
static const char* gs_ComboBoxDirectories[] = {
	"materials/effects",
	"materials/overlays",
	"materials/skybox",
};

//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel overlay page
//---------------------------------------------------------------------------------
CEffectsPanelOverlayPage::CEffectsPanelOverlayPage(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//make the overlay list
	m_OverlayList = new COverlayPageList(this, "OverlayList", "Overlays:");
	m_OverlayList->GetTooltip()->SetEnabled(true);
	m_OverlayList->GetTooltip()->SetTooltipFormatToMultiLine();
	m_OverlayList->GetTooltip()->SetTooltipDelay(100);
	m_OverlayList->GetTooltip()->SetText("Hold shift and press the up/down arrow keys when an overlay is selected to change the render order of the overlays!");

	//make overlay text entry
	m_OverlayTextEntry = new vgui::ComboBox(this, "OverlayTextEntry", 20, true);
	m_OverlayTextEntry->SetMaximumCharCount(254);

	//recursively go through each .vmt file in the gs_ComboBoxDirectories[i] dir and add the .vmts to the text entry
	for (int i = 0; i < SIZE_OF_ARRAY(gs_ComboBoxDirectories); i++)
		AddVMTEffectsRecursive(m_OverlayTextEntry, gs_ComboBoxDirectories[i]);

	//make types combo box
	m_TypeComboBox = new vgui::ComboBox(this, "TypesComboBox", 10, false);
	m_TypeComboBox->SetMaximumCharCount(254);

	//add types
	for (int i = 0; i < SIZE_OF_ARRAY(g_OverlayDrawStrings); i++)
		m_TypeComboBox->AddItem(g_OverlayDrawStrings[i], nullptr);

	m_TypeComboBox->ActivateItem(0);

	//make texts and sliders
	m_RedText = new vgui::Label(this, "RedTextLabel", "Red Color = -1");
	m_GreenText = new vgui::Label(this, "GreenTextLabel", "Green Color = -1");
	m_BlueText = new vgui::Label(this, "BlueTextLabel", "Blue Color = -1");
	m_AlphaText = new vgui::Label(this, "AlphaTextLabel", "Alpha = -1");
	
	m_RedSlider = new WheelSlider(this, "RedSlider");
	m_GreenSlider = new WheelSlider(this, "GreenSlider");
	m_BlueSlider = new WheelSlider(this, "BlueSlider");
	m_AlphaSlider = new WheelSlider(this, "AlphaSlider");

	m_RedSlider->SetRange(0, 255);
	m_GreenSlider->SetRange(0, 255);
	m_BlueSlider->SetRange(0, 255);
	m_AlphaSlider->SetRange(0, 255);

	//make buttons
	m_AddButton = new vgui::Button(this, "AddButton", "Add Overlay", this, ADD_OVERLAY_BUTTON_COMMAWND);
	m_ChangeOverlay = new vgui::Button(this, "ChangeButton", "Change Selected Overlay", this, CHANGE_OVERLAY_BUTTON_COMMAWND);
	m_RemoveOverlay = new vgui::Button(this, "RemoveOverlay", "Remove Selected Overlay", this, REMOVE_OVERLAY_BUTTON_COMMAWND);

	//reset the things to the defaults
	ResetEffects();

	//set the bounds for each item
	PerformLayout();

	//add this to the overlay pages list
	g_EffectOverlayPages.AddToTail(this);
}

//---------------------------------------------------------------------------------
// Purpose: destructor
//---------------------------------------------------------------------------------
CEffectsPanelOverlayPage::~CEffectsPanelOverlayPage()
{
	//clear the overlays
	m_OverlayList->ClearOverlays();

	//remove this from the overlay pages list
	g_EffectOverlayPages.FindAndRemove(this);
}

//---------------------------------------------------------------------------------
// Purpose: Populates the overlay text entry with all the files from in a directory
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::AddVMTEffectsRecursive(vgui::ComboBox* combo, const char* directory)
{
	// Build search path for .vmt files
	char searchPath[MAX_PATH];
	Q_snprintf(searchPath, sizeof(searchPath), "%s/*.vmt", directory);

	FileFindHandle_t findHandle;
	const char* fileName = g_pFullFileSystem->FindFirstEx(searchPath, nullptr, &findHandle);

	// Add .vmt files in this directory
	while (fileName)
	{
		if (!g_pFullFileSystem->FindIsDirectory(findHandle))
		{
			// Trim "materials/" prefix so entries look like "effects/...vmt"
			const char* relative = directory;
			if (!Q_strnicmp(directory, "materials/", 10))
				relative = directory + 10;

			//get the entry
			char entry[MAX_PATH];
			Q_snprintf(entry, sizeof(entry), "%s/%s", relative, fileName);
			
			//strip the file extention
			Q_StripExtension(entry, entry, sizeof(entry));

			//add the item
			combo->AddItem(entry, NULL);
		}

		//get the next item
		fileName = g_pFullFileSystem->FindNext(findHandle);
	}

	g_pFullFileSystem->FindClose(findHandle);

	// Now find subdirectories and recurse into them
	char dirSearch[MAX_PATH];
	Q_snprintf(dirSearch, sizeof(dirSearch), "%s/*", directory);

	FileFindHandle_t dirHandle;
	const char* dirName = g_pFullFileSystem->FindFirstEx(dirSearch, nullptr, &dirHandle);

	while (dirName)
	{
		if (g_pFullFileSystem->FindIsDirectory(dirHandle) && Q_stricmp(dirName, ".") && Q_stricmp(dirName, ".."))
		{
			char subDir[MAX_PATH];
			Q_snprintf(subDir, sizeof(subDir), "%s/%s", directory, dirName);
			AddVMTEffectsRecursive(combo, subDir);
		}
		dirName = g_pFullFileSystem->FindNext(dirHandle);
	}

	g_pFullFileSystem->FindClose(dirHandle);
}

//---------------------------------------------------------------------------------
// Purpose: Resets all the overlays on this page
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::ResetEffects()
{
	//reset the overlay list
	m_OverlayList->ClearOverlays();

	//reset draw mode
	m_TypeComboBox->ActivateItem(0);

	//reset the text entry
	m_OverlayTextEntry->SetText("");

	//reset the sliders
	m_RedSlider->SetValue(255);
	m_GreenSlider->SetValue(255);
	m_BlueSlider->SetValue(255);
	m_AlphaSlider->SetValue(255);
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::ReadFromFile(KeyValues* keyvalues)
{
	//clear everything
	ResetEffects();

	//find the view KeyValues
	KeyValues* overlays = keyvalues->FindKey("Overlays");
	if (!overlays)
		return;

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(overlays, subkey)
	{
		//add the overlay
		m_OverlayList->AddOverlay(subkey->GetString("OverlayName"), subkey->GetInt("Red", 255), subkey->GetInt("Green", 255), subkey->GetInt("Blue", 255), subkey->GetInt("Alpha", 255), (OverlayDrawType_e)subkey->GetInt("DrawType", (int)OverlayDrawType_e::Draw_Always));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::WriteToFile(KeyValues* keyvalues)
{
	//create a new KeyValues*
	KeyValues* overlays = new KeyValues("Overlays");
	
	CUtlVector<OverlayButtonData_t*> data;
	m_OverlayList->GetOverlayData(data);

	//go through each material
	for (int i = 0; i < data.Count(); i++)
	{
		//create a new subkey. This will store the overlay name, x, y, w, and h
		KeyValues* newoverlay = new KeyValues("Overlay");

		//set effect name
		newoverlay->SetString("OverlayName", data[i]->name);
		newoverlay->SetInt("Red", data[i]->r);
		newoverlay->SetInt("Green", data[i]->g);
		newoverlay->SetInt("Blue", data[i]->b);
		newoverlay->SetInt("Alpha", data[i]->a);
		newoverlay->SetInt("DrawType", (int)data[i]->drawtype);

		//add to overlays
		overlays->AddSubKey(newoverlay);
	}

	//add the overlays to the base subkey
	keyvalues->AddSubKey(overlays);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the overlay text's
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::SetOverlayText(OverlayButtonData_t& newtext)
{
	m_OverlayTextEntry->SetText(newtext.name);
	m_TypeComboBox->ActivateItem((int)newtext.drawtype);
	m_RedSlider->SetValue(newtext.r);
	m_GreenSlider->SetValue(newtext.g);
	m_BlueSlider->SetValue(newtext.b);
	m_AlphaSlider->SetValue(newtext.a);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::OnCommand(const char* pszCommand)
{
	//check for add command
	if (!Q_strcmp(pszCommand, ADD_OVERLAY_BUTTON_COMMAWND))
	{
		//get the text
		char buf[255];
		m_OverlayTextEntry->GetText(buf, sizeof(buf));

		//see if the overlay exists
		if (m_OverlayList->HasOverlay(buf))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr("Error: Overlay \"%s\" Is already added!", buf));
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}

		//see if we add the overlay
		if (!m_OverlayList->AddOverlay(buf, m_RedSlider->GetValue(), m_GreenSlider->GetValue(), m_BlueSlider->GetValue(), m_AlphaSlider->GetValue(), (OverlayDrawType_e)m_TypeComboBox->GetActiveItem()))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", CFmtStr("Error: Material \"%s\" is an error material!", buf));
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
		}
	}
	
	//check for change command
	else if (!Q_strcmp(pszCommand, CHANGE_OVERLAY_BUTTON_COMMAWND))
	{
		//get the text
		char buf[255];
		m_OverlayTextEntry->GetText(buf, sizeof(buf));

		//see if the overlay is selected/if we can set the material
		if (!m_OverlayList->ChangeSelectedOverlay(buf, m_RedSlider->GetValue(), m_GreenSlider->GetValue(), m_BlueSlider->GetValue(), m_AlphaSlider->GetValue(), (OverlayDrawType_e)m_TypeComboBox->GetActiveItem()))
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No overlay selected OR Got an invalid or already existing material!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}
	}
	
	//check for remove command
	else if (!Q_strcmp(pszCommand, REMOVE_OVERLAY_BUTTON_COMMAWND))
	{
		//see if the overlay is selected
		if (!m_OverlayList->RemoveSelectedOverlay())
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No overlay selected!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}
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
void CEffectsPanelOverlayPage::OnTick()
{
	m_RedText->SetText(CFmtStr("Red Color = %d", m_RedSlider->GetValue()));
	m_GreenText->SetText(CFmtStr("Green Color = %d", m_GreenSlider->GetValue()));
	m_BlueText->SetText(CFmtStr("Blue Color = %d", m_BlueSlider->GetValue()));
	m_AlphaText->SetText(CFmtStr("Alpha = %d", m_AlphaSlider->GetValue()));
}

//---------------------------------------------------------------------------------
// Purpose: Performs all of the screen overlays
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::PerformScreenOverlay(int x, int y, int w, int h)
{
	m_OverlayList->PaintOverlays(x, y, w, h);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelOverlayPage::PerformLayout()
{
	//set the overlay list
	m_OverlayList->SetBounds(5, 10, EFFECTS_PAGE_WIDTH, 275);

	//half page width
	int halfWidth = (EFFECTS_PAGE_WIDTH - 10) / 2;

	//set text entry (left)
	m_OverlayTextEntry->SetBounds(5, 290, halfWidth, 22);

	//set type combo box (right)
	m_TypeComboBox->SetBounds(10 + halfWidth, 290, halfWidth, 22);

	//sliders and text's
	m_RedText->SetBounds(5, 312, 131, 20);
	m_GreenText->SetBounds(136, 312, 131, 20);
	m_BlueText->SetBounds(267, 312, 131, 20);
	m_AlphaText->SetBounds(398, 312, 131, 20);
	
	m_RedSlider->SetBounds(5, 332, 131, 20);
	m_GreenSlider->SetBounds(136, 332, 131, 20);
	m_BlueSlider->SetBounds(267, 332, 131, 20);
	m_AlphaSlider->SetBounds(398, 332, 131, 20);

	//buttons
	m_AddButton->SetBounds(5, 360, 170, 22);
	m_ChangeOverlay->SetBounds(182, 360, 170, 22);
	m_RemoveOverlay->SetBounds(359, 360, 170, 22);
}