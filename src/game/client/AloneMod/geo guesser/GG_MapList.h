//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Maplist for geo-guesser options panel
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef _GG_MAPLIST_H
#define _GG_MAPLIST_H

#ifdef WIN32
#pragma once
#endif

//vgui headers
#include "vgui_controls/Divider.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/CheckButton.h"
#include "GG_MainPanel.h"

//map list panel
class CGG_MapListPanel : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(CGG_MapListPanel, vgui::Divider)
public:
	//constructor
	CGG_MapListPanel(CGG_MainPanel* parent, const char* name);

	//title
	void SetTitle(const char* title);

	//button funcs
	void FormatList(bool CheckForSrollWheelRange = true);
	void PopulateButtons();

	//select funcs
	void SelectAll(bool select);
	bool HasAnySelected();

	//pos/size
	void SetBounds(int x, int y, int w, int h) override;

	//scroll stuff
	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

	//check button callback
	MESSAGE_FUNC_PTR(CheckButtonChecked, "CheckButtonChecked", panel);

private:
	//parent
	CGG_MainPanel* m_Parent = nullptr;

	//title stuff
	vgui::Label* m_Title = nullptr;
	vgui::Divider* m_TitleDivider = nullptr;

	//scroll wheel
	vgui::ScrollBar* m_ScrollBar = nullptr;

	//buttons
	CUtlVector<vgui::CheckButton*> m_Buttons;
};

#endif //_GG_MAPLIST_H