#include "cbase.h"
#include "INewGamePanel.h"
#include "filesystem.h"
#include "fmtstr.h"
#include "AloneMod/Amod_SharedDefs.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/QueryBox.h"
#include "AloneMod/IOptionsPanel.h"
#include "AloneMod/DynamicSky.h"
#include "vgui/ILocalize.h"
#include "ienginevgui.h"

//new game panel game list struct
struct GameListInfo_t
{
	//prefix and name
	char name[128];
	char prefix[32];
	int CurrentChapterIndex = 0;
	CUtlVector<const char*> ChapterNames;

	//has themes? ALSO OBSOLETE
	bool UsesThemes = true;

	//chapter image info.		OBSOLETE
	struct ChapterImageInfo_t
	{
		bool allowDay;		//show day
		bool allowNight;	//show night

		int min;			//min/start chapter
		int max;			//max/end chapter
	};
	CUtlVector<ChapterImageInfo_t> m_ChapterImageInfo;
};

#define COMMAND_CHAPTER_1 COMMAND_CHAPTER_PREFIX "1"
#define COMMAND_CHAPTER_2 COMMAND_CHAPTER_PREFIX "2"
#define COMMAND_CHAPTER_3 COMMAND_CHAPTER_PREFIX "3"
#define COMMAND_CHAPTER_PREFIX "LoadChapter"
#define COMMAND_PREV "PrevChapter"
#define COMMAND_NEXT "NextChapter"

#define MAX_CHAPTERS 999

//new game panel
class CNewGamePanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CNewGamePanel, vgui::Frame)
public:
	CNewGamePanel(vgui::VPANEL parent);
	~CNewGamePanel();

	//initalizes the elements
	void InitElements();
	void InitThemesComboBox();		//init the themes combo box

	//map functions
	void LoadNewGameInfo(const char* filename);
	void ActivateGame(int gameindex = 0);
	void SelectPage(int page);

	//other panel funcs
	void OnCommand(const char* pszCommand);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
private:
	friend class CNewGamePanelInterface;

	GameListInfo_t* m_CurrentSelectedGameInfo;
	CUtlVector<GameListInfo_t> m_GameInfo;		//array of game info
	int m_SelectedGameIndex = 0;				//the selected game index

	//chapter texts
	vgui::Label* m_ChapterNames[3];
	
	//chapter images
	vgui::ImagePanel* m_ChapterImages[3];

	//chapter buttons
	vgui::Button* m_ChapterButtons[3];

	//previous and next chapter
	vgui::Button* m_NextButton;
	vgui::Button* m_PrevButton;

	//game combo box
	vgui::ComboBox* m_GameComboBox;

	//the themes combo box
	vgui::ComboBox* m_ThemesComboBox;

	//m_bShouldShowChangeModal
	bool m_bShouldShowChangeModal = true;
};

//-----------------------------------------------------------------------
// Purpose: Returns the map name from the config file
//-----------------------------------------------------------------------
bool GetMapFromConfig(const char* input, char* output, int outlen)
{
	//open file
	FileHandle_t fh = filesystem->Open(input, "r", "MOD");
	if (fh)
	{
		char buffer[1024];
		while (filesystem->ReadLine(buffer, sizeof(buffer), fh))
		{
			//check for 'map' first
			char* p = Q_strstr(buffer, "map ");
			if (!p)
				continue;

			p = p + Q_strlen("map ");

			//eat white spaces
			while (*p && isspace((unsigned char)*p))
				p++;

			//check p
			if (!*p)
				continue;

			//remove \r\n or just \n
			char* tmp = p;
			while (*p)
			{
				if (*p == '\r' || *p == '\n')
				{
					*p = '\0';
					break;
				}

				p++;
			}


			//copy the map name
			Q_strncpy(output, tmp, outlen);
			return true;
		}

		filesystem->Close(fh);
	}

	return false;
}

//previous file
static KeyValues* previousfile = nullptr;
static char previousefilemod[512];

//-----------------------------------------------------------------------
// Purpose: This function returns the day/night 'allowed' states for the map selector panel.
//-----------------------------------------------------------------------
bool GetMapDayEnabledStateFromTimeinfoFromFile(const char* mapname, bool& allowDaytime, bool& allowNighttime, const char* modname)
{
	//check the current mod is amod_timeinfo_load_mod.
	extern ConVar amod_timeinfo_load_mod;
	if (!Q_stricmp(modname, amod_timeinfo_load_mod.GetString()))
	{
		MapTimeInfo_t& info = GetMapTimeInfo(mapname);
		allowDaytime = info.AllowDaytime;
		allowNighttime = info.AllowNightTime;
		return true;
	}

	//check inside previousfile
	if (previousfile && !Q_stricmp(previousefilemod, modname))
	{
		KeyValues* find = previousfile->FindKey(mapname);
		if (find)
		{
			allowDaytime = find->GetBool("AllowDayTime");
			allowNighttime = find->GetBool("AllowNightTime");
			return true;
		}
	}

	//delete previousfile
	previousfile->deleteThis();
	previousfile = nullptr;
	previousefilemod[0] = '\0';

	//get the mod folder
	char modfolder[512];
	Q_strncpy(modfolder, CFmtStr("resource/time_info%s%s", modname[0] ? "/" : "", modname), sizeof(modfolder));

	//go through all files in the amod_timeinfo_load_directory.GetString()/* directory
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst(CFmtStr("%s/*.txt", modfolder), &handle);
	while (firstfile)
	{
		//dont read the . filenames
		if (!Q_stricmp(firstfile, ".") || !Q_stricmp(firstfile, ".."))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//load the file
		if (filesystem->FindIsDirectory(handle) || !strchr(firstfile, '.'))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//load the file
		previousfile = new KeyValues("TimeInfo");
		if (previousfile->LoadFromFile(filesystem, CFmtStr("%s/%s", modfolder, firstfile)))
		{
			KeyValues* find = previousfile->FindKey(mapname);
			if (find)
			{
				allowDaytime = find->GetBool("AllowDayTime");
				allowNighttime = find->GetBool("AllowNightTime");
				Q_strncpy(previousefilemod, modname, sizeof(previousefilemod));
				break;
			}
		}

		//delete previousfile
		previousfile->deleteThis();
		previousfile = nullptr;

		firstfile = filesystem->FindNext(handle);
	}

	filesystem->FindClose(handle);
	return true;
}

//-----------------------------------------------------------------------
// Purpose: Constructor for the new game panel
//-----------------------------------------------------------------------
CNewGamePanel::CNewGamePanel(vgui::VPANEL parent) : BaseClass(nullptr, "NewGamePanel")
{
	//set parent
	SetParent(parent);

	//set other panel stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	//set title
	SetTitle("#Amod_NewGamePanel_Title", false);
	SetSize(520, 215);
	MoveToCenterOfScreen();

	//init the elements
	InitElements();

	//invalidate
	m_SelectedGameIndex = -1;
	m_CurrentSelectedGameInfo = nullptr;

	//load all the new game lists in the games/* folder BUT always load resource/gamelist.txt first
	LoadNewGameInfo("resource/gamelist.txt");

	{
		FileFindHandle_t handle;
		const char* firstfile = filesystem->FindFirst("resource/games/*.txt", &handle);
		while (firstfile)
		{
			//dont read the . filenames
			if (!Q_stricmp(firstfile, ".") || !Q_stricmp(firstfile, ".."))
			{
				firstfile = filesystem->FindNext(handle);
				continue;
			}

			//see if its a file or not
			if (!filesystem->FindIsDirectory(handle) && strchr(firstfile, '.'))
				LoadNewGameInfo(CFmtStr("resource/games/%s", firstfile));

			firstfile = filesystem->FindNext(handle);
		}

		filesystem->FindClose(handle);
	}

	//select the current game
	m_GameComboBox->ActivateItem(m_SelectedGameIndex);
	ActivateGame(m_SelectedGameIndex);

	//set m_bShouldShowChangeModal
	m_bShouldShowChangeModal = false;
}

