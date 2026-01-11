#include "cbase.h"
#include "EffectsPanelSettingsPage.h"
#include "vgui_controls/QueryBox.h"
#include "vgui_controls/PropertyDialog.h"
#include "vgui/ISurface.h"
#include "filesystem.h"
#include "fmtstr.h"


ConVar amod_effects_panel_autoload_files("amod_effects_panel_autoload_files", "0");


//settings autoload file list


//---------------------------------------------------------------------------------
// Purpose: Constructor for the overlay list panel
//---------------------------------------------------------------------------------
CAutoLoadPageList::CAutoLoadPageList(vgui::Panel* parent, const char* name, const char* title)
	: BaseClass(parent, name)
{
	//create the title
	m_Title = new vgui::Label(this, "_Title", title);
	m_Title->SetBounds(0, 0, EFFECTS_PAGE_WIDTH - 20, AUTOLOAD_LIST_PANEL_TITLE_HEIGHT);
	m_Title->SetContentAlignment(vgui::Label::a_center);

	//make the scroll bar
	m_ScrollBar = new vgui::ScrollBar(this, "_ScrollBar", true);
	m_ScrollBar->SetBounds(EFFECTS_PAGE_WIDTH - 20, 0, 20, 273);
	m_ScrollBar->SetRangeWindow(1);
	m_ScrollBar->AddActionSignalTarget(this);

	//set the settings page
	m_SettingsPage = dynamic_cast<CEffectsPanelSettingsPage*>(parent);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse wheel moved
//---------------------------------------------------------------------------------
void CAutoLoadPageList::OnMouseWheeled(int delta)
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
void CAutoLoadPageList::OnScrollBarSliderMoved()
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
			m_ButtonList[i]->SetPos(AUTOLOAD_LIST_PANEL_BUTTON_X_OFFSET, AUTOLOAD_LIST_PANEL_TITLE_HEIGHT + (AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT * i) - (AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT * pos));
	}
}

