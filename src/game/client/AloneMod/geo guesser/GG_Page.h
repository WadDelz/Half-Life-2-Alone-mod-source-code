//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main panel for the half-life 2 geoguesser
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __GG_PAGE_H
#define __GG_PAGE_H

#ifdef WIN32
#pragma once
#endif

//headers
#include "GG_MainPanel.h"
#include "vgui_controls/Panel.h"

//base page class for the geo-guesser panel
class I_GG_Page
{
public:
	I_GG_Page(CGG_MainPanel* parent, const char* name);
	~I_GG_Page();
	
	//gets the parent
	inline CGG_MainPanel* GetPanel() { return m_Panel; }

	//gets the name
	inline const char* GetName() { return m_Name; }

	//child function
	int AddChild(vgui::Panel* panel);
	inline CUtlVector<vgui::Panel*>& GetChildren() { return m_Children; }

	//navigate functions
	virtual void NavigateTo();
	virtual void PostNavigateTo() {};

	//this is the name of the file that will get loaded for the children
	//when this page gets navigated to
	void SetSettingsFile(const char* filename) { m_SettingsFile = filename; }

	//loads the positions and stuff for each child
	void LoadSettings(const char* filename);

	//virtual funcs
	virtual void OnCommand(const char* command) {}
	virtual void OnTextChanged() {}
	virtual void OnSliderMoved(vgui::Panel* slider) {}

public:
	//page visibility function
	void SetVisible(bool visible);
	inline bool IsVisible() { return m_bVisible; }

private:
	//is this page visible/active
	bool m_bVisible = false;

	//panel this page belongs to
	CGG_MainPanel* m_Panel = nullptr;

	//title
	const char* m_Name = nullptr;
	const char* m_SettingsFile = nullptr;

	//array of children
	CUtlVector<vgui::Panel*> m_Children;

private:
	//font holder
	struct GG_FontHolder
	{
		const char* Name;
		vgui::HFont Font;
	};
	CUtlVector<GG_FontHolder> m_Fonts;
};

#endif //__GG_PAGE_H