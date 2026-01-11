#include "cbase.h"
#include "fmtstr.h"
#include "filesystem.h"
#include "ISongPanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <ienginevgui.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ToolTip.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <engine/IEngineSound.h>

using namespace vgui;

//should the song panel draw or not?
ConVar cl_drawsongpanel("cl_drawsongpanel", "0", FCVAR_CLIENTDLL, "Sets the state of the alone mod Song panel");

//invalid song guid
#define INVALID_SONG_GUID (-1)

//alone mod random song
struct AloneModRandomSong_t
{
	const char* Name = nullptr;							//name of random song set

	CUtlVector<const char*> RandomSongs;				//name of random song. Chooses this or normal song
};

//holds all the songs
struct AloneModSong_t
{
	const char* Name;									//display name

	//song stuff
	const char* SongName = nullptr;						//name of song.
	AloneModRandomSong_t* RandomSong = nullptr;			//name of random song set.
};




//function to stop every song from playing
void StopAllSongs()
{
	//get sound list and loop through it
	CUtlVector<SndInfo_t> soundinfo;
	enginesound->GetActiveSounds(soundinfo);
	for (int i = 0; i < soundinfo.Count(); i++)
	{
		//get filename
		char buf[512];
		filesystem->String(soundinfo[i].m_filenameHandle, buf, sizeof(buf));

		//if the sound is a music file then stop it
		if (Q_strstr(buf, "music") == buf)
			enginesound->StopSoundByGuid(soundinfo[i].m_nGuid);
	}
}




//playlist save panel
#define PLAYLIST_SAVE_COMMAND "SavePlaylist"

class CPlaylistSavePanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CPlaylistSavePanel, vgui::Frame)
public:
	CPlaylistSavePanel(vgui::VPANEL parent);

	//panel funcs
	void OnCommand(const char* pszCommand);

	~CPlaylistSavePanel();
private:
	//save playlist panel text entry
	class CPlaylistSaveTextEntry : public vgui::TextEntry
	{
		DECLARE_CLASS_SIMPLE(CPlaylistSaveTextEntry, vgui::TextEntry);
	public:
		CPlaylistSaveTextEntry(vgui::Panel* parent, const char* name) : BaseClass(parent, name) {};

		void OnKeyTyped(wchar_t unichar)
		{
			//valid chars
			wchar_t ValidChars[] = {
				// lowercase a-z
				'a','b','c','d','e','f','g','h','i','j',
				'k','l','m','n','o','p','q','r','s','t',
				'u','v','w','x','y','z',

				// uppercase A-Z
				'A','B','C','D','E','F','G','H','I','J',
				'K','L','M','N','O','P','Q','R','S','T',
				'U','V','W','X','Y','Z',

				// digits 0-9
				'0','1','2','3','4','5','6','7','8','9',

				// extra symbols (only valid ones)
				'_','-','=','+',
				']','}','[','{',
				'@','#','$','%','&','^','(',')',
				' ',
			};


			for (int i = 0; i < SIZE_OF_ARRAY(ValidChars); i++)
			{
				if (unichar == ValidChars[i])
				{
					BaseClass::OnKeyTyped(unichar);
					return;
				}
			}
		}
	};

	//playlist filename
	CPlaylistSaveTextEntry* m_PlaylistFilename = nullptr;
};

//singleton
static CPlaylistSavePanel* g_PlaylistSavePanel = nullptr;

//-------------------------------------------------------------------------------------------------------
// Purpose: constructor for playlist save panel
//-------------------------------------------------------------------------------------------------------
CPlaylistSavePanel::CPlaylistSavePanel(vgui::VPANEL parent)
	: BaseClass(NULL, "PlaylistSavePanel")
{
	//set the parent
	SetParent(parent);

	//set keyboard stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set other panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
	SetDeleteSelfOnClose(true);

	SetSize(250, 100);
	MoveToCenterOfScreen();
	SetTitle("Save Playlist.", false);

	//set the panel scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//make the elements
	Label* PlaylistNameLabel = new Label(this, "PlaylistNameLabel", "Playlist Name:");
	PlaylistNameLabel->SetBounds(5, 20, 240, 20);
	PlaylistNameLabel->SetContentAlignment(Label::Alignment::a_center);

	m_PlaylistFilename = new CPlaylistSaveTextEntry(this, "PlaylistName");
	m_PlaylistFilename->SetBounds(5, 40, 240, 25);
	m_PlaylistFilename->SetMaximumCharCount(64);

	Button* SaveButton = new Button(this, "SavePlaylist", "Save Playlist");
	SaveButton->SetBounds(5, 70, 240, 25);
	SaveButton->SetCommand(PLAYLIST_SAVE_COMMAND);

	MoveToFront();
	Activate();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: called on close
//-------------------------------------------------------------------------------------------------------
CPlaylistSavePanel::~CPlaylistSavePanel()
{
	g_PlaylistSavePanel = nullptr;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: called on close
//-------------------------------------------------------------------------------------------------------
void CPlaylistSavePanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, PLAYLIST_SAVE_COMMAND))
	{
		//check the filename
		if (m_PlaylistFilename->GetTextLength() <= 0)
		{
			//show error
			QueryBox* box = new QueryBox("Error", "Got empty playlist filename!", this);
			box->DoModal(this);
			box->Activate();
			return;
		}

		//tell the song panel write the file
		char buf[64];
		m_PlaylistFilename->GetText(buf, sizeof(buf));
		songpanel->SavePlaylistToFile(buf);

		//close this panel
		delete this;
		return;
	}

	BaseClass::OnCommand(pszCommand);
}





//playlist load panel
#define PLAYLIST_LOAD_COMMAND "LoadPlaylist"

class CPlaylistLoadPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CPlaylistLoadPanel, vgui::Frame)
public:
	CPlaylistLoadPanel(vgui::VPANEL parent);

	//panel funcs
	void LoadPlaylists();
	void OnCommand(const char* pszCommand);

	~CPlaylistLoadPanel();
private:
	ComboBox* m_PlaylistFilename = nullptr;
};

//singleton
static CPlaylistLoadPanel* g_PlaylistLoadPanel = nullptr;

//-------------------------------------------------------------------------------------------------------
// Purpose: constructor for playlist save panel
//-------------------------------------------------------------------------------------------------------
CPlaylistLoadPanel::CPlaylistLoadPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "PlaylistLoadPanel")
{
	//set the parent
	SetParent(parent);

	//set keyboard stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set other panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);
	SetDeleteSelfOnClose(true);

	SetSize(250, 100);
	MoveToCenterOfScreen();
	SetTitle("Load Playlist.", false);

	//set the panel scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//make the elements
	Label* PlaylistNameLabel = new Label(this, "PlaylistNameLabel", "Playlist Name:");
	PlaylistNameLabel->SetBounds(5, 20, 240, 20);
	PlaylistNameLabel->SetContentAlignment(Label::Alignment::a_center);

	m_PlaylistFilename = new ComboBox(this, "PlaylistName", 15, false);
	m_PlaylistFilename->SetBounds(5, 40, 240, 25);
	m_PlaylistFilename->SetMaximumCharCount(64);

	//load the playlist
	LoadPlaylists();

	//check the amount of items
	if (m_PlaylistFilename->GetItemCount() <= 0)
	{
		delete this;
		g_PlaylistLoadPanel = nullptr;
		return;
	}

	m_PlaylistFilename->ActivateItem(0);

	Button* SaveButton = new Button(this, "LoadPlaylist", "Load Playlist");
	SaveButton->SetBounds(5, 70, 240, 25);
	SaveButton->SetCommand(PLAYLIST_LOAD_COMMAND);

	MoveToFront();
	Activate();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Load all playlist filenames into the combo box
//-------------------------------------------------------------------------------------------------------
void CPlaylistLoadPanel::LoadPlaylists()
{
	// find all txt files in MOD/playlist/
	FileFindHandle_t handle;
	const char* pszFilename = filesystem->FindFirstEx("playlists/*.txt", "MOD", &handle);

	while (pszFilename)
	{
		// skip directories
		if (!filesystem->FindIsDirectory(handle))
		{
			//check extention
			if (Q_stricmp(V_GetFileExtension(pszFilename), "txt"))
				continue;

			//strip extention
			char buf[64];
			V_StripExtension(pszFilename, buf, sizeof(buf));

			// add file to combo box
			m_PlaylistFilename->AddItem(buf, nullptr);
		}

		// next file
		pszFilename = filesystem->FindNext(handle);
	}

	filesystem->FindClose(handle);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: called on close
//-------------------------------------------------------------------------------------------------------
CPlaylistLoadPanel::~CPlaylistLoadPanel()
{
	g_PlaylistLoadPanel = nullptr;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: called on close
//-------------------------------------------------------------------------------------------------------
void CPlaylistLoadPanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, PLAYLIST_LOAD_COMMAND))
	{
		//check the filename
		if (m_PlaylistFilename->GetTextLength() <= 0)
		{
			//show error
			QueryBox* box = new QueryBox("Error", "Got empty playlist filename!", this);
			box->DoModal(this);
			box->Activate();
			return;
		}

		//tell the song panel write the file
		char buf[64];
		m_PlaylistFilename->GetText(buf, sizeof(buf));
		songpanel->LoadPlaylistFromFile(buf);

		//close this panel
		Close();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//playlist save/load functions
void PlaylistSavePanel_Open()
{
	if (!g_PlaylistSavePanel)
		g_PlaylistSavePanel = new CPlaylistSavePanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));
}

void PlaylistSavePanel_Close()
{
	if (g_PlaylistSavePanel)
		delete g_PlaylistSavePanel;

	g_PlaylistSavePanel = nullptr;
}

void PlaylistLoadPanel_Open(vgui::Frame* songpanel)
{
	if (!g_PlaylistLoadPanel)
	{
		//HACK: make the playlist load panel
		g_PlaylistLoadPanel = (CPlaylistLoadPanel*)0x001;
		CPlaylistLoadPanel* tmp = new CPlaylistLoadPanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));
		if (g_PlaylistLoadPanel)
			g_PlaylistLoadPanel = tmp;
	}

	if (!g_PlaylistLoadPanel)
	{
		//error
		QueryBox* box = new QueryBox("Error", "Got 0 saved playlists!", songpanel);
		box->DoModal(songpanel);
		box->Activate();
	}
}

void PlaylistLoadPanel_Close()
{
	if (g_PlaylistLoadPanel)
		delete g_PlaylistLoadPanel;

	g_PlaylistLoadPanel = nullptr;
}






//song panel notification panel
class CSongPanelNotificationPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CSongPanelNotificationPanel, vgui::Panel);

	CSongPanelNotificationPanel(const char* name);

	//paint
	virtual void Paint() override;

	//adds a message
	virtual void AddMessage(const char* text);

	~CSongPanelNotificationPanel();

