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
#include "AloneMod/IOptionsPanel.h"
#include "AloneMod/DynamicSky.h"

#if !AMOD_DAYTIME_EDITION
extern ConVar amod_day;
#endif

//new game panel game list struct
struct GameListInfo_t
{
	//prefix and name
	char name[128];
	char prefix[32];
	int CurrentChapterIndex = 0;
	CUtlVector<const char*> ChapterNames;

	//day info
	struct DayInfo_t
	{
		ConVar* convar;
		int min;
		int max;
	};
	CUtlVector<DayInfo_t> m_DayInfo;
};

#define COMMAND_CHAPTER_PREFIX "LoadChapter"
#define COMMAND_CHAPTER_1 COMMAND_CHAPTER_PREFIX "1"
#define COMMAND_CHAPTER_2 COMMAND_CHAPTER_PREFIX "2"
#define COMMAND_CHAPTER_3 COMMAND_CHAPTER_PREFIX "3"
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
};

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
	SetTitle("Chapter Select", false);
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
	m_PrevButton = new vgui::Button(this, "PreviousButton", "Previous");
	m_PrevButton->SetBounds(10, 185, 80, 25);
	m_PrevButton->SetCommand(COMMAND_PREV);
	m_PrevButton->SetEnabled(false);

	m_NextButton = new vgui::Button(this, "NextButton", "Next");
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
	//get the active item
	char oldtext[128];
	m_ThemesComboBox->GetText(oldtext, sizeof(oldtext));

	//clear the combo box
	m_ThemesComboBox->RemoveAll();

	//ALWAYS load the default theme
	m_ThemesComboBox->AddItem("THEME: Default", new KeyValues("resource/time_info"));

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

		//check the text with the previous text
		const char* text = CFmtStr("THEME: %s", firstfile);
		if (!Q_stricmp(oldtext, text))
			index = curr;

		//add the item
		m_ThemesComboBox->AddItem(text, new KeyValues(CFmtStr("resource/time_info/%s", firstfile)));
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

		//get the chapter names
		for (int i = 1; i < MAX_CHAPTERS; i++)
		{
			//check for the string
			const char* chapter = tokens->GetString(CFmtStr("%s_Chapter%d_Title", info.prefix, i), nullptr);
			if (!chapter)
				break;

			info.ChapterNames.AddToTail(_strdup(chapter));
		}

		//get the day info
		KeyValues* day = game->FindKey("DayInfo");
		if (!day)
		{
			AloneModEnglish->deleteThis();
			continue;
		}

		FOR_EACH_TRUE_SUBKEY(day, days)
		{
			//check for the convar
			ConVar* convar = cvar->FindVar(days->GetString("Convar"));
			if (!convar)
				continue;

			//add the info
			GameListInfo_t::DayInfo_t& dayinfo = info.m_DayInfo[info.m_DayInfo.AddToTail()];
			dayinfo.min = days->GetInt("StartChapter");
			dayinfo.max = days->GetInt("EndChapter");
			dayinfo.convar = convar;
		}

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
		m_ChapterButtons[i]->SetText(CFmtStr("Load Chapter %d", realindex));

		//check for daytime
		const char* sDayTime = "";
		for (int j = 0; j < m_CurrentSelectedGameInfo->m_DayInfo.Count(); j++)
		{
			//check the info
			GameListInfo_t::DayInfo_t& info = m_CurrentSelectedGameInfo->m_DayInfo[j];
			
			if (info.convar->GetBool() && (realindex >= info.min && realindex <= info.max))
			{
				sDayTime = "_day";
				break;
			}
		}

		//see if an image exists with the chapters/(FE)/%s/chapters%d%s name where (FE) is just V_GetFileName(m_ThemesComboBox->GetActiveItemUserData()->GetName()).
		char buf[512] = { 0 };

		if (m_ThemesComboBox->GetActiveItem() != 0)
			Q_snprintf(buf, sizeof(buf), "chapters/%s/%s/chapter%d%s", V_GetFileName(m_ThemesComboBox->GetActiveItemUserData()->GetName()), m_CurrentSelectedGameInfo->prefix, realindex, sDayTime);

		if (m_ThemesComboBox->GetActiveItem() == 0 || IsErrorMaterial(materials->FindMaterial(CFmtStr("vgui/%s", buf), TEXTURE_GROUP_VGUI, false)))
		{
			//resort to default filepath
			Q_snprintf(buf, sizeof(buf), "chapters/default/%s/chapter%d%s", m_CurrentSelectedGameInfo->prefix, realindex, sDayTime);
		}

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
}

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
		extern ConVar amod_timeinfo_load_directory;
		amod_timeinfo_load_directory.SetValue(m_ThemesComboBox->GetActiveItemUserData()->GetName());
		engine->ClientCmd("amod_timeinfo_reset_noreload");
		
		int index = Q_atoi(pszCommand + Q_strlen(COMMAND_CHAPTER_PREFIX));
		engine->ClientCmd(CFmtStr("exec \"%s/chapter%d", m_CurrentSelectedGameInfo->prefix, (m_CurrentSelectedGameInfo->CurrentChapterIndex * 3) + index));
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
		ActivateGame(m_GameComboBox->GetActiveItem());
		return;
	}

	//check for the themes combo box. This is because if we change the theme. There is a chance
	//the theme uses its own custom chapter images.
	else if (kv->GetPtr("Panel") == m_ThemesComboBox)
	{
		//select the active page
		if (m_CurrentSelectedGameInfo)
			SelectPage(m_CurrentSelectedGameInfo->CurrentChapterIndex);

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
// Purpose: Alone mod day change callbacks
//-----------------------------------------------------------------------
void AmodDayChangeCallback(IConVar* var, const char* olds, float oldf)
{
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