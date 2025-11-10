#include "cbase.h"
#include "EffectsPanelLightingPage.h"
#include "vgui_controls/PropertyDialog.h"
#include "fmtstr.h"


//list of lighting pages. This is so i can loop through each effect page and call page->UpdateLighting() function;
static CUtlVector<CEffectsPanelLightingPage*> g_EffectsLightingPages;

//updates all of the lights
void Amod_EffectsPanelUpdateLighting()
{
	for (int i = 0; i < g_EffectsLightingPages.Count(); i++)
		g_EffectsLightingPages[i]->UpdateLighting();
}



//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel lighting page
//---------------------------------------------------------------------------------
CEffectsPanelLightingPage::CEffectsPanelLightingPage(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//make buttons
	m_AddButton = new vgui::Button(this, "AddButton", "Add Light", this, ADD_LIGHT_BUTTON_COMMAWND);
	m_ChangeOverlay = new vgui::Button(this, "ChangeButton", "Change Selected Light", this, CHANGE_LIGHT_BUTTON_COMMAWND);
	m_RemoveOverlay = new vgui::Button(this, "RemoveOverlay", "Remove Selected Light", this, REMOVE_LIGHT_BUTTON_COMMAWND);

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
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::ReadFromFile(KeyValues* keyvalues)
{
	//clear everything
	ResetEffects();

	//find the view KeyValues
	KeyValues* lighting = keyvalues->FindKey("Lighting");
	if (!lighting)
		return;
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::WriteToFile(KeyValues* keyvalues)
{
	//create a new KeyValues*
	KeyValues* lighting = new KeyValues("Lighting");

	//add the lighting to the base subkey
	keyvalues->AddSubKey(lighting);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the lighting text's
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::SetLightingText()
{
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::OnCommand(const char* pszCommand)
{
	if (0)
	{
	}

	//call base function
	else
	{
		BaseClass::OnCommand(pszCommand);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Updates the lights
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::UpdateLighting()
{
}

//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelLightingPage::PerformLayout()
{
	//buttons
	m_AddButton->SetBounds(5, 370, 170, 22);
	m_ChangeOverlay->SetBounds(182, 370, 170, 22);
	m_RemoveOverlay->SetBounds(359, 370, 170, 22);
}