//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Maplist for geo-guesser options panel
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_MapList.h"
#include "fmtstr.h"
#include "vgui_controls/Tooltip.h"

#define MAP_LIST_TITLE_HEIGHT 22
#define MAP_LIST_SCROLL_BAR_WIDTH 20
#define MAP_LIST_CHECKBUTTON_X -5
#define MAP_LIST_CHECKBUTTON_HEIGHT 20
#define MAP_LIST_CHECKBUTTON_CHILD_X_OFFSET 15
#define MAP_LIST_CHECKBUTTON_COMMAND_PREFIX "CheckButton"

//---------------------------------------------------------------------------------
// Purpose: Constructor for options page map list
//---------------------------------------------------------------------------------
CGG_MapListPanel::CGG_MapListPanel(CGG_MainPanel* parent, const char* name)
	: BaseClass(parent, name), m_Parent(parent)
{
	//create the title stuff
	m_TitleDivider = new vgui::Divider(this, "TitleDivider");
	m_Title = new vgui::Label(this, "Title", "Include Maps:");
	m_Title->SetContentAlignment(vgui::Label::Alignment::a_center);

	//create the scroll bar
	m_ScrollBar = new vgui::ScrollBar(this, "ScrollBar", true);
	m_ScrollBar->SetRangeWindow(1);
	m_ScrollBar->AddActionSignalTarget(this);

	//add the buttons
	PopulateButtons();
}

//---------------------------------------------------------------------------------
// Purpose: Sets the title for the map list geo-guesser panel
//---------------------------------------------------------------------------------
void CGG_MapListPanel::SetTitle(const char* title)
{
	if (m_Title)
		m_Title->SetText(title);
}

//---------------------------------------------------------------------------------
// Purpose: Formats the check buttons based on the scroll bars value
//---------------------------------------------------------------------------------
void CGG_MapListPanel::FormatList(bool CheckForSrollWheelRange)
{
	//get the scroll bar value
	int scroll_amount = m_ScrollBar->GetValue();
	int scroll_amount_set = 1;

	//y pos
	int y_position = MAP_LIST_TITLE_HEIGHT - MAP_LIST_CHECKBUTTON_HEIGHT;

	//panel height
	int height = GetTall();
	//format all of the buttons
	for (int i = 0; i < m_CheckButtons.Count(); i++)
	{
		//store the map button
		MapButton_t& mapbutton = m_CheckButtons[i];

		//store vgui button
		CGG_MapListCheckButton* parent = mapbutton.m_CheckButton;

		//increment button y pos
		y_position += MAP_LIST_CHECKBUTTON_HEIGHT;

		//hide if scrolled off top or bottom
		if (y_position - MAP_LIST_TITLE_HEIGHT < scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT || y_position >= height + scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT)
		{
			parent->SetVisible(false);
		}
		else
		{
			parent->SetVisible(true);
			parent->SetBounds(MAP_LIST_CHECKBUTTON_X, y_position - scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT, GetWide() - MAP_LIST_SCROLL_BAR_WIDTH, MAP_LIST_CHECKBUTTON_HEIGHT);
		}

		//increment scroll count if needed
		if (y_position + MAP_LIST_CHECKBUTTON_HEIGHT - height >= 0)
			scroll_amount_set++;

		//should the children draw?
		bool ShouldChildrenDraw = parent->IsSelected() && parent->ShouldShowPositions();

		//add the children ONLY if the parent is selected
		for (int j = 0; j < mapbutton.m_ChildButtons.Count(); j++)
		{
			//store vgui button
			vgui::CheckButton* child = mapbutton.m_ChildButtons[j];

			//increment button y pos
			if (ShouldChildrenDraw)
				y_position += MAP_LIST_CHECKBUTTON_HEIGHT;

			//hide if scrolled off top or bottom
			if (y_position - MAP_LIST_TITLE_HEIGHT < scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT || y_position >= height + scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT || !ShouldChildrenDraw)
			{
				child->SetVisible(false);
			}
			else
			{
				child->SetVisible(true);
				child->SetBounds(MAP_LIST_CHECKBUTTON_X + MAP_LIST_CHECKBUTTON_CHILD_X_OFFSET, y_position - scroll_amount * MAP_LIST_CHECKBUTTON_HEIGHT, GetWide() - MAP_LIST_SCROLL_BAR_WIDTH - MAP_LIST_CHECKBUTTON_CHILD_X_OFFSET, MAP_LIST_CHECKBUTTON_HEIGHT);
			}

			if (ShouldChildrenDraw && y_position + MAP_LIST_CHECKBUTTON_HEIGHT - height >= 0)
				scroll_amount_set++;
		}
	}

	//set the new range
	if (CheckForSrollWheelRange)
		m_ScrollBar->SetRange(0, scroll_amount_set);
}

