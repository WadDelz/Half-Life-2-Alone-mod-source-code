#include "cbase.h"
#include "fmtstr.h"
#include "filesystem.h"
#include "ISongPanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Slider.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>
#include <engine/IEngineSound.h>

using namespace vgui;

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

//vgui song panel class
#define COMMAND_RESET_PITCH "ResetPitch"
#define COMMAND_PLAY_SONG "PlaySong"
#define COMMAND_STOP_SONG "StopSong"

class CSongPanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CSongPanel, vgui::Frame);
public:

	//constrctor and destructor
	CSongPanel(vgui::VPANEL parent);
	~CSongPanel() {};

	//initalizes the panel items
	void Init();

	//called on level change and new level
	void OnLevelChange();
	void OnNewLevel();

	//song stuff
	int m_SongGuid = 0;					//the song guid (unique identifoer)
	int m_StartPitch = 100;				//the pitch the song was at when played for the first time
	float m_SongStartTime = 0.0f;		//the time (gpglobals->curtime) the song started
	float m_TimeElapsed = 0.0f;			//how long the song has been going for
	const char* m_CurrPlaying = nullptr;
protected:
	//functions
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);
	void OnClose();

	//keyboard stuff
	void OnKeyCodePressed(KeyCode code);
public:
	//keyvalues file
	KeyValues* m_kvMain;
	ComboBox* m_SongsComboBox;

	//sliders for volume and pitch
	Slider* m_VolumeSlider = nullptr;
	Slider* m_PitchSlider = nullptr;

	//transition through levels check button
	CheckButton* m_TransThroughLevelCheckButton = nullptr;

	//bottom buttons
	Button* m_PlayButton = nullptr;
	Button* m_StopButton = nullptr;
	Button* m_ResetPitchButton = nullptr;

	//list of songs and random songs
	CUtlVector<AloneModRandomSong_t> m_RandomSongs;
	CUtlVector<AloneModSong_t> m_Songs;

	//has this panel been initalized yet
	bool bInit = false;

};

//-------------------------------------------------------------------------------------------------------
// Purpose: called from server when a new level is loaded
//-------------------------------------------------------------------------------------------------------
CSongPanel::CSongPanel(vgui::VPANEL parent)
	: BaseClass(NULL, "SongPanel")
{
	//initalize all the variables
	m_SongGuid = INVALID_SONG_GUID;
	m_StartPitch = 0;
	m_SongStartTime = 0.0f;
	m_TimeElapsed = 0.0f;
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
	vgui::ivgui()->AddTickSignal(GetVPanel(), 100);

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
	ConWarning("Got NULL Subkey: \"" string "\"In File: \"SongPanel.txt\" Aborting Panel Load!\n");			\
	return;																									\
}