private:
	//message print font
	vgui::HFont m_MessageFont;

	//show message
	float m_NextMessageEndTime = 0.0f;
	char* m_Message = nullptr;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor for song notification panel
//-----------------------------------------------------------------------------
CSongPanelNotificationPanel::CSongPanelNotificationPanel(const char* name)
	: BaseClass(nullptr, name)
{
	m_Message = nullptr;

	//set parent
	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));

	//set other stuff
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);
	SetVisible(true);
	RequestFocus();
	MoveToFront();

	//size panel
	SetBounds(0, 0, 700, 45);

	//set font for text's
	vgui::IScheme* scheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));
	if (scheme)
		m_MessageFont = scheme->GetFont("Default", true);
}

//-----------------------------------------------------------------------------
// Purpose: Adds a notification
//-----------------------------------------------------------------------------
void CSongPanelNotificationPanel::AddMessage(const char* text)
{
	if (m_Message)
		free(m_Message);

	if (text)
	{
		m_Message = strdup(text);
		m_NextMessageEndTime = gpGlobals->curtime + 7.5f;
	}
	else
	{
		m_Message = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on panel paint
//-----------------------------------------------------------------------------
void CSongPanelNotificationPanel::Paint()
{
	//dont paint when paused
	if (engine->IsPaused() || engine->IsDrawingLoadingImage() || !m_Message)
		return;

	//draw background
	vgui::surface()->DrawSetColor(Color(40, 40, 40, 35));
	vgui::surface()->DrawFilledRectFastFade(0, 0, 700, 45, 400, 700, 150, 0, true);

	//set text stuff
	vgui::surface()->DrawSetTextPos(10, 10);
	vgui::surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	vgui::surface()->DrawSetTextFont(m_MessageFont);

	const char* text = m_Message;
	int len = Q_strlen(text);

	//make a new wchar_t* array and copy the contents from the string to the buffer
	wchar_t* buffer = new wchar_t[len + 1];
	for (int i = 0; i < len; i++)
		buffer[i] = (wchar_t)(unsigned char)text[i];

	//null terminate
	buffer[len] = '\0';

	//print text
	vgui::surface()->DrawPrintText(buffer, len);

	//delete buffer
	delete[] buffer;

	//check m_NextMessageEndTime
	if (gpGlobals->curtime >= m_NextMessageEndTime)
	{
		//delete m_Message
		free(m_Message);
		m_Message = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSongPanelNotificationPanel::~CSongPanelNotificationPanel()
{
	if (m_Message)
		free(m_Message);
}







//playlist progress bar
class CSongPanelProgressBar : public Slider
{
	DECLARE_CLASS_SIMPLE(CSongPanelProgressBar, Slider);

public:

	//constructor
	CSongPanelProgressBar(Panel* parent, char const* panelName) :
		BaseClass(parent, panelName)
	{
		SetPaintEnabled(false);
		SetRange(0, 100);
	}

	//sets the progress bar amount
	void SetProgress(float frac)
	{
		SetValue((int)(frac * 100.0f + 0.5f), false);
	}

	//paints the background
	virtual void PaintBackground()
	{
		//BaseClass::PaintBackground();

		int w, h;
		GetSize(w, h);

		float frac = (float)GetValue() * 0.01f;

		int barend = (int)((float)(w - 2) * frac + 0.5f);

		surface()->DrawSetColor(GetBgColor());
		surface()->DrawFilledRect(0, 0, w, h);
		surface()->DrawSetColor(GetFgColor());
		surface()->DrawFilledRect(1, 1, barend, h - 1);
	}

	void OnMousePressed(vgui::MouseCode code)
	{
		return;
	}

	//paints the background
	virtual void ApplySchemeSettings(IScheme* pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		SetFgColor(Color(200, 255, 200, 255));
		SetBgColor(pScheme->GetColor("BorderDark", Color(0, 0, 0, 255)));
	}
};






//playlist button
class CPlaylistButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE(CPlaylistButton, vgui::Button)

	CPlaylistButton(vgui::Panel* parent, const char* name, const char* text, vgui::Panel* target = nullptr)
		: BaseClass(parent, name, text, target)
	{
		m_ColorSelected = Color(200, 200, 200, 200);
		m_FgColorSelected = Color(0, 0, 0, 255);

		m_ColorPlaying = Color(255, 235, 200, 200);
		m_FgColorPlaying = Color(0, 0, 0, 255);
	}

	//apply scheme settings
	void ApplySchemeSettings(vgui::IScheme* scheme)
	{
		BaseClass::ApplySchemeSettings(scheme);

		m_ColorNotSelected = GetButtonArmedBgColor();
		m_FgColorNotSelected = GetButtonArmedFgColor();
	}

	//paints the background
	void PaintBackground()
	{
		if (m_bIsSelected)
			SetBgColor(m_ColorSelected);
		else if (m_bIsPlaying)
			SetBgColor(m_ColorPlaying);
		else
			SetBgColor(m_ColorNotSelected);

		BaseClass::PaintBackground();
	}

	//paints
	void Paint()
	{
		if (m_bIsSelected)
			SetFgColor(m_FgColorSelected);
		else if (m_bIsPlaying)
			SetFgColor(m_FgColorPlaying);
		else
			SetFgColor(m_FgColorNotSelected);

		BaseClass::Paint();
	}

	//is this selected or not
	bool m_bIsSelected = false;
	bool m_bIsPlaying = false;
	static Color m_ColorSelected;
	static Color m_ColorPlaying;
	static Color m_ColorNotSelected;
	static Color m_FgColorSelected;
	static Color m_FgColorPlaying;
	static Color m_FgColorNotSelected;
};

Color CPlaylistButton::m_ColorSelected = Color();
Color CPlaylistButton::m_ColorPlaying = Color();
Color CPlaylistButton::m_ColorNotSelected = Color();
Color CPlaylistButton::m_FgColorSelected = Color();
Color CPlaylistButton::m_FgColorPlaying = Color();
Color CPlaylistButton::m_FgColorNotSelected = Color();






//playlist queued songs stuff
static CUtlVector<const char*> g_QueuedSongs;
static CUtlVector<AloneModSong_t*> g_QueuedSongData;
static int g_CurrentQueuedSongIndex = 0;
static bool g_bIsPlayingPlaylist = false;

#define PLAYLIST_QUEUED_SONG_RANDOM "%:RANDOM:%"



//song playlist panel
#define MAX_PLAYLIST_BUTTONS 8
#define PLAYLIST_COMMAND_SAVE "SavePlaylist"
#define PLAYLIST_COMMAND_LOAD "LoadPlaylist"

class CSongPlaylist : public Divider
{
	DECLARE_CLASS_SIMPLE(CSongPlaylist, Divider)
public:
	//constructor
	CSongPlaylist(Panel* parent, const char* name);

	void CreateElements();

	//song stuff
	void AddSong(AloneModSong_t* song);
	void RemoveSelectedSong();
	void ChangeSelectedSong(AloneModSong_t* song);
	void SetSongPlayingToPlaying(int index, bool playing);
	void MoveSelectedSong(bool up);
	void ClearSongPlaying();
	void ClearSelection();
	void CollectPlaylist();
	void ClearEverything();

	//other
	void OnCommand(const char* pszCommand);
	void OnMousePressed(vgui::MouseCode code);
	void OnKeyCodePressed(vgui::MouseCode code);

	virtual void OnMouseWheeled(int delta);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");

private:
	friend class CSongPanel;

	//adds a button
	void AddButton(const char* name);

	//buttons/songs
	CUtlVector<CPlaylistButton*> m_Buttons;
	CUtlVector<AloneModSong_t*> m_Songs;

	//slider
	ScrollBar* m_ScrollBar;

	//button y offset
	int m_iCurrentButtonY = 24;
};


//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for playlist test
//-------------------------------------------------------------------------------------------------------
CSongPlaylist::CSongPlaylist(Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//create playlist label
	Label* PlaylistLabel = new Label(this, "PlaylistLabel", "Playlist:");
	PlaylistLabel->SetBounds(5, 2, 200, 20);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Creates the elements for the song panel (scroll bar, save button, load button, etc...)
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::CreateElements()
{
	int wide, tall;
	GetSize(wide, tall);

	//create the scroll bar
	m_ScrollBar = new ScrollBar(this, "ScrollBar", true);
	m_ScrollBar->SetBounds(wide - 20, 0, 20, tall - 24);
	m_ScrollBar->AddActionSignalTarget(this);

	//create bottom buttons
	vgui::Button* SaveButton = new Button(this, "SaveButton", "Save Playlist");
	SaveButton->SetBounds(0, tall - 24, wide / 2, 22);
	SaveButton->AddActionSignalTarget(this);
	SaveButton->SetCommand(PLAYLIST_COMMAND_SAVE);

	vgui::Button* LoadButton = new Button(this, "LoadButton", "Load Playlist");
	LoadButton->SetBounds(wide / 2, tall - 24, wide / 2, 22);
	LoadButton->AddActionSignalTarget(this);
	LoadButton->SetCommand(PLAYLIST_COMMAND_LOAD);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Adds a song to the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::AddSong(AloneModSong_t* song)
{
	//add the song if the playlist is playing
	m_Songs.AddToTail(song);

	//add the song to the queued songs if the playlist is playing
	if (g_bIsPlayingPlaylist)
	{
		const char* add = song->SongName;
		if (!add)
		{
			//choose a random song
			add = PLAYLIST_QUEUED_SONG_RANDOM;
		}

		g_QueuedSongs.AddToTail(add);
		g_QueuedSongData.AddToTail(song);
	}

	//add the button
	AddButton(song->Name);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Removes a song from the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::RemoveSelectedSong()
{
	int index = -1;

	//get selected index
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		if (m_Buttons[i]->m_bIsSelected)
		{
			index = i;
			break;
		}
	}

	//check for invalid index
	if (index == -1)
	{
		//show error
		QueryBox* box = new QueryBox("Error", "No song currently selected!", GetParent());
		box->DoModal(dynamic_cast<Frame*>(GetParent()));
		box->Activate();

		return;
	}

	//if that song is playing then play the next song and remove this song
	bool ShouldPlay = false;
	if (g_bIsPlayingPlaylist && g_CurrentQueuedSongIndex >= 0 && g_CurrentQueuedSongIndex < g_QueuedSongData.Count() && index == g_CurrentQueuedSongIndex)
	{
		if (g_CurrentQueuedSongIndex == g_QueuedSongData.Count() - 1 && g_CurrentQueuedSongIndex > 0)
			g_CurrentQueuedSongIndex--;

		ShouldPlay = true;
	}

	//delete the currend queued song at the index i
	g_QueuedSongs.Remove(index);
	g_QueuedSongData.Remove(index);
	m_Songs.Remove(index);

	//delete the selected button
	delete m_Buttons[index];
	m_Buttons.Remove(index);
	m_iCurrentButtonY -= 24;

	//fix up the buttons
	int y = 24;
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		m_Buttons[i]->SetPos(5, y);
		m_Buttons[i]->SetCommand(CFmtStr("PlaylistItem:%d", i));

		y = y + 24;
	}

	//select next (or previous) item
	if (index >= m_Buttons.Count())
		index--;

	if (m_Buttons.Count() > 0)
		GetParent()->OnCommand(CFmtStr("PlaylistItem:%d", index));

	//check to see if we should enable/disable the scroll wheel
	if (m_Buttons.Count() < MAX_PLAYLIST_BUTTONS)
	{
		m_ScrollBar->SetRange(0, 0);
		m_ScrollBar->SetValue(0);
	}
	else
	{
		//store scroll bar value
		int value = m_ScrollBar->GetValue();

		//set range
		int min, max;
		m_ScrollBar->GetRange(min, max);
		m_ScrollBar->SetRange(min, max - 1);

		if (value == max)
			m_ScrollBar->SetValue(max - 1);
		else
			m_ScrollBar->SetValue(value);
	}

	//format the buttons
	OnScrollBarSliderMoved();

	//start the next song
	if (ShouldPlay)
		songpanel->StartPlaylist();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Adds a song to the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::ChangeSelectedSong(AloneModSong_t* song)
{
	int index = -1;

	//get selected index
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		if (m_Buttons[i]->m_bIsSelected)
		{
			index = i;
			break;
		}
	}

	//check for invalid index or to see if index == g_CurrentQueuedSongIndex
	if (index == -1)
	{
		//show error
		QueryBox* box = new QueryBox("Error", "No song currently selected!", GetParent());
		box->DoModal(dynamic_cast<Frame*>(GetParent()));
		box->Activate();
		return;
	}

	//set button text
	m_Buttons[index]->SetText(song->Name);

	//set both m_Songs and g_QueuedSongs
	m_Songs[index] = song;

	//check g_QueuedSongData
	if (g_QueuedSongData.Count() > 0)
		g_QueuedSongData[index] = song;

	//check g_QueuedSongs
	if (g_QueuedSongs.Count() > 0)
	{
		//get songname
		const char* songname = song->SongName;
		if (!songname)
			songname = PLAYLIST_QUEUED_SONG_RANDOM;

		g_QueuedSongs[index] = songname;
	}

	//see if index == g_CurrentQueuedSongIndex and g_bIsPlayingPlaylist
	if (index == g_CurrentQueuedSongIndex && g_bIsPlayingPlaylist)
		songpanel->StartPlaylist();

	//clear selected
	ClearSelection();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Adds a song to the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::SetSongPlayingToPlaying(int index, bool playing)
{
	//check for invalid index or to see if index == g_CurrentQueuedSongIndex
	if (index < 0 || index >= m_Buttons.Count())
		return;

	//set button text
	m_Buttons[index]->m_bIsPlaying = playing;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Moves the selected song up/down
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::MoveSelectedSong(bool up)
{
	int index = -1;

	//get selected index
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		if (m_Buttons[i]->m_bIsSelected)
		{
			index = i;
			break;
		}
	}

	//check for invalid index or to see if index == g_CurrentQueuedSongIndex
	if (index == -1)
	{
		//show error
		QueryBox* box = new QueryBox("Error", "No song currently selected!", GetParent());
		box->DoModal(dynamic_cast<Frame*>(GetParent()));
		box->Activate();
		return;
	}

	//see if we can move up/down
	if ((!up && index >= m_Buttons.Count() - 1) || (up && index <= 0))
		return;

	//store temp data
	CPlaylistButton* button = m_Buttons[index];
	AloneModSong_t* song = m_Songs[index];

	int value = up ? index - 1 : index + 1;

	//handle the queue if we have 1
	if (g_bIsPlayingPlaylist && index < g_QueuedSongs.Count() && index < g_QueuedSongData.Count() && index >= 0)
	{
		AloneModSong_t* queuesong = g_QueuedSongData[index];
		const char* queuesongname = g_QueuedSongs[index];

		g_QueuedSongData[index] = g_QueuedSongData[value];
		g_QueuedSongs[index] = g_QueuedSongs[value];

		g_QueuedSongData[value] = queuesong;
		g_QueuedSongs[value] = queuesongname;
	}

	//move
	m_Songs[index] = m_Songs[value];
	m_Buttons[index] = m_Buttons[value];

	//set new value's
	m_Buttons[value] = button;
	m_Songs[value] = song;

	//move the queued song index if needed
	if (g_bIsPlayingPlaylist)
	{
		if (g_CurrentQueuedSongIndex == value)
			g_CurrentQueuedSongIndex = index;
		else if (g_CurrentQueuedSongIndex == index)
			g_CurrentQueuedSongIndex = value;
	}

	//move the buttons
	int x1, y1, x2, y2;
	m_Buttons[index]->GetPos(x1, y1);
	m_Buttons[value]->GetPos(x2, y2);

	//swap their positions
	m_Buttons[index]->SetPos(x2, y2);
	m_Buttons[value]->SetPos(x1, y1);

	//set the scroll bar value
	if (value < m_ScrollBar->GetValue())
		m_ScrollBar->SetValue(m_ScrollBar->GetValue() - 1);
	else if (value > m_ScrollBar->GetValue() + MAX_PLAYLIST_BUTTONS)
		m_ScrollBar->SetValue(m_ScrollBar->GetValue() + 1);

	//fix up the commands
	for (int i = 0; i < m_Buttons.Count(); i++)
		m_Buttons[i]->SetCommand(CFmtStr("PlaylistItem:%d", i));
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Adds a song to the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::ClearSongPlaying()
{
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		m_Buttons[i]->m_bIsPlaying = false;
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Clears the current selected item
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::ClearSelection()
{
	//get selected index
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		m_Buttons[i]->m_bIsSelected = false;
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Adds a button into the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::AddButton(const char* name)
{
	//get the panel size
	int wide, tall;
	GetSize(wide, tall);

	//adds a song button
	CPlaylistButton* songbutton = new CPlaylistButton(this, "SongButton", name, this);
	songbutton->AddActionSignalTarget(GetParent());
	songbutton->SetBounds(5, m_iCurrentButtonY, wide - 30, 22);
	songbutton->SetCommand(CFmtStr("PlaylistItem:%d", m_Buttons.Count()));

	//add the button to the button array
	m_Buttons.AddToTail(songbutton);

	//increment the current y
	m_iCurrentButtonY += 24;

	//select the button
	OnCommand(CFmtStr("PlaylistItem:%d", m_Buttons.Count() - 1));

	//check ammount of buttons and change scroll wheel if needed
	if (m_Buttons.Count() >= MAX_PLAYLIST_BUTTONS)
	{
		m_ScrollBar->SetRange(0, m_Buttons.Count() - MAX_PLAYLIST_BUTTONS);
		m_ScrollBar->SetRangeWindow(1);
		m_ScrollBar->SetValue(m_Buttons.Count() - MAX_PLAYLIST_BUTTONS);
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Collects all of the playlist's songs
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::CollectPlaylist()
{
	g_QueuedSongData.RemoveAll();
	g_QueuedSongs.RemoveAll();

	//add all the songs into the queue
	for (int i = 0; i < m_Songs.Count(); i++)
	{
		//get the song name
		const char* add = m_Songs[i]->SongName;
		if (!add)
		{
			//if no song name then choose the random song
			add = PLAYLIST_QUEUED_SONG_RANDOM;
		}

		g_QueuedSongs.AddToTail(add);
		g_QueuedSongData.AddToTail(m_Songs[i]);
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Clears everything
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::ClearEverything()
{
	//delete the buttons
	for (int i = 0; i < m_Buttons.Count(); i++)
		delete m_Buttons[i];

	//clear the arrays
	m_Buttons.RemoveAll();
	m_Songs.RemoveAll();

	//clear the queue
	g_QueuedSongData.RemoveAll();
	g_QueuedSongs.RemoveAll();
	g_CurrentQueuedSongIndex = 0;
	
	//tell the song panel to start the playlist (should result in it stopping the playlist)
	if (g_bIsPlayingPlaylist)
		songpanel->StartPlaylist();

	//stop the playlist
	g_bIsPlayingPlaylist = false;

	//reset the scroll bar
	m_ScrollBar->SetRange(0, 0);
	m_ScrollBar->SetValue(0);

	//reset the index
	m_iCurrentButtonY = 24;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, PLAYLIST_COMMAND_SAVE))
	{
		//check playlist size
		if (m_Buttons.Count() <= 0)
		{
			QueryBox* box = new QueryBox("Error", "Playlist is empty!", GetParent());
			box->DoModal(dynamic_cast<Frame*>(GetParent()));
			box->Activate();
			return;
		}

		PlaylistLoadPanel_Close();
		PlaylistSavePanel_Open();
	}
	else if (!Q_strcmp(pszCommand, PLAYLIST_COMMAND_LOAD))
	{
		PlaylistSavePanel_Close();
		PlaylistLoadPanel_Open(dynamic_cast<vgui::Frame*>(GetParent()));
	}
	else if (Q_strstr(pszCommand, "PlaylistItem:") == pszCommand)
	{
		//get the index
		int index = atoi(pszCommand + Q_strlen("PlaylistItem:"));

		//check for valid index
		if (index < 0 || index >= m_Buttons.Count())
			return;

		//select button
		for (int i = 0; i < m_Buttons.Count(); i++)
		{
			if (i == index)
			{
				m_Buttons[i]->m_bIsSelected = true;
			}
			else
			{
				m_Buttons[i]->m_bIsSelected = false;
			}
		}

		//set the scroll bar value
		if (index < m_ScrollBar->GetValue())
			m_ScrollBar->SetValue(m_ScrollBar->GetValue() - 1);
		else if (index > m_ScrollBar->GetValue() + MAX_PLAYLIST_BUTTONS)
			m_ScrollBar->SetValue(m_ScrollBar->GetValue() + 1);
	}

	//set this as the focus
	RequestFocus();

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on mouse press
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::OnMousePressed(vgui::MouseCode code)
{
	BaseClass::OnMousePressed(code);

	ClearSelection();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on mouse press
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::OnKeyCodePressed(vgui::MouseCode code)
{
	//must be an arrow key
	if (code == vgui::MouseCode::KEY_UP || code == vgui::MouseCode::KEY_DOWN)
	{
		bool up = code == vgui::MouseCode::KEY_UP;

		//get selected index
		int index = -1;

		//get selected index
		for (int i = 0; i < m_Buttons.Count(); i++)
		{
			if (m_Buttons[i]->m_bIsSelected)
			{
				index = i;
				break;
			}
		}

		if (index == -1)
			return;

		//if shift is held then move the item. Else select it
		if (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LSHIFT) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RSHIFT))
			MoveSelectedSong(up);
		else
			GetParent()->OnCommand(CFmtStr("PlaylistItem:%d", up ? index - 1 : index + 1));

		return;
	}
	else if (code == vgui::KeyCode::KEY_DELETE)
	{
		RemoveSelectedSong();
		return;
	}

	BaseClass::OnKeyCodePressed(code);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on mouse press
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::OnScrollBarSliderMoved()
{
	int value = m_ScrollBar->GetValue();

	//get the value and move everything up by that amount
	int iCurr = 24;
	for (int i = 0; i < m_Buttons.Count(); i++)
	{
		m_Buttons[i]->SetPos(5, iCurr - (24 * value));

		iCurr += 24;

		//check to see if its more then value + MAX_PLAYLIST_BUTTONS
		if (i == value + MAX_PLAYLIST_BUTTONS)
			iCurr += 500;
	}

	//set m_Buttons[value] to be not visible (just hide it)
	if (value > 0)
		m_Buttons[value - 1]->SetPos(5, -30);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on mouse press
//-------------------------------------------------------------------------------------------------------
void CSongPlaylist::OnMouseWheeled(int delta)
{
	BaseClass::OnMouseWheeled(delta);

	//check scroll bar bounds
	int min, max;
	m_ScrollBar->GetRange(min, max);

	if ((m_ScrollBar->GetValue() <= min && delta > 0) || (m_ScrollBar->GetValue() >= max && delta < 0))
		return;

	//handle scroll for scroll wheel
	m_ScrollBar->SetValue(m_ScrollBar->GetValue() - delta);
}








//slider for song panel
class CSongPanelSlider : public vgui::Slider
{
	DECLARE_CLASS_SIMPLE(CSongPanelSlider, vgui::Slider);
public:
	//constructor
	CSongPanelSlider(vgui::Panel* parent, const char* name) : BaseClass(parent, name)
	{
	}

	//called on mouse press
	void OnMousePressed(vgui::MouseCode code)
	{
		GetTooltip()->HideTooltip();

		if (code == vgui::MouseCode::MOUSE_RIGHT)
			SetValue(100);
		else
			BaseClass::OnMousePressed(code);
	}

	//called on mouse wheeled
	void OnMouseWheeled(int delta)
	{
		SetValue(GetValue() + delta);
		BaseClass::OnMouseWheeled(delta);
	}
};









//vgui song panel class
#define COMMAND_PREVIOUS_SONG "PreviousSong"
#define COMMAND_NEXT_SONG "NextSong"
#define COMMAND_PLAY_SONG "PlaySong"
#define COMMAND_STOP_SONG "StopSong"

#define COMMAND_PLAYLIST_ADD_SONG "AddSong"
#define COMMAND_PLAYLIST_REMOVE_SONG "RemoveSong"
#define COMMAND_PLAYLIST_CHANGE_SONG "ChangeSong"
#define COMMAND_PLAYLIST_MOVE_SONG_UP "MoveSongUp"
#define COMMAND_PLAYLIST_MOVE_SONG_DOWN "MoveSongDown"

#define COMMAND_PLAY_PLAYLIST "PlayPlaylist"

class CSongPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CSongPanel, vgui::Frame);
public:

	//constrctor and destructor
	CSongPanel(vgui::VPANEL parent);
	~CSongPanel();

	//initalizes the panel items
	void Init();

	//called on level change and new level
	void OnLevelChange();
	void OnNewLevel();

	//playlist functions
	void StartPlaylist(bool bMessWithPitch = true);
	void SavePlaylist(const char* filename);
	void LoadPlaylist(const char* filename);

	//song stuff
	int m_SongGuid = 0;					//the song guid (unique identifoer)
	float m_SongStartTime = 0.0f;		//the time (gpglobals->curtime) the song started
	float m_TimeElapsed = 0.0f;			//how long the song has been going for
	float m_RealTimeElapsed = 0.0f;		//for display only
	const char* m_CurrPlaying = nullptr;
	float m_SongDuration = 0.0f;
protected:
	//functions
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);
	void OnClose();

	//keyboard stuff
	void OnKeyCodePressed(KeyCode code);
public:
	//keyvalues file
	KeyValues* m_kvMain = nullptr;
	ComboBox* m_SongsComboBox;

	//playlist adding/other button
	Button* m_AddSongButton = nullptr;
	Button* m_RemoveSongButton = nullptr;
	Button* m_ChangeSongButton = nullptr;

	Button* m_MoveUpButton = nullptr;
	Button* m_MoveDownButton = nullptr;

	Label* m_VolumeLabel = nullptr;
	Label* m_PitchLabel = nullptr;

	//sliders for volume and pitch
	CSongPanelSlider* m_VolumeSlider = nullptr;
	CSongPanelSlider* m_PitchSlider = nullptr;

	//playlist buttons
	Button* m_NextSongButton = nullptr;
	Button* m_PreviousSongButton = nullptr;

	//bottom buttons
	Button* m_PlayButton = nullptr;
	Button* m_PlayPlaylistButton = nullptr;
	Button* m_StopButton = nullptr;

	//playlist
	CSongPlaylist* m_Playlist;

	//progress stuff
	CSongPanelProgressBar* m_ProgressBar;
	Label* m_DurationText;
	Label* m_ElapsedText;
	Label* m_CurrentlyPlayingText;

	//list of songs and random songs
	CUtlVector<AloneModRandomSong_t> m_RandomSongs;
	CUtlVector<AloneModSong_t> m_Songs;

	//has this panel been initalized yet
	bool bInit = false;

	//hacks:
	float LastUpdateTime = 0.0f;
	float DisplayLastUpdateTime = 0.0f;
};

//-------------------------------------------------------------------------------------------------------
// Purpose: called from server when a new level is loaded
//-------------------------------------------------------------------------------------------------------
CSongPanel::CSongPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "SongPanel")
{
	//initalize all the variables
	m_SongGuid = INVALID_SONG_GUID;
	m_SongStartTime = 0.0f;
	m_TimeElapsed = 0.0f;
	m_RealTimeElapsed = 0.0f;
	m_CurrPlaying = nullptr;

	//set the parent
	SetParent(parent);

	//set keyboard stuff
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	//set other panel stuff
	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(true);

	//set the panel scheme
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	//add a 100ms tick signal
	vgui::ivgui()->AddTickSignal(GetVPanel(), 50);

	//load the song panel keyvalues file
	m_kvMain = new KeyValues("SongPanel");
	m_kvMain->UsesEscapeSequences(true);

	if (!m_kvMain->LoadFromFile(filesystem, "resource/panels/SongPanel.txt", "MOD"))
	{
		ConWarning("Error Opening SongPanel.txt Aborting Panel Load\n");
		return;
	}

	//initalize all the panel stuff
	Init();
}

//macro for finding keyvalues
#define SONG_PANEL_KEYVALUES_FIND(name, string)																\
KeyValues* name = m_kvMain->FindKey(string);																\
if (!name)																									\
{																											\
	m_kvMain->deleteThis();																					\
	ConWarning("Got NULL Subkey: \"" string "\" In File: \"SongPanel.txt\" Aborting Panel Load!\n");		\
	m_kvMain = nullptr;																						\
	return;																									\
}

//-------------------------------------------------------------------------------------------------------
// Purpose: initalizes all the panel stuff
//-------------------------------------------------------------------------------------------------------
void CSongPanel::Init()
{
	//get all the elements
	SONG_PANEL_KEYVALUES_FIND(PlayButtonKv, "PlayButton");
	SONG_PANEL_KEYVALUES_FIND(PreviousSongButtonKv, "PreviousSongButton");
	SONG_PANEL_KEYVALUES_FIND(NextSongButtonKv, "NextSongButton");
	SONG_PANEL_KEYVALUES_FIND(PlayPlaylistButtonKv, "PlayPlaylistButton");
	SONG_PANEL_KEYVALUES_FIND(StopButtonKv, "StopButton");
	SONG_PANEL_KEYVALUES_FIND(VolumeSliderKv, "SliderVol");
	SONG_PANEL_KEYVALUES_FIND(VolumeTextKv, "VolumeText");
	SONG_PANEL_KEYVALUES_FIND(Divider1Kv, "Divider1");
	SONG_PANEL_KEYVALUES_FIND(AddSongButtonKv, "AddSongButton");
	SONG_PANEL_KEYVALUES_FIND(RemoveSongButtonKv, "RemoveSongButton");
	SONG_PANEL_KEYVALUES_FIND(ChangeSongButtonKv, "ChangeSongButton");
	SONG_PANEL_KEYVALUES_FIND(MoveSongUpButtonKv, "MoveSongUpButton");
	SONG_PANEL_KEYVALUES_FIND(MoveSongDownButtonKv, "MoveSongDownButton");
	SONG_PANEL_KEYVALUES_FIND(PitchSliderKv, "SliderPitch");
	SONG_PANEL_KEYVALUES_FIND(PitchTextKv, "PitchText");
	SONG_PANEL_KEYVALUES_FIND(SongListKv, "SongList");
	SONG_PANEL_KEYVALUES_FIND(PlaylistTestKv, "PlaylistTest");
	SONG_PANEL_KEYVALUES_FIND(ProgressBarKv, "ProgressBar");
	SONG_PANEL_KEYVALUES_FIND(DurationTextKv, "DurationText");
	SONG_PANEL_KEYVALUES_FIND(ElapsedTextKv, "ElapsedText");
	SONG_PANEL_KEYVALUES_FIND(CurrentlyPlayingTextKv, "CurrentlyPlayingText");

	//now get all the songs
	KeyValues* SongsSubkey = SongListKv->FindKey("Songs");
	if (!SongsSubkey)
	{
		m_kvMain->deleteThis();
		m_kvMain = nullptr;
		ConWarning("Got NULL Subkey: \"Songs\" For SongList In File: \"SongPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//must have songs inside
	if (!SongsSubkey->GetFirstSubKey())
	{
		m_kvMain->deleteThis();
		m_kvMain = nullptr;
		ConWarning("Got No keyvalue Pair In Subkey: \"Songs\" For Songs In File: \"SongPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//must have songs inside
	FOR_EACH_SUBKEY(SongsSubkey, Songs)
	{
		const char* name = Songs->GetName();

		//look for song with same name
		for (int i = 0; i < m_Songs.Count(); i++)
		{
			if (!Q_strcmp(m_Songs[i].Name, name))
			{
				m_kvMain->deleteThis();
				m_kvMain = nullptr;
				ConWarning("Got Multiple Of The Same Song Name In Subkey: \"Songs\" For Songs In File: \"SongPanel.txt\" Aborting Panel Load!\n");
				return;
			}
		}

		//check for [random]
		if (!Q_stricmp(name, "[random]"))
		{
			//store random song
			AloneModRandomSong_t& RandomSongItem = m_RandomSongs[m_RandomSongs.AddToTail()];

			//go through each value
			FOR_EACH_VALUE(Songs, RandomSong)
			{
				//check for random set name
				if (!Q_strcmp(RandomSong->GetName(), "name"))
					RandomSongItem.Name = RandomSong->GetString();

				//check for a song name
				if (!Q_strcmp(RandomSong->GetName(), "song"))
					RandomSongItem.RandomSongs.AddToTail(RandomSong->GetString());
			}

			//check for the random name
			if (!RandomSongItem.Name)
			{
				m_kvMain->deleteThis();
				m_kvMain = nullptr;
				ConWarning("Failed to get name for random song section inside \"SongPanel.txt\" Aborting Panel Load!\n");
				return;
			}

			//the song MUST not have a . in its name
			if (Q_strstr(RandomSongItem.Name, "."))
			{
				m_kvMain->deleteThis();
				m_kvMain = nullptr;
				ConWarning("Random song section name must NOT have a . inside it for \"SongPanel.txt\" Aborting Panel Load!\n");
				return;
			}
		}
		else
		{
			//create a new song item
			AloneModSong_t song;
			song.Name = Songs->GetName();

			//check the song's actuall song too see if its a random song or not
			const char* SongName = Songs->GetString();
			for (int i = 0; i < m_RandomSongs.Count(); i++)
			{
				//check song name
				if (!Q_strcmp(SongName, m_RandomSongs[i].Name))
				{
					song.RandomSong = &m_RandomSongs[i];
					break;
				}
			}

			//check for random song. If nullptr then set the song's song name
			if (!song.RandomSong)
			{
				song.SongName = SongName;
			}

			//add the song to the array
			m_Songs.AddToTail(song);
		}
	}

	//set title and bounds of panel
	SetTitle(m_kvMain->GetString("Title", "Title"), false);
	SetBounds(m_kvMain->GetInt("XPos"), m_kvMain->GetInt("YPos"), m_kvMain->GetInt("Wide"), m_kvMain->GetInt("Tall"));

	//create the songs combo box
	m_SongsComboBox = new ComboBox(this, "SongsComboBox", 20, false);
	m_SongsComboBox->SetBounds(SongListKv->GetInt("XPos"), SongListKv->GetInt("YPos"), SongListKv->GetInt("Wide"), SongListKv->GetInt("Tall"));

	//add all the song names
	for (int i = 0; i < m_Songs.Count(); i++)
		m_SongsComboBox->AddItem(m_Songs[i].Name, nullptr);

	m_SongsComboBox->ActivateItem(SongListKv->GetInt("ActiveItem"));

	//add song to playlist button
	m_AddSongButton = new vgui::Button(this, "AddSongButton", AddSongButtonKv->GetString("Text"));
	m_AddSongButton->SetBounds(AddSongButtonKv->GetInt("XPos"), AddSongButtonKv->GetInt("YPos"), AddSongButtonKv->GetInt("Wide"), AddSongButtonKv->GetInt("Tall"));
	m_AddSongButton->SetCommand(COMMAND_PLAYLIST_ADD_SONG);

	m_AddSongButton->GetTooltip()->SetTooltipDelay(500);
	m_AddSongButton->GetTooltip()->SetTooltipFormatToMultiLine();
	m_AddSongButton->GetTooltip()->SetText("Adds the selected song from the dropdown menu above into the playlist.");

	//remove song from playlist button
	m_RemoveSongButton = new vgui::Button(this, "RemoveSongButton", RemoveSongButtonKv->GetString("Text"));
	m_RemoveSongButton->SetBounds(RemoveSongButtonKv->GetInt("XPos"), RemoveSongButtonKv->GetInt("YPos"), RemoveSongButtonKv->GetInt("Wide"), RemoveSongButtonKv->GetInt("Tall"));
	m_RemoveSongButton->SetCommand(COMMAND_PLAYLIST_REMOVE_SONG);

	m_RemoveSongButton->GetTooltip()->SetTooltipDelay(500);
	m_RemoveSongButton->GetTooltip()->SetTooltipFormatToMultiLine();
	m_RemoveSongButton->GetTooltip()->SetText("Removes the selected song from inside the playlist.");

	//change song for playlist button
	m_ChangeSongButton = new vgui::Button(this, "ChangeSongButton", ChangeSongButtonKv->GetString("Text"));
	m_ChangeSongButton->SetBounds(ChangeSongButtonKv->GetInt("XPos"), ChangeSongButtonKv->GetInt("YPos"), ChangeSongButtonKv->GetInt("Wide"), ChangeSongButtonKv->GetInt("Tall"));
	m_ChangeSongButton->SetCommand(COMMAND_PLAYLIST_CHANGE_SONG);

	m_ChangeSongButton->GetTooltip()->SetTooltipDelay(500);
	m_ChangeSongButton->GetTooltip()->SetTooltipFormatToMultiLine();
	m_ChangeSongButton->GetTooltip()->SetText("When a song is selected in the playlist, Select a song from the dropdown menu above and press this button. That will change the selected song inside the playlist.");

	//move song down song for playlist button
	m_MoveDownButton = new vgui::Button(this, "MoveSongDown", MoveSongDownButtonKv->GetString("Text"));
	m_MoveDownButton->SetBounds(MoveSongDownButtonKv->GetInt("XPos"), MoveSongDownButtonKv->GetInt("YPos"), MoveSongDownButtonKv->GetInt("Wide"), MoveSongDownButtonKv->GetInt("Tall"));
	m_MoveDownButton->SetCommand(COMMAND_PLAYLIST_MOVE_SONG_DOWN);

	m_MoveDownButton->GetTooltip()->SetTooltipDelay(500);
	m_MoveDownButton->GetTooltip()->SetTooltipFormatToMultiLine();
	m_MoveDownButton->GetTooltip()->SetText("Moves the selected button down the playlist");

	//move song up song for playlist button
	m_MoveUpButton = new vgui::Button(this, "MoveSongUp", MoveSongUpButtonKv->GetString("Text"));
	m_MoveUpButton->SetBounds(MoveSongUpButtonKv->GetInt("XPos"), MoveSongUpButtonKv->GetInt("YPos"), MoveSongUpButtonKv->GetInt("Wide"), MoveSongUpButtonKv->GetInt("Tall"));
	m_MoveUpButton->SetCommand(COMMAND_PLAYLIST_MOVE_SONG_UP);

	m_MoveUpButton->GetTooltip()->SetTooltipDelay(500);
	m_MoveUpButton->GetTooltip()->SetTooltipFormatToMultiLine();
	m_MoveUpButton->GetTooltip()->SetText("Moves the selected button up the playlist");

	//create divider between volume label and change song button
	Divider* divider1 = new Divider(this, "Divider1");
	divider1->SetBounds(Divider1Kv->GetInt("XPos"), Divider1Kv->GetInt("YPos"), Divider1Kv->GetInt("Wide"), Divider1Kv->GetInt("Tall"));

	//volume label
	m_VolumeLabel = new Label(this, "VolumeLabel", VolumeTextKv->GetString("Text"));
	m_VolumeLabel->SetBounds(VolumeTextKv->GetInt("XPos"), VolumeTextKv->GetInt("YPos"), VolumeTextKv->GetInt("Wide"), VolumeTextKv->GetInt("Tall"));

	//volume slider
	m_VolumeSlider = new CSongPanelSlider(this, "VolumeSlider");
	m_VolumeSlider->SetBounds(VolumeSliderKv->GetInt("XPos"), VolumeSliderKv->GetInt("YPos"), VolumeSliderKv->GetInt("Wide"), VolumeSliderKv->GetInt("Tall"));
	m_VolumeSlider->SetRange(VolumeSliderKv->GetInt("Min"), VolumeSliderKv->GetInt("Max"));
	m_VolumeSlider->SetValue(100);

	m_VolumeSlider->GetTooltip()->SetTooltipDelay(500);
	m_VolumeSlider->GetTooltip()->SetTooltipFormatToMultiLine();
	m_VolumeSlider->GetTooltip()->SetText("The volume at which the song will play at. You can use the the right mouse button to reset the slider if needed.");

	//pitch label
	m_PitchLabel = new Label(this, "PitchLabel", PitchTextKv->GetString("Text"));
	m_PitchLabel->SetBounds(PitchTextKv->GetInt("XPos"), PitchTextKv->GetInt("YPos"), PitchTextKv->GetInt("Wide"), PitchTextKv->GetInt("Tall"));

	//change pitch slider
	m_PitchSlider = new CSongPanelSlider(this, "PitchSlider");
	m_PitchSlider->SetBounds(PitchSliderKv->GetInt("XPos"), PitchSliderKv->GetInt("YPos"), PitchSliderKv->GetInt("Wide"), PitchSliderKv->GetInt("Tall"));
	m_PitchSlider->SetRange(PitchSliderKv->GetInt("Min"), PitchSliderKv->GetInt("Max"));
	m_PitchSlider->SetValue(100);

	m_PitchSlider->GetTooltip()->SetTooltipDelay(500);
	m_PitchSlider->GetTooltip()->SetTooltipFormatToMultiLine();
	m_PitchSlider->GetTooltip()->SetText("The pitch and speed at which the song will play at. You can use the the right mouse button to reset the slider if needed.");

	//add the previous song button
	m_PreviousSongButton = new Button(this, "PreviousSong", PreviousSongButtonKv->GetString("Text", "Button"));
	m_PreviousSongButton->SetBounds(PreviousSongButtonKv->GetInt("XPos"), PreviousSongButtonKv->GetInt("YPos"), PreviousSongButtonKv->GetInt("Wide"), PreviousSongButtonKv->GetInt("Tall"));
	m_PreviousSongButton->SetCommand(COMMAND_PREVIOUS_SONG);

	//add the next song button
	m_NextSongButton = new Button(this, "NextSong", NextSongButtonKv->GetString("Text", "Button"));
	m_NextSongButton->SetBounds(NextSongButtonKv->GetInt("XPos"), NextSongButtonKv->GetInt("YPos"), NextSongButtonKv->GetInt("Wide"), NextSongButtonKv->GetInt("Tall"));
	m_NextSongButton->SetCommand(COMMAND_NEXT_SONG);

	//add the play button
	m_PlayButton = new Button(this, "PlayButton", PlayButtonKv->GetString("Text", "Button"));
	m_PlayButton->SetBounds(PlayButtonKv->GetInt("XPos"), PlayButtonKv->GetInt("YPos"), PlayButtonKv->GetInt("Wide"), PlayButtonKv->GetInt("Tall"));
	m_PlayButton->SetCommand(COMMAND_PLAY_SONG);

	//add the play playlist button
	m_PlayPlaylistButton = new Button(this, "PlayPlaylistButton", PlayPlaylistButtonKv->GetString("Text", "Button"));
	m_PlayPlaylistButton->SetBounds(PlayPlaylistButtonKv->GetInt("XPos"), PlayPlaylistButtonKv->GetInt("YPos"), PlayPlaylistButtonKv->GetInt("Wide"), PlayPlaylistButtonKv->GetInt("Tall"));
	m_PlayPlaylistButton->SetCommand(COMMAND_PLAY_PLAYLIST);

	//add the stop button
	m_StopButton = new Button(this, "StopButton", StopButtonKv->GetString("Text", "Button"));
	m_StopButton->SetBounds(StopButtonKv->GetInt("XPos"), StopButtonKv->GetInt("YPos"), StopButtonKv->GetInt("Wide"), StopButtonKv->GetInt("Tall"));
	m_StopButton->SetCommand(COMMAND_STOP_SONG);

	//add the playlist
	m_Playlist = new CSongPlaylist(this, "PlaylistSection");
	m_Playlist->SetBounds(PlaylistTestKv->GetInt("XPos"), PlaylistTestKv->GetInt("YPos"), PlaylistTestKv->GetInt("Wide"), PlaylistTestKv->GetInt("Tall"));
	m_Playlist->CreateElements();

	m_Playlist->GetTooltip()->SetTooltipDelay(500);
	m_Playlist->GetTooltip()->SetTooltipFormatToMultiLine();
	m_Playlist->GetTooltip()->SetText("The playlist section holds on to all of the playlist songs.\nWhen a playlist button is light grey it means that that song is currently selected.\nWhen the playlist button is yellow it means that that is the current song that is playing.\nPress on the playlist section to clear the selected button!\nPress the arrow keys when an item is selected to change the selected item.\nHold shift when pressing the arrow keys to move the selected item.");

	//add the progress bar
	m_ProgressBar = new CSongPanelProgressBar(this, "ProgressBar");
	m_ProgressBar->SetBounds(ProgressBarKv->GetInt("XPos"), ProgressBarKv->GetInt("YPos"), ProgressBarKv->GetInt("Wide"), ProgressBarKv->GetInt("Tall"));

	//add the duration text
	m_DurationText = new Label(this, "DurationText", "Duration: 0:00");
	m_DurationText->SetBounds(DurationTextKv->GetInt("XPos"), DurationTextKv->GetInt("YPos"), DurationTextKv->GetInt("Wide"), DurationTextKv->GetInt("Tall"));

	//add the elapsed text
	m_ElapsedText = new Label(this, "ElapsedText", "Elapsed: 0:00 (Speed 1.0x)");
	m_ElapsedText->SetBounds(ElapsedTextKv->GetInt("XPos"), ElapsedTextKv->GetInt("YPos"), ElapsedTextKv->GetInt("Wide"), ElapsedTextKv->GetInt("Tall"));

	//add the elapsed text
	m_CurrentlyPlayingText = new Label(this, "CurrentlyPlayingText", "Currently Playing:");
	m_CurrentlyPlayingText->SetBounds(CurrentlyPlayingTextKv->GetInt("XPos"), CurrentlyPlayingTextKv->GetInt("YPos"), CurrentlyPlayingTextKv->GetInt("Wide"), CurrentlyPlayingTextKv->GetInt("Tall"));
	m_CurrentlyPlayingText->SetContentAlignment(vgui::Label::a_center);

	//initalized panel
	bInit = true;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on level change
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnLevelChange()
{
	if (!bInit)
		return;

	//clear the notification panel
	songpanel->AddNotification(nullptr);

	LastUpdateTime = gpGlobals->curtime;
	DisplayLastUpdateTime = gpGlobals->curtime;

	//set the time the song started for this new level
	m_SongStartTime = gpGlobals->curtime;

	//if no song is playing then return
	if (!m_CurrPlaying)
		return;

	//make sure the sound is precached
	if (!enginesound->IsSoundPrecached(m_CurrPlaying))
		enginesound->PrecacheSound(m_CurrPlaying);

	//get the volume for the song
	float volume = ((float)m_VolumeSlider->GetValue() / 100);

	//stop all currently playing songs
	StopAllSongs();

	//play the song
	enginesound->EmitAmbientSound(m_CurrPlaying, volume, 100, SND_SHOULDPAUSE, m_TimeElapsed + m_SongStartTime);

	//HACK: to get fuckass timing to work
	enginesound->EmitAmbientSound(m_CurrPlaying, volume, m_PitchSlider->GetValue(), SND_CHANGE_PITCH | SND_SHOULDPAUSE);

	//get the song guid
	m_SongGuid = enginesound->GetGuidForLastSoundEmitted();
	if (m_SongGuid == 0)
	{
		m_SongGuid = INVALID_SONG_GUID;
		m_CurrPlaying = nullptr;

		//start the next playlist song if we need to
		if (g_bIsPlayingPlaylist)
		{
			g_CurrentQueuedSongIndex++;

			StartPlaylist(false);
		}
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on new level
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnNewLevel()
{
	if (!bInit)
		return;

	//clear the notification panel
	songpanel->AddNotification(nullptr);

	m_Playlist->ClearSongPlaying();
	m_ProgressBar->SetProgress(0.0f);

	//reset all the variables
	LastUpdateTime = gpGlobals->curtime;
	DisplayLastUpdateTime = gpGlobals->curtime;
	m_SongGuid = INVALID_SONG_GUID;
	m_SongStartTime = 0.0f;
	m_TimeElapsed = 0.0f;
	m_RealTimeElapsed = 0.0f;
	m_CurrPlaying = nullptr;
	g_QueuedSongs.RemoveAll();
	g_bIsPlayingPlaylist = false;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Starts the playlist
//-------------------------------------------------------------------------------------------------------
void CSongPanel::StartPlaylist(bool bMessWithPitch)
{
	if (!bInit)
		return;

	//reset the pitch slider
	if (bMessWithPitch)
		m_PitchSlider->SetValue(100);

	m_ProgressBar->SetProgress(0.0f);

	//stop the current song if needed
	if (m_SongGuid != INVALID_SONG_GUID && m_CurrPlaying)
		enginesound->StopSoundByGuid(m_SongGuid);

	//reset the variables
	m_SongStartTime = 0.0f;
	m_TimeElapsed = 0.0f;
	m_RealTimeElapsed = 0.0f;
	m_CurrPlaying = nullptr;
	m_SongGuid = INVALID_SONG_GUID;

	//check the playlist size
	if (g_CurrentQueuedSongIndex >= g_QueuedSongs.Count())
	{
		g_QueuedSongs.RemoveAll();
		g_CurrentQueuedSongIndex = 0;
		g_bIsPlayingPlaylist = false;

		//display notification
		songpanel->AddNotification("Playlist Finished!");

		//clear the selected playlist item
		m_Playlist->ClearSelection();
		m_Playlist->ClearSongPlaying();
		return;
	}

	if (g_CurrentQueuedSongIndex < 0)
		g_CurrentQueuedSongIndex = 0;

	//get the volume for the song
	float volume = ((float)m_VolumeSlider->GetValue() / 100);

	//select the button at index g_CurrentQueuedSongIndex
	//OnCommand(CFmtStr("PlaylistItem:%d", g_CurrentQueuedSongIndex));

	//set the playing stuff
	m_Playlist->SetSongPlayingToPlaying(g_CurrentQueuedSongIndex - 1, false);
	m_Playlist->SetSongPlayingToPlaying(g_CurrentQueuedSongIndex, true);
	m_Playlist->SetSongPlayingToPlaying(g_CurrentQueuedSongIndex + 1, false);

	//precache the song if needed
	m_CurrPlaying = g_QueuedSongs[g_CurrentQueuedSongIndex];

	//check for random
	if (!Q_strcmp(m_CurrPlaying, PLAYLIST_QUEUED_SONG_RANDOM) && g_QueuedSongData[g_CurrentQueuedSongIndex]->RandomSong)
	{
		m_CurrPlaying = g_QueuedSongData[g_CurrentQueuedSongIndex]->RandomSong->RandomSongs[random->RandomInt(0, g_QueuedSongData[g_CurrentQueuedSongIndex]->RandomSong->RandomSongs.Count())];
	}

	if (!enginesound->IsSoundPrecached(m_CurrPlaying))
		enginesound->PrecacheSound(m_CurrPlaying);

	//stop currently playing songs
	StopAllSongs();

	//play the song
	enginesound->EmitAmbientSound(m_CurrPlaying, volume, m_PitchSlider->GetValue(), SND_SHOULDPAUSE);

	LastUpdateTime = gpGlobals->curtime;
	DisplayLastUpdateTime = gpGlobals->curtime;

	//get duration
	m_SongDuration = enginesound->GetSoundDuration(m_CurrPlaying);

	//find name by song name for notification
	const char* name = m_CurrPlaying;
	for (int i = 0; i < m_Songs.Count(); i++)
	{
		if (m_Songs[i].SongName && !Q_strcmp(m_Songs[i].SongName, m_CurrPlaying))
		{
			name = m_Songs[i].Name;
			break;
		}
	}

	//display notification
	songpanel->AddNotification(CFmtStr("Now Playing: %s", name));

	//check the song guid
	m_SongGuid = enginesound->GetGuidForLastSoundEmitted();
	if (m_SongGuid == 0)
	{
		m_SongGuid = -1;
		m_CurrPlaying = nullptr;
	}

	//we are playing the playlist
	g_bIsPlayingPlaylist = true;
	m_SongStartTime = gpGlobals->curtime;

	//select correct combo box song
	for (int i = 0; i < m_Songs.Count(); i++)
	{
		if (&m_Songs[i] == g_QueuedSongData[g_CurrentQueuedSongIndex])
		{
			m_SongsComboBox->ActivateItem(i);
			return;
		}
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Saves the playlist file
//-------------------------------------------------------------------------------------------------------
void CSongPanel::SavePlaylist(const char* filename)
{
	if (!bInit)
		return;

	//create a new keyvalues* file
	KeyValues* file = new KeyValues("PlaylistFile");

	//add all the songs
	for (int i = 0; i < m_Playlist->m_Songs.Count(); i++)
	{
		KeyValues* song = new KeyValues("song");
		song->SetString(nullptr, m_Playlist->m_Songs[i]->Name);
		file->AddSubKey(song);
	}

	//write the file
	filesystem->CreateDirHierarchy("playlists/", "MOD");
	if (!file->SaveToFile(filesystem, CFmtStr("playlists/%s.txt", filename), "MOD"))
	{
		//show error
		QueryBox* box = new QueryBox("Error", CFmtStr("Failed to write playlist to file: \"playlists/%s.txt\"!", filename), this);
		box->DoModal(this);
		box->Activate();
	}

	file->deleteThis();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Loads the playlist from the file
//-------------------------------------------------------------------------------------------------------
void CSongPanel::LoadPlaylist(const char* filename)
{
	if (!bInit)
		return;

	//create a new keyvalues* file
	KeyValues* file = new KeyValues("PlaylistFile");
	if (!file->LoadFromFile(filesystem, CFmtStr("playlists/%s.txt", filename)))
	{
		//show error
		QueryBox* box = new QueryBox("Error", CFmtStr("Failed to load playlist file: \"playlists/%s.txt\"!", filename), this);
		box->DoModal(this);
		box->Activate();

		file->deleteThis();
		return;
	}

	m_Playlist->ClearEverything();

	//get all the song names
	FOR_EACH_VALUE(file, song)
	{
		//must have "song" key
		if (Q_strcmp(song->GetName(), "song"))
			continue;

		const char* songname = song->GetString();

		//find and add the song
		for (int i = 0; i < m_Songs.Count(); i++)
		{
			if (!Q_strcmp(m_Songs[i].Name, songname))
			{
				m_Playlist->AddSong(&m_Songs[i]);
				break;
			}
		}
	}

	//move scroll wheel
	m_Playlist->ClearSelection();
	m_Playlist->m_ScrollBar->SetValue(0);
	m_Playlist->OnScrollBarSliderMoved();

	file->deleteThis();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on tick (every 100ms)
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnTick()
{
	//call the base class function
	BaseClass::OnTick();
	SetVisible(cl_drawsongpanel.GetBool());	

	//close the save panel if needed
	if (g_PlaylistSavePanel && !cl_drawsongpanel.GetBool())
		PlaylistSavePanel_Close();
	
	//also the load panel
	if (g_PlaylistLoadPanel && !cl_drawsongpanel.GetBool())
		PlaylistLoadPanel_Close();

	//if not initalized then return
	if (!bInit || engine->IsDrawingLoadingImage())
		return;

	//if no playlist is playing then disable the playlist button
	m_NextSongButton->SetEnabled(g_bIsPlayingPlaylist && g_CurrentQueuedSongIndex < g_QueuedSongs.Count() - 1);
	m_PreviousSongButton->SetEnabled(g_bIsPlayingPlaylist && g_CurrentQueuedSongIndex > 0);

	//get the previous and current pitch
	static int PreviousPitch = 100;
	int Pitch = m_PitchSlider->GetValue();

	m_PitchLabel->SetText(CFmtStr("Song Pitch/Speed: %d", Pitch));
	m_VolumeLabel->SetText(CFmtStr("Song Volume: %.2f", (float)m_VolumeSlider->GetValue() / 100));

	//update m_CurrentlyPlayingText
	if (!m_CurrPlaying)
	{
		m_CurrentlyPlayingText->SetText("Currently Playing:");
	}
	else
	{
		m_CurrentlyPlayingText->SetText(CFmtStr("Currently Playing: %s", m_CurrPlaying));

		//look for song with same name
		for (int i = 0; i < m_Songs.Count(); i++)
		{
			if (!Q_strcmp(m_Songs[i].SongName, m_CurrPlaying))
			{
				m_CurrentlyPlayingText->SetText(CFmtStr("Currently Playing: %s", m_Songs[i].Name));
				break;
			}
		}
	}

	//if the song isnt active then return
	if (m_SongGuid == INVALID_SONG_GUID || !m_CurrPlaying)
	{
		m_DurationText->SetText("Duration: 0:00");
		m_ElapsedText->SetText("Elapsed: 0:00 (Speed 1.0x)");
		return;
	}
	else
	{
		//update the duration text
		int minutes = (int)((m_SongDuration) / 60.0f);
		int seconds = (int)((m_SongDuration) - (60 * minutes));

		char Durationstr[256];
		Q_snprintf(Durationstr, sizeof(Durationstr), "Duration: %i:%02i", minutes, seconds);

		m_DurationText->SetText(Durationstr);
	}

	//set the progress bar

	//get the previous and current volume
	static int PreviousVolume = 0;
	int Volume = m_VolumeSlider->GetValue();

	//compare the previous and current volume slider value
	if (Volume != PreviousVolume)
	{
		float flVol = ((float)Volume / 100);
		enginesound->SetVolumeByGuid(m_SongGuid, flVol);
	}

	//set the previous volume value
	PreviousVolume = Volume;

	//compare the previous and current pitch slider value
	if (Pitch != PreviousPitch)
	{
		enginesound->EmitAmbientSound(m_CurrPlaying, Volume, Pitch, SND_CHANGE_PITCH | SND_SHOULDPAUSE);

		if (PreviousPitch != 100)
		{
			m_TimeElapsed -= ((gpGlobals->curtime - LastUpdateTime) * (((float)PreviousPitch / 100) - 1));
		}


		//when the pitch changes the playback speed for the song also does.
		//Do this to fix the duration for when transitioning through levels.
		//(its abit broken but it works)
		LastUpdateTime = gpGlobals->curtime;
	}

	//set the previous pitch value
	PreviousPitch = Pitch;

	//check to see if the song is running. if not then reset the variables
	CUtlVector<SndInfo_t> info; 
	enginesound->GetActiveSounds(info);
	if (!enginesound->IsSoundStillPlaying(m_SongGuid))
	{
		m_CurrPlaying = nullptr;
		m_SongGuid = INVALID_SONG_GUID;

		//if we are playing a playlist then check the next song
		if (g_bIsPlayingPlaylist)
		{
			//increment the g_CurrentQueuedSongIndex
			g_CurrentQueuedSongIndex++;

			StartPlaylist();
		}
		else
		{
			//clear progress bar
			m_ProgressBar->SetValue(0);
		}
	}
	else
	{
		m_RealTimeElapsed = m_RealTimeElapsed - ((gpGlobals->curtime - DisplayLastUpdateTime) * (((float)Pitch / 100) - 1.0f));

		DisplayLastUpdateTime = gpGlobals->curtime;

		//update the progress bar
		float elapsed = (-(m_RealTimeElapsed) + (gpGlobals->curtime - m_SongStartTime)) / m_SongDuration;
		elapsed = clamp(elapsed, 0.0f, 1.0f);

		//set progress bar
		m_ProgressBar->SetProgress(elapsed);

		//update the elapsed text
		elapsed = (-(m_RealTimeElapsed) + (gpGlobals->curtime - m_SongStartTime)) / m_SongDuration;
		elapsed = clamp(elapsed, 0.0f, 1.0f);

		float playingtime = m_SongDuration * elapsed;

		int minutes = (int)(playingtime / 60.0f);
		int seconds = (int)(playingtime - (60 * minutes));

		char Durationstr[256];
		Q_snprintf(Durationstr, sizeof(Durationstr), "Elapsed: %i:%02i (Speed %.2fx)", minutes, seconds, (float)Pitch / 100);

		m_ElapsedText->SetText(Durationstr);
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on key press
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnKeyCodePressed(KeyCode code)
{
	//call the base function
	BaseClass::OnKeyCodePressed(code);

	//get the bind to toggle the song panel
	const char* bind = engine->Key_LookupBinding("ToggleSongPanel");
	if (!bind)
		return;

	//get the string of the key that was pressed
	const char* DisplayCode = (const char*)KeyCodeToDisplayString(code);

	//compare the 2 strings and if they match. close the panel
	if (!Q_stricmp(bind, DisplayCode))
		cl_drawsongpanel.SetValue(0);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on close
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnClose()
{
	cl_drawsongpanel.SetValue(0);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on command
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnCommand(const char* pcCommand)
{
	BaseClass::OnCommand(pcCommand);

	//check for stop command
	if (!Q_strcmp(pcCommand, COMMAND_STOP_SONG))
	{
		if (m_SongGuid != INVALID_SONG_GUID && m_CurrPlaying)
			enginesound->StopSoundByGuid(m_SongGuid);

		m_CurrPlaying = nullptr;

		//clear the selected playlist song
		m_Playlist->ClearSelection();
		m_Playlist->ClearSongPlaying();
		m_ProgressBar->SetProgress(0.0f);

		//clear the playlist queue
		g_QueuedSongs.RemoveAll();
		g_bIsPlayingPlaylist = false;
	}
	//search for the select item command
	else if (Q_strstr(pcCommand, "PlaylistItem:") == pcCommand)
	{
		//get the index
		int index = atoi(pcCommand + Q_strlen("PlaylistItem:"));

		//check for valid index
		if (index < 0 || index >= m_Playlist->m_Songs.Count())
			return;

		//select song
		for (int i = 0; i < m_Songs.Count(); i++)
		{
			if (&m_Songs[i] == m_Playlist->m_Songs[index])
			{
				m_SongsComboBox->ActivateItem(i);
				break;
			}
		}

		m_Playlist->OnCommand(pcCommand);
	}
	//play the playlist
	else if (!Q_strcmp(pcCommand, COMMAND_PLAY_PLAYLIST))
	{
		//clear selected song
		m_Playlist->ClearSelection();
		m_ProgressBar->SetProgress(0.0f);

		//reset the index
		g_CurrentQueuedSongIndex = 0;

		//clear the playlist queue
		g_QueuedSongs.RemoveAll();
		g_bIsPlayingPlaylist = false;

		//collect the playlist songs and start the playlist
		m_Playlist->CollectPlaylist();

		//check for empty playlist
		if (g_QueuedSongs.Count() <= 0)
		{
			QueryBox* error = new QueryBox("Empty Playlist", "Got empty playlist!", this);
			error->DoModal(this);
			error->Activate();
			return;
		}

		StartPlaylist();
	}
	//previous song
	else if (!Q_strcmp(pcCommand, COMMAND_PREVIOUS_SONG))
	{
		//decrement the current queued song
		g_CurrentQueuedSongIndex--;

		m_Playlist->ClearSelection();

		//play the previous song
		StartPlaylist(false);
	}
	//next song
	else if (!Q_strcmp(pcCommand, COMMAND_NEXT_SONG))
	{
		//increment the current queued song
		g_CurrentQueuedSongIndex++;

		m_Playlist->ClearSelection();

		//play the next song
		StartPlaylist(false);
	}
	//add song
	else if (!Q_strcmp(pcCommand, COMMAND_PLAYLIST_ADD_SONG))
	{
		m_Playlist->AddSong(&m_Songs[m_SongsComboBox->GetActiveItem()]);
	}
	//remove song
	else if (!Q_strcmp(pcCommand, COMMAND_PLAYLIST_REMOVE_SONG))
	{
		m_Playlist->RemoveSelectedSong();
	}
	//change selected song
	else if (!Q_strcmp(pcCommand, COMMAND_PLAYLIST_CHANGE_SONG))
	{
		m_Playlist->ChangeSelectedSong(&m_Songs[m_SongsComboBox->GetActiveItem()]);
	}
	//move selected song down
	else if (!Q_strcmp(pcCommand, COMMAND_PLAYLIST_MOVE_SONG_DOWN))
	{
		m_Playlist->MoveSelectedSong(false);
	}
	//move selected song up
	else if (!Q_strcmp(pcCommand, COMMAND_PLAYLIST_MOVE_SONG_UP))
	{
		m_Playlist->MoveSelectedSong(true);
	}
	//check for play song command
	else if (!Q_strcmp(pcCommand, COMMAND_PLAY_SONG))
	{
		//reset the pitch slider
		m_PitchSlider->SetValue(100);

		//clear the playlist queue
		g_QueuedSongs.RemoveAll();
		g_bIsPlayingPlaylist = false;
		m_ProgressBar->SetProgress(0.0f);

		//get the selected song
		AloneModSong_t* song = &m_Songs[m_SongsComboBox->GetActiveItem()];
		if (!song)
			return;

		//store the song name
		const char* songname = nullptr;

		//check for either a random song set or a normal song
		if (song->RandomSong)
		{
			//store the random songs
			CUtlVector<const char*>& RandomSongNames = song->RandomSong->RandomSongs;

			//check the size of the random songs
			if (RandomSongNames.Count() <= 0)
				return;

			//choose a random song from the song names
			songname = RandomSongNames[random->RandomInt(0, RandomSongNames.Count() - 1)];
		}
		else
			songname = song->SongName;

		//if a song is playing then stop if
		if (m_SongGuid != INVALID_SONG_GUID && m_CurrPlaying)
		{
			enginesound->StopSoundByGuid(m_SongGuid);
			m_CurrPlaying = nullptr;
		}

		//play the song
		if (enginesound->PrecacheSound(songname))
		{
			//set the member variables
			m_CurrPlaying = songname;

			//get the volume
			float volume = ((float)m_VolumeSlider->GetValue() / 100);

			//stop currently playing songs
			StopAllSongs();

			//play the song
			enginesound->EmitAmbientSound(songname, volume, m_PitchSlider->GetValue(), SND_SHOULDPAUSE);

			LastUpdateTime = gpGlobals->curtime;
			DisplayLastUpdateTime = gpGlobals->curtime;

			//get duration
			m_SongDuration = enginesound->GetSoundDuration(songname);

			//display notification
			songpanel->AddNotification(CFmtStr("Now Playing: %s", song->Name));

			//get the song guid
			m_SongGuid = enginesound->GetGuidForLastSoundEmitted();
			if (m_SongGuid == 0)
				m_SongGuid = INVALID_SONG_GUID;
		}

		//reset the variables
		m_SongStartTime = gpGlobals->curtime;
		m_TimeElapsed = 0.0f;
		m_RealTimeElapsed = 0.0f;
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Destructor for song panel
//-------------------------------------------------------------------------------------------------------
CSongPanel::~CSongPanel()
{
	//make sure to delete the keyvalues
	if (m_kvMain)
		m_kvMain->deleteThis();

	m_kvMain = nullptr;
}









//song panel interface
class CAmodSongPanellInterface : public SongPanel
{
public:
	CSongPanel* SongPanel;
	CSongPanelNotificationPanel* SongNotificationPanel;
public:

	//constructor
	CAmodSongPanellInterface()
	{
		SongPanel = nullptr;
		SongNotificationPanel = nullptr;
	}

	//creates this panel
	void Create(vgui::VPANEL parent)
	{
		SongPanel = new CSongPanel(parent);
		SongNotificationPanel = new CSongPanelNotificationPanel("SongNotificationPanel");
	}

	//deletes/destroys the panel
	void Destroy()
	{
		if (SongPanel)
		{
			SongPanel->SetParent((vgui::Panel*)NULL);
			delete SongPanel;
		}

		//set SongPanel to nullptr
		SongPanel = nullptr;

		if (SongNotificationPanel)
		{
			SongNotificationPanel->SetParent((vgui::Panel*)NULL);
			delete SongNotificationPanel;
		}

		//set SongPanel to nullptr
		SongNotificationPanel = nullptr;
	}

	//activates the panel
	void Activate(void)
	{
		if (SongPanel)
			SongPanel->Activate();
	}

	//called on level change
	void OnLevelChange()
	{
		SongPanel->OnLevelChange();
	}

	//called on level shutdown
	void OnLevelShutdown()
	{
		if (!SongPanel->m_CurrPlaying)
			return;

		//set the time elapsed for the current song
		SongPanel->m_TimeElapsed = SongPanel->m_TimeElapsed - (gpGlobals->curtime - SongPanel->m_SongStartTime);
		SongPanel->m_RealTimeElapsed = SongPanel->m_RealTimeElapsed - (gpGlobals->curtime - SongPanel->m_SongStartTime);
		enginesound->StopSoundByGuid(SongPanel->m_SongGuid);

		//check pitch slider
		int pitch = SongPanel->m_PitchSlider->GetValue();
		if (pitch != 100)
			SongPanel->m_TimeElapsed -= ((gpGlobals->curtime - SongPanel->LastUpdateTime) * (((float)pitch / 100) - 1));


		//so for abit i couldnt figure out why sometimes the song would stop at a level transition and i think i figured it out.
		//i didnt set the song guid to -1 (invalid) and if the 'if (!enginesound->IsSoundStillPlaying(SongGuid))' inside CSongPanel::OnTick()
		//would return false just after the level transition and just before the code to start the song again runs, then it would cause the song 
		//to not play.
		SongPanel->m_SongGuid = INVALID_SONG_GUID;
	}

	//called on new level
	void OnNewLevel()
	{
		SongPanel->OnNewLevel();
	}

	//starts the next playlist song
	void StartPlaylist()
	{
		SongPanel->StartPlaylist();
	}

	//adds a notification
	void AddNotification(const char* message)
	{
		SongNotificationPanel->AddMessage(message);
	}

	//saves the playlist to a file
	void SavePlaylistToFile(const char* filename)
	{
		SongPanel->SavePlaylist(filename);
	}

	//loads a playlist from a file
	void LoadPlaylistFromFile(const char* filename)
	{
		SongPanel->LoadPlaylist(filename);
	}
};
static CAmodSongPanellInterface g_SPanel;
SongPanel* songpanel = (SongPanel*)&g_SPanel;

//-------------------------------------------------------------------------------------------------------
// Purpose: called from server when a level transition has happened
//-------------------------------------------------------------------------------------------------------
CON_COMMAND(amod_songpanel_changelevel, "I had to make this a hack cause you cant get the gpGlobals->eLoadType on the client")
{
	songpanel->OnLevelChange();
}

//-------------------------------------------------------------------------------------------------------
// Purpose: called from server when a new level is loaded
//-------------------------------------------------------------------------------------------------------
CON_COMMAND(amod_songpanel_newlevel, "I had to make this a hack cause you cant get the gpGlobals->eLoadType on the client")
{
	songpanel->OnNewLevel();
}

//toggles the song panel
CON_COMMAND_F(ToggleSongPanel, "Toggles The Alone Mod Song Panel", FCVAR_HIDDEN)
{
	cl_drawsongpanel.SetValue(!cl_drawsongpanel.GetBool());
	songpanel->Activate();
};



//HACK: i used to call the enginesound->EmitAmbientSound insid the _amod_playsound
//console command, BUT i would sometimes have issues where the game would lag ALOT
//for some reason. The only explination i could have for the lag is due to the enginesound->EmitAmbientSound
//function is getting called on the client from the server when the server is currently running. 
//So instead we will wait for the client to start running code to then run the song.
struct CurrentSongItem_t
{
	char filename[512];
	float volume;
	float duration;
	int pitch;
	bool InUse = false;
};
static CurrentSongItem_t s_CurrentSongItem;

class CAutoPlaySongSystem : public CAutoGameSystemPerFrame
{
public:
	void Update(float frametime) override
	{
		if (s_CurrentSongItem.InUse)
		{
			//make sure the sound is precached
			if (!enginesound->IsSoundPrecached(s_CurrentSongItem.filename))
				enginesound->PrecacheSound(s_CurrentSongItem.filename);

			//play the song at normal speed then set the pitch so we start at the correct time
			enginesound->EmitAmbientSound(s_CurrentSongItem.filename, s_CurrentSongItem.volume, 100, SND_SHOULDPAUSE, s_CurrentSongItem.duration);

			//check pitch so the song starts at the correct time
			if (s_CurrentSongItem.pitch != 100)
				enginesound->EmitAmbientSound(s_CurrentSongItem.filename, s_CurrentSongItem.volume, s_CurrentSongItem.pitch, SND_CHANGE_PITCH);

			s_CurrentSongItem.InUse = false;
		}
	}
};
static CAutoPlaySongSystem s_AutoSongPlaHackySystem;

//command to play sound from server. Used for transitioning songs through levels
CON_COMMAND_F(_amod_playsound, "", FCVAR_HIDDEN)
{
	//check to see if we should play (if a song from the song panel is playing then dont play)
	if (atoi(args.Arg(5)) == 1)
	{
		if (g_SPanel.SongPanel->m_CurrPlaying)
		{
			engine->ClientCmd("amod_remove_previous_song");
			return;
		}
	}

	//stop all songs
	StopAllSongs();

	//get args
	const char* songname = args.Arg(1);
	float volume = atof(args.Arg(2));
	int pitch = atoi(args.Arg(3));
	float duration = atof(args.Arg(4));

	//set s_CurrentSongItem
	Q_strncpy(s_CurrentSongItem.filename, songname, sizeof(s_CurrentSongItem.filename));
	s_CurrentSongItem.volume = volume / 10;
	s_CurrentSongItem.pitch = pitch;
	s_CurrentSongItem.duration = duration;
	s_CurrentSongItem.InUse = true;
}

//debug
CON_COMMAND(amod_spanel_reset, "")
{
	g_SPanel.Destroy();
	g_SPanel.Create(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));
}