//---------------------------------------------------------------------------------
// Purpose: Adds a file button
//---------------------------------------------------------------------------------
void CAutoLoadPageList::AddFile(const char* File)
{
	//get the bounds
	int x = AUTOLOAD_LIST_PANEL_BUTTON_X_OFFSET;
	int y = AUTOLOAD_LIST_PANEL_TITLE_HEIGHT + (AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT * m_ButtonList.Count());
	int w = GetWide() - x - (AUTOLOAD_LIST_PANEL_BUTTON_X_OFFSET * 2) - AUTOLOAD_LIST_PANEL_SCROLL_BAR_WIDTH;
	int h = AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT;

	//make a new button
	CAutoLoadPageButton* button = new CAutoLoadPageButton(this, "OverlayButton", File);
	button->SetBounds(x, y, w, h);
	button->SetCommand(CFmtStr(AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count()));

	//add the button to the list of buttons
	m_ButtonList.AddToTail(button);

	//select this
	OnCommand(CFmtStr(AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", m_ButtonList.Count() - 1));

	//check the list size
	if (m_ButtonList.Count() > AUTOLOAD_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL)
	{
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetRange(0, m_ButtonList.Count() - AUTOLOAD_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
		m_ScrollBar->SetValue(m_ButtonList.Count() - AUTOLOAD_LIST_PANEL_MAX_ITEMS_BEFORE_SCROLL);
	}
	else
	{
		m_ScrollBar->SetRange(0, 0);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Returns all of the files
//---------------------------------------------------------------------------------
void CAutoLoadPageList::GetAutoloadFiles(CUtlVector<char*>& data)
{
	//go through each button data
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CAutoLoadPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		char *text = new char[FILENAME_MAX];
		button->GetText(text, FILENAME_MAX);
		data.AddToTail(text);

		//char temp[FILENAME_MAX];
		//button->GetText(temp, sizeof(temp));
		//data.AddToTail(strdup(temp));
	}
}


//---------------------------------------------------------------------------------
// Purpose: Returns the numbner of buttons
//---------------------------------------------------------------------------------
int CAutoLoadPageList::GetFileCount()
{
	return m_ButtonList.Count();
}

//---------------------------------------------------------------------------------
// Purpose: Removes the selected file
//---------------------------------------------------------------------------------
bool CAutoLoadPageList::RemoveSelectedFile()
{
	int SelectedIndex = -1;
	for (int i = 0; i < m_ButtonList.Count(); i++)
	{
		//check for the button
		CAutoLoadPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//check the selected state
		if (!button->IsButtonSelected())
			continue;

		SelectedIndex = i;

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
		CAutoLoadPageButton* button = m_ButtonList[i];
		if (!button)
			continue;

		//reset the command
		button->SetCommand(CFmtStr(AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX  "%d", i));

		//reset the bounds
		button->SetPos(AUTOLOAD_LIST_PANEL_BUTTON_X_OFFSET, AUTOLOAD_LIST_PANEL_TITLE_HEIGHT + (AUTOLOAD_LIST_PANEL_BUTTON_HEIGHT * i));
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
	OnCommand(CFmtStr(AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX "%d", SelectedIndex));

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CAutoLoadPageList::OnCommand(const char* pszCommand)
{
	if (StringHasPrefix(pszCommand, AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX))
	{
		//get the index
		int index = Q_atoi(pszCommand + Q_strlen(AUTOLOAD_LIST_PANEL_BUTTON_COMMAND_PREFIX));

		//select this button and de-select every other button
		for (int i = 0; i < m_ButtonList.Count(); i++)
		{
			//check for the button
			CAutoLoadPageButton* button = m_ButtonList[i];
			if (!button)
				continue;

			//select the button ONLY if i == index
			button->SetButtonSelected(i == index);

			//set the overlay page text if i == index
			if (i == index)
			{
				char text[FILENAME_MAX];
				button->GetText(text, sizeof(text));

				m_SettingsPage->SetFileText(text);
			}
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}


//setting page



//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel view effects page
//---------------------------------------------------------------------------------
CEffectsPanelSettingsPage::CEffectsPanelSettingsPage(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//make file list
	m_FileList = new CAutoLoadPageList(this, "FileList", "Autoload List:");

	//create the file text entry
	m_FileTextEntry = new vgui::TextEntry(this, "FileTextEntry");
	m_FileTextEntry->SetMaximumCharCount(FILENAME_MAX);
	
	//create the file label
	m_FileLabel = new vgui::Label(this, "FileLabel", "Autoload File Name:");
	m_FileLabel->SetContentAlignment(vgui::Label::Alignment::a_center);

	//create select buttons
	m_SelectFileButton = new vgui::Button(this, "SelectFileButton", "Choose File", this, COMMAND_SELECT_FILE);
	m_SelectFolderButton = new vgui::Button(this, "SelectFolderButton", "Choose Folder", this, COMMAND_SELECT_FOLDER);
	
	//create add/remove buttons
	m_AddToListButton = new vgui::Button(this, "AddButton", "Add to list", this, COMMAND_ADD);
	m_RemoveButton = new vgui::Button(this, "RemoveButton", "Remove selected from list", this, COMMAND_REMOVE);

	//add tooltips
	ADD_TOOLTIP(m_FileList, 100, "This contains a list of every single file and folder that will get loaded into the effects panel when a new map starts. To add a file/folder use the 'select file' or 'select folder' button to get the file/folder(s) name then add the folder with the add button.\n\nIf you make a file or have a folder with the name of a map in the maps directory then it will only load that folder/file if the current map name is the name of the folder/file.\n\nThe file and files in the folder and folders sub folders must be of the .amf extention (or else it wont load),\n\nAll the effects in the effects panel will get cleared when a level is shutdown unless the 'Should Autoload Files?' button isnt checked.", true);

	//set the bounds for each item
	PerformLayout();
}

//---------------------------------------------------------------------------------
// Purpose: Resets all the effects on this page
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::ResetEffects()
{
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::ReadFromFile(KeyValues* keyvalues, bool reset)
{
	//set the amod_effects_panel_autoload_files convar
	amod_effects_panel_autoload_files.SetValue(keyvalues->GetBool("ShouldAutoload"));

	//go through each key
	FOR_EACH_VALUE(keyvalues, file)
	{
		//must have "file" name
		if (!Q_stricmp(file->GetName(), "file"))
			m_FileList->AddFile(file->GetString());
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::WriteToFile(KeyValues* keyvalues)
{
	//get the amod_effects_panel_autoload_files convar
	keyvalues->SetBool("ShouldAutoload", amod_effects_panel_autoload_files.GetBool());

	//get the data
	CUtlVector<char*> Files;
	m_FileList->GetAutoloadFiles(Files);

	//add all the files
	for (int i = 0; i < Files.Count(); i++)
	{
		//add the key
		KeyValues* key = new KeyValues("file");
		key->SetString(nullptr, Files[i]);
		keyvalues->AddSubKey(key);

		//delete the file
		delete Files[i];
	}
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::SetFileText(const char* pszCommand)
{
	m_FileTextEntry->SetText(pszCommand);
}

//---------------------------------------------------------------------------------
// Purpose: Function to get the all the map names
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::AddMapsFilenameRecursive(const char* path, CUtlVector<char*>& MapList)
{
	//build search path (ex: "maps/*")
	char searchPath[512];
	Q_snprintf(searchPath, sizeof(searchPath), "%s/*", path);

	//find the first file
	FileFindHandle_t handle;
	const char* pFile = g_pFullFileSystem->FindFirst(searchPath, &handle);

	//loop through all files and directories
	while (pFile)
	{
		//full path to file or directory (ex: maps/episodic/d1_trainstation_01.bsp)
		char fullPath[512];
		Q_snprintf(fullPath, sizeof(fullPath), "%s/%s", path, pFile);

		//if directory → recurse into it
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			//skip "." and ".."
			if (pFile[0] != '.')
				AddMapsFilenameRecursive(fullPath, MapList);

			//move to next item
			pFile = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		//get extension position
		const char* ext = Q_strrchr(pFile, '.');

		//skip files without an extension
		if (!ext)
		{
			//move to next item
			pFile = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		//only accept .bsp files
		if (Q_stricmp(ext, ".bsp") != 0)
		{
			//move to next item
			pFile = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		//length of the name without extension
		int namelen = ext - pFile;

		//allocate memory for map name (no extension)
		char* mapname = (char*)malloc(namelen + 1);

		//copy filename without extension
		Q_memcpy(mapname, pFile, namelen);
		mapname[namelen] = 0;

		//add the map name to our list
		MapList.AddToTail(mapname);

		//move to next file
		pFile = g_pFullFileSystem->FindNext(handle);
	}

	//close search
	g_pFullFileSystem->FindClose(handle);
}

//---------------------------------------------------------------------------------
// Purpose: Loads a file
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::LoadFile(const char* filename, CUtlVector<char*>& MapList)
{
	//get the filename to compare against the current map name
	char file[1028];
	Q_StripExtension(filename, file, sizeof(file));
	const char* filename_for_map = V_GetFileName(file);

	//see if the filename is the name of a map. If so see if the current map name is that of the filename
	for (int i = 0; i < MapList.Count(); i++)
	{
		//compare the filename to the mapname at the maplist[i]
		if (!Q_strcmp(filename_for_map, MapList[i]))
		{
			//compare the filename to the current map name
			if (Q_strcmp(filename_for_map, CAutoGameSystem::MapName()))
				return;

			break;
		}
	}

	//tell the effects panel to load the file
	g_EffectsPanelInterface->LoadFile(filename);
}

//---------------------------------------------------------------------------------
// Purpose: Loads a folder
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::LoadFolder(const char* path, CUtlVector<char*>& MapList)
{
	//get the folder name without extension for map checking
	char basefolder_noext[1028];
	Q_StripExtension(V_GetFileName(path), basefolder_noext, sizeof(basefolder_noext));
	const char* basefolder_for_map = V_GetFileName(basefolder_noext);

	//see if the folder name is the name of a map. If so see if the current map name is that of the folder
	for (int i = 0; i < MapList.Count(); i++)
	{
		if (!Q_strcmp(basefolder_for_map, MapList[i]))
		{
			if (Q_strcmp(basefolder_for_map, CAutoGameSystem::MapName()))
				return;

			break;
		}
	}

	FileFindHandle_t handle;
	const char* entry = g_pFullFileSystem->FindFirst(CFmtStr("%s/*", path), &handle);

	while (entry)
	{
		if (!Q_strcmp(entry, ".") || !Q_strcmp(entry, ".."))
		{
			entry = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		char fullpath[MAX_PATH];
		Q_snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry);

		//check if entry is a directory
		if (g_pFullFileSystem->FindIsDirectory(handle))
		{
			//get folder name without extension for map checking
			char folder_noext[1028];
			Q_StripExtension(entry, folder_noext, sizeof(folder_noext));
			const char* folder_for_map = V_GetFileName(folder_noext);

			//see if the folder name is the name of a map. If so see if the current map name is that of the folder
			for (int i = 0; i < MapList.Count(); i++)
			{
				if (!Q_strcmp(folder_for_map, MapList[i]))
				{
					if (Q_strcmp(folder_for_map, CAutoGameSystem::MapName()))
						goto skip_folder;

					break;
				}
			}

			LoadFolder(fullpath, MapList);

		skip_folder:
			entry = g_pFullFileSystem->FindNext(handle);
			continue;
		}

		//load .amf files
		const char* ext = V_GetFileExtension(entry);
		if (ext && !Q_stricmp(ext, "amf"))
			LoadFile(fullpath, MapList);

		entry = g_pFullFileSystem->FindNext(handle);
	}

	g_pFullFileSystem->FindClose(handle);
}


//---------------------------------------------------------------------------------
// Purpose: Called on map load
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::OnMapLoad()
{
	//see if we do autoload
	if (!amod_effects_panel_autoload_files.GetBool())
		return;

	//get all the files
	CUtlVector<char*> Files;
	m_FileList->GetAutoloadFiles(Files);

	//check the file count. This is so we dont have to gather all the map names
	if (Files.Count() <= 0)
		return;

	//populate the map lists. This is so if we get a file/folder name with the same name as a map, then we will only load
	//that file if the current map name = the name of the file
	CUtlVector<char*> MapList;

	AddMapsFilenameRecursive("maps", MapList);

	//now go through every file and handle it
	for (int i = 0; i < Files.Count(); i++)
	{
		//see if the current item is a directory
		if (g_pFullFileSystem->IsDirectory(Files[i]))
		{
			LoadFolder(Files[i], MapList);
		}
		else if (g_pFullFileSystem->FileExists(Files[i]))
		{
			LoadFile(Files[i], MapList);
		}

		//delete the file
		delete Files[i];
	}

	//we need to call g_EffectsPanelInterface->OnTick() so every panel does what it needs to
	g_EffectsPanelInterface->CallOnTick();

	//delete all the maps
	for (int i = 0; i < MapList.Count(); i++)
		delete MapList[i];

	//clear files
	MapList.RemoveAll();
	Files.RemoveAll();
}

//---------------------------------------------------------------------------------
// Purpose: Called on map shutdown
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::OnMapShutdown()
{
	//dont reset if empty
	if (m_FileList->GetFileCount() <= 0 || !amod_effects_panel_autoload_files.GetBool())
		return;

	//tell the effects panel to reset everything
	g_EffectsPanelInterface->ResetEverything();
}

//---------------------------------------------------------------------------------
// Purpose: Called on command
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::OnCommand(const char* pszCommand)
{
	//check for add command
	if (!Q_stricmp(pszCommand, COMMAND_ADD))
	{
		//get the text from the file text entry
		char text[FILENAME_MAX] = { 0 };
		m_FileTextEntry->GetText(text, sizeof(text));

		//check to see if its empty
		if (!text[0])
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: Cant have empty file!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}

		m_FileList->AddFile(text);
	}
	
	//check for remove command
	else if (!Q_stricmp(pszCommand, COMMAND_REMOVE))
	{
		//check to see if its empty
		if (!m_FileList->RemoveSelectedFile())
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//show error message
			vgui::QueryBox* error = new vgui::QueryBox("Error", "Error: No file selected!");
			error->SetOKButtonText("Ok");
			error->SetCancelButtonVisible(false);
			error->AddActionSignalTarget(this);
			error->DoModal(dynamic_cast<vgui::PropertyDialog*>(GetParent()));
			return;
		}
	}
	
	//check for load file
	else if (!Q_stricmp(pszCommand, COMMAND_SELECT_FILE))
	{
		//delete m_FileDialog if it isnt nullptr
		if (m_FileDialog)
		{
			delete m_FileDialog;
			m_FileDialog = nullptr;
		}

		//create the dialog
		m_FileDialog = new vgui::FileOpenDialog(this, "Save Preset", true);
		m_FileDialog->AddFilter("*.amf", "Amod Filter Files (*.amf)", true);
		m_FileDialog->AddFilter("*.*", "All Files (*.*)", false);
		m_FileDialog->AddActionSignalTarget(this);

		//set the starting directory
		char buf[512];
		g_pFullFileSystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));
		m_FileDialog->SetStartDirectory(buf);

		//activate the file dialog
		m_FileDialog->DoModal(false);
		m_FileDialog->Activate();
	}
	
	//check for load folder
	else if (!Q_stricmp(pszCommand, COMMAND_SELECT_FOLDER))
	{
		//delete m_FileDialog if it isnt nullptr
		if (m_FileDialog)
		{
			delete m_FileDialog;
			m_FileDialog = nullptr;
		}

		//create the dialog
		m_FileDialog = new vgui::FileOpenDialog(this, "Save Preset", vgui::FileOpenDialogType_t::FOD_SELECT_DIRECTORY);
		m_FileDialog->AddFilter("*.amf", "Amod Filter Files (*.amf)", true);
		m_FileDialog->AddFilter("*.*", "All Files (*.*)", false);
		m_FileDialog->AddActionSignalTarget(this);

		//set the starting directory
		char buf[512];
		g_pFullFileSystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));
		m_FileDialog->SetStartDirectory(buf);

		//activate the file dialog
		m_FileDialog->DoModal(false);
		m_FileDialog->Activate();
	}

	//default
	else
	{
		BaseClass::OnCommand(pszCommand);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called when a file/folder is selected
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::OnFileSelected(const char* pszFileName)
{
	//check the relative path
	char buf[512];
	filesystem->FullPathToRelativePath(pszFileName, buf, sizeof(buf));
	
	if (Q_strrchr(buf, ':'))
		m_FileTextEntry->SetText(pszFileName);
	else
		m_FileTextEntry->SetText(buf);
}

//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelSettingsPage::PerformLayout()
{
	//set stuff
	m_FileList->SetBounds(5, 10, EFFECTS_PAGE_WIDTH, 275);
	m_FileLabel->SetBounds(5, 286, EFFECTS_PAGE_WIDTH, 22);
	m_FileTextEntry->SetBounds(5, 310, EFFECTS_PAGE_WIDTH, 22);
	m_SelectFileButton->SetBounds(5, 337, EFFECTS_PAGE_WIDTH / 2 - 2, 22);
	m_SelectFolderButton->SetBounds(EFFECTS_PAGE_WIDTH / 2 + 8, 337, EFFECTS_PAGE_WIDTH / 2 - 2, 22);
	m_AddToListButton->SetBounds(5, 364, EFFECTS_PAGE_WIDTH / 2 - 2, 22);
	m_RemoveButton->SetBounds(EFFECTS_PAGE_WIDTH / 2 + 8, 364, EFFECTS_PAGE_WIDTH / 2 - 2, 22);

	BaseClass::PerformLayout();
}