//-------------------------------------------------------------------------------------------------------
// Purpose: initalizes all the panel stuff
//-------------------------------------------------------------------------------------------------------
void CSongPanel::Init()
{
	//get all the elements
	SONG_PANEL_KEYVALUES_FIND(PlayButtonKv, "PlayButton");
	SONG_PANEL_KEYVALUES_FIND(StopButtonKv, "StopButton");
	SONG_PANEL_KEYVALUES_FIND(ResetPitchButtonKv, "ResetPitch");
	SONG_PANEL_KEYVALUES_FIND(VolumeSliderKv, "SliderVol");
	SONG_PANEL_KEYVALUES_FIND(VolumeTextKv, "VolumeText");
	SONG_PANEL_KEYVALUES_FIND(PitchSliderKv, "SliderPitch");
	SONG_PANEL_KEYVALUES_FIND(PitchTextKv, "PitchText");
	SONG_PANEL_KEYVALUES_FIND(TransitionThroughLevelsButtonKv, "TransCB");
	SONG_PANEL_KEYVALUES_FIND(SongListKv, "SongList");
	
	//now get all the songs
	KeyValues* SongsSubkey = SongListKv->FindKey("Songs");
	if (!SongsSubkey)
	{
		m_kvMain->deleteThis();
		ConWarning("Got NULL Subkey: \"Songs\" For SongList In File: \"SongPanel.txt\" Aborting Panel Load!\n");
		return;
	}

	//must have songs inside
	if (!SongsSubkey->GetFirstSubKey())
	{
		m_kvMain->deleteThis();
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
				ConWarning("Failed to get name for random song section inside \"SongPanel.txt\" Aborting Panel Load!\n");
				return;
			}

			//the song MUST not have a . in its name
			if (Q_strstr(RandomSongItem.Name, "."))
			{
				m_kvMain->deleteThis();
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
	for (int i = 0; i < m_Songs.Size(); i++)
		m_SongsComboBox->AddItem(m_Songs[i].Name, nullptr);

	m_SongsComboBox->ActivateItem(SongListKv->GetInt("ActiveItem"));

	//volume label
	Label* VolumeLabel = new Label(this, "VolumeLabel", VolumeTextKv->GetString("Text"));
	VolumeLabel->SetBounds(VolumeTextKv->GetInt("XPos"), VolumeTextKv->GetInt("YPos"), VolumeTextKv->GetInt("Wide"), VolumeTextKv->GetInt("Tall"));

	//volume slider
	m_VolumeSlider = new Slider(this, "VolumeSlider");
	m_VolumeSlider->SetBounds(VolumeSliderKv->GetInt("XPos"), VolumeSliderKv->GetInt("YPos"), VolumeSliderKv->GetInt("Wide"), VolumeSliderKv->GetInt("Tall"));
	m_VolumeSlider->SetRange(VolumeSliderKv->GetInt("Min"), VolumeSliderKv->GetInt("Max"));
	m_VolumeSlider->SetValue(100);

	//pitch label
	Label* PitchLabel = new Label(this, "PitchLabel", PitchTextKv->GetString("Text"));
	PitchLabel->SetBounds(PitchTextKv->GetInt("XPos"), PitchTextKv->GetInt("YPos"), PitchTextKv->GetInt("Wide"), PitchTextKv->GetInt("Tall"));

	//change pitch slider
	m_PitchSlider = new Slider(this, "PitchSlider");
	m_PitchSlider->SetBounds(PitchSliderKv->GetInt("XPos"), PitchSliderKv->GetInt("YPos"), PitchSliderKv->GetInt("Wide"), PitchSliderKv->GetInt("Tall"));
	m_PitchSlider->SetRange(PitchSliderKv->GetInt("Min"), PitchSliderKv->GetInt("Max"));
	m_PitchSlider->SetValue(100);

	//transition through levels check button
	m_TransThroughLevelCheckButton = new CheckButton(this, "TransitionThroughLevels", TransitionThroughLevelsButtonKv->GetString("Text"));
	m_TransThroughLevelCheckButton->SetBounds(TransitionThroughLevelsButtonKv->GetInt("XPos"), TransitionThroughLevelsButtonKv->GetInt("YPos"), TransitionThroughLevelsButtonKv->GetInt("Wide"), TransitionThroughLevelsButtonKv->GetInt("Tall"));
	m_TransThroughLevelCheckButton->SetSelected(true);

	//add the play button
	m_PlayButton = new Button(this, "PlayButton", PlayButtonKv->GetString("Text", "Button"));
	m_PlayButton->SetBounds(PlayButtonKv->GetInt("XPos"), PlayButtonKv->GetInt("YPos"), PlayButtonKv->GetInt("Wide"), PlayButtonKv->GetInt("Tall"));
	m_PlayButton->SetCommand(COMMAND_PLAY_SONG);

	//add the stop button
	m_StopButton = new Button(this, "StopButton", StopButtonKv->GetString("Text", "Button"));
	m_StopButton->SetBounds(StopButtonKv->GetInt("XPos"), StopButtonKv->GetInt("YPos"), StopButtonKv->GetInt("Wide"), StopButtonKv->GetInt("Tall"));
	m_StopButton->SetCommand(COMMAND_STOP_SONG);

	//add the reset pitch button
	m_ResetPitchButton = new Button(this, "ResetPitch", ResetPitchButtonKv->GetString("Text", "Button"));
	m_ResetPitchButton->SetBounds(ResetPitchButtonKv->GetInt("XPos"), ResetPitchButtonKv->GetInt("YPos"), ResetPitchButtonKv->GetInt("Wide"), ResetPitchButtonKv->GetInt("Tall"));
	m_ResetPitchButton->SetCommand(COMMAND_RESET_PITCH);

	//initalized panel
	bInit = true;
}

ConVar cl_drawsongpanel("cl_drawsongpanel", "0", FCVAR_CLIENTDLL, "Sets the state of the alone mod Song panel");

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on level change
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnLevelChange()
{
	//set the time the song started for this new level
	m_SongStartTime = gpGlobals->curtime;

	//if no song is playing then return or the transition through levels check button isnt
	//checked then return
	if (!m_CurrPlaying || !m_TransThroughLevelCheckButton->IsSelected())
		return;

	//make sure the sound is precached
	if (!enginesound->IsSoundPrecached(m_CurrPlaying))
		enginesound->PrecacheSound(m_CurrPlaying);

	//get the volume for the song
	float volume = ((float)m_VolumeSlider->GetValue() / 100);

	//play the song
	enginesound->EmitAmbientSound(m_CurrPlaying, volume, m_StartPitch, SND_SHOULDPAUSE, m_TimeElapsed + m_SongStartTime);

	//get the song guid
	m_SongGuid = enginesound->GetGuidForLastSoundEmitted();
	if (m_SongGuid == 0)
		m_SongGuid = INVALID_SONG_GUID;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on new level
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnNewLevel()
{
	//reset all the variables
	m_SongGuid = INVALID_SONG_GUID;
	m_SongStartTime = 0.0f;
	m_TimeElapsed = 0.0f;
	m_StartPitch = 100;
	m_CurrPlaying = nullptr;
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called on tick (every 100ms)
//-------------------------------------------------------------------------------------------------------
void CSongPanel::OnTick()
{
	//call the base class function
	BaseClass::OnTick();
	SetVisible(cl_drawsongpanel.GetBool());

	//if not initalized then return
	if (!bInit)
		return;

	//if the song isnt active then return
	if (m_SongGuid == INVALID_SONG_GUID || !m_CurrPlaying)
		return;

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

	//check to see if the song is running. if not then reset the variables
	if (!enginesound->IsSoundStillPlaying(m_SongGuid))
	{
		m_CurrPlaying = nullptr;
		m_SongGuid = INVALID_SONG_GUID;
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

	//check for reset pitch command
	if (!Q_strcmp(pcCommand, COMMAND_RESET_PITCH))
	{
		m_PitchSlider->SetValue(100);
	}
	//check for stop command
	else if (!Q_strcmp(pcCommand, COMMAND_STOP_SONG))
	{
		if (m_SongGuid != INVALID_SONG_GUID && m_CurrPlaying)
			enginesound->StopSoundByGuid(m_SongGuid);

		m_CurrPlaying = nullptr;
	}
	
	//check for play song command
	else if (!Q_strcmp(pcCommand, COMMAND_PLAY_SONG))
	{
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
			if (RandomSongNames.Size() <= 0)
				return;

			//choose a random song from the song names
			songname = RandomSongNames[random->RandomInt(0, RandomSongNames.Size())];
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
			m_StartPitch = m_PitchSlider->GetValue();
			m_CurrPlaying = songname;

			//get the volume
			float volume = ((float)m_VolumeSlider->GetValue() / 100);

			//play the song
			enginesound->EmitAmbientSound(songname, volume, m_StartPitch, SND_SHOULDPAUSE);

			//get the song guid
			m_SongGuid = enginesound->GetGuidForLastSoundEmitted();
			if (m_SongGuid == 0)
				m_SongGuid = INVALID_SONG_GUID;
		}

		//reset the variables
		m_SongStartTime = gpGlobals->curtime;
		m_TimeElapsed = 0.0f;
	}
}





//song panel interface
class CAmodSongPanellInterface : public SongPanel
{
public:
	CSongPanel* SongPanel;
public:

	//constructor
	CAmodSongPanellInterface()
	{
		SongPanel = nullptr;
	}

	//creates this panel
	void Create(vgui::VPANEL parent)
	{
		SongPanel = new CSongPanel(parent);
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
		enginesound->StopSoundByGuid(SongPanel->m_SongGuid);

		//so for abit i couldnt figure out why sometimes the song would stop at a level transition and i think i figured it out.
		//i didnt set the song guid to -1 (invalid) and if the 'if (!enginesound->IsSoundStillPlaying(SongGuid))' inside CSongPanel::OnTick()
		//would return false just after the level transition and just before the code to start the song again runs, then it would cause the song 
		//to not play.
		SongPanel->m_SongGuid = INVALID_SONG_GUID;
	}

	void OnNewLevel()
	{
		SongPanel->OnNewLevel();
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

//command to play sound from server. Used for transitioning songs through levels
CON_COMMAND_F(_amod_playsound, "", FCVAR_HIDDEN)
{
	//check to see if we should play (if a song from the song panel is playing then dont play)
	if (atoi(args.Arg(4)) == 1)
	{
		if (g_SPanel.SongPanel->m_CurrPlaying)
			return;
	}

	enginesound->EmitAmbientSound(args.Arg(1), atof(args.Arg(2)), 100, SND_SHOULDPAUSE, atof(args.Arg(3)));
}