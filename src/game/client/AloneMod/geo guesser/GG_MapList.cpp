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

#define MAP_LIST_TITLE_HEIGHT 22
#define MAP_LIST_SCROLL_BAR_WIDTH 20
#define MAP_LIST_CHECKBUTTON_HEIGHT 20

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

	//panel height
	int height = GetTall();

	//format all of the buttons
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		//store the button
		vgui::CheckButton* button = m_Buttons[i];

		//button y pos
		int y = 22 + (MAP_LIST_CHECKBUTTON_HEIGHT * (i - scroll_amount));

		//if i < scroll_amount then set the bounds to be invalid
		if (i < scroll_amount)
			button->SetBounds(-100, -100, GetWide() - 20, MAP_LIST_CHECKBUTTON_HEIGHT);
		else
			button->SetBounds(-5, y, GetWide() - 20, MAP_LIST_CHECKBUTTON_HEIGHT);

		if (y + MAP_LIST_CHECKBUTTON_HEIGHT - height >= 0)
			scroll_amount_set++;
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
		//create the button
		vgui::CheckButton* button = new vgui::CheckButton(this, CFmtStr("CheckButton%d", i), m_Parent->GetStringForSymbol(info.mapdata[i]->MapName));
		button->SetSelected(info.mapdata[i]->enabled);
		button->AddActionSignalTarget(this);

		//add the button
		m_Buttons.AddToTail(button);
	}

	//format the list
	FormatList();
}

//---------------------------------------------------------------------------------
// Purpose: Selects/deselects all the buttons
//---------------------------------------------------------------------------------
void CGG_MapListPanel::SelectAll(bool select)
{
	for (int i = 0; i < m_Buttons.Count(); i++)
		m_Buttons[i]->SetSelected(select);
}

//---------------------------------------------------------------------------------
// Purpose: Returns if there are any buttons selected
//---------------------------------------------------------------------------------
bool CGG_MapListPanel::HasAnySelected()
{
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		if (m_Buttons[i]->IsSelected())
			return true;
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

	//enable/disable the button for the config
	CGG_MainPanel::GeoGuesserInfo_t& info = m_Parent->GetGGInfo();

	//check for map name with same name as button
	for (int i = 0; i < info.mapdata.Count(); i++)
	{
		if (!Q_strcmp(text, m_Parent->GetStringForSymbol(info.mapdata[i]->MapName)))
		{
			info.mapdata[i]->enabled = button->IsSelected();
			break;
		}
	}
}