//-----------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------
CNewGamePanel::~CNewGamePanel()
{
	//delete all the game info
	for (int g = 0; g < m_GameInfo.Count(); g++)
	{
		for (int c = 0; c < m_GameInfo[g].ChapterNames.Count(); c++)
			free((void*)m_GameInfo[g].ChapterNames[c]);
	}
}

//-----------------------------------------------------------------------
// Purpose: Initalizes the elements
//-----------------------------------------------------------------------
void CNewGamePanel::InitElements()
{
	//create the 3 chapter labels
	m_ChapterNames[0] = new vgui::Label(this, "Chapter1Label", "");
	m_ChapterNames[0]->SetBounds(10, 20, 170, 20);

	m_ChapterNames[1] = new vgui::Label(this, "Chapter2Label", "");
	m_ChapterNames[1]->SetBounds(180, 20, 165, 20);

	m_ChapterNames[2] = new vgui::Label(this, "Chapter3Label", "");
	m_ChapterNames[2]->SetBounds(350, 20, 165, 20);

	//create the images
	m_ChapterImages[0] = new vgui::ImagePanel(this, "Chapter1Image");
	m_ChapterImages[0]->SetBounds(10, 40, 160, 90);

	m_ChapterImages[1] = new vgui::ImagePanel(this, "Chapter2Image");
	m_ChapterImages[1]->SetBounds(180, 40, 160, 90);

	m_ChapterImages[2] = new vgui::ImagePanel(this, "Chapter3Image");
	m_ChapterImages[2]->SetBounds(350, 40, 160, 90);

	//create the buttons
	m_ChapterButtons[0] = new vgui::Button(this, "Chapter1Button", "Load Chapter 1");
	m_ChapterButtons[0]->SetBounds(10, 135, 160, 20);
	m_ChapterButtons[0]->SetCommand(COMMAND_CHAPTER_1);

	m_ChapterButtons[1] = new vgui::Button(this, "Chapter2Button", "Load Chapter 2");
	m_ChapterButtons[1]->SetBounds(180, 135, 160, 20);
	m_ChapterButtons[1]->SetCommand(COMMAND_CHAPTER_2);

	m_ChapterButtons[2] = new vgui::Button(this, "Chapter3Button", "Load Chapter 3");
	m_ChapterButtons[2]->SetBounds(350, 135, 160, 20);
	m_ChapterButtons[2]->SetCommand(COMMAND_CHAPTER_3);

	//create the next and previous button
	m_PrevButton = new vgui::Button(this, "PreviousButton", "#Amod_NewGamePanel_Previous");
	m_PrevButton->SetBounds(10, 185, 80, 25);
	m_PrevButton->SetCommand(COMMAND_PREV);
	m_PrevButton->SetEnabled(false);

	m_NextButton = new vgui::Button(this, "NextButton", "#Amod_NewGamePanel_Next");
	m_NextButton->SetBounds(430, 185, 80, 25);
	m_NextButton->SetCommand(COMMAND_NEXT);
	m_NextButton->SetEnabled(false);

	//make the game combo box
	m_GameComboBox = new vgui::ComboBox(this, "GameComboBox", 10, false);
	m_GameComboBox->SetBounds(100, 160, 320, 25);

	//make the themes combo box
	m_ThemesComboBox = new vgui::ComboBox(this, "ThemesComboBox", 10, false);
	m_ThemesComboBox->SetBounds(100, 185, 320, 25);

	InitThemesComboBox();
}

//-----------------------------------------------------------------------
// Purpose: Initalizes the themes combo box
//-----------------------------------------------------------------------
void CNewGamePanel::InitThemesComboBox()
{
	//clear the combo box
	m_ThemesComboBox->RemoveAll();

	//ALWAYS load the default theme
	wchar_t* themetext = g_pVGuiLocalize->Find("Amod_NewGamePanel_ThemeText");
	if (!themetext) themetext = L"THEME";

	m_ThemesComboBox->AddItem("#Amod_NewGamePanel_DefaultThemeText", new KeyValues(""));

	//theme convar
	extern ConVar amod_timeinfo_load_mod;

	//active item
	int index = 0, curr = 1;

	//go through all files in the time_info/* directory and gather each folder
	FileFindHandle_t handle;
	const char* firstfile = filesystem->FindFirst("resource/time_info/*", &handle);
	while (firstfile)
	{
		//dont read the . filenames
		if (!Q_stricmp(firstfile, ".") || !Q_stricmp(firstfile, ".."))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//must be a directory.
		if (!filesystem->FindIsDirectory(handle))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//ignore _IGNORE directory
		if (!Q_stricmp(firstfile, "_IGNORE"))
		{
			firstfile = filesystem->FindNext(handle);
			continue;
		}

		//check the amod_time_properties_load_mod with the previous text
		if (!Q_stricmp(amod_timeinfo_load_mod.GetString(), firstfile))
			index = curr;

		//get the theme text
		wchar_t wchar_file[128];
		g_pVGuiLocalize->ConvertANSIToUnicode(firstfile, wchar_file, sizeof(wchar_file));

		wchar_t theme[512];
		swprintf(theme, SIZE_OF_ARRAY(theme), L"%ws %ws", themetext, wchar_file);

		//add the item
		m_ThemesComboBox->AddItem(theme, new KeyValues(firstfile));
		firstfile = filesystem->FindNext(handle);
		curr++;
	}

	m_ThemesComboBox->ActivateItem(index);
}

//-----------------------------------------------------------------------
// Purpose: Loads the new game info
//-----------------------------------------------------------------------
void CNewGamePanel::LoadNewGameInfo(const char* filename)
{
	//load the file
	KeyValues* gamelist = new KeyValues("gamelist");
	if (!gamelist->LoadFromFile(filesystem, filename, "MOD"))
	{
		ConWarning("Error: Failed to load the gamelist for the new game panel!\n%s\n", filename);
		gamelist->deleteThis();
		return;
	}

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(gamelist, game)
	{
		//load the resource file. I could call this before the for loop but cant be bothered changing it. This runs once on startup so it shouldnt be an issue
		const char* resource = game->GetString("resource", "resource/HL2_AloneMod_english.txt");
		KeyValues* AloneModEnglish = new  KeyValues("AloneModEnglish");
		if (!AloneModEnglish->LoadFromFile(filesystem, resource, "MOD"))
		{
			ConWarning("Error: Failed to load the '%s' file for the new game panel!\n%s->%s\n", resource, filename, game->GetName());
			AloneModEnglish->deleteThis();
			continue;
		}

		//get the tokens
		KeyValues* tokens = AloneModEnglish->FindKey("Tokens");
		if (!tokens)
		{
			ConWarning("Error: Failed to find the 'Tokens' key for %s for the new game panel!\n%s\n", filename);
			AloneModEnglish->deleteThis();
			continue;
		}

		//check for default
		if (game->GetBool("Default") && m_SelectedGameIndex == -1)
			m_SelectedGameIndex = m_GameInfo.Count();

		//get the game info
		GameListInfo_t& info = m_GameInfo[m_GameInfo.AddToTail()];
		Q_strncpy(info.name, game->GetName(), sizeof(info.name));
		Q_strncpy(info.prefix, game->GetString("Prefix"), sizeof(info.prefix));

		//does this have themes?
		info.UsesThemes = game->GetBool("HasThemes", true);

		//get the chapter names
		int i = 1;
		while (true)
		{
			//check for the string
			const char* chapter = tokens->GetString(CFmtStr("%s_Chapter%d_Title", info.prefix, i++), nullptr);
			if (!chapter)
				break;

			info.ChapterNames.AddToTail(_strdup(chapter));
		}

		//get the chapter image info. OBSOLETE
		//KeyValues* ChapterImageInfo = game->FindKey("ChapterImageInfo");
		//if (!ChapterImageInfo)
		//{
		//	AloneModEnglish->deleteThis();
		//	continue;
		//}
		//
		//FOR_EACH_TRUE_SUBKEY(ChapterImageInfo, _info)
		//{
		//	//add the info
		//	GameListInfo_t::ChapterImageInfo_t& chapter_info = info.m_ChapterImageInfo[info.m_ChapterImageInfo.AddToTail()];
		//	chapter_info.min = _info->GetInt("StartChapter");
		//	chapter_info.max = _info->GetInt("EndChapter");
		//	chapter_info.allowDay = _info->GetBool("AllowShowDay");
		//	chapter_info.allowNight = _info->GetBool("AllowShowNight");
		//}

		//delete the alone mod resource config
		AloneModEnglish->deleteThis();
	}

	//check our index
	if (m_SelectedGameIndex == -1 && m_GameInfo.Count() > 0)
		m_SelectedGameIndex = 0;

	//clear m_GameComboBox
	m_GameComboBox->RemoveAll();

	//add the games to the combo box
	for (int i = 0; i < m_GameInfo.Count(); i++)
		m_GameComboBox->AddItem(CFmtStr("Game: %s", m_GameInfo[i].name), nullptr);

	//delete the configs
	gamelist->deleteThis();
}

