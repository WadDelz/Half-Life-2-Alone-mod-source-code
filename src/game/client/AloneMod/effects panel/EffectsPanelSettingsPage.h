#ifndef __EFFECTSPANELSETTINGSPAGE_H
#define __EFFECTSPANELSETTINGSPAGE_H

#include "EffectsPanel.h"
#include "EffectsPanelConvarPage.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Button.h"

#ifdef _WIN32
#pragma once
#endif


//lighting list button
class CAutoLoadPageButton : public CConvarPageConvarButtonBase
{
	DECLARE_CLASS_SIMPLE(CAutoLoadPageButton, CConvarPageConvarButtonBase);
public:
	//constructor
	CAutoLoadPageButton(vgui::Panel* parent, const char* name, const char* text) :
		BaseClass(parent, name, text)
	{
	}
private:
	friend class CAutoLoadPageList;
};




///overlay page convar list
#define AUTOLOAD_LIST_PANEL_TITLE_HEIGHT 20
#define AUTOLOAD_LIST_PANEL_SCROLL_BAR_WIDTH 20
#define AUTOLOAD_LIST_PANEL_BUTTON_X_OFFSET 5
#define AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT 22
#define AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX "AutoloadButton:"
#define AUTOLOAD_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL 10

class CAutoLoadPageList : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(CAutoLoadPageList, vgui::Divider);
public:
	//constructor
	CAutoLoadPageList(vgui::Panel* parent, const char* name, const char* title);

	//scroll bar moved and slider
	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

	//overlay funcs
	void AddFile(const char* File);
	bool RemoveSelectedFile();
	void GetAutoloadFiles(CUtlVector<char*>& data);
	int GetFileCount();

	//panel functions
	void OnCommand(const char* pszCommand);

private:
	//array of buttons
	CUtlVector<CAutoLoadPageButton*> m_ButtonList;

	//title text
	vgui::Label* m_Title;

	//scroll wheel
	vgui::ScrollBar* m_ScrollBar;

	//settings page
	class CEffectsPanelSettingsPage* m_SettingsPage;
};



//settings page
#define COMMAND_SELECT_FILE "SelectFile"
#define COMMAND_SELECT_FOLDER "SelectFolder"
#define COMMAND_ADD "Add"
#define COMMAND_REMOVE "Remove"



class CEffectsPanelSettingsPage : public IEffectsPanelPage
{
	DECLARE_CLASS_SIMPLE(CEffectsPanelSettingsPage, IEffectsPanelPage);
public:
	//constructor
	CEffectsPanelSettingsPage(vgui::Panel* parent, const char* name);

	//save/load functions
	void ResetEffects();
	void ReadFromFile(KeyValues* keyvalues, bool reset = false);
	void WriteToFile(KeyValues* keyvalues);

	//other funcs
	void OnCommand(const char* pszCommand);
	void SetFileText(const char* pszCommand);
	MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);

	//file functions
	void AddMapsFilenameRecursive(const char* path, CUtlVector<char*>& MapList);
	void LoadFile(const char* path, CUtlVector<char*>& MapList);
	void LoadFolder(const char* foldername, CUtlVector<char*>& MapList);

	//map funcs
	void OnMapLoad();
	void OnMapShutdown();

	//sets all the bounds for each item
	void PerformLayout();

private:
	//file dialog stuff
	vgui::FileOpenDialog* m_FileDialog = nullptr;

	//list
	CAutoLoadPageList* m_FileList;

	//buttons and text entry
	vgui::Label* m_FileLabel;
	vgui::TextEntry* m_FileTextEntry;
	vgui::Button* m_SelectFileButton;
	vgui::Button* m_SelectFolderButton;
	vgui::Button* m_AddToListButton;
	vgui::Button* m_RemoveButton;
};

#endif //__EFFECTSPANELSETTINGSPAGE_H