//---------------------------------------------------------------------------------
// Purpose: Adds all the map names as buttons
//---------------------------------------------------------------------------------
void CGG_MapListPanel::PopulateButtons()
{
	//get the info
	CGG_MainPanel::GeoGuesserInfo_t& info = m_Parent->GetGGInfo();

	//add them
	for (int i = 0; i < info.mapdata.Count(); i++)
	{
		//button info
		MapButton_t& mapbutton = m_CheckButtons[m_CheckButtons.AddToTail()];

		//create the parent
		CGG_MapListCheckButton* parent = new CGG_MapListCheckButton(this, CFmtStr(MAP_LIST_CHECKBUTTON_COMMAND_PREFIX  "%d", i), m_Parent->GetStringForSymbol(info.mapdata[i]->MapName));
		parent->SetSelected(info.mapdata[i]->enabled);
		parent->AddActionSignalTarget(this);
		
		//create the children
		for (int j = 0; j < info.mapdata[i]->MapLocations.Count(); j++)
		{
			const char* text = m_Parent->GetStringForSymbol(info.mapdata[i]->MapLocations[j].image);

			//make child
			vgui::CheckButton* child = new vgui::CheckButton(this, CFmtStr(MAP_LIST_CHECKBUTTON_COMMAND_PREFIX "%d:%d", i, j), text);
			child->SetSelected(info.mapdata[i]->enabled && info.mapdata[i]->MapLocations[j].enabled);
			child->AddActionSignalTarget(this);

			//add tooltip
			child->GetTooltip()->SetEnabled(true);
			child->GetTooltip()->SetText(text);
			child->GetTooltip()->SetTooltipDelay(500);
			child->GetTooltip()->SetTooltipFormatToSingleLine();

			//add the child
			mapbutton.m_ChildButtons.AddToTail(child);
		}

		//add the parent
		mapbutton.m_CheckButton = parent;
	}

	//format the list
	FormatList();
}

//---------------------------------------------------------------------------------
// Purpose: Selects/deselects all the buttons
//---------------------------------------------------------------------------------
void CGG_MapListPanel::SelectAll(bool select)
{
	//go through all buttons
	for (int i = 0; i < m_CheckButtons.Count(); i++)
	{
		m_CheckButtons[i].m_CheckButton->SetSelected(select);
		
		//go through all child buttons
		for (int j = 0; j < m_CheckButtons[i].m_ChildButtons.Count(); j++)
			m_CheckButtons[i].m_ChildButtons[j]->SetSelected(select);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Selects/deselects all the child buttons
//---------------------------------------------------------------------------------
void CGG_MapListPanel::SelectAllChildren(bool select)
{
	//go through all buttons
	for (int i = 0; i < m_CheckButtons.Count(); i++)
	{
		if (m_CheckButtons[i].m_CheckButton->IsSelected())
		{
			//go through all child buttons
			for (int j = 0; j < m_CheckButtons[i].m_ChildButtons.Count(); j++)
				m_CheckButtons[i].m_ChildButtons[j]->SetSelected(select);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns if there are any buttons selected
//---------------------------------------------------------------------------------
bool CGG_MapListPanel::HasAnySelected()
{
	//see if we have ANY maps that are selected. If not then navigate to the options panel first instead of the main menu
	for (int i = 0; i < m_CheckButtons.Count(); i++)
	{
		if (m_CheckButtons[i].m_CheckButton->IsSelected())
		{
			//see if we have any children that are enabled
			for (int j = 0; j < m_CheckButtons[i].m_ChildButtons.Count(); j++)
			{
				if (m_CheckButtons[i].m_ChildButtons[j]->IsSelected())
					return true;
			}
		}
	}

	return false;
}

//---------------------------------------------------------------------------------
// Purpose: Override for the set bounds function
//---------------------------------------------------------------------------------
void CGG_MapListPanel::SetBounds(int x, int y, int w, int h)
{
	//call base function
	BaseClass::SetBounds(x, y, w, h);

	//set the title stuff bounds
	m_Title->SetBounds(0, 0, w - MAP_LIST_SCROLL_BAR_WIDTH, MAP_LIST_TITLE_HEIGHT);
	m_TitleDivider->SetBounds(-2, MAP_LIST_TITLE_HEIGHT, w - MAP_LIST_SCROLL_BAR_WIDTH + 4, 1);

	//set scroll wheel
	m_ScrollBar->SetBounds(w - MAP_LIST_SCROLL_BAR_WIDTH, 0, MAP_LIST_SCROLL_BAR_WIDTH, h - 2);

	//format the button list
	FormatList();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on scroll bar moved
//-------------------------------------------------------------------------------------------------------
void CGG_MapListPanel::OnScrollBarSliderMoved()
{
	//format the list
	FormatList(false);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on mouse wheeled
//-------------------------------------------------------------------------------------------------------
void CGG_MapListPanel::OnMouseWheeled(int delta)
{
	BaseClass::OnMouseWheeled(delta);

	//handle scroll for scroll wheel
	m_ScrollBar->SetValue(m_ScrollBar->GetValue() - delta);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a check button is checked
//-------------------------------------------------------------------------------------------------------
void CGG_MapListPanel::CheckButtonChecked(vgui::Panel* panel)
{
	//check for check button
	vgui::CheckButton* button = dynamic_cast<vgui::CheckButton*>(panel);
	if (!button)
		return;

	//get the button text
	char text[128];
	button->GetText(text, sizeof(text));

	//get the name command
	const char* name = button->GetName();

	//get index
	int index = Q_atoi(name + Q_strlen(MAP_LIST_CHECKBUTTON_COMMAND_PREFIX));

	//enable/disable the button for the config
	CGG_MainPanel::GeoGuesserInfo_t& info = m_Parent->GetGGInfo();

	//see if its a parent or child
	if (Q_strrchr(name, ':'))
	{
		//get map index
		int mapindex = Q_atoi(Q_strstr(name, ":") + 1);

		//set enabled
		info.mapdata[index]->MapLocations[mapindex].enabled = button->IsSelected();
	}
	else
	{
		//set enabled
		info.mapdata[index]->enabled = button->IsSelected();
		FormatList();

		//select all child buttons
		//for (int i = 0; i < m_CheckButtons[index].m_ChildButtons.Count(); i++)
		//	m_CheckButtons[index].m_ChildButtons[i]->SetSelected(true);
	}
}