//-----------------------------------------------------------------------
// Purpose: Activates the chapter
//-----------------------------------------------------------------------
void CNewGamePanel::ActivateGame(int gameindex)
{
	//reset the chapter index
	m_SelectedGameIndex = gameindex;

	//get the info
	if (gameindex >= m_GameInfo.Count() || gameindex < 0)
	{
		ConWarning("Error: Got invalid index: %d for the NewGamePanel::ActivateGame(int);\n", gameindex);
		return;
	}

	//select the new game
	m_CurrentSelectedGameInfo = &m_GameInfo[gameindex];
	SelectPage(m_CurrentSelectedGameInfo->CurrentChapterIndex);
}

//-----------------------------------------------------------------------
// Purpose: Sets the panel to the next page
//-----------------------------------------------------------------------
void CNewGamePanel::SelectPage(int page)
{
	//the actuall index should be page * 3;
	int index = page * 3;

	//check the chapters
	if (!m_CurrentSelectedGameInfo || index >= m_CurrentSelectedGameInfo->ChapterNames.Count() || index < 0)
		return;

	//set the new index
	m_CurrentSelectedGameInfo->CurrentChapterIndex = page;

	//hide all the elements first
	for (int i = 0; i < 3; i++)
	{
		m_ChapterNames[i]->SetVisible(false);
		m_ChapterImages[i]->SetVisible(false);
		m_ChapterButtons[i]->SetVisible(false);
	}

	//go through all the elements
	for (int i = 0; i < 3; i++)
	{
		//real correct index
		int realindex = index + i + 1;

		//bounds check
		if (index + i >= m_CurrentSelectedGameInfo->ChapterNames.Count())
			break;

		//set the text + image + button text
		m_ChapterNames[i]->SetText(m_CurrentSelectedGameInfo->ChapterNames[index + i]);

		//get the chapter text
		{
			wchar_t* chaptertext = g_pVGuiLocalize->Find("Amod_NewGamePanel_LoadChapter");
			wchar_t buf[128];
			swprintf(buf, SIZE_OF_ARRAY(buf), L"%ws %d", chaptertext, realindex);

			m_ChapterButtons[i]->SetText(buf);
		}

		//get the suffex 
		extern ConVar amod_day;
		bool daytime = amod_day.GetBool();
		const char* sDayTime = daytime ? "_day" : "";

		//check we allow for daytime modes
		char output[64] = { '\0' };



		//check if m_ThemesComboBox->GetActiveItemUserData()->GetName() is not amod_timeinfo_load_mod.GetString()
		bool succeed = true;
		extern ConVar amod_timeinfo_load_mod;

		if (Q_stricmp(m_ThemesComboBox->GetActiveItemUserData()->GetName(), amod_timeinfo_load_mod.GetString()))
		{
			succeed = GetMapFromConfig(CFmtStr("cfg/%s/chapter%d.cfg", m_CurrentSelectedGameInfo->prefix, realindex), output, sizeof(output));
		}

		if (succeed)
		{
			//get the time enabled state from the file
			bool allowDaytime = true;
			bool allowNighttime = true;
			if (GetMapDayEnabledStateFromTimeinfoFromFile(output, allowDaytime, allowNighttime, m_ThemesComboBox->GetActiveItemUserData()->GetName()))
			{
				//check info
				if (!allowDaytime || !allowNighttime && !(allowDaytime && allowNighttime))
					sDayTime = allowDaytime ? "_day" : "";
			}
		}

		//check the chapter image info
		//for (int j = 0; j < m_CurrentSelectedGameInfo->m_ChapterImageInfo.Count(); j++)
		//{
		//	//check the info
		//	GameListInfo_t::ChapterImageInfo_t& info = m_CurrentSelectedGameInfo->m_ChapterImageInfo[j];
		//
		//	if ((realindex >= info.min && realindex <= info.max))
		//	{
		//		//check to see if both day and night are disabled. If so then just do what it would do if both were enabled
		//		if (!info.allowDay && !info.allowNight)
		//			continue;
		//	
		//		if (!info.allowDay)
		//			sDayTime = "";
		//		else
		//			sDayTime = "_day";
		//	
		//		break;
		//	}
		//}




		//see if an image exists with the chapters/(FE)/%s/chapters%d%s name where (FE) is just V_GetFileName(m_ThemesComboBox->GetActiveItemUserData()->GetName()).
		char buf[512] = { 0 };

		if (m_ThemesComboBox->GetActiveItem() != 0 && m_CurrentSelectedGameInfo->UsesThemes)
			Q_snprintf(buf, sizeof(buf), "chapters/%s/%s/chapter%d%s", V_GetFileName(m_ThemesComboBox->GetActiveItemUserData()->GetName()), m_CurrentSelectedGameInfo->prefix, realindex, sDayTime);

		//resort to default filepath
		if (!m_CurrentSelectedGameInfo->UsesThemes || m_ThemesComboBox->GetActiveItem() == 0 || IsErrorMaterial(materials->FindMaterial(CFmtStr("vgui/%s", buf), TEXTURE_GROUP_VGUI, false)))
			Q_snprintf(buf, sizeof(buf), "chapters/default/%s/chapter%d%s", m_CurrentSelectedGameInfo->prefix, realindex, sDayTime);

		m_ChapterImages[i]->SetImage(buf);

		//set visibility
		m_ChapterNames[i]->SetVisible(true);
		m_ChapterButtons[i]->SetVisible(true);
		m_ChapterImages[i]->SetVisible(true);
	}

	m_PrevButton->SetEnabled(true);
	m_NextButton->SetEnabled(true);

	//check for the buttons
	if (m_CurrentSelectedGameInfo->CurrentChapterIndex <= 0)
		m_PrevButton->SetEnabled(false);
	if (!m_ChapterButtons[2]->IsVisible() || index + 3 >= m_CurrentSelectedGameInfo->ChapterNames.Count())
		m_NextButton->SetEnabled(false);

	//disable the themes combo box if needed
	if (!m_CurrentSelectedGameInfo->UsesThemes)
	{
		m_ThemesComboBox->SetEnabled(false);
		m_ThemesComboBox->ActivateItem(0);
	}
	else
		m_ThemesComboBox->SetEnabled(true);
}

#define COMMAND_CHANGE_THEME "ConfirmThemeChange"
#define COMMAND_CHANGE_THEME_CONFIRM "ConfirmThemeChangeConfirm"

//-----------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------
void CNewGamePanel::OnCommand(const char* pszCommand)
{
	//check for next or previous command
	if (!Q_stricmp(pszCommand, COMMAND_NEXT))
	{
		SelectPage(m_CurrentSelectedGameInfo->CurrentChapterIndex + 1);
		return;
	}

	else if (!Q_stricmp(pszCommand, COMMAND_PREV))
	{
		SelectPage(m_CurrentSelectedGameInfo->CurrentChapterIndex - 1);
		return;
	}

	//check for COMMAND_CHAPTER_PREFIX
	else if (!Q_strnicmp(pszCommand, COMMAND_CHAPTER_PREFIX, Q_strlen(COMMAND_CHAPTER_PREFIX)))
	{
		//load the theme
		extern ConVar amod_timeinfo_load_mod;

		//store the previous amod_timeinfo_load_mod value
		char prev[128];
		Q_strncpy(prev, amod_timeinfo_load_mod.GetString(), sizeof(prev));

		//check for UsesThemes
		if (m_CurrentSelectedGameInfo->UsesThemes && m_ThemesComboBox->GetActiveItem() != 0)
			amod_timeinfo_load_mod.SetValue(m_ThemesComboBox->GetActiveItemUserData()->GetName());
		else
			amod_timeinfo_load_mod.SetValue("");

		//compare and see if we need to reload
		if (Q_stricmp(prev, amod_timeinfo_load_mod.GetString()))
			engine->ClientCmd("amod_timeinfo_reset_noreload");

		//get and exec the map
		int index = Q_atoi(pszCommand + Q_strlen(COMMAND_CHAPTER_PREFIX));
		engine->ClientCmd(CFmtStr("exec \"%s/chapter%d", m_CurrentSelectedGameInfo->prefix, (m_CurrentSelectedGameInfo->CurrentChapterIndex * 3) + index));
		return;
	}

	//check for CONFIRM_THEME_CHANGE_COMMAND
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_THEME))
	{
		extern ConVar amod_timeinfo_load_mod;

		//ask if the user wants to reload the theme now
		if (Q_stricmp(amod_timeinfo_load_mod.GetString(), m_ThemesComboBox->GetActiveItemUserData()->GetName()))
		{
			//show the modal
			QueryBox* modal = new QueryBox("#Amod_NewGamePanel_ReloadThemeNow_Title", "#Amod_NewGamePanel_ReloadThemeNow_Desc", this);
			modal->MoveToCenterOfScreen();
			modal->Activate();
			modal->DoModal();
			modal->SetOKCommand(new KeyValues("Command", "command", COMMAND_CHANGE_THEME_CONFIRM));
			modal->AddActionSignalTarget(this);
		}

		return;
	}

	//check for COMMAND_CHANGE_THEME_CONFIRM
	else if (!Q_stricmp(pszCommand, COMMAND_CHANGE_THEME_CONFIRM))
	{
		//set our theme
		//load the theme
		extern ConVar amod_timeinfo_load_mod;

		//store the previous amod_timeinfo_load_mod value
		char prev[128];
		Q_strncpy(prev, amod_timeinfo_load_mod.GetString(), sizeof(prev));

		//check for UsesThemes
		amod_timeinfo_load_mod.SetValue(m_ThemesComboBox->GetActiveItemUserData()->GetName());

		//compare and see if we need to reload
		if (Q_stricmp(prev, amod_timeinfo_load_mod.GetString()))
			engine->ClientCmd("amod_timeinfo_reset");

		return;
	}
	
	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Called on text changed
//-----------------------------------------------------------------------------
void CNewGamePanel::OnTextChanged(KeyValues* kv)
{
	//check the panel
	if (kv->GetPtr("Panel") == m_GameComboBox)
	{
		//remove this panels focus IMMEDIATLY so when we close the newgamepanel it doesnt reset the combo box
		m_NextButton->RequestFocus();

		ActivateGame(m_GameComboBox->GetActiveItem());
		return;
	}

	//check for the themes combo box. This is because if we change the theme. There is a chance
	//the theme uses its own custom chapter images.
	else if (kv->GetPtr("Panel") == m_ThemesComboBox)
	{
		//select the active page to reset the theme
		if (m_CurrentSelectedGameInfo)
			SelectPage(m_CurrentSelectedGameInfo->CurrentChapterIndex);

		//remove this panels focus IMMEDIATLY so when we close the newgamepanel it doesnt reset the combo box
		m_NextButton->RequestFocus();

		//hack
		if (!m_bShouldShowChangeModal)
		{
			m_bShouldShowChangeModal = true;
			return;
		}

		//check for g_bHasTimePropertiesPanelBeenOpen
		extern bool g_bHasTimePropertiesPanelBeenOpen;
		static bool bShown = false;
		if (g_bHasTimePropertiesPanelBeenOpen && !bShown)
		{
			bShown = true;

			//show the modal
			QueryBox* modal = new QueryBox("#Amod_NewGamePanel_TimePropertiesWarning_Title", "#Amod_NewGamePanel_TimePropertiesWarning_Desc", this);
			modal->MoveToCenterOfScreen();
			modal->Activate();
			modal->DoModal();
			modal->SetCancelCommand(new KeyValues("Command", "Command", COMMAND_CHANGE_THEME));
			modal->SetOKCommand(new KeyValues("Command", "Command", COMMAND_CHANGE_THEME));
			return;
		}

		OnCommand(COMMAND_CHANGE_THEME);
		return;
	}

	//select the new game
}





//new game panel interface
class CNewGamePanelInterface : public INewGamePanel
{
public:
	//creates the new game panel
	virtual void Create(vgui::VPANEL parent)
	{
		if (m_NewGamePanel)
			return;

		m_NewGamePanel = new CNewGamePanel(parent);
		m_NewGamePanel->SetVisible(false);
	}

	//destroys the new game panel
	virtual void Activate(void)
	{
		if (m_NewGamePanel)
		{
			m_NewGamePanel->Activate();
			m_NewGamePanel->MoveToCenterOfScreen();

			//reset the themes combo box
			m_NewGamePanel->InitThemesComboBox();
		}
	}

	//does a day check
	virtual void DoDayCheck(void)
	{
		if (m_NewGamePanel && m_NewGamePanel->m_CurrentSelectedGameInfo)
			m_NewGamePanel->SelectPage(m_NewGamePanel->m_CurrentSelectedGameInfo->CurrentChapterIndex);
	}

	//destroys the new game panel
	virtual void Destroy(void)
	{
		if (m_NewGamePanel)
			delete m_NewGamePanel;

		m_NewGamePanel = nullptr;
	}
private:
	CNewGamePanel* m_NewGamePanel = nullptr;
};

//new game panel interface singleton
static CNewGamePanelInterface g_sNewGamePanelInterface;
INewGamePanel* newgamepanel = &g_sNewGamePanelInterface;

//-----------------------------------------------------------------------
// Purpose: Opens the new game panel
//-----------------------------------------------------------------------
CON_COMMAND(togglenewgamepanel, "Toggles the alone mod new game panel")
{
	g_sNewGamePanelInterface.Activate();
}

//-----------------------------------------------------------------------
// Purpose: Debug to reset the new game panel
//-----------------------------------------------------------------------
CON_COMMAND(amod_reset_newgamepanel, "Resets the new game panel for debug purposes")
{
	g_sNewGamePanelInterface.Destroy();
	g_sNewGamePanelInterface.Create(enginevgui->GetPanel(VGuiPanel_t::PANEL_GAMEUIDLL));
	g_sNewGamePanelInterface.Activate();
}

#if FOG_CUBE_TRIGGER_TEST
//bool for time info triggers
extern bool g_bJustStartedLevel;
#endif

//-----------------------------------------------------------------------
// Purpose: Alone mod day change callbacks
//-----------------------------------------------------------------------
void AmodDayChangeCallback(IConVar* var, const char* olds, float oldf)
{
#if FOG_CUBE_TRIGGER_TEST
	//reset the cube trigger active states
	void ResetCubeTriggerData();
	ResetCubeTriggerData();
#endif

	//reset the skybox panel
	skyboxpanel->ResetPanel();

	//check for daytime for the new game panel
	g_sNewGamePanelInterface.DoDayCheck();

	//check the old value
	if (ConVarRef(var).GetBool() == (bool)oldf || !engine->IsConnected())
		return;

#if USES_DYNAMIC_SKY
	if (g_PModBase_DynamicSkybox_bUse)
	{
		engine->ClientCmd("_amod_day_do");
		return;
	}
#endif

	//reload map if needed
	if (engine->IsInGame() && engine->IsLevelMainMenuBackground())
	{
		engine->ClientCmd(CFmtStr("map_background %s", szMapName));
	}
	else
	{
		engine->ClientCmd("save __tmpquick01; load __tmpquick01");
	}

}






















//new game editor panel. This code below is VERY rushed so its pretty garbage but it works and i need to release the next update very soon.
// 
// 
//EDIT: THIS IS NOW AN OBSOLETE PANEL. USED TO BE USED TO MODIFY THE CHAPTER TIME INFO DATA





















#include <vgui_controls/ListPanel.h>
#include <vgui_controls/TreeView.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Slider.h>
#include "effects panel/EffectsPanel.h"

//game editor struct
struct GameListInfoEditor_t
{
	//prefix and name
	char name[128];
	char prefix[32];

	//resource filename
	char resourceFilename[256];

	//resoruce strings
	CUtlVector<char*> resourceStrings;

	//has themes?
	bool UsesThemes = true;
	bool isDefault = true;

	//chapter image info
	struct ChapterImageInfo_t
	{
		char name[128];		//name of this info

		bool allowDay;		//show day
		bool allowNight;	//show night

		int min;			//min/start chapter
		int max;			//max/end chapter
	};
	CUtlVector<ChapterImageInfo_t> m_ChapterImageInfo;
};

//game editor base struct
struct GameListInfoEditorBase_t
{
	char filename[256];
	CUtlVector<GameListInfoEditor_t> info;
};

//tree view node type
enum class TreeViewNodeClass_e {
	Node_Name,
	Node_AllowDay,
	Node_AllowNight,
	Node_StartChapter,
	Node_EndChapter,
};



#define DATA_CHECK_BUTTON_COMMAND "DataCheckButton"
#define DATA_ADD_COMMAND "AddChapterInfoCommand"
#define DATA_REMOVE_COMMAND "RemoveChapterInfoCommand"
#define SAVE_NEW_GAME_PANEL_COMMAND "SaveConfigSettings"
#define RELOAD_NEW_GAME_PANEL_COMMAND "ReloadNewGamePanel"
#define HAS_THEMES_COMMAND "HasThemesCommand"

//new game editor panel
class CNewGameEditorPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CNewGameEditorPanel, Frame)
public:
	CNewGameEditorPanel();
	~CNewGameEditorPanel();

	//loads a new game info
	void LoadNewGameInfo(const char* filename);

	//command funcs
	void OnCommand(const char* pszCommand);

	//called on think
	void PerformLayout();
	void OnThink();

	//add
	int AddGameInfoToTreeView(GameListInfoEditor_t::ChapterImageInfo_t& item, GameListInfoEditor_t* info, int root);

	//called when an item is selected
	MESSAGE_FUNC_PARAMS(OnItemSelected, "ItemSelected", data);
	MESSAGE_FUNC_PARAMS(OnItemDeselected, "ItemDeselected", data);
	MESSAGE_FUNC_PARAMS(OnSliderValueChanged, "SliderMoved", data);
	MESSAGE_FUNC_PARAMS(OnTreeViewItemSelected, "TreeViewItemSelected", data);
	MESSAGE_FUNC_PARAMS(OnTreeViewItemDeselected, "TreeViewItemSelectionCleared", data);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);
private:
	//game lists
	vgui::ListPanel* m_GameList;

	//game chapter image info tree view
	vgui::TreeView* m_ChapterImageInfo;

	//bottom data label + others
	Label* m_DataLabel;

	TextEntry* m_DataTextEntry;
	CheckButton* m_DataCheckButton;
	WheelSlider* m_DataSlider;

	//add + remove button
	Button* m_AddButton, *m_RemoveButton;

	//save + reload new game panel
	Button* m_SaveButton, *m_ResetNewGamePanelButton;

	//allow themes check button
	CheckButton* m_HasThemesCheckButton;

	//the base file data
	CUtlVector<GameListInfoEditorBase_t> m_GameListInfo;

	//the file data used for the game list + reading + writing
	CUtlVector<GameListInfoEditor_t*> m_ReadWriteGameListInfo;

	//current selected range's new game image
	ImagePanel* m_CurrentNewGameImage, *m_CurrentNewGameImageDay;

	//current selected file data + file mode + others
	GameListInfoEditor_t* m_CurrentSelectedList = nullptr;
	GameListInfoEditor_t::ChapterImageInfo_t* m_CurrentSelectedData = nullptr;
	TreeViewNodeClass_e m_CurrentSelectedType;
	int m_CurrentNodeIndex = 0;
};

//singleton
static CNewGameEditorPanel* s_NewGameEditor = nullptr;

//----------------------------------------------------------------------------------------------------
// Purpose: Constructor for the new game editor panel
//----------------------------------------------------------------------------------------------------
CNewGameEditorPanel::CNewGameEditorPanel() : BaseClass(nullptr, "CNewGameEditorPanel")
{
	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_GAMEUIDLL));

	//set our stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	//set title
	SetTitle("New Game Editor Panel", false);
	SetSize(450, 600);
	MoveToCenterOfScreen();

	//make the list panel
	m_GameList = new vgui::ListPanel(this, "ListView");
	m_GameList->SetKeyBoardInputEnabled(false);
	m_GameList->AddColumnHeader(0, "Game", "Game", m_GameList->GetWide(), 0);
	m_GameList->SetMultiselectEnabled(false);
	m_GameList->SetColumnSortable(0, false);

	//create the chapter image info
	m_ChapterImageInfo = new vgui::TreeView(this, "ChapterImageInfo");
	m_ChapterImageInfo->MakeReadyForUse();
	m_ChapterImageInfo->SetAllowMultipleSelections(false);		//well ur just fucking useless
	m_ChapterImageInfo->SetMultipleItemDragEnabled(false);		//so are you cunt

	//create our data stuff
	m_DataLabel = new Label(this, "DataLabel", "");

	m_DataTextEntry = new TextEntry(this, "DataText");
	m_DataTextEntry->SetMaximumCharCount(32);
	m_DataTextEntry->SetVisible(false);
	m_DataTextEntry->AddActionSignalTarget(this);

	m_DataCheckButton = new CheckButton(this, "DataCheckButton", "");
	m_DataCheckButton->SetVisible(false);
	m_DataCheckButton->SetCommand(DATA_CHECK_BUTTON_COMMAND);
	m_DataCheckButton->SetText("Allowed?");

	m_DataSlider = new WheelSlider(this, "DataSlider");
	m_DataSlider->SetVisible(false);
	m_DataSlider->SetRange(1, 250);
		
	//add the m_AddButton
	m_AddButton = new Button(this, "AddButton", "Add Chapter Image Info");
	m_AddButton->SetCommand(DATA_ADD_COMMAND);
	m_AddButton->SetEnabled(false);

	//make the remove button
	m_RemoveButton = new Button(this, "RemoveButton", "Remove Chapter Image Info");
	m_RemoveButton->SetCommand(DATA_REMOVE_COMMAND);
	m_RemoveButton->SetEnabled(false);
		
	//add the save button
	m_SaveButton = new Button(this, "SaveButton", "Save configs");
	m_SaveButton->SetCommand(SAVE_NEW_GAME_PANEL_COMMAND);

	//make the remove button
	m_ResetNewGamePanelButton = new Button(this, "ResetNewGamePanelButton", "Reload new game panels");
	m_ResetNewGamePanelButton->SetCommand(RELOAD_NEW_GAME_PANEL_COMMAND);

	//create the themes check button
	m_HasThemesCheckButton = new CheckButton(this, "HasThemesCheckButton", "Has Themes");
	m_HasThemesCheckButton->SetEnabled(false);
	m_HasThemesCheckButton->SetSelected(false);
	m_HasThemesCheckButton->SetCommand(HAS_THEMES_COMMAND);

	//make the chapter image
	m_CurrentNewGameImage = new ImagePanel(this, "ChapterImage");
	m_CurrentNewGameImage->SetVisible(false);

	//make the 2nd image
	m_CurrentNewGameImageDay = new ImagePanel(this, "ChapterImage");
	m_CurrentNewGameImageDay->SetVisible(false);

	//load all the new game lists in the games/* folder BUT always load resource/gamelist.txt first
	{
		LoadNewGameInfo("resource/gamelist.txt");

		FileFindHandle_t handle;
		const char* firstfile = filesystem->FindFirst("resource/games/*.txt", &handle);
		while (firstfile)
		{
			//dont read the . filenames
			if (!Q_stricmp(firstfile, ".") || !Q_stricmp(firstfile, ".."))
			{
				firstfile = filesystem->FindNext(handle);
				continue;
			}

			//see if its a file or not
			if (!filesystem->FindIsDirectory(handle) && strchr(firstfile, '.'))
				LoadNewGameInfo(CFmtStr("resource/games/%s", firstfile));

			firstfile = filesystem->FindNext(handle);
		}

		filesystem->FindClose(handle);
	}
	
	//now load the m_ReadWriteGameListInfo onto the game list
	m_ReadWriteGameListInfo.RemoveAll();
	for (int i = 0; i < m_GameListInfo.Count(); i++)
	{
		for (int j = 0; j < m_GameListInfo[i].info.Count(); j++)
		{
			//add to the list view
			KeyValues* item = new KeyValues("item");
			item->SetString("Game", CFmtStr("%s = %s", m_GameListInfo[i].info[j].name, m_GameListInfo[i].filename + Q_strlen("resource/")));
			m_GameList->AddItem(item, m_ReadWriteGameListInfo.Count(), false, false);

			//add to the m_ReadWriteGameListInfo first
			m_ReadWriteGameListInfo.AddToTail(&m_GameListInfo[i].info[j]);
		}
	}

	//select index 0
	m_GameList->SetSingleSelectedItem(0);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Destructor for the new game editor panel
//----------------------------------------------------------------------------------------------------
CNewGameEditorPanel::~CNewGameEditorPanel()
{
	//delete all the game info
	for (int g = 0; g < m_ReadWriteGameListInfo.Count(); g++)
	{
		for (int c = 0; c < m_ReadWriteGameListInfo[g]->resourceStrings.Count(); c++)
			free((void*)m_ReadWriteGameListInfo[g]->resourceStrings[c]);
	}

	//clear our singleton
	s_NewGameEditor = nullptr;
} 

//-----------------------------------------------------------------------
// Purpose: Called when the panels layout is being performed
//-----------------------------------------------------------------------
void CNewGameEditorPanel::PerformLayout()
{
	//this looks terrible i know. Deal with it. Cant be bothered loading a file to do this
	m_GameList->SetBounds(10, 30, 430, 200);
	m_ChapterImageInfo->SetBounds(10, 240, 430, 200);
	m_DataLabel->SetBounds(10, 450, 135, 20);
	m_DataTextEntry->SetBounds(150, 450, 285, 20);
	m_DataCheckButton->SetBounds(180, 450, 275, 20);
	m_DataSlider->SetBounds(150, 450, 285, 20);
	m_AddButton->SetBounds(10, 475, 214, 20);
	m_RemoveButton->SetBounds(226, 475, 214, 20);
	m_HasThemesCheckButton->SetBounds(170, 496, 150, 22);
	m_CurrentNewGameImage->SetBounds(65, 518, 160, 90);
	m_CurrentNewGameImageDay->SetBounds(235, 518, 160, 90);
	m_SaveButton->SetBounds(10, m_CurrentNewGameImage->IsVisible() ? 613 : 518, 214, 20);
	m_ResetNewGamePanelButton->SetBounds(226, m_CurrentNewGameImage->IsVisible() ? 613 : 518, 214, 20);
	
	//call the base func
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------
// Purpose: Adds a game info into the game info list
//-----------------------------------------------------------------------
int CNewGameEditorPanel::AddGameInfoToTreeView(GameListInfoEditor_t::ChapterImageInfo_t& item, GameListInfoEditor_t* info, int root)
{
	KeyValues* name = new KeyValues("ImageInfoName");
	name->SetString("Text", item.name);
	name->SetInt("Type", (int)TreeViewNodeClass_e::Node_Name);
	name->SetPtr("Data", &item);
	int newnode = m_ChapterImageInfo->AddItem(name, root);

	//add the sub items (start, end, allow day, allow night)
	{
		KeyValues* allowday = new KeyValues("allowday");
		allowday->SetString("Text", CFmtStr("Allowed To Show Day Images = %d", item.allowDay));
		allowday->SetInt("Type", (int)TreeViewNodeClass_e::Node_AllowDay);
		allowday->SetPtr("Data", &item);
		m_ChapterImageInfo->AddItem(allowday, newnode);
	}
	{
		KeyValues* allownight = new KeyValues("allownight");
		allownight->SetString("Text", CFmtStr("Allowed To Show Night Images = %d", item.allowNight));
		allownight->SetInt("Type", (int)TreeViewNodeClass_e::Node_AllowNight);
		allownight->SetPtr("Data", &item);
		m_ChapterImageInfo->AddItem(allownight, newnode);
	}
	{
		KeyValues* startchapter = new KeyValues("allowday");
		startchapter->SetString("Text", CFmtStr("Start Chapter = %s", info->resourceStrings[max(item.min - 1, 0)]));
		startchapter->SetInt("Type", (int)TreeViewNodeClass_e::Node_StartChapter);
		startchapter->SetPtr("Data", &item);
		m_ChapterImageInfo->AddItem(startchapter, newnode);
	}
	{
		KeyValues* endchapter = new KeyValues("allowday");
		endchapter->SetString("Text", CFmtStr("Ending Chapter = %s", info->resourceStrings[max(item.max - 1, 0)]));		//how tf did this go under the radar for so long!! It used to be a %s and work?. EDIT: nevermind im dumb
		endchapter->SetInt("Type", (int)TreeViewNodeClass_e::Node_EndChapter);
		endchapter->SetPtr("Data", &item);
		m_ChapterImageInfo->AddItem(endchapter, newnode);
	}

	return newnode;
}

//-----------------------------------------------------------------------
// Purpose: Called when an item is selected from the game list
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnItemSelected(KeyValues* data)
{
	//get the GameListInfoEditor_t. BUT do a range check first
	unsigned short index = m_GameList->GetItemUserData(m_GameList->GetSelectedItem(0));
	if (index < 0 || index >= m_ReadWriteGameListInfo.Count())
		return;

	m_CurrentNodeIndex = 0;

	//get the info
	GameListInfoEditor_t* info = m_ReadWriteGameListInfo[index];
	m_CurrentSelectedList = info;
	m_CurrentSelectedData = nullptr;

	//set the m_HasThemesCheckButton
	m_HasThemesCheckButton->SetEnabled(true);
	m_HasThemesCheckButton->SetSelected(m_CurrentSelectedList->UsesThemes);

	//enable the add button
	m_AddButton->SetEnabled(true);
	m_RemoveButton->SetEnabled(false);

	//set the range of our slider
	m_DataSlider->SetRange(1, info->resourceStrings.Count());

	//reset the tree view
	m_ChapterImageInfo->RemoveAll();
	int root = m_ChapterImageInfo->AddItem(new KeyValues("Root", "Text", "Chapter Image Info"), -1);

	//add all the items
	for (int i = 0; i < info->m_ChapterImageInfo.Count(); i++)
	{
		//add the item
		AddGameInfoToTreeView(info->m_ChapterImageInfo[i], info, root);
	}
}

//-----------------------------------------------------------------------
// Purpose: Called when an item is deselected from the games list
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnItemDeselected(KeyValues* data)
{
	//if no item is selected then reset everything
	if (m_GameList->GetSelectedItemsCount() <= 0)
	{
		m_ChapterImageInfo->RemoveAll();
		m_CurrentNodeIndex = 0;
		m_CurrentSelectedData = nullptr;
		m_CurrentSelectedList = nullptr;
		
		//clear the elements
		m_HasThemesCheckButton->SetEnabled(false);
		m_HasThemesCheckButton->SetSelected(false);
		m_AddButton->SetEnabled(false);
		m_RemoveButton->SetEnabled(false);
		m_DataSlider->SetVisible(false);
		m_CurrentNewGameImage->SetVisible(false);
		m_CurrentNewGameImageDay->SetVisible(false);
		m_DataTextEntry->SetVisible(false);
		m_DataCheckButton->SetVisible(false);
		m_DataLabel->SetText("");
	}
}

//-----------------------------------------------------------------------
// Purpose: Called when an item is de-selected from the tree view
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnTreeViewItemDeselected(KeyValues* data)
{
	//check the number of selected elements
	if (m_ChapterImageInfo->GetSelectedItemCount() > 0)
		return;

	//clear (almost) everything
	m_CurrentNodeIndex = 0;
	m_RemoveButton->SetEnabled(false);
	m_DataSlider->SetVisible(false);
	m_CurrentNewGameImage->SetVisible(false);
	m_CurrentNewGameImageDay->SetVisible(false);
	m_DataTextEntry->SetVisible(false);
	m_DataCheckButton->SetVisible(false);
	m_DataLabel->SetText("");
	m_CurrentSelectedData = nullptr;
}

//-----------------------------------------------------------------------
// Purpose: Called when an item is selected from the tree view
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnTreeViewItemSelected(KeyValues* data)
{
	//get the data
	int index = data->GetInt("itemIndex");
	KeyValues* userdata = m_ChapterImageInfo->GetItemData(index);

	//hide everything
	m_DataLabel->SetText("");
	m_DataTextEntry->SetVisible(false);
	m_DataCheckButton->SetVisible(false);
	m_DataSlider->SetVisible(false);
	m_CurrentNewGameImage->SetVisible(false);
	m_CurrentNewGameImageDay->SetVisible(false);
	m_RemoveButton->SetEnabled(false);

	//if we are the root node then dont do anything
	if (index == m_ChapterImageInfo->GetRootItemIndex())
		return;

	//only enable the remove button if the current selected nodes parent is the root node
	m_RemoveButton->SetEnabled(m_ChapterImageInfo->GetItemParent(index) == m_ChapterImageInfo->GetRootItemIndex());

	//set our current data
	m_CurrentSelectedData = (GameListInfoEditor_t::ChapterImageInfo_t*)userdata->GetPtr("Data");
	m_CurrentSelectedType = (TreeViewNodeClass_e)userdata->GetInt("Type");
	m_CurrentNodeIndex = index;


	//set the chapter images
	bool min = m_CurrentSelectedType == TreeViewNodeClass_e::Node_StartChapter;
	const char* image = CFmtStr("chapters/default/%s/chapter%d", m_CurrentSelectedList->prefix, min ? m_CurrentSelectedData->min : m_CurrentSelectedData->max);

	m_CurrentNewGameImage->SetImage(image);
	m_CurrentNewGameImageDay->SetImage(CFmtStr("%s_day", image));


	//set our items text based on the type
	switch (m_CurrentSelectedType)
	{
	case TreeViewNodeClass_e::Node_Name:
		m_DataTextEntry->SetText(m_CurrentSelectedData->name);
		m_DataTextEntry->SetVisible(true);
		m_DataLabel->SetText("Image Info Name:");
		break;
	case TreeViewNodeClass_e::Node_AllowDay:
		m_DataCheckButton->SetVisible(true);
		m_DataCheckButton->SetSelected(m_CurrentSelectedData->allowDay);
		m_DataLabel->SetText("Allowed To Show Day:");
		break;
	case TreeViewNodeClass_e::Node_AllowNight:
		m_DataCheckButton->SetVisible(true);
		m_DataCheckButton->SetSelected(m_CurrentSelectedData->allowNight);
		m_DataLabel->SetText("Allow To Show Night:");
		break;
	case TreeViewNodeClass_e::Node_StartChapter:
		m_CurrentNewGameImage->SetVisible(true);
		m_CurrentNewGameImageDay->SetVisible(true);
		m_DataSlider->SetVisible(true);
		m_DataSlider->SetValue(m_CurrentSelectedData->min);
		m_DataLabel->SetText("Starting Chapter:");
		break;
	case TreeViewNodeClass_e::Node_EndChapter:
		m_CurrentNewGameImage->SetVisible(true);
		m_CurrentNewGameImageDay->SetVisible(true);
		m_DataSlider->SetVisible(true);
		m_DataSlider->SetValue(m_CurrentSelectedData->max);
		m_DataLabel->SetText("Ending Chapter:");
		break;
	}
}

//-----------------------------------------------------------------------
// Purpose: Called when text is changed inside the/a text entry
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnTextChanged(KeyValues* data)
{
	//check the current item
	if (!m_CurrentSelectedData)
		return;

	//set the m_CurrentSelectedData->name
	m_DataTextEntry->GetText(m_CurrentSelectedData->name, sizeof(m_CurrentSelectedData->name));

	//change the nodes text
	KeyValues* node_data = m_ChapterImageInfo->GetItemData(m_CurrentNodeIndex);
	node_data->SetString("Text", m_CurrentSelectedData->name);
	m_ChapterImageInfo->ModifyItem(m_CurrentNodeIndex, node_data);
}

//-----------------------------------------------------------------------
// Purpose: Called when a sliders value is changed
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnSliderValueChanged(KeyValues* data)
{
	//check the current data
	if (!m_CurrentSelectedData)
		return;

	bool min = m_CurrentSelectedType == TreeViewNodeClass_e::Node_StartChapter;

	//set the chapter images
	const char* image = CFmtStr("chapters/default/%s/chapter%d", m_CurrentSelectedList->prefix, min ? m_CurrentSelectedData->min : m_CurrentSelectedData->max);
	m_CurrentNewGameImage->SetImage(image);
	m_CurrentNewGameImageDay->SetImage(CFmtStr("%s_day", image));

	//set the internal data
	if (min)
	{
		m_CurrentSelectedData->min = m_DataSlider->GetValue();
	}
	else
	{
		m_CurrentSelectedData->max = m_DataSlider->GetValue();
	}

	//re-name the node
	KeyValues* node_data = m_ChapterImageInfo->GetItemData(m_CurrentNodeIndex);
	node_data->SetString("Text", CFmtStr("%s Chapter = %s", min ? "Starting" : "Ending", m_CurrentSelectedList->resourceStrings[max(m_DataSlider->GetValue() - 1, 0)]));	//do m_DataSlider->GetValue() - 1 because the range starts at 1
	m_ChapterImageInfo->ModifyItem(m_CurrentNodeIndex, node_data);
}

//-----------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnThink()
{
	//check the chapter image is visible
	if (m_CurrentNewGameImage->IsVisible())
	{
		SetSize(450, 636);
	}
	else
	{
		SetSize(450, 542);
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------
void CNewGameEditorPanel::OnCommand(const char* pszCommand)
{
	//check for DATA_CHECK_BUTTON_COMMAND
	if (!Q_stricmp(pszCommand, DATA_CHECK_BUTTON_COMMAND))
	{
		//check the current data
		if (!m_CurrentSelectedData)
			return;

		bool nighttime = m_CurrentSelectedType == TreeViewNodeClass_e::Node_AllowNight;

		//set the internal data
		if (nighttime)
			m_CurrentSelectedData->allowNight = m_DataCheckButton->IsSelected();
		else
			m_CurrentSelectedData->allowDay = m_DataCheckButton->IsSelected();

		//re-name the node
		KeyValues* data = m_ChapterImageInfo->GetItemData(m_CurrentNodeIndex);
		data->SetString("Text", CFmtStr("Allowed To Show %s Images = %d", nighttime ? "Night" : "Day", m_DataCheckButton->IsSelected()));
		m_ChapterImageInfo->ModifyItem(m_CurrentNodeIndex, data);
		return;
	}

	//check for the remove command
	else if (!Q_stricmp(pszCommand, DATA_REMOVE_COMMAND))
	{
		//the current selected nodes index must be the root node
		int parent = m_ChapterImageInfo->GetRootItemIndex();
		if (m_ChapterImageInfo->GetItemParent(m_CurrentNodeIndex) != parent || !m_CurrentSelectedList)
			return;

		//reomve the node;
		m_ChapterImageInfo->RemoveItem(m_CurrentNodeIndex, false, true);
		m_ChapterImageInfo->AddSelectedItem(m_CurrentNodeIndex - 1, true, true, true);

		//remove from the array
		int index = -1;
		for (int i = 0; i < m_CurrentSelectedList->m_ChapterImageInfo.Count(); i++)
		{
			if (&m_CurrentSelectedList->m_ChapterImageInfo[i] == m_CurrentSelectedData)
			{
				index = i;
				break;
			}
		}

		if (index != -1)
			m_CurrentSelectedList->m_ChapterImageInfo.Remove(index);

		//reset the text entry by calling OnItemSelected
		OnItemSelected(nullptr);

		//select the current node - 5
		m_ChapterImageInfo->AddSelectedItem(m_CurrentNodeIndex - 5, true, true, true);
		m_ChapterImageInfo->ExpandItem(m_CurrentNodeIndex - 5, true);
	}

	//check for the add or remove command
	else if (!Q_stricmp(pszCommand, DATA_ADD_COMMAND))
	{
		//check the current info
		if (!m_CurrentSelectedList)
			return;

		//get the item
		GameListInfoEditor_t::ChapterImageInfo_t& add = m_CurrentSelectedList->m_ChapterImageInfo[m_CurrentSelectedList->m_ChapterImageInfo.AddToTail()];
		memset(&add, 0, sizeof(add));

		//always set the min + max
		add.min = 1;
		add.max = 1;

		Q_strncpy(add.name, "New Image Data", sizeof(add.name));

		//add a node
		int index = AddGameInfoToTreeView(add, m_CurrentSelectedList, m_ChapterImageInfo->GetRootItemIndex());

		//add the item
		m_ChapterImageInfo->AddSelectedItem(index, true, true, true);
		m_ChapterImageInfo->ExpandItem(index, true);
	}

	//check for the HAS_THEMES_COMMAND
	else if (!Q_stricmp(pszCommand, HAS_THEMES_COMMAND))
	{
		//must have our data
		if (!m_CurrentSelectedList)
			return;

		m_CurrentSelectedList->UsesThemes = m_HasThemesCheckButton->IsSelected();
		return;
	}
	
	//check for the reload command
	else if (!Q_stricmp(pszCommand, RELOAD_NEW_GAME_PANEL_COMMAND))
	{
		g_sNewGamePanelInterface.Destroy();
		g_sNewGamePanelInterface.Create(enginevgui->GetPanel(VGuiPanel_t::PANEL_GAMEUIDLL));
		g_sNewGamePanelInterface.Activate();
		return;
	}

	//check for the save command
	else if (!Q_stricmp(pszCommand, SAVE_NEW_GAME_PANEL_COMMAND))
	{
		for (int i = 0; i < m_GameListInfo.Count(); i++)
		{
			//write everything into the keyvalues
			KeyValues* keyvalues = new KeyValues("GameList");

			//get all the sub data
			for (int j = 0; j < m_GameListInfo[i].info.Count(); j++)
			{
				//get the info
				GameListInfoEditor_t* info = &m_GameListInfo[i].info[j];

				//write the prefix + name
				KeyValues* data = new KeyValues(info->name);
				data->SetString("Prefix", info->prefix);
				data->SetBool("HasThemes", info->UsesThemes);
				data->SetBool("Default", info->isDefault);

				//write the ChapterImageInfo
				KeyValues* ChapterImageInfo = new KeyValues("ChapterImageInfo");

				//write all the chapter infos
				for (int z = 0; z < info->m_ChapterImageInfo.Count(); z++)
				{
					KeyValues* chapter = new KeyValues(info->m_ChapterImageInfo[z].name);
					chapter->SetBool("AllowShowDay", info->m_ChapterImageInfo[z].allowDay);
					chapter->SetBool("AllowShowNight", info->m_ChapterImageInfo[z].allowNight);
					chapter->SetInt("StartChapter", info->m_ChapterImageInfo[z].min);
					chapter->SetInt("EndChapter", info->m_ChapterImageInfo[z].max);
					ChapterImageInfo->AddSubKey(chapter);
				}

				//add the chapter info to data
				data->AddSubKey(ChapterImageInfo);

				//add the data to the keyvalues
				keyvalues->AddSubKey(data);
			}

			//save then delete the keyvalues
			keyvalues->SaveToFile(filesystem, m_GameListInfo[i].filename, "MOD");
			keyvalues->deleteThis();
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------
// Purpose: Loads the new game info
//-----------------------------------------------------------------------
void CNewGameEditorPanel::LoadNewGameInfo(const char* filename)
{
	KeyValues* gamelist = new KeyValues("gamelist");
	if (!gamelist->LoadFromFile(filesystem, filename, "MOD"))
		return;

	//make the base game list
	GameListInfoEditorBase_t& base_info = m_GameListInfo[m_GameListInfo.AddToTail()];
	Q_strncpy(base_info.filename, filename, sizeof(base_info.filename));

	//go through each subkey
	FOR_EACH_TRUE_SUBKEY(gamelist, game)
	{
		//load the resource file. I could call this before the for loop but cant be bothered changing it. This runs once on startup so it shouldnt be an issue
		const char* resource = game->GetString("resource", "resource/HL2_AloneMod_english.txt");
		KeyValues* AloneModEnglish = new  KeyValues("AloneModEnglish");
		if (!AloneModEnglish->LoadFromFile(filesystem, resource, "MOD"))
		{
			ConWarning("Error: Failed to load the '%s' file for the new game editor panel!\n%s->%s\n", resource, filename, game->GetName());
			AloneModEnglish->deleteThis();
			continue;
		}

		//get the tokens
		KeyValues* tokens = AloneModEnglish->FindKey("Tokens");
		if (!tokens)
		{
			ConWarning("Error: Failed to find the 'Tokens' key for %s for the new game editor panel!\n%s\n", filename);
			AloneModEnglish->deleteThis();
			continue;
		}

		//get the actuall game list
		GameListInfoEditor_t& info = base_info.info[base_info.info.AddToTail()];
		Q_strncpy(info.name, game->GetName(), sizeof(info.name));
		Q_strncpy(info.prefix, game->GetString("Prefix"), sizeof(info.prefix));

		//get the resource file
		Q_strncpy(info.resourceFilename, resource, sizeof(info.resourceFilename));
		
		//does this have themes?
		info.UsesThemes = game->GetBool("HasThemes", true);

		//is this the default
		info.isDefault = game->GetBool("Default");

		//get the chapter names
		int i = 1;
		while (true)
		{
			//check for the string
			const char* chapter = tokens->GetString(CFmtStr("%s_Chapter%d_Title", info.prefix, i++), nullptr);
			if (!chapter)
				break;

			info.resourceStrings.AddToTail(_strdup(chapter));
		}

		//get the chapter image info
		KeyValues* ChapterImageInfo = game->FindKey("ChapterImageInfo");
		if (ChapterImageInfo)
		{
			FOR_EACH_TRUE_SUBKEY(ChapterImageInfo, _info)
			{
				//add the info
				GameListInfoEditor_t::ChapterImageInfo_t& chapter_info = info.m_ChapterImageInfo[info.m_ChapterImageInfo.AddToTail()];

				//get the name
				Q_strncpy(chapter_info.name, _info->GetName(), sizeof(chapter_info.name));

				//get the other data
				chapter_info.min = _info->GetInt("StartChapter");
				chapter_info.max = _info->GetInt("EndChapter");
				chapter_info.allowDay = _info->GetBool("AllowShowDay");
				chapter_info.allowNight = _info->GetBool("AllowShowNight");
			}
		}
	}
}

//command to optn eht new game editor
//CON_COMMAND(open_new_game_editor_panel, "")
//{
//	if (!s_NewGameEditor)
//		s_NewGameEditor = new CNewGameEditorPanel();
//
//	s_NewGameEditor->Activate();
//}