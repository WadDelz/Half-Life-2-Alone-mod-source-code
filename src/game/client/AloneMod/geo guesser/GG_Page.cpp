//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main panel for the half-life 2 geoguesser
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_Page.h"
#include "filesystem.h"

//vgui headers
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Tooltip.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui/ISurface.h"

//---------------------------------------------------------------------------------
// Purpose: Constructor for the geo-guesser page
//---------------------------------------------------------------------------------
I_GG_Page::I_GG_Page(CGG_MainPanel* parent, const char* name)
{
	m_Name = name;
	m_Panel = parent;
}

//---------------------------------------------------------------------------------
// Purpose: Destructor for the geo-guesser page
//---------------------------------------------------------------------------------
I_GG_Page::~I_GG_Page()
{
	//delete all the fonts
	for (int i = 0; i < m_Fonts.Count(); i++)
		free((char*)m_Fonts[i].Name);
}

//---------------------------------------------------------------------------------
// Purpose: Adds a child to the children list if not already added
//---------------------------------------------------------------------------------
int I_GG_Page::AddChild(vgui::Panel* panel)
{
	//check for the child
	if (!panel)
		return m_Children.InvalidIndex();

	//index for the panel
	int index = m_Children.Find(panel);

	//check for if the panel is already on this page
	if (index != m_Children.InvalidIndex())
		return index;

	//set the child's visibility
	panel->SetVisible(false);

	//add the panel
	return m_Children.AddToTail(panel);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the visibility for this page
//---------------------------------------------------------------------------------
void I_GG_Page::SetVisible(bool visible) 
{ 
	//set each childrens visibility
	for (int i = 0; i < m_Children.Count(); i++) 
		m_Children[i]->SetVisible(visible);

	//set m_bVisible
	m_bVisible = visible;
};

//---------------------------------------------------------------------------------
// Purpose: Navigates to this page
//---------------------------------------------------------------------------------
void I_GG_Page::NavigateTo()
{
	m_Panel->NavigateToPage(this);

	//load the settings
	LoadSettings(m_SettingsFile);

	//call PostNavigateTo
	PostNavigateTo();
}

ConVar gg_page_animate_time("gg_page_animate_time", "0.1");

#define PAGE_ANIMATE_TIME gg_page_animate_time.GetFloat()
#define PAGE_ANIMATE_MODE vgui::AnimationController::Interpolators_e::INTERPOLATOR_SIMPLESPLINE

//---------------------------------------------------------------------------------
// Purpose: Loads the settings for each child
//---------------------------------------------------------------------------------
void I_GG_Page::LoadSettings(const char* filename)
{
	//check for the filename
	if (!filename)
		return;

	//load the file
	KeyValues* file = new KeyValues(filename);
	file->UsesEscapeSequences(true);
	if (!file->LoadFromFile(filesystem, filename, "MOD"))
	{
		ConWarning("Geo-Guesser: Failed to load controller settings for \"%s\"", filename);
		return;
	}
	
	//set the width/height of the page
	m_Panel->SetTitle(file->GetString("Title"), true);

	{
		//get panel bounds
		int x, y, w, h;
		m_Panel->GetBounds(x, y, w, h);

		//get the center x
		int centerX = x + w / 2;
		int centerY = y + h / 2;

		//get the width and height
		int newW = file->GetInt("Width");
		int newH = file->GetInt("Height");

		//get the newx and new y
		int newX = centerX - newW / 2;
		int newY = centerY - newH / 2;

		//set the panel bounds
		//m_Panel->SetBounds(newX, newY, newW, newH);
		vgui::GetAnimationController()->RunAnimationCommand(m_Panel, "xpos", newX, 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(m_Panel, "ypos", newY, 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(m_Panel, "wide", newW, 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(m_Panel, "tall", newH, 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
	}

	//check for fonts subkey
	KeyValues* font = file->FindKey("Fonts");
	if (font)
	{
		//go through each font
		FOR_EACH_TRUE_SUBKEY(font, subkey)
		{
			//get the font name
			const char* name = subkey->GetString("Name");

			//see if the font already exists
			bool found = false;
			for (int i = 0; i < m_Fonts.Count(); i++)
			{
				if (!Q_strcmp(m_Fonts[i].Name, name))
				{
					found = true;
					break;
				}
			}

			//did we find the font?
			if (found)
				continue;

			//create the font and add it to the fonts
			vgui::HFont CustomFont = vgui::surface()->CreateFont();
			vgui::surface()->SetFontGlyphSet(CustomFont, subkey->GetString("FontName"), subkey->GetInt("Size", 18), subkey->GetInt("Weight", 500), subkey->GetInt("Blur", 0), 0, vgui::ISurface::FONTFLAG_ANTIALIAS);

			//add the font
			m_Fonts.AddToTail(GG_FontHolder{ strdup(name), CustomFont });
		}
	}

	//go through each child
	for (int i = 0; i < m_Children.Count(); i++)
	{
		//get the child
		vgui::Panel* panel = m_Children[i];

		//look for the subkey with the panel's name
		KeyValues* subkey = file->FindKey(panel->GetName());
		if (!subkey)
			continue;

		//get bounds, text, and other stuff
		//panel->SetBounds(subkey->GetInt("X"), subkey->GetInt("Y"), subkey->GetInt("Width"), subkey->GetInt("Height"));

		vgui::GetAnimationController()->RunAnimationCommand(panel, "xpos", subkey->GetInt("X"), 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(panel, "ypos", subkey->GetInt("Y"), 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(panel, "wide", subkey->GetInt("Width"), 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);
		vgui::GetAnimationController()->RunAnimationCommand(panel, "tall", subkey->GetInt("Height"), 0.0f, PAGE_ANIMATE_TIME, PAGE_ANIMATE_MODE);

		//set zpos
		if (subkey->FindKey("Z"))
			panel->SetZPos(subkey->GetInt("Z"));

		//check for tooltip stuff
		if (subkey->GetBool("EnableTooltip"))
		{
			panel->GetTooltip()->SetEnabled(true);
			panel->GetTooltip()->SetText(subkey->GetString("TooltipText"));
			panel->GetTooltip()->SetTooltipDelay(subkey->GetInt("TooltipDelay"));

			if (subkey->GetBool("TooltipMultiline"))
				panel->GetTooltip()->SetTooltipFormatToMultiLine();
			else
				panel->GetTooltip()->SetTooltipFormatToSingleLine();
		}

		//call the slider's callback if its a slider
		vgui::Slider* slider = dynamic_cast<vgui::Slider*>(panel);
		if (slider)
			slider->SendSliderMovedMessage();

		//check to see if the panel can get its text set
		vgui::Label* label = dynamic_cast<vgui::Label*>(panel);
		if (label)
		{
			//set the text
			if (subkey->FindKey("Text"))
				label->SetText(subkey->GetString("Text"));

			//center if needed
			if (subkey->GetBool("Center"))
				label->SetContentAlignment(vgui::Label::Alignment::a_center);

			//check for font
			const char* font = subkey->GetString("Font", nullptr);
			if (font)
			{
				//find the font
				for (int i = 0; i < m_Fonts.Count(); i++)
				{
					if (!Q_strcmp(m_Fonts[i].Name, font))
					{
						label->SetFont(m_Fonts[i].Font);
						break;
					}
				}
			}
		}

		//check to see if the panel can get its de-pressed/hover sound set
		vgui::Button* button = dynamic_cast<vgui::Button*>(panel);
		if (button)
		{
			button->SetDepressedSound(subkey->GetString("Sound_Pressed", nullptr));
			button->SetArmedSound(subkey->GetString("Sound_Hover", nullptr));
		}
	
		//check to see if the panel can get its fill color set
		vgui::ImagePanel* image = dynamic_cast<vgui::ImagePanel*>(panel);
		if (image && subkey->FindKey("FillColor"))
		{
			//get rgba
			int rgba[4];
			UTIL_StringToIntArray(rgba, 4, subkey->GetString("FillColor"));

			//set fill color
			image->SetFillColor(Color(rgba[0], rgba[1], rgba[2], rgba[3]));
		}
	}

	//delete the file
	file->deleteThis();
}