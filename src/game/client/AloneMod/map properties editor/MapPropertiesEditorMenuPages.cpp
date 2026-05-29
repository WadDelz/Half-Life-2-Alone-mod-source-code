#include "cbase.h"
#include "MapPropertiesEditorMenuPages.h"
#include "MapPropertiesEditorMenuPanel.h"
#include "../IOptionsPanel.h"
#include <vgui_controls/PropertySheet.h>

//copied state
static bool gs_HasCopiedState[2];
static MapTimeInfo_t gs_CopiedState;

//Clears the copied state of the panel.
void ClearCopiedState()
{
	gs_HasCopiedState[0] = false;
	gs_HasCopiedState[1] = false;
}

//pages
static int CurrentNightComboBoxItem = 0;
static int CurrentDayComboBoxItem = 0;

//scroll bar values
static int CurrentScrolledNightValue = 0;
static int CurrentScrolledDayValue = 0;

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
		if (IsNightPage)
			memset(&gs_CopiedState.NightInfo, 0, sizeof(MapTimeInfo_t::NightInfo_t));		//sizeof(MapTimeInfo_t::NightInfo_t) looks cooler in editor then sizeof(gs_CopiedState.NightInfo)
		else
			memset(&gs_CopiedState.DayInfo, 0, sizeof(MapTimeInfo_t::DayInfo_t));
	}

	//create our top combo box
	m_FileList = new CMapPropertiesEditorComboBox(this, "FileList", 10, false);
	m_FileList->AddActionSignalTarget(this);

	//make the scroll bar
	m_ScrollBar = new ScrollBar(this, "ScrollBar", true);
	m_ScrollBar->AddActionSignalTarget(this);
	m_ScrollBar->SetValue((m_bIsNightPage ? CurrentScrolledNightValue : CurrentScrolledDayValue));

	//add each page
	CUtlVector<MapTimeInfoBase_t>& base = GetDayNightInfo();

	for (int i = 0; i < base.Count(); i++)
		m_FileList->AddItem(base[i].filename, nullptr);

	m_FileList->ActivateItem(IsNightPage ? CurrentNightComboBoxItem : CurrentDayComboBoxItem);
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

	(m_bIsNightPage ? CurrentNightComboBoxItem : CurrentDayComboBoxItem) = index;

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

		//reload server settings. This could have been the map that was changed.
		engine->ClientCmd("_amod_day_do");
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

		//get the map text
		wchar_t mapnameUnicode[256];
		g_pVGuiLocalize->ConvertANSIToUnicode(base[i].mapname, mapnameUnicode, sizeof(mapnameUnicode));

		wchar_t text[256];
		swprintf(text, SIZE_OF_ARRAY(text), L"%ls: %ls", g_pVGuiLocalize->Find("MapProperties_MapLabel_Text"), mapnameUnicode);

		//create the mapname text
		Label* maptext = new Label(divider, "MapText", text);
		maptext->SetBounds(10, 5, 365, 40);
		maptext->SetFont(m_MapTextFont);
		maptext->SetContentAlignment(Label::Alignment::a_northwest);

		const char* skyName = m_bIsNightPage ? StringFromMapTimeStringTableIndex(base[i].NightInfo.Skybox) : StringFromMapTimeStringTableIndex(base[i].DayInfo.Skybox);

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

		//is this enabled
		bool enabled = m_bIsNightPage ? base[i].AllowNightTime : base[i].AllowDaytime;

		//if both AllowNightTime and AllowDayTime are false. Then set this to true 
		if (!base[i].AllowDaytime && !base[i].AllowNightTime)
		{
			enabled = true;
			base[i].AllowDaytime = true;
			base[i].AllowNightTime = true;
		}

		//create the enabled check button
		CheckButton* enabledCheckButton = new CheckButton(divider, "enabledCheckButton", m_bIsNightPage ? "#MapProperties_CheckButton_AllowNightTime" : "#MapProperties_CheckButton_AllowDayTime");
		enabledCheckButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 92, 218, 24);
		enabledCheckButton->SetCommand(CFmtStr(ENABLE_MAP_PREFIX "%d", i));
		enabledCheckButton->AddActionSignalTarget(this);
		enabledCheckButton->SetSelected(enabled);
		ADD_TOOLTIP(enabledCheckButton, 100, "#MapProperties_CheckButton_AllowTime_Tooltip", true)
			
		//create the flip time check button
		CheckButton* flipCheckButton = new CheckButton(divider, "flipCheckButton", "#MapProperties_CheckButton_FlipTime");
		flipCheckButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 115, 218, 24);
		flipCheckButton->SetCommand(CFmtStr(FLIP_MAP_PREFIX "%d", i));
		flipCheckButton->AddActionSignalTarget(this);
		flipCheckButton->SetEnabled((base[i].AllowDaytime && base[i].AllowNightTime) || (!base[i].AllowDaytime && !base[i].AllowNightTime));
		flipCheckButton->SetSelected(base[i].FlipTimes);
		ADD_TOOLTIP(flipCheckButton, 100, "#MapProperties_CheckButton_FlipTime_Tooltip", true)

		//create the copy map settings button
		Button* copyButton = new Button(divider, "copyButton", "#MapProperties_Button_CopyState");
		copyButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 61, 105, 24);
		copyButton->SetCommand(CFmtStr(COPY_MAP_STATE_PREFIX "%d", i));
		copyButton->AddActionSignalTarget(this);
		ADD_TOOLTIP(copyButton, 100, "#MapProperties_Button_CopyState_ToolTip", true)

		//create the paste map settings button
		Button* pasteButton = new Button(divider, "pasteButton", "#MapProperties_Button_PasteState");
		pasteButton->SetBounds(114, MAP_CONTAINER_HEIGHT - 61, 105, 24);
		pasteButton->SetCommand(CFmtStr(PASTE_MAP_STATE_PREFIX"%d", i));
		pasteButton->AddActionSignalTarget(this);
		ADD_TOOLTIP(pasteButton, 100, "#MapProperties_Button_PasteState_ToolTip", true)

		//create the modify map settings button
		Button* modifyButton = new Button(divider, "ModifyMapSettings", "#MapProperties_Button_ModifyMapState");
		modifyButton->SetBounds(4, MAP_CONTAINER_HEIGHT - 35, 355, 24);
		modifyButton->SetCommand(CFmtStr(MODIFY_MAP_PREFIX "%d", i));
		modifyButton->AddActionSignalTarget(this);
		modifyButton->SetEnabled(enabled);

		//hack: when enabledCheckButton gets checked. We will need a way to toggle the modify button on/off.
		//		To fix this i can use 'checkButton->GetNavUp()' in OnCommand
		enabledCheckButton->SetNavUp(modifyButton);
		enabledCheckButton->SetNavDown(flipCheckButton);

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
void CMapPropertiesEditorPageBase::OnCommand(KeyValues* data)
{
	const char* cmd = data->GetString("command");

	//is the ctrl key down?
	bool ctrl = vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LCONTROL) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RCONTROL);

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
		CopyTimeInfoData(info, gs_CopiedState, m_bIsNightPage, m_bIsNightPage);
	}

	//check for paste state
	else if (Q_strstr(cmd, PASTE_MAP_STATE_PREFIX))
	{
		//we must have a copied state
		if (!gs_HasCopiedState[ctrl ? !m_bIsNightPage : m_bIsNightPage])
		{
			//show error
			QueryBox* modal = new QueryBox("#MapProperties_Query_NoCopiedStates_Title", "#MapProperties_Query_NoCopiedStates_Desc", this);
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
		CopyTimeInfoData(gs_CopiedState, info, ctrl ? !m_bIsNightPage : m_bIsNightPage, m_bIsNightPage);

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

	//check for ENABLE_MAP_PREFIX
	else if (Q_strstr(cmd, ENABLE_MAP_PREFIX))
	{
		//get the index
		int index = Q_atoi(cmd + Q_strlen(ENABLE_MAP_PREFIX));

		//get the time info
		CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
		if (index < 0 || index >= baseinfo[m_FileList->GetActiveItem()].base.Count())
			return;

		MapTimeInfo_t& info = baseinfo[m_FileList->GetActiveItem()].base[index];

		//get the check button
		CheckButton* checkbutton = (CheckButton*)data->GetPtr("panel");
		if (m_bIsNightPage)
			info.AllowNightTime = checkbutton->IsSelected();
		else
			info.AllowDaytime = checkbutton->IsSelected();

		//if both are disabled then spit out a query box and enable both
		if (!info.AllowDaytime && !info.AllowNightTime)
		{
			//play a sound and show the query box
			surface()->PlaySound("ui/buttonclickrelease.wav");

			QueryBox* modal = new QueryBox("#MapProperties_EnableTime_BothDisabled_Title", "#MapProperties_EnableTime_BothDisabled_Description", this);
			modal->MoveToCenterOfScreen();
			modal->Activate();
			modal->DoModal();

			//set the enabled state
			info.AllowDaytime = true;
			info.AllowNightTime = true;
			checkbutton->SetSelected(true);

			//tell our child page to re-populate to apply the changes
			PropertySheet* parent = dynamic_cast<PropertySheet*>(GetParent());
			if (parent)
			{
				CMapPropertiesEditorPageBase* page = dynamic_cast<CMapPropertiesEditorPageBase*>(parent->GetPage(!!m_bIsNightPage));
				if (page)
				{
					page->m_FileList->PostMessage(page, new KeyValues("TextChanged"));
				}
			}
		}

		//get the button to disable/enable. this uses the hack i mentioned before.
		checkbutton->GetNavUp()->SetEnabled(checkbutton->IsSelected());
		checkbutton->GetNavDown()->SetEnabled(!info.AllowDaytime || !info.AllowNightTime);

		//send the data to the server. The only time we do this is when we change something (i.e paste data from 1 panel to another).
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


	//check for ENABLE_MAP_PREFIX
	else if (Q_strstr(cmd, FLIP_MAP_PREFIX))
	{
		//get the index
		int index = Q_atoi(cmd + Q_strlen(FLIP_MAP_PREFIX));

		//get the time info
		CUtlVector<MapTimeInfoBase_t>& baseinfo = GetDayNightInfo();
		if (index < 0 || index >= baseinfo[m_FileList->GetActiveItem()].base.Count())
			return;

		MapTimeInfo_t& info = baseinfo[m_FileList->GetActiveItem()].base[index];

		//get the check button
		CheckButton* checkbutton = (CheckButton*)data->GetPtr("panel");
		info.FlipTimes = checkbutton->IsSelected();

		//send the data to the server. The only time we do this is when we change something (i.e paste data from 1 panel to another).
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

	BaseClass::OnCommand(cmd);
}