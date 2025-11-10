//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: a vgui panel that allows you to create and test soundscapes in game
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "c_soundscape.h"
#include "vgui_soundscape_maker.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Menu.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/RichText.h>
#include <vgui/ISystem.h>
#include <engine/IEngineSound.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include <ienginevgui.h>
#include <filesystem.h>
#include <usermessages.h>
#include <fmtstr.h>

//graph panel for debugging

class CGraphPanel : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE(CGraphPanel, vgui::Panel);

	CGraphPanel(Panel* parent, const char* panelName);

	//think and paint
	virtual void OnThink();
	virtual void Paint();

	//start and stop functions
	virtual void Start();
	virtual void Stop();
	virtual void Restart();
	virtual void Clear();

	//Adding/doing stuff to lines functions
	virtual void AddLine(bool ascending, unsigned char r, unsigned char g, unsigned char b, float speed, float flGraphWidthFraction = 1.0f);
	virtual void RemoveLine(int index);

	//set functions
	virtual void SetDuration(float seconds) { m_flDuration = seconds; }
	virtual void SetHorizontalLinesMax(float seconds) { m_nHorizontalLinesMax = seconds; }
	virtual void SetMaxTextValue(float maxvalue) { m_flMaxValue = maxvalue; }

	//other
	virtual void ApplySchemeSettings(vgui::IScheme* scheme);

	//sets the font
	void SetFont(vgui::HFont font) { m_Font = font; }
	inline vgui::HFont GetFont() { return m_Font; };

	//gets the number of lines
	int GetNumLines() { return m_Lines.Count(); }

private:

	//line information
	struct LineInfo
	{
		float startTime;
		float offset;
		float elapsedWhenStopped;
		float m_flGraphWidthFraction;
		bool ascending;
		unsigned char r, g, b;
		float speed;
	};

	//lines and other stuff
	CUtlVector<LineInfo> m_Lines;
	float m_flDuration;
	float m_flTimeOffset;
	bool m_bRunning;

	//mad horizontal lines
	int m_nHorizontalLinesMax = 2;
	float m_flMaxValue = 1.0f;

	vgui::HFont m_Font;
};

extern vgui::ILocalize* g_pVGuiLocalize;

//-----------------------------------------------------------------------------
// Purpose: Graph panel
//-----------------------------------------------------------------------------
CGraphPanel::CGraphPanel(Panel* parent, const char* panelName)
	: BaseClass(parent, panelName)
{
	SetPaintBackgroundEnabled(false);
	m_flDuration = 2.0f;
	m_bRunning = false;
	m_flTimeOffset = 0.0f;
	m_Font = vgui::INVALID_FONT;
	SetBgColor(Color(0, 0, 0, 255));
}

//-----------------------------------------------------------------------------
// Purpose: Called when this panel thinks
//-----------------------------------------------------------------------------
void CGraphPanel::OnThink()
{
	if (m_bRunning)
		Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: Called when this panel paints
//-----------------------------------------------------------------------------
void CGraphPanel::Paint()
{
	//get size
	int w, h;
	GetSize(w, h);

	//set fill background
	vgui::surface()->DrawSetColor(GetBgColor());
	vgui::surface()->DrawFilledRect(0, 0, w, h);

	//draw horizontal grid lines
	if (m_nHorizontalLinesMax > 1)
	{
		vgui::surface()->DrawSetColor(100, 100, 100, 128); //grey lines

		//draw lines
		for (int i = 0; i < m_nHorizontalLinesMax; ++i)
		{
			float frac = (float)i / (m_nHorizontalLinesMax - 1);
			int y = h - (int)(frac * h);

			//draw the line
			vgui::surface()->DrawLine(0, y, w, y);

			float labelValue = frac * m_flMaxValue;

			char buf[32];
			Q_snprintf(buf, sizeof(buf), "%.2f", labelValue);

			vgui::surface()->DrawSetTextFont(GetFont());
			vgui::surface()->DrawSetTextColor(255, 255, 255, 255);

			//set text pos
			if (labelValue == m_flMaxValue)
				vgui::surface()->DrawSetTextPos(5, y);
			else if (labelValue <= 0.0f)
				vgui::surface()->DrawSetTextPos(5, y - 14);
			else
				vgui::surface()->DrawSetTextPos(5, y - 8);

			wchar_t wbuf[32];
			g_pVGuiLocalize->ConvertANSIToUnicode(buf, wbuf, sizeof(wbuf));
			vgui::surface()->DrawPrintText(wbuf, wcslen(wbuf));
		}
	}

	//get now time
	float now = vgui::system()->GetFrameTime();

	//now draw the graphs
	for (int i = 0; i < m_Lines.Count(); ++i)
	{
		//get line info
		const LineInfo& line = m_Lines[i];

		//set color
		vgui::surface()->DrawSetColor(line.r, line.g, line.b, 255);

		//do stuff
		float elapsed = m_bRunning ? (now - line.startTime - line.offset) : line.elapsedWhenStopped;
		if (elapsed < 0.0f)
			continue;

		float effectiveDuration = m_flDuration / line.speed;

		if (elapsed > effectiveDuration)
			elapsed = effectiveDuration;

		float progress = elapsed / effectiveDuration;

		int lastX = -1;
		int lastY = -1;

		// Calculate the maximum possible width fraction for this line considering the offset
		float maxWidthForLine = line.m_flGraphWidthFraction - line.offset;
		if (maxWidthForLine < 0.0f)
			maxWidthForLine = 0.0f; // prevent negative width

		//make 128 line points
		for (int j = 0; j < 128; ++j)
		{
			float t = (float)j / 127.0f;

			// Stop drawing if t > progress for this line
			if (t > progress)
				break;

			float curve;
			if (line.ascending)
				curve = t * t * t;
			else
				curve = 1.0f - t * t;

			// Calculate the X position using offset + scaled max width for this line
			float combinedPos = line.offset + t * maxWidthForLine;

			// Stop if combinedPos is beyond max graph width fraction (to not draw outside graph area)
			if (combinedPos > line.m_flGraphWidthFraction)
				break;

			int x = (int)(w * combinedPos);
			int y = (int)(h - curve * h);

			//draw the line
			if (lastX >= 0 && lastY >= 0)
				vgui::surface()->DrawLine(lastX, lastY, x, y);

			lastX = x;
			lastY = y;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Starts drawing the graphs
//-----------------------------------------------------------------------------
void CGraphPanel::Start()
{
	//check for not running
	if (!m_bRunning)
	{
		float now = vgui::system()->GetFrameTime();
		for (int i = 0; i < m_Lines.Count(); ++i)
		{
			if (m_Lines[i].elapsedWhenStopped < m_flDuration / m_Lines[i].speed)
				m_Lines[i].startTime = now - m_Lines[i].elapsedWhenStopped;
		}

		//start running
		m_bRunning = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Stops drawing the graphs
//-----------------------------------------------------------------------------
void CGraphPanel::Stop()
{
	//check for running
	if (m_bRunning)
	{
		float now = vgui::system()->GetFrameTime();
		for (int i = 0; i < m_Lines.Count(); ++i)
			m_Lines[i].elapsedWhenStopped = now - m_Lines[i].startTime;

		//stop running
		m_bRunning = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Clears the line graph
//-----------------------------------------------------------------------------
void CGraphPanel::Clear()
{
	m_Lines.RemoveAll();
	m_flTimeOffset = 0.0f;
	m_bRunning = false;
}

//-----------------------------------------------------------------------------
// Purpose: Resets the lines
//-----------------------------------------------------------------------------
void CGraphPanel::Restart()
{
	for (int i = 0; i < m_Lines.Count(); i++)
	{
		m_Lines[i].startTime = vgui::system()->GetFrameTime();
		m_Lines[i].elapsedWhenStopped = 0.0f;
	}

	m_flTimeOffset += 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a line to the line graph
//-----------------------------------------------------------------------------
void CGraphPanel::AddLine(bool ascending, unsigned char r, unsigned char g, unsigned char b, float speed, float flGraphWidthFraction)
{
	//check speed
	if (speed <= 0.0f)
		speed = 1.0f;

	//create line info
	LineInfo line;
	line.startTime = vgui::system()->GetFrameTime();
	line.ascending = ascending;
	line.m_flGraphWidthFraction = flGraphWidthFraction;
	line.offset = m_flTimeOffset;
	line.elapsedWhenStopped = 0.0f;
	line.r = r;
	line.g = g;
	line.b = b;
	line.speed = speed;

	//add to lines array
	m_Lines.AddToTail(line);
	m_flTimeOffset += 0.1f;
}

//-----------------------------------------------------------------------------
// Purpose: Removes a line graph
//-----------------------------------------------------------------------------
void CGraphPanel::RemoveLine(int index)
{
	//bounds check
	if (index >= m_Lines.Count() || index < 0)
		return;

	//remove the line
	m_Lines.Remove(index);

	//move everything down
	m_flTimeOffset = 0.0f;
	for (int i = 0; i < m_Lines.Count(); ++i)
	{
		m_Lines[i].offset = m_flTimeOffset;
		m_flTimeOffset += 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when scheme settings are set
//-----------------------------------------------------------------------------
void CGraphPanel::ApplySchemeSettings(vgui::IScheme* scheme)
{
	SetFont(scheme->GetFont("Default", IsProportional()));
}

//selected text mode
enum class SoundscapeMode
{
	Mode_Random,
	Mode_Soundscape,
	Mode_Looping,
};

//soundscape clipboard type
enum class SoundscapeClipboardType
{
	Type_SoundscapeNone,
	Type_SoundscapeName,
	Type_SoundscapeData,
	Type_SoundscapeRandomWave,
};

//dsp effects
static const char* g_DspEffects[] = {
	"Default",
	"Normal (off)",
	"Generic",
	"Metal Small",
	"Metal Medium",
	"Metal Large",
	"Tunnel Small",
	"Tunnel Medium",
	"Tunnel Large",
	"Chamber Small",
	"Chamber Medium",
	"Chamber Large",
	"Bright Small",
	"Bright Medium",
	"Bright Large",
	"Water 1",
	"Water 2",
	"Water 3",
	"Concrete Small",
	"Concrete Medium",
	"Concrete Large",
	"Big 1",
	"Big 2",
	"Big 3",
	"Cavern Small",
	"Cavern Medium",
	"Cavern Large",
	"Weirdo 1",
	"Weirdo 2",
	"Weirdo 3",
};

//sound levels
static const char* g_SoundLevels[] = {
	"SNDLVL_50dB",
	"SNDLVL_55dB",
	"SNDLVL_IDLE",
	"SNDLVL_TALKING",
	"SNDLVL_60dB",
	"SNDLVL_65dB",
	"SNDLVL_STATIC",
	"SNDLVL_70dB",
	"SNDLVL_NORM",
	"SNDLVL_75dB",
	"SNDLVL_80dB",
	"SNDLVL_85dB",
	"SNDLVL_90dB",
	"SNDLVL_95dB",
	"SNDLVL_100dB",
	"SNDLVL_105dB",
	"SNDLVL_120dB",
	"SNDLVL_130dB",
	"SNDLVL_GUNFIRE",
	"SNDLVL_140dB",
	"SNDLVL_150dB"
};

bool g_bSSMHack = false;

//max clipboard size
#define MAX_CLIPBOARD_ITEMS 10

//current clipboard stuff
static CUtlVector<KeyValues*> CurrClipboardName;		//for soundscape name
static CUtlVector<KeyValues*> CurrClipboardData;		//for soundscape data
static CUtlVector<KeyValues*> CurrClipboardRandom;		//for random wave

//-----------------------------------------------------------------------------
// Purpose: Helper funciton to compare vector and string
//-----------------------------------------------------------------------------
int Q_vecstr(const CUtlVector<wchar_t>& vec, int startindex, int endindex, const char* substr)
{
	//check for null substring
	if (!substr || !*substr)
		return -1;

	//store variables
	int substrLen = Q_strlen(substr);

	//search for match
	for (int i = startindex; i <= endindex; ++i)
	{
		bool match = true;
		for (int j = 0; j < substrLen; ++j)
		{
			wchar_t wc = vec[i + j];
			char ch = substr[j];
			if (wc != ch)
			{
				match = false;
				break;
			}
		}

		//found match
		if (match)
			return i;
	}

	//didnt find match
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Helper funciton to compare vector and string but reversed
//-----------------------------------------------------------------------------
int Q_vecrstr(const CUtlVector<wchar_t>& vec, int startindex, int endindex, const char* substr)
{
	//check for null substring
	if (!substr || !*substr)
		return -1;

	//store variables
	int substrLen = Q_strlen(substr);

	//search for match
	for (int i = endindex - substrLen + 1; i >= startindex; --i)
	{
		bool match = true;
		for (int j = 0; j < substrLen; ++j)
		{
			wchar_t wc = vec[i + j];
			char ch = substr[j];
			if (wc != ch)
			{
				match = false;
				break;
			}
		}

		//found match
		if (match)
			return i;
	}

	//didnt find match
	return -1;
}

//holds all the sound names
static CUtlVector<char*> g_SoundDirectories;

//-----------------------------------------------------------------------------
// Purpose: Sort function for utl vector
//-----------------------------------------------------------------------------
static int VectorSortFunc(char* const* p1, char* const* p2)
{
	return Q_stricmp(*p1, *p2);
}

//-----------------------------------------------------------------------------
// Purpose: Sort function for utl vector
//-----------------------------------------------------------------------------
static int VectorSortFunc(const char* const* p1, const char* const* p2)
{
	return Q_stricmp(*p1, *p2);
}

//-----------------------------------------------------------------------------
// Purpose: Gets all the sound names and stores them into g_SoundDirectories
//-----------------------------------------------------------------------------
static void GetSoundNames()
{
	//first off clear the sound array first
	for (int i = 0; i < g_SoundDirectories.Count(); i++)
		free(g_SoundDirectories[i]);

	g_SoundDirectories.RemoveAll();

	//directories to search
	CUtlVector<char*> directoriesToSearch;
	directoriesToSearch.AddToTail(strdup("sound"));

	//loop until all directories have been processed
	while (directoriesToSearch.Count() > 0)
	{
		//take the last added directory (depth-first search)
		char* currentDir = directoriesToSearch[directoriesToSearch.Count() - 1];
		directoriesToSearch.Remove(directoriesToSearch.Count() - 1);

		//create a wildcard path to search all files and subdirs
		char searchPath[MAX_PATH];
		Q_snprintf(searchPath, sizeof(searchPath), "%s/*", currentDir);

		FileFindHandle_t findHandle;
		const char* filename = g_pFullFileSystem->FindFirst(searchPath, &findHandle);

		while (filename)
		{
			//ignore special directories
			if (Q_strcmp(filename, ".") != 0 && Q_strcmp(filename, "..") != 0)
			{
				char fullPath[MAX_PATH];
				Q_snprintf(fullPath, sizeof(fullPath), "%s/%s", currentDir, filename);

				//if it's a directory, add it to the list for later processing
				if (g_pFullFileSystem->FindIsDirectory(findHandle))
				{
					directoriesToSearch.AddToTail(strdup(fullPath));
				}
				else
				{
					//check file extension and print if it's .wav or .mp3
					const char* ext = V_GetFileExtension(filename);
					if (ext && (!Q_stricmp(ext, "wav") || !Q_stricmp(ext, "mp3")))
					{
						g_SoundDirectories.AddToTail(strdup(fullPath + 6));
					}
				}
			}

			// Move to next file
			filename = g_pFullFileSystem->FindNext(findHandle);
		}

		//free the memory
		g_pFullFileSystem->FindClose(findHandle);
		free(currentDir);
	}

	//
	g_SoundDirectories.Sort(VectorSortFunc);
}


//text entry for text edit panel


class CTextPanelTextEntry : public vgui::TextEntry
{
public:
	DECLARE_CLASS_SIMPLE(CTextPanelTextEntry, vgui::TextEntry)

	//constructor
	CTextPanelTextEntry(vgui::Panel* parent, const char* panelName)
		: TextEntry(parent, panelName)
	{
		SetMultiline(true);
	}

	//called on keycode typed
	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		//check for enter or enter
		if (code == KEY_ENTER || code == KEY_PAD_ENTER)
			InsertString("\n");

		//check for tab
		else if (code == KEY_TAB)
			InsertString("    ");

		//do other key code
		else
			BaseClass::OnKeyCodeTyped(code);
	}

	//called on keycode pressed
	void OnKeyCodePressed(vgui::KeyCode code)
	{
		if (code == KEY_ENTER || code == KEY_PAD_ENTER
			|| code == KEY_TAB)
			return;

		BaseClass::OnKeyCodePressed(code);
	}

	//called on keycode insert
	void OnKeyTyped(wchar_t c)
	{
		//if (c == '{')
		//{
		//	//insert:
		//	//{
		//	//
		//	//}
		//	//and set cursor in the middle
		//	BaseClass::OnKeyTyped(c);
		//	InsertString("\n\n}    ");
		//	GotoLeft();
		//	GotoUp();
		//	SelectNoText();
		//}
		if (c == '"')
		{
			//check next item
			if (_cursorPos < m_TextStream.Count())
			{

				//check for " so you dont insert string inside string
				if (m_TextStream[_cursorPos] == '"')
				{
					GotoRight();
					SelectNone();
					return;
				}

			}

			//insert:
			//""
			//and set cursor in the middle
			BaseClass::OnKeyTyped(c);
			InsertString("\"");
			GotoLeft();
			SelectNone();
		}
		else
		{
			BaseClass::OnKeyTyped(c);
		}
	}
};



//simple clipboard panel
class CSoundscapeClipboard : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeClipboard, vgui::Frame)

	CSoundscapeClipboard(SoundscapeClipboardType type);

	//creates all the buttons
	void CreateButtons();

	//other
	void OnCommand(const char* pszCommand);
	void OnClose();

private:
	SoundscapeClipboardType m_Type;
};

//static soundscape clipboard panel
static CSoundscapeClipboard* g_SoundscapeClipboard;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSoundscapeClipboard::CSoundscapeClipboard(SoundscapeClipboardType type)
	: BaseClass(nullptr, "SoundscapeMakerClipboard"), m_Type(type)
{
	//get the size of the panel
	int tall = 30;

	switch (type)
	{
	case SoundscapeClipboardType::Type_SoundscapeName:
		tall += 29 * CurrClipboardName.Count();
		break;
	case SoundscapeClipboardType::Type_SoundscapeData:
		tall += 29 * CurrClipboardData.Count();
		break;
	case SoundscapeClipboardType::Type_SoundscapeRandomWave:
		tall += 29 * CurrClipboardRandom.Count();
		break;
	}

	SetParent(enginevgui->GetPanel(VGuiPanel_t::PANEL_TOOLS));
	SetCloseButtonVisible(true);
	SetSize(300, tall);
	MoveToCenterOfScreen();
	SetTitle("Soundscape Clipboard", true);
	SetSizeable(false);
	SetDeleteSelfOnClose(true);

	SetVisible(true);
	RequestFocus();
	MoveToFront();

	CreateButtons();
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the clipboard buttons
//-----------------------------------------------------------------------------
void CSoundscapeClipboard::CreateButtons()
{
	switch (m_Type)
	{
	case SoundscapeClipboardType::Type_SoundscapeName:
	{
		//add all the buttons
		for (int i = 0; i < CurrClipboardName.Count(); i++)
		{
			vgui::Button* button = new vgui::Button(this, CFmtStr("PasteButton%d", i), CFmtStr("%.50s", CurrClipboardName[i]->GetName()));
			button->SetBounds(10, 29 + (i * 27), 280, 25);
			button->SetCommand(CFmtStr("$PASTE%d", i));
		}
		break;
	}
	case SoundscapeClipboardType::Type_SoundscapeData:
	{
		//add all the buttons
		for (int i = 0; i < CurrClipboardData.Count(); i++)
		{
			vgui::Button* button = new vgui::Button(this, CFmtStr("PasteButton%d", i), "");

			//set text
			const char* name = CurrClipboardData[i]->GetName();
			if (!Q_stricmp(name, "playrandom"))
			{
				button->SetText("playrandom");
			}
			else if (!Q_stricmp(name, "playlooping"))
			{
				const char* looping = CurrClipboardData[i]->GetString("wave");
				if (strlen(looping) > 25)
					looping += strlen(looping) - 25;

				button->SetText(CFmtStr("%s : '%s'", name, looping));
			}
			else
			{
				const char* looping = CurrClipboardData[i]->GetString("name");
				if (strlen(looping) > 25)
					looping += strlen(looping) - 25;

				button->SetText(CFmtStr("%s : '%s'", name, looping));
			}

			//set other stuff
			button->SetBounds(10, 29 + (i * 27), 280, 25);
			button->SetCommand(CFmtStr("$PASTE%d", i));
		}
		break;
	}
	case SoundscapeClipboardType::Type_SoundscapeRandomWave:
	{
		//add all the buttons
		for (int i = 0; i < CurrClipboardRandom.Count(); i++)
		{
			vgui::Button* button = new vgui::Button(this, CFmtStr("PasteButton%d", i), CurrClipboardRandom[i]->GetString());
			button->SetBounds(10, 29 + (i * 27), 280, 25);
			button->SetCommand(CFmtStr("$PASTE%d", i));
		}
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when focus is killed
//-----------------------------------------------------------------------------
void CSoundscapeClipboard::OnClose()
{
	g_SoundscapeClipboard = nullptr;

	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeClipboard::OnCommand(const char* command)
{
	if (Q_stristr(command, "$PASTE") == command)
	{
		//get index
		int index = atoi(command + 6);

		//so this is what i am gonna do:
		//	1. copy KeyValue from index <index> to the top of the clipboard
		//	2. call g_SoundscapeMaker.PasteFromClipboard((int)m_Type);
		//	3. remove keyvalues at last index of clipboard CUtlVector
		switch (m_Type)
		{
		case SoundscapeClipboardType::Type_SoundscapeName:
			if (index >= CurrClipboardName.Count() || CurrClipboardName.Count() <= 0)
				return;

			CurrClipboardName.AddToTail(CurrClipboardName[index]);
			g_SoundscapeMaker->PasteFromClipboard((int)m_Type);
			CurrClipboardName.Remove(CurrClipboardName.Count() - 1);
			break;
		case SoundscapeClipboardType::Type_SoundscapeData:
			if (index >= CurrClipboardData.Count() || CurrClipboardData.Count() <= 0)
				return;

			CurrClipboardData.AddToTail(CurrClipboardData[index]);
			g_SoundscapeMaker->PasteFromClipboard((int)m_Type);
			CurrClipboardData.Remove(CurrClipboardData.Count() - 1);
			break;
		case SoundscapeClipboardType::Type_SoundscapeRandomWave:
			if (index >= CurrClipboardRandom.Count() || CurrClipboardRandom.Count() <= 0)
				return;

			CurrClipboardRandom.AddToTail(CurrClipboardRandom[index]);
			g_SoundscapeMaker->PasteFromClipboard((int)m_Type);
			CurrClipboardRandom.Remove(CurrClipboardRandom.Count() - 1);
			break;
		}
	}

	BaseClass::OnCommand(command);
}



//soundscape maker text editor panel
#define TEXT_PANEL_WIDTH 760
#define TEXT_PANEL_HEIGHT 630

#define TEXT_PANEL_COMMAND_SET "Set"
#define TEXT_PANEL_COMMAND_SET_OK "SetOk"
#define TEXT_PANEL_COMMAND_FIND "FInd"

class CSoundscapeTextPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeTextPanel, vgui::Frame);

	CSoundscapeTextPanel(vgui::VPANEL parent, const char* name);

	//sets the keyvalues
	void Set(KeyValues* keyvalues);
	void RecursiveSetText(KeyValues* keyvalues, CUtlBuffer& buffer, int indent);

	//other
	void OnCommand(const char* pszCommand);
	void PerformLayout();
	void OnClose() { BaseClass::OnClose(); }

private:
	CTextPanelTextEntry* m_Text;
	vgui::Button* m_SetButton;
	vgui::TextEntry* m_FindTextEntry;
	vgui::Button* m_FindButton;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSoundscapeTextPanel::CSoundscapeTextPanel(vgui::VPANEL parent, const char* name)
	: BaseClass(nullptr, name)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(true);
	SetMoveable(true);
	SetVisible(false);
	SetMinimumSize(575, 120);

	int ScreenWide, ScreenTall;
	vgui::surface()->GetScreenSize(ScreenWide, ScreenTall);

	SetTitle("Soundscape Text Editor", true);
	SetSize(TEXT_PANEL_WIDTH, TEXT_PANEL_HEIGHT);
	SetPos((ScreenWide - TEXT_PANEL_WIDTH) / 2, (ScreenTall - TEXT_PANEL_HEIGHT) / 2);



	//make text entry
	m_Text = new CTextPanelTextEntry(this, "EditBox");
	m_Text->SetBounds(5, 25, TEXT_PANEL_WIDTH - 10, TEXT_PANEL_HEIGHT - 55);
	m_Text->SetEnabled(true);
	m_Text->SetMultiline(true);
	m_Text->SetVerticalScrollbar(true);

	//make set button
	m_SetButton = new vgui::Button(this, "SetButton", "Apply Changes To Keyvalue Maker");
	m_SetButton->SetBounds(5, TEXT_PANEL_HEIGHT - 27, 250, 25);
	m_SetButton->SetCommand(TEXT_PANEL_COMMAND_SET);

	//make find text entry
	m_FindTextEntry = new vgui::TextEntry(this, "FindTextEntry");
	m_FindTextEntry->SetBounds(450, TEXT_PANEL_HEIGHT - 27, 200, 25);

	//make find button
	m_FindButton = new vgui::Button(this, "FindButton", "Find String");
	m_FindButton->SetBounds(655, TEXT_PANEL_HEIGHT - 27, 100, 25);
	m_FindButton->SetCommand(TEXT_PANEL_COMMAND_FIND);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the keyvalues
//-----------------------------------------------------------------------------
void CSoundscapeTextPanel::Set(KeyValues* keyvalues)
{
	//write everything into a buffer
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);

	//now write the keyvalues
	KeyValues* pCurrent = keyvalues;
	while (pCurrent)
	{
		RecursiveSetText(pCurrent, buf, 0);

		//put a newline
		if (pCurrent->GetNextTrueSubKey())
			buf.PutChar('\n');

		//get next
		pCurrent = pCurrent->GetNextTrueSubKey();
	}

	//write that to the m_Text
	m_Text->SetText((const char*)buf.Base());
}

//-----------------------------------------------------------------------------
// Purpose: Recursively writes to a util buffer
//-----------------------------------------------------------------------------
void CSoundscapeTextPanel::RecursiveSetText(KeyValues* keyvalues, CUtlBuffer& buffer, int indent)
{
	//write \t indent
	for (int i = 0; i < indent; i++)
		buffer.PutString("    ");

	//write name
	buffer.PutChar('"');
	buffer.PutString(keyvalues->GetName());
	buffer.PutString("\"\n");

	//write {
	for (int i = 0; i < indent; i++)
		buffer.PutString("    ");

	buffer.PutString("{\n");

	//increment indent
	indent++;

	//write all the keys first
	FOR_EACH_VALUE(keyvalues, value)
	{
		for (int i = 0; i < indent; i++)
			buffer.PutString("    ");

		//write name and value
		buffer.PutChar('"');
		buffer.PutString(value->GetName());
		buffer.PutString("\"    ");

		buffer.PutChar('"');
		buffer.PutString(value->GetString());
		buffer.PutString("\"\n");
	}

	//write all the subkeys now
	FOR_EACH_TRUE_SUBKEY(keyvalues, value)
	{
		//increment indent
		RecursiveSetText(value, buffer, indent);

		if (value->GetNextTrueSubKey())
			buffer.PutChar('\n');
	}

	//decrement indent
	indent--;

	//write ending }
	for (int i = 0; i < indent; i++)
		buffer.PutString("    ");

	buffer.PutString("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeTextPanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, TEXT_PANEL_COMMAND_SET))
	{
		//play sound
		vgui::surface()->PlaySound("ui/buttonclickrelease.wav");

		//check first incase you accidentally press it
		vgui::QueryBox* popup = new vgui::QueryBox("Set File?", "Are you sure you want to set the current keyvalues for the keyvalue maker?\nIf there are errors then this could break the keyvalue file.", this);
		popup->SetOKCommand(new KeyValues("Command", "command", TEXT_PANEL_COMMAND_SET_OK));
		popup->SetCancelButtonVisible(false);
		popup->AddActionSignalTarget(this);
		popup->DoModal(this);
		return;
	}

	//set text
	if (!Q_strcmp(pszCommand, TEXT_PANEL_COMMAND_SET_OK))
	{
		//get string
		int len = m_Text->GetTextLength() + 1;

		char* buf = new char[len];
		m_Text->GetText(buf, len);

		g_SoundscapeMaker->SetBuffer(buf);

		//delete string
		delete[] buf;

		//hide this
		SetVisible(false);
		return;
	}

	//find text
	if (!Q_strcmp(pszCommand, TEXT_PANEL_COMMAND_FIND))
	{
		//get buffer
		char buf[128];
		m_FindTextEntry->GetText(buf, sizeof(buf));

		int index = m_Text->_cursorPos + 1;
		int find = -1;

		//go in reversed order if holding shift
		if (vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT))
		{

			//see if we find index
			find = Q_vecrstr(m_Text->m_TextStream, 0, index - 2, buf);
			if (find == -1)

				//look again
				find = Q_vecrstr(m_Text->m_TextStream, index, m_Text->m_TextStream.Count() - 1, buf);

		}
		else
		{

			//see if we find index
			find = Q_vecstr(m_Text->m_TextStream, index, m_Text->m_TextStream.Count(), buf);
			if (find == -1)

				//look again
				find = Q_vecstr(m_Text->m_TextStream, 0, index, buf);

		}

		//check for invalid index
		if (find == -1)
		{
			//play an error sound
			vgui::surface()->PlaySound("resource/warning.wav");

			//get text
			char error[512];
			Q_snprintf(error, sizeof(error), "Couldnt find any instances of '%s'", buf);

			//show an error
			vgui::QueryBox* popup = new vgui::QueryBox("No Instances Found", error, this);
			popup->SetOKButtonText("Ok");
			popup->SetCancelButtonVisible(false);
			popup->AddActionSignalTarget(this);
			popup->DoModal(this);

			return;
		}

		//get number of newlines
		/*int newline = 0;
		int column = 0;
		for (int i = 0; i < find; i++)
		{
			if (m_Text->m_TextStream[i] == '\n')
			{
				newline++;
				column = 0;
			}
			else
			{
				column++;
			}
		}*/

		//select that
		m_Text->_cursorPos = find;
		m_Text->_select[0] = find;
		m_Text->_select[1] = find + Q_strlen(buf);
		m_Text->LayoutVerticalScrollBarSlider();
		m_Text->Repaint();
		m_Text->RequestFocus();

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Called on panel size changed
//-----------------------------------------------------------------------------
void CSoundscapeTextPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize(wide, tall);

	if (m_Text)
		m_Text->SetBounds(5, 25, wide - 10, tall - 55);

	if (m_SetButton)
		m_SetButton->SetBounds(5, tall - 27, 250, 25);

	if (m_FindTextEntry)
		m_FindTextEntry->SetBounds(wide - 310, tall - 27, 200, 25);

	if (m_FindButton)
		m_FindButton->SetBounds(wide - 105, tall - 27, 100, 25);
}

//soundscape settings panel
static CSoundscapeTextPanel* g_SoundscapeTextPanel = nullptr;




//soundscape maker text editor panel
#define DEBUG_PANEL_WIDTH 725
#define DEBUG_PANEL_HEIGHT 530

#define DEBUG_PANEL_COMMAND_CLEAR "Clear"

class CSoundscapeDebugPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeDebugPanel, vgui::Frame);

	CSoundscapeDebugPanel(vgui::VPANEL parent, const char* name);

	//sets the keyvalues
	void AddMessage(Color color, const char* text);

	//other
	void OnCommand(const char* pszCommand);
	void PerformLayout();
	void OnClose() { BaseClass::OnClose(); }

private:
	vgui::RichText* m_Text;
	vgui::Button* m_ClearButton;
	vgui::Label* m_SoundscapesFadingInText;

public:
	CGraphPanel* m_PanelSoundscapesFadingIn;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSoundscapeDebugPanel::CSoundscapeDebugPanel(vgui::VPANEL parent, const char* name)
	: BaseClass(nullptr, name)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(true);
	SetMoveable(true);
	SetVisible(false);
	SetMinimumSize(575, 280);

	SetTitle("Soundscape Debug Panel", true);
	SetSize(DEBUG_PANEL_WIDTH, DEBUG_PANEL_HEIGHT);
	SetPos(0, 0);



	//make text entry
	m_Text = new vgui::RichText(this, "DebugText");
	m_Text->SetBounds(5, 25, DEBUG_PANEL_WIDTH - 10, DEBUG_PANEL_HEIGHT - 55);
	m_Text->SetEnabled(true);
	m_Text->SetVerticalScrollbar(true);

	//make clear button
	m_ClearButton = new vgui::Button(this, "ClearButton", "Clear");
	m_ClearButton->SetBounds(5, DEBUG_PANEL_HEIGHT - 215, DEBUG_PANEL_WIDTH - 10, 25);
	m_ClearButton->SetCommand(DEBUG_PANEL_COMMAND_CLEAR);

	//make fading in label
	m_SoundscapesFadingInText = new vgui::Label(this, "LabelFadingIn", "Soundscapes Fading In");
	m_SoundscapesFadingInText->SetBounds(5, DEBUG_PANEL_HEIGHT - 187, DEBUG_PANEL_WIDTH - 10, 20);

	//make soundscapes fading in thing
	m_PanelSoundscapesFadingIn = new CGraphPanel(this, "SoundscapesFadingIn");
	m_PanelSoundscapesFadingIn->SetBounds(5, DEBUG_PANEL_HEIGHT - 165, DEBUG_PANEL_WIDTH - 10, 155);
	m_PanelSoundscapesFadingIn->SetMaxTextValue(1.0f);
	m_PanelSoundscapesFadingIn->SetHorizontalLinesMax(5);
}

//-----------------------------------------------------------------------------
// Purpose: adds a message to the debug panel
//-----------------------------------------------------------------------------
void CSoundscapeDebugPanel::AddMessage(Color color, const char* text)
{
	m_Text->InsertColorChange(color);
	m_Text->InsertString(text);

	m_Text->SetMaximumCharCount(100000);
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeDebugPanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, DEBUG_PANEL_COMMAND_CLEAR))
	{
		//clear the text
		m_Text->SetText("");
		m_Text->GotoTextEnd();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Called on panel size changed
//-----------------------------------------------------------------------------
void CSoundscapeDebugPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize(wide, tall);

	m_Text->SetBounds(5, 25, wide - 10, tall - 245);
	m_ClearButton->SetBounds(5, tall - 215, wide - 10, 25);
	m_PanelSoundscapesFadingIn->SetBounds(5, tall - 165, wide - 10, 155);
	m_SoundscapesFadingInText->SetBounds(5, tall - 187, wide - 10, 20);
}

//soundscape debug panel
static CSoundscapeDebugPanel* g_SoundscapeDebugPanel = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Function to print text to debug panel
//-----------------------------------------------------------------------------
void SoundscapePrint(Color color, const char* msg, ...)
{
	//format string
	va_list args;
	va_start(args, msg);

	char buf[2048];
	Q_vsnprintf(buf, sizeof(buf), msg, args);
	g_SoundscapeDebugPanel->AddMessage(color, buf);

	va_end(args);
}

//-----------------------------------------------------------------------------
// Purpose: Function to add a line to the soundscape debug panel
//-----------------------------------------------------------------------------
void SoundscapeAddLine(Color color, float speed, float width, bool accending)
{
	if (g_SoundscapeDebugPanel->m_PanelSoundscapesFadingIn->GetNumLines() <= 6)
		g_SoundscapeDebugPanel->m_PanelSoundscapesFadingIn->AddLine(accending, color.r(), color.g(), color.b(), speed, width);
}

//-----------------------------------------------------------------------------
// Purpose: Function to get debug line num
//-----------------------------------------------------------------------------
int SoundscapeGetLineNum()
{
	return g_SoundscapeDebugPanel->m_PanelSoundscapesFadingIn->GetNumLines();
}

//vector positions
Vector g_SoundscapePositions[] = {
	vec3_origin,
	vec3_origin,
	vec3_origin,
	vec3_origin,
	vec3_origin,
	vec3_origin,
	vec3_origin,
	vec3_origin
};

#define SETTINGS_PANEL_WIDTH 350
#define SETTINGS_PANEL_HEIGHT 277

#define SETTINGS_PANEL_COMMAND_POS1 "GetPos0"
#define SETTINGS_PANEL_COMMAND_POS2 "GetPos1"
#define SETTINGS_PANEL_COMMAND_POS3 "GetPos2"
#define SETTINGS_PANEL_COMMAND_POS4 "GetPos3"
#define SETTINGS_PANEL_COMMAND_POS5 "GetPos4"
#define SETTINGS_PANEL_COMMAND_POS6 "GetPos5"
#define SETTINGS_PANEL_COMMAND_POS7 "GetPos6"
#define SETTINGS_PANEL_COMMAND_POS8 "GetPos7"
#define SETTINGS_PANEL_COMMAND_SHOW "ShowPositions"
#define SETTINGS_PANEL_COMMAND_DEBUG "Debug"

#define MAX_SOUNDSCAPES 8

//soundscape maker settings panel
class CSoundscapeSettingsPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeSettingsPanel, vgui::Frame);

	CSoundscapeSettingsPanel(vgui::VPANEL parent, const char* name);

	//other
	void OnCommand(const char* pszCommand);

	//sets the text
	void SetItem(int index, const Vector& value);

	//message funcs
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);

	~CSoundscapeSettingsPanel();

private:
	//position text entries
	vgui::TextEntry* m_TextEntryPos0;
	vgui::TextEntry* m_TextEntryPos1;
	vgui::TextEntry* m_TextEntryPos2;
	vgui::TextEntry* m_TextEntryPos3;
	vgui::TextEntry* m_TextEntryPos4;
	vgui::TextEntry* m_TextEntryPos5;
	vgui::TextEntry* m_TextEntryPos6;
	vgui::TextEntry* m_TextEntryPos7;
	vgui::CheckButton* m_ShowSoundscapePositions;
	vgui::Button* m_ShowSoundscapeDebug;

	friend class CSoundscapeMaker;
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSoundscapeSettingsPanel::CSoundscapeSettingsPanel(vgui::VPANEL parent, const char* name)
	: BaseClass(nullptr, name)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(false);

	//set the size and pos
	int ScreenWide, ScreenTall;
	vgui::surface()->GetScreenSize(ScreenWide, ScreenTall);

	SetTitle("Soundscape Maker Settings", true);
	SetSize(SETTINGS_PANEL_WIDTH, SETTINGS_PANEL_HEIGHT);
	SetPos((ScreenWide - SETTINGS_PANEL_WIDTH) / 2, (ScreenTall - SETTINGS_PANEL_HEIGHT) / 2);



	//load settings
	KeyValues* settings = new KeyValues("settings");
	if (!settings->LoadFromFile(filesystem, "cfg/soundscape_maker.txt", "MOD"))
		ConWarning("Failed to load settings for 'cfg/soundscape_maker.txt'. Using default settings.");

	//get positions
	const char* pos0 = settings->GetString("Position0", "0 0 0");
	const char* pos1 = settings->GetString("Position1", "0 0 0");
	const char* pos2 = settings->GetString("Position2", "0 0 0");
	const char* pos3 = settings->GetString("Position3", "0 0 0");
	const char* pos4 = settings->GetString("Position4", "0 0 0");
	const char* pos5 = settings->GetString("Position5", "0 0 0");
	const char* pos6 = settings->GetString("Position6", "0 0 0");
	const char* pos7 = settings->GetString("Position7", "0 0 0");

	//create position text 1
	m_TextEntryPos0 = new vgui::TextEntry(this, "PosTextEntry0");
	m_TextEntryPos0->SetEnabled(true);
	m_TextEntryPos0->SetText(pos0 ? pos0 : "0 0 0");
	m_TextEntryPos0->SetBounds(5, 30, 230, 20);
	m_TextEntryPos0->SetMaximumCharCount(32);

	//create position 1 button
	vgui::Button* m_ButtonPos0 = new vgui::Button(this, "PosButton0", "Find Position 0", this, SETTINGS_PANEL_COMMAND_POS1);
	m_ButtonPos0->SetBounds(240, 30, 100, 20);

	//create position text 1
	m_TextEntryPos1 = new vgui::TextEntry(this, "PosTextEntry1");
	m_TextEntryPos1->SetEnabled(true);
	m_TextEntryPos1->SetText(pos1 ? pos1 : "0 0 0");
	m_TextEntryPos1->SetBounds(5, 55, 230, 20);
	m_TextEntryPos1->SetMaximumCharCount(32);

	//create position 2 button
	vgui::Button* m_ButtonPos1 = new vgui::Button(this, "PosButton1", "Find Position 1", this, SETTINGS_PANEL_COMMAND_POS2);
	m_ButtonPos1->SetBounds(240, 55, 100, 20);

	//create position text 3
	m_TextEntryPos2 = new vgui::TextEntry(this, "PosTextEntry0");
	m_TextEntryPos2->SetEnabled(true);
	m_TextEntryPos2->SetText(pos2 ? pos2 : "0 0 0");
	m_TextEntryPos2->SetBounds(5, 80, 230, 20);
	m_TextEntryPos2->SetMaximumCharCount(32);

	//create position 1 button
	vgui::Button* m_ButtonPos2 = new vgui::Button(this, "PosButton2", "Find Position 2", this, SETTINGS_PANEL_COMMAND_POS3);
	m_ButtonPos2->SetBounds(240, 80, 100, 20);

	// create position text 4
	m_TextEntryPos3 = new vgui::TextEntry(this, "PosTextEntry3");
	m_TextEntryPos3->SetEnabled(true);
	m_TextEntryPos3->SetText(pos3 ? pos3 : "0 0 0");
	m_TextEntryPos3->SetBounds(5, 105, 230, 20);
	m_TextEntryPos3->SetMaximumCharCount(32);

	// create position 4 button
	vgui::Button* m_ButtonPos3 = new vgui::Button(this, "PosButton3", "Find Position 3", this, SETTINGS_PANEL_COMMAND_POS4);
	m_ButtonPos3->SetBounds(240, 105, 100, 20);

	// create position text 5
	m_TextEntryPos4 = new vgui::TextEntry(this, "PosTextEntry4");
	m_TextEntryPos4->SetEnabled(true);
	m_TextEntryPos4->SetText(pos4 ? pos4 : "0 0 0");
	m_TextEntryPos4->SetBounds(5, 130, 230, 20);
	m_TextEntryPos4->SetMaximumCharCount(32);

	// create position 5 button
	vgui::Button* m_ButtonPos4 = new vgui::Button(this, "PosButton4", "Find Position 4", this, SETTINGS_PANEL_COMMAND_POS5);
	m_ButtonPos4->SetBounds(240, 130, 100, 20);

	// create position text 6
	m_TextEntryPos5 = new vgui::TextEntry(this, "PosTextEntry5");
	m_TextEntryPos5->SetEnabled(true);
	m_TextEntryPos5->SetText(pos5 ? pos5 : "0 0 0");
	m_TextEntryPos5->SetBounds(5, 155, 230, 20);
	m_TextEntryPos5->SetMaximumCharCount(32);

	// create position 6 button
	vgui::Button* m_ButtonPos5 = new vgui::Button(this, "PosButton5", "Find Position 5", this, SETTINGS_PANEL_COMMAND_POS6);
	m_ButtonPos5->SetBounds(240, 155, 100, 20);

	// create position text 7
	m_TextEntryPos6 = new vgui::TextEntry(this, "PosTextEntry6");
	m_TextEntryPos6->SetEnabled(true);
	m_TextEntryPos6->SetText(pos6 ? pos6 : "0 0 0");
	m_TextEntryPos6->SetBounds(5, 180, 230, 20);
	m_TextEntryPos6->SetMaximumCharCount(32);

	// create position 7 button
	vgui::Button* m_ButtonPos6 = new vgui::Button(this, "PosButton6", "Find Position 6", this, SETTINGS_PANEL_COMMAND_POS7);
	m_ButtonPos6->SetBounds(240, 180, 100, 20);

	// create position text 8
	m_TextEntryPos7 = new vgui::TextEntry(this, "PosTextEntry7");
	m_TextEntryPos7->SetEnabled(true);
	m_TextEntryPos7->SetText(pos7 ? pos7 : "0 0 0");
	m_TextEntryPos7->SetBounds(5, 205, 230, 20);
	m_TextEntryPos7->SetMaximumCharCount(32);

	// create position 8 button
	vgui::Button* m_ButtonPos7 = new vgui::Button(this, "PosButton7", "Find Position 7", this, SETTINGS_PANEL_COMMAND_POS8);
	m_ButtonPos7->SetBounds(240, 205, 100, 20);

	// create show soundscape positions checkbox
	m_ShowSoundscapePositions = new vgui::CheckButton(this, "ShowCheckox", "Show Soundscape Positions");
	m_ShowSoundscapePositions->SetBounds(75, 225, 200, 20);
	m_ShowSoundscapePositions->SetCommand(SETTINGS_PANEL_COMMAND_SHOW);
	m_ShowSoundscapePositions->SetSelected(settings->GetBool("ShowSoundscapes", false));

	//set convar value
	ConVar* cv = cvar->FindVar("__ss_draw");
	if (cv)
		cv->SetValue(m_ShowSoundscapePositions->IsSelected());

	//create divider
	vgui::Divider* div = new vgui::Divider(this, "Divider");
	div->SetBounds(-2, 247, SETTINGS_PANEL_WIDTH + 4, 2);

	//create debug thing
	m_ShowSoundscapeDebug = new vgui::Button(this, "DebugInfo", "Show soundscape debug panel");
	m_ShowSoundscapeDebug->SetBounds(20, 254, SETTINGS_PANEL_WIDTH - 40, 20);
	m_ShowSoundscapeDebug->SetCommand(SETTINGS_PANEL_COMMAND_DEBUG);

	//set server positions
	ConCommand* cc = cvar->FindCommand("__ss_maker_set");
	if (cc)
	{
		CCommand args;

		//do pos 0
		if (pos0)
		{
			args.Tokenize(CFmtStr("ssmaker 0 %s 1", pos0));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[0].Base(), pos0);
		}

		//do pos 1
		if (pos1)
		{
			args.Tokenize(CFmtStr("ssmaker 1 %s 1", pos1));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[1].Base(), pos1);
		}

		//do pos 2
		if (pos2)
		{
			args.Tokenize(CFmtStr("ssmaker 2 %s 1", pos2));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[2].Base(), pos2);
		}

		//do pos 3
		if (pos3)
		{
			args.Tokenize(CFmtStr("ssmaker 3 %s 1", pos3));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[3].Base(), pos3);
		}

		//do pos 4
		if (pos4)
		{
			args.Tokenize(CFmtStr("ssmaker 4 %s 1", pos4));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[4].Base(), pos4);
		}

		//do pos 5
		if (pos5)
		{
			args.Tokenize(CFmtStr("ssmaker 5 %s 1", pos5));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[5].Base(), pos5);
		}

		//do pos 6
		if (pos6)
		{
			args.Tokenize(CFmtStr("ssmaker 6 %s 1", pos6));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[6].Base(), pos6);
		}

		//do pos 7
		if (pos7)
		{
			args.Tokenize(CFmtStr("ssmaker 7 %s", pos7));
			cc->Dispatch(args);

			UTIL_StringToVector(g_SoundscapePositions[7].Base(), pos7);
		}
	}

	//delete settings
	settings->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeSettingsPanel::OnCommand(const char* pszCommand)
{
	if (Q_strstr(pszCommand, "GetPos") == pszCommand)
	{
		//search for number
		pszCommand = pszCommand + 6;

		//execute command
		static ConCommand* cc = cvar->FindCommand("__ss_maker_start");
		if (cc)
		{
			//hide everything first
			g_SoundscapeMaker->SetAllVisible(false);

			CCommand args;
			args.Tokenize(CFmtStr("ssmaker %d", atoi(pszCommand)));
			cc->Dispatch(args);
		}

		return;
	}

	else if (!Q_strcmp(pszCommand, SETTINGS_PANEL_COMMAND_SHOW))
	{
		static ConVar* cv = cvar->FindVar("__ss_draw");
		if (cv)
			cv->SetValue(m_ShowSoundscapePositions->IsSelected());

		return;
	}

	//handle debug thing
	else if (!Q_strcmp(pszCommand, SETTINGS_PANEL_COMMAND_DEBUG))
	{
		g_SoundscapeDebugPanel->SetVisible(true);
		g_SoundscapeDebugPanel->RequestFocus();
		g_SoundscapeDebugPanel->MoveToFront();
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
void CSoundscapeSettingsPanel::SetItem(int index, const Vector& value)
{
	const char* text = CFmtStr("%.3f %.3f %.3f", value.x, value.y, value.z);

	//check index
	switch (index)
	{
	case 0:
		m_TextEntryPos0->RequestFocus();
		m_TextEntryPos0->SetText(text);
		g_SoundscapePositions[0] = value;
		break;

	case 1:
		m_TextEntryPos1->RequestFocus();
		m_TextEntryPos1->SetText(text);
		g_SoundscapePositions[1] = value;
		break;

	case 2:
		m_TextEntryPos2->RequestFocus();
		m_TextEntryPos2->SetText(text);
		g_SoundscapePositions[2] = value;
		break;
	case 3:
		m_TextEntryPos3->RequestFocus();
		m_TextEntryPos3->SetText(text);
		g_SoundscapePositions[3] = value;
		break;

	case 4:
		m_TextEntryPos4->RequestFocus();
		m_TextEntryPos4->SetText(text);
		g_SoundscapePositions[4] = value;
		break;

	case 5:
		m_TextEntryPos5->RequestFocus();
		m_TextEntryPos5->SetText(text);
		g_SoundscapePositions[5] = value;
		break;

	case 6:
		m_TextEntryPos6->RequestFocus();
		m_TextEntryPos6->SetText(text);
		g_SoundscapePositions[6] = value;
		break;

	case 7:
		m_TextEntryPos7->RequestFocus();
		m_TextEntryPos7->SetText(text);
		g_SoundscapePositions[7] = value;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on text changed
//-----------------------------------------------------------------------------
void CSoundscapeSettingsPanel::OnTextChanged(KeyValues* kv)
{
	static ConCommand* cc = cvar->FindCommand("__ss_maker_set");

	//check focus
	if (m_TextEntryPos0->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos0->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[0].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 0 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos1->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos1->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[1].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 1 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos2->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos2->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[2].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 2 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos3->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos3->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[3].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 3 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos4->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos4->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[4].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 4 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos5->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos5->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[5].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 5 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos6->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos6->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[6].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 6 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

	//check focus
	if (m_TextEntryPos7->HasFocus())
	{
		//get text
		char buf[512];
		m_TextEntryPos7->GetText(buf, sizeof(buf));

		//convert to vector
		UTIL_StringToVector(g_SoundscapePositions[7].Base(), buf);

		//do command
		if (cc)
		{
			CCommand args;
			args.Tokenize(CFmtStr("ssmaker 7 %s 1", buf));
			cc->Dispatch(args);
		}

		return;
	}

}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CSoundscapeSettingsPanel::~CSoundscapeSettingsPanel()
{
	//save everything
	KeyValues* settings = new KeyValues("settings");

	//get text's
	char text0[64];
	char text1[64];
	char text2[64];
	char text3[64];
	char text4[64];
	char text5[64];
	char text6[64];
	char text7[64];

	m_TextEntryPos0->GetText(text0, sizeof(text0));
	m_TextEntryPos1->GetText(text1, sizeof(text1));
	m_TextEntryPos2->GetText(text2, sizeof(text2));
	m_TextEntryPos3->GetText(text3, sizeof(text3));
	m_TextEntryPos4->GetText(text4, sizeof(text4));
	m_TextEntryPos5->GetText(text5, sizeof(text5));
	m_TextEntryPos6->GetText(text6, sizeof(text6));
	m_TextEntryPos7->GetText(text7, sizeof(text7));

	//save text entries
	settings->SetString("Position0", text0);
	settings->SetString("Position1", text1);
	settings->SetString("Position2", text2);
	settings->SetString("Position3", text3);
	settings->SetString("Position4", text4);
	settings->SetString("Position5", text5);
	settings->SetString("Position6", text6);
	settings->SetString("Position7", text7);

	//save check buttons
	settings->SetBool("ShowSoundscapes", m_ShowSoundscapePositions->IsSelected());

	//save to file
	settings->SaveToFile(filesystem, "cfg/soundscape_maker.txt", "MOD");
	settings->deleteThis();
}

//static soundscape settings panel
static CSoundscapeSettingsPanel* g_SettingsPanel = nullptr;


#define BUTTON_MENU_COMMAND_COPY_CLIPBOARD "CopyClipboard"

//button
class CSoundscapeButton : public vgui::Button
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeButton, vgui::Button)

	CSoundscapeButton(vgui::Panel* parent, const char* name, const char* text, vgui::Panel* target = nullptr, const char* command = nullptr, KeyValues* kv = nullptr, SoundscapeClipboardType type = SoundscapeClipboardType::Type_SoundscapeNone)
		: BaseClass(parent, name, text, target, command), m_bIsSelected(false), m_KeyValues(kv), m_KeyValuesType(type)
	{
		m_ColorSelected = Color(200, 200, 200, 200);
		m_FgColorSelected = Color(0, 0, 0, 255);
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
		else
			SetBgColor(m_ColorNotSelected);

		BaseClass::PaintBackground();
	}

	//paints
	void Paint()
	{
		if (m_bIsSelected)
			SetFgColor(m_FgColorSelected);
		else
			SetFgColor(m_FgColorNotSelected);

		BaseClass::Paint();
	}

	//mouse release
	void OnMouseReleased(vgui::MouseCode code)
	{
		if (code != vgui::MouseCode::MOUSE_RIGHT)
			return BaseClass::OnMouseReleased(code);

		//this should never happen but just in case
		if (!m_KeyValues)
			return;

		//get cursor pos
		int x, y;
		vgui::surface()->SurfaceGetCursorPos(x, y);

		//show menu
		vgui::Menu* menu = new vgui::Menu(this, "Clipboard");
		menu->AddMenuItem("CopyToClipboard", "Copy", BUTTON_MENU_COMMAND_COPY_CLIPBOARD, this);
		menu->SetBounds(x, y, 200, 50);
		menu->SetVisible(true);

		BaseClass::Paint();
	}

	//mouse release
	void OnCommand(const char* pszCommand)
	{
		if (!Q_strcmp(pszCommand, BUTTON_MENU_COMMAND_COPY_CLIPBOARD))
		{
			//create copy of keyvalues
			switch (m_KeyValuesType)
			{
			case SoundscapeClipboardType::Type_SoundscapeName:
			{
				//copy
				if (CurrClipboardName.Count() >= MAX_CLIPBOARD_ITEMS)
				{
					CurrClipboardName[0]->deleteThis();
					CurrClipboardName.Remove(0);
				}

				CurrClipboardName.AddToTail(m_KeyValues->MakeCopy());

				//debug message
				SoundscapePrint(Color(255, 255, 255, 255), "Soundscape: '%s' Coppied to clipboard.\n", m_KeyValues->GetName());
				break;
			}
			case SoundscapeClipboardType::Type_SoundscapeData:
			{
				//copy
				if (CurrClipboardData.Count() >= MAX_CLIPBOARD_ITEMS)
				{
					CurrClipboardData[0]->deleteThis();
					CurrClipboardData.Remove(0);
				}

				//make copy
				CurrClipboardData.AddToTail(m_KeyValues->MakeCopy());

				//debug message
				SoundscapePrint(Color(255, 255, 255, 255), "Soundscape Data: '%s' Coppied to clipboard.\n", m_KeyValues->GetName());
				break;
			}
			case SoundscapeClipboardType::Type_SoundscapeRandomWave:
			{
				//copy
				if (CurrClipboardRandom.Count() >= MAX_CLIPBOARD_ITEMS)
				{
					CurrClipboardRandom[0]->deleteThis();
					CurrClipboardRandom.Remove(0);
				}

				CurrClipboardRandom.AddToTail(m_KeyValues->MakeCopy());

				//debug message
				SoundscapePrint(Color(255, 255, 255, 255), "Soundscape Random Wave: '%s' Coppied to clipboard.\n", m_KeyValues->GetString());
				break;
			}
			}
		}
	}

	//is this selected or not
	bool m_bIsSelected;
	static Color m_ColorSelected;
	static Color m_ColorNotSelected;
	static Color m_FgColorSelected;
	static Color m_FgColorNotSelected;

	KeyValues* m_KeyValues = nullptr;
	SoundscapeClipboardType m_KeyValuesType;
};

Color CSoundscapeButton::m_ColorSelected = Color();
Color CSoundscapeButton::m_ColorNotSelected = Color();
Color CSoundscapeButton::m_FgColorSelected = Color();
Color CSoundscapeButton::m_FgColorNotSelected = Color();


//soundscape combo box

class CSoundListComboBox : public vgui::ComboBox
{
public:
	DECLARE_CLASS_SIMPLE(CSoundListComboBox, vgui::ComboBox);

	CSoundListComboBox(Panel* parent, const char* panelName, int numLines, bool allowEdit) :
		BaseClass(parent, panelName, numLines, allowEdit) {}

	//on key typed. check for menu item with text inside it and if found then
	//select that item.
	void OnKeyTyped(wchar_t unichar)
	{
		//check for ctrl or shift down
		if (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LCONTROL) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RCONTROL) || unichar == '`')
			return;

		//open up this combo box
		if (unichar == 13)
		{
			ShowMenu();
			return;
		}

		BaseClass::OnKeyTyped(unichar);

		//check for backspace
		if (unichar == 8 || unichar == '_')
			return;

		//get text
		char buf[512];
		GetText(buf, sizeof(buf));

		//start from current index + 1
		int start = GetMenu()->GetActiveItem() + 1;

		//look for sound with same name starting from the start first
		for (int i = start; i < g_SoundDirectories.Count(); i++)
		{
			if (Q_stristr(g_SoundDirectories[i], buf))
			{
				GetMenu()->SetCurrentlyHighlightedItem(i);
				return;
			}
		}

		//now cheeck from 0 to the start
		for (int i = 0; i < start; i++)
		{
			if (Q_stristr(g_SoundDirectories[i], buf))
			{
				GetMenu()->SetCurrentlyHighlightedItem(i);
				return;
			}
		}
	}
};


//sounds list panel

#define SOUND_LIST_PANEL_WIDTH 375
#define SOUND_LIST_PANEL_HEIGHT 255
#define SOUND_LIST_PLAY_COMMAND "PlaySound"
#define SOUND_LIST_STOP_COMMAND "StopSound"
#define SOUND_LIST_INSERT_COMMAND "Insert"
#define SOUND_LIST_RELOAD_COMMAND "Reload"
#define SOUND_LIST_SEARCH_COMMAND "Search"

class CSoundListPanel : public vgui::Frame
{
public:
	DECLARE_CLASS_SIMPLE(CSoundListPanel, vgui::Frame);

	CSoundListPanel(vgui::VPANEL parent, const char* name);

	//initalizes sound combo box
	void InitalizeSounds();
	void InitalizeSoundscapes(CUtlVector<const char*>& OtherSoundscapes);

	//sets if this is currently using the soundscape panel or sound panel
	void SetIsUsingSoundPanel(bool bUsing);

	//other
	void OnCommand(const char* pszCommand);
	void OnClose();

private:
	friend class CSoundscapeMaker;

	//are we currently in the 'sound' panel or 'soundscape' panel
	bool bCurrentlyInSoundPanel = true;

	CSoundListComboBox* m_SoundsList;		//for sounds
	CSoundListComboBox* m_SoundscapesList;	//for soundscapes
	vgui::TextEntry* m_SearchText;
	vgui::Button* m_SearchButton;
	vgui::Button* m_PlayButton;
	vgui::Button* m_StopSoundButton;
	vgui::Button* m_InsertButton;
	vgui::Button* m_ReloadSounds;

	//current sound guid
	int m_iSongGuid = -1;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CSoundListPanel::CSoundListPanel(vgui::VPANEL parent, const char* name)
	: BaseClass(nullptr, name)
{
	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(false);

	//set the size and pos
	int ScreenWide, ScreenTall;
	vgui::surface()->GetScreenSize(ScreenWide, ScreenTall);

	SetTitle("Sounds List", true);
	SetSize(SOUND_LIST_PANEL_WIDTH, SOUND_LIST_PANEL_HEIGHT);
	SetPos((ScreenWide - SOUND_LIST_PANEL_WIDTH) / 2, (ScreenTall - SOUND_LIST_PANEL_HEIGHT) / 2);

	//create combo box's
	m_SoundsList = new CSoundListComboBox(this, "SoundsList", 20, true);
	m_SoundsList->SetBounds(5, 25, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_SoundsList->AddActionSignalTarget(this);
	m_SoundsList->SetVisible(true);

	m_SoundscapesList = new CSoundListComboBox(this, "SoundscapesList", 20, true);
	m_SoundscapesList->SetBounds(5, 25, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_SoundscapesList->AddActionSignalTarget(this);
	m_SoundscapesList->SetVisible(false);

	//make divider
	vgui::Divider* divider1 = new vgui::Divider(this, "Divider");
	divider1->SetBounds(-5, 48, SOUND_LIST_PANEL_WIDTH + 10, 2);

	//create text
	vgui::Label* label1 = new vgui::Label(this, "FindSound", "Find Sound");
	label1->SetBounds(147, 51, 120, 20);

	//create text entry
	m_SearchText = new vgui::TextEntry(this, "SearchTextEntry");
	m_SearchText->SetBounds(5, 75, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_SearchText->SetEnabled(true);
	m_SearchText->SetText("");

	//create search for button
	m_SearchButton = new vgui::Button(this, "SearchButton", "Search For");
	m_SearchButton->SetBounds(5, 100, SOUND_LIST_PANEL_WIDTH - 15, 20);;
	m_SearchButton->SetEnabled(true);
	m_SearchButton->SetCommand(SOUND_LIST_SEARCH_COMMAND);

	//make divider
	vgui::Divider* divider2 = new vgui::Divider(this, "Divider");
	divider2->SetBounds(-5, 124, SOUND_LIST_PANEL_WIDTH + 10, 2);

	//create text
	vgui::Label* label2 = new vgui::Label(this, "SoundButtons", "Sound Buttons");
	label2->SetBounds(140, 127, 120, 20);

	//create play button
	m_PlayButton = new vgui::Button(this, "PlayButton", "Play Sound", this);
	m_PlayButton->SetBounds(5, 150, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_PlayButton->SetCommand(SOUND_LIST_PLAY_COMMAND);

	//create stop sound button
	m_StopSoundButton = new vgui::Button(this, "StopSound", "Stop Sound", this);
	m_StopSoundButton->SetBounds(5, 175, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_StopSoundButton->SetCommand(SOUND_LIST_STOP_COMMAND);

	//create sound insert button
	m_InsertButton = new vgui::Button(this, "InsertSound", "Insert Sound", this);
	m_InsertButton->SetBounds(5, 225, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_InsertButton->SetCommand(SOUND_LIST_INSERT_COMMAND);

	//create reload sounds button
	m_ReloadSounds = new vgui::Button(this, "ReloadSounds", "Reload Sounds", this);
	m_ReloadSounds->SetBounds(5, 200, SOUND_LIST_PANEL_WIDTH - 15, 20);
	m_ReloadSounds->SetCommand(SOUND_LIST_RELOAD_COMMAND);
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundListPanel::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, SOUND_LIST_SEARCH_COMMAND))
	{
		//get text
		char buf[512];
		m_SearchText->GetText(buf, sizeof(buf));

		//check for shift key
		bool shift = (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LSHIFT) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RSHIFT));

		//vector of texts
		CUtlVector<char*> SoundNames;
		CSoundListComboBox* SoundList = bCurrentlyInSoundPanel ? m_SoundsList : m_SoundscapesList;

		//if we are in soundscape mode then set the SoundNames to all the soundscapes. else set SoundNames to g_SoundDirectories
		if (!bCurrentlyInSoundPanel)
		{
			for (int i = 0; i < m_SoundscapesList->GetItemCount(); i++)
			{
				//insert
				char* tmpbuf = new char[512];
				m_SoundscapesList->GetItemText(i, tmpbuf, 512);

				SoundNames.AddToTail(tmpbuf);
			}
		}
		else
		{
			SoundNames = g_SoundDirectories;
		}

		if (shift)
		{
			//start from current index - 1
			int start = SoundList->GetMenu()->GetActiveItem() - 1;

			//look for sound with same name starting from the start first and going down
			for (int i = start; i >= 0; i--)
			{
				if (Q_stristr(SoundNames[i], buf))
				{
					//select item
					SoundList->GetMenu()->SetCurrentlyHighlightedItem(i);
					SoundList->ActivateItem(i);

					//set text
					SoundList->SetText(SoundNames[i]);

					//delete all soundscapes if we need to
					if (!bCurrentlyInSoundPanel) for (int i = 0; i < SoundNames.Count(); i++)
						delete[] SoundNames[i];

					return;
				}
			}


			//now cheeck from the SoundNames to the start
			for (int i = SoundNames.Count() - 1; i > start; i--)
			{
				if (Q_stristr(SoundNames[i], buf))
				{
					//select item
					SoundList->GetMenu()->SetCurrentlyHighlightedItem(i);
					SoundList->ActivateItem(i);

					//set text
					SoundList->SetText(SoundNames[i]);

					//delete all soundscapes if we need to
					if (!bCurrentlyInSoundPanel) for (int i = 0; i < SoundNames.Count(); i++)
						delete[] SoundNames[i];

					return;
				}
			}
		}
		else
		{
			//start from current index + 1
			int start = SoundList->GetMenu()->GetActiveItem() + 1;

			//look for sound with same name starting from the start first
			for (int i = start; i < SoundNames.Count(); i++)
			{
				if (Q_stristr(SoundNames[i], buf))
				{
					//select item
					SoundList->GetMenu()->SetCurrentlyHighlightedItem(i);
					SoundList->ActivateItem(i);

					//set text
					SoundList->SetText(SoundNames[i]);

					//delete all soundscapes if we need to
					if (!bCurrentlyInSoundPanel) for (int i = 0; i < SoundNames.Count(); i++)
						delete[] SoundNames[i];

					return;
				}
			}


			//now cheeck from 0 to the start
			for (int i = 0; i < start; i++)
			{
				if (Q_stristr(SoundNames[i], buf))
				{
					//select item
					SoundList->GetMenu()->SetCurrentlyHighlightedItem(i);
					SoundList->ActivateItem(i);

					//set text
					SoundList->SetText(SoundNames[i]);

					//delete all soundscapes if we need to
					if (!bCurrentlyInSoundPanel) for (int i = 0; i < SoundNames.Count(); i++)
						delete[] SoundNames[i];

					return;
				}
			}
		}

		//delete all soundscapes if we need to
		if (!bCurrentlyInSoundPanel) for (int i = 0; i < SoundNames.Count(); i++)
			delete[] SoundNames[i];

		return;
	}
	else if (!Q_strcmp(pszCommand, SOUND_LIST_PLAY_COMMAND))
	{
		//get the sound
		char buf[512];
		m_SoundsList->GetText(buf, sizeof(buf));

		//stop the sound
		if (enginesound->IsSoundStillPlaying(m_iSongGuid))
		{
			enginesound->StopSoundByGuid(m_iSongGuid);
			m_iSongGuid = -1;
		}

		//precache and play the sound
		if (!enginesound->IsSoundPrecached(buf))
			enginesound->PrecacheSound(buf);

		enginesound->EmitAmbientSound(buf, 1, 100);
		m_iSongGuid = enginesound->GetGuidForLastSoundEmitted();
		return;
	}
	else if (!Q_strcmp(pszCommand, SOUND_LIST_STOP_COMMAND))
	{
		//stop the sound
		if (m_iSongGuid != -1 && enginesound->IsSoundStillPlaying(m_iSongGuid))
		{
			enginesound->StopSoundByGuid(m_iSongGuid);
			m_iSongGuid = -1;
		}

		return;
	}
	else if (!Q_strcmp(pszCommand, SOUND_LIST_INSERT_COMMAND))
	{
		//make not visible
		SetVisible(false);

		//stop the sound
		if (enginesound->IsSoundStillPlaying(m_iSongGuid))
		{
			enginesound->StopSoundByGuid(m_iSongGuid);
			m_iSongGuid = -1;
		}

		//get the sound
		char buf[512];

		if (bCurrentlyInSoundPanel)
			m_SoundsList->GetText(buf, sizeof(buf));
		else
			m_SoundscapesList->GetText(buf, sizeof(buf));

		//set the sound text
		g_SoundscapeMaker->SetSoundText(buf);
		return;
	}
	else if (!Q_strcmp(pszCommand, SOUND_LIST_RELOAD_COMMAND))
	{
		if (bCurrentlyInSoundPanel)
		{
			//clear everything for the combo box and reload it
			m_SoundsList->RemoveAll();
			InitalizeSounds();
		}
		else
		{
			//clear everything for the combo box and reload it
			m_SoundscapesList->RemoveAll();

			bool bPrev = g_bSSMHack;
			g_bSSMHack = true;

			//reload all the soundscape files
			enginesound->StopAllSounds(true);

			g_SoundscapeSystem.StartNewSoundscape(nullptr);
			g_SoundscapeSystem.RemoveAll();
			g_SoundscapeSystem.Init();

			g_bSSMHack = bPrev;

			//load all the temporary soundscapes
			CUtlVector<const char*> OtherSoundscapes;
			for (KeyValues* curr = g_SoundscapeMaker->GetPanelFile(); curr; curr = curr->GetNextKey())
			{
				if (curr == g_SoundscapeMaker->GetPanelSelected())
					continue;

				OtherSoundscapes.AddToTail(curr->GetName());
			}

			InitalizeSoundscapes(OtherSoundscapes);
		}

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Called on panel close
//-----------------------------------------------------------------------------
void CSoundListPanel::OnClose()
{
	OnCommand(SOUND_LIST_STOP_COMMAND);
	BaseClass::OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Initalizes the sounds list
//-----------------------------------------------------------------------------
void CSoundListPanel::InitalizeSounds()
{
	//get the sound array
	GetSoundNames();

	//add all the sounds
	for (int i = 0; i < g_SoundDirectories.Size(); i++)
		m_SoundsList->AddItem(g_SoundDirectories[i], nullptr);

	m_SoundsList->ActivateItem(0);
}

//-----------------------------------------------------------------------------
// Purpose: Initalizes the soundscape list
//-----------------------------------------------------------------------------
void CSoundListPanel::InitalizeSoundscapes(CUtlVector<const char*>& OtherSoundscapes)
{
	//remove everything
	m_SoundscapesList->RemoveAll();

	//add all the soundscapes
	for (int i = 0; i < g_SoundscapeSystem.m_soundscapes.Count(); i++)
		OtherSoundscapes.AddToTail(g_SoundscapeSystem.m_soundscapes[i]->GetName());

	OtherSoundscapes.Sort(VectorSortFunc);

	//quickly remove duplicatesd
	for (int i = 1; i < OtherSoundscapes.Count(); )
	{
		if (!Q_strcmp(OtherSoundscapes[i], OtherSoundscapes[i - 1]))
		{
			OtherSoundscapes.Remove(i);
			continue;
		}
		i++;
	}

	for (int i = 0; i < OtherSoundscapes.Size(); i++)
		m_SoundscapesList->AddItem(OtherSoundscapes[i], nullptr);

	m_SoundscapesList->ActivateItem(0);
}

//-----------------------------------------------------------------------------
// Purpose: Sets if this panel is currently the sound panel or soundscape 
//			selector panel.
//-----------------------------------------------------------------------------
void CSoundListPanel::SetIsUsingSoundPanel(bool bUsing)
{
	bCurrentlyInSoundPanel = bUsing;

	//disable stuff
	if (bUsing)
	{
		//set 'reload' text
		m_ReloadSounds->SetText("Reload Sounds");

		m_SoundscapesList->SetVisible(false);
		m_SoundsList->SetVisible(true);

		//enable the play button
		m_PlayButton->SetEnabled(true);
		m_StopSoundButton->SetEnabled(true);

		//set texts
		m_PlayButton->SetText("Play Sound");
		m_StopSoundButton->SetText("Stop Sound");
		m_InsertButton->SetText("Insert Sound");

		//set title
		SetTitle("Sounds List", true);
	}
	else
	{
		//set 'reload' text
		m_ReloadSounds->SetText("Reload Soundscapes");

		m_SoundscapesList->SetVisible(true);
		m_SoundsList->SetVisible(false);

		//disable the play button
		m_PlayButton->SetEnabled(false);
		m_StopSoundButton->SetEnabled(false);

		//set texts
		m_PlayButton->SetText("Play Soundscape");
		m_StopSoundButton->SetText("Stop Soundscape");
		m_InsertButton->SetText("Insert Soundscape");

		//set stuff
		SetTitle("Soundscape List", true);
	}
}

//static sound list instance
static CSoundListPanel* g_SoundPanel = nullptr;


//soundscape list


#define ADD_SOUNDSCAPE_COMMAND "AddSoundscape"
#define PASTE_FROM_CLIBOARD_COMMAND "PasteFromClipboard"
#define OPEN_CLIBOARD_COMMAND "OpenClipboard"


//soundscape list class
class CSoundscapeList : public vgui::Divider
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeList, vgui::Divider);

	//constructor
	CSoundscapeList(vgui::Panel* parent, const char* name, const char* text, int text_x_pos, int max, int width, int height);

	//menu item stuff
	virtual void AddButton(const char* name, const char* text, const char* command, vgui::Panel* parent, KeyValues* add, SoundscapeClipboardType type);
	virtual void Clear();

	//other
	virtual void OnMouseWheeled(int delta);
	virtual void OnMouseReleased(vgui::MouseCode code);

	virtual void OnCommand(const char* pszCommand);
	virtual void PaintBackground();

	virtual void OnKeyCodePressed(vgui::KeyCode code);

	//message funcs
	MESSAGE_FUNC_INT(ScrollBarMoved, "ScrollBarSliderMoved", position);

protected:
	friend class CSoundscapeMaker;

	//keyvalue list.
	KeyValues* m_Keyvalues = nullptr;

	//says "Soundscapes List"
	vgui::Label* m_pLabel;
	vgui::ScrollBar* m_pSideSlider;

	//menu
	vgui::Menu* menu;

	//menu button stuff
	CUtlVector<CSoundscapeButton*> m_MenuButtons;
	int m_iCurrentY;
	int m_iMax;
	int m_AmtAdded;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor for soundscape list panel
//-----------------------------------------------------------------------------
CSoundscapeList::CSoundscapeList(vgui::Panel* parent, const char* name, const char* text, int text_x_pos, int max, int width, int height)
	: BaseClass(parent, name)
{
	//create the text
	m_pLabel = new vgui::Label(this, "ListsText", text);
	m_pLabel->SetVisible(true);
	m_pLabel->SetBounds(text_x_pos, 2, 150, 20);

	//create the side slider
	m_pSideSlider = new vgui::ScrollBar(this, "ListsSlider", true);
	m_pSideSlider->SetBounds(width - 20, 0, 20, height - 2);
	m_pSideSlider->SetValue(0);
	m_pSideSlider->SetEnabled(false);
	m_pSideSlider->SetRange(0, 0);
	m_pSideSlider->SetButtonPressedScrollValue(1);
	m_pSideSlider->SetRangeWindow(0);
	m_pSideSlider->AddActionSignalTarget(this);

	m_iCurrentY = 22;
	m_iMax = max;
	m_Keyvalues = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: adds a button to the soundscape list
//-----------------------------------------------------------------------------
void CSoundscapeList::AddButton(const char* name, const char* text, const char* command, vgui::Panel* parent, KeyValues* add, SoundscapeClipboardType type)
{
	//create a new button
	CSoundscapeButton* button = new CSoundscapeButton(this, name, text, parent, command, add, type);
	button->SetBounds(5, m_iCurrentY, GetWide() - 30, 20);

	//increment current y
	m_iCurrentY = m_iCurrentY + 22;

	//add button to array
	m_MenuButtons.AddToTail(button);

	//if the count is more then m_iMax then set slider value
	if (m_MenuButtons.Count() > m_iMax)
	{
		int max = m_MenuButtons.Count() - m_iMax;

		m_pSideSlider->SetRange(0, max);
		m_pSideSlider->SetRangeWindow(1);
		m_pSideSlider->SetEnabled(true);
	}

	m_AmtAdded++;

	//check to see if we need to scroll down
	if (m_MenuButtons.Count() >= m_iMax)
		OnMouseWheeled(-1);
}

//-----------------------------------------------------------------------------
// Purpose: Clears everything for this list
//-----------------------------------------------------------------------------
void CSoundscapeList::Clear()
{
	//reset the slider
	m_pSideSlider->SetValue(0);
	m_pSideSlider->SetEnabled(false);
	m_pSideSlider->SetRange(0, 0);
	m_pSideSlider->SetButtonPressedScrollValue(1);
	m_pSideSlider->SetRangeWindow(0);

	//delete and clear the buttons
	for (int i = 0; i < m_MenuButtons.Count(); i++)
		m_MenuButtons[i]->DeletePanel();

	m_MenuButtons.RemoveAll();

	//reset current y
	m_iCurrentY = 22;

	m_AmtAdded = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a mouse is wheeled
//-----------------------------------------------------------------------------
void CSoundscapeList::OnMouseWheeled(int delta)
{
	//check for scroll down
	if (delta == -1)
		m_pSideSlider->SetValue(m_pSideSlider->GetValue() + 1);

	//check for scroll up
	else if (delta == 1)
		m_pSideSlider->SetValue(m_pSideSlider->GetValue() - 1);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a mouse code is released
//-----------------------------------------------------------------------------
void CSoundscapeList::OnMouseReleased(vgui::MouseCode code)
{
	if (code != vgui::MouseCode::MOUSE_RIGHT)
		return;

	//get cursor pos
	int x, y;
	vgui::surface()->SurfaceGetCursorPos(x, y);

	//create menu
	menu = new vgui::Menu(this, "Menu");
	menu->AddMenuItem("AddSoundscape", "Add Soundscape", ADD_SOUNDSCAPE_COMMAND, this);

	//check clipboard item
	if (CurrClipboardName.Count() > 0)
	{
		menu->AddSeparator();
		menu->AddMenuItem("PasteFromClipboard", "Paste", PASTE_FROM_CLIBOARD_COMMAND, this);
		menu->AddMenuItem("OpenClipboard", "Open Clipboard", OPEN_CLIBOARD_COMMAND, this);
	}

	menu->SetBounds(x, y, 200, 50);
	menu->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeList::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, ADD_SOUNDSCAPE_COMMAND))
	{
		const char* name = CFmtStr("New Soundscape %d", m_AmtAdded);

		//add to keyvalues file
		KeyValues* kv = new KeyValues(name);
		KeyValues* tmp = m_Keyvalues;
		KeyValues* tmp2 = tmp;

		AddButton(name, name, name, GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeName);

		//get last subkey
		while (tmp != nullptr)
		{
			tmp2 = tmp;
			tmp = tmp->GetNextTrueSubKey();
		}

		//add to last subkey
		tmp2->SetNextKey(kv);

		GetParent()->OnCommand(name);
		return;
	}
	else if (!Q_strcmp(pszCommand, PASTE_FROM_CLIBOARD_COMMAND))
	{
		int index = CurrClipboardName.Count() - 1;

		const char* name = CFmtStr("%s - (Copy %d)", CurrClipboardName[index]->GetName(), m_AmtAdded);

		//add to keyvalues file
		KeyValues* kv = new KeyValues(name);
		CurrClipboardName[index]->CopySubkeys(kv);

		KeyValues* tmp = m_Keyvalues;
		KeyValues* tmp2 = tmp;

		AddButton(name, name, name, GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeName);

		//get last subkey
		while (tmp != nullptr)
		{
			tmp2 = tmp;
			tmp = tmp->GetNextTrueSubKey();
		}

		//add to last subkey
		tmp2->SetNextKey(kv);

		GetParent()->OnCommand(name);
		return;
	}
	else if (!Q_strcmp(pszCommand, OPEN_CLIBOARD_COMMAND))
	{
		if (g_SoundscapeClipboard)
			g_SoundscapeClipboard->DeletePanel();

		g_SoundscapeClipboard = new CSoundscapeClipboard(SoundscapeClipboardType::Type_SoundscapeName);
		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Paints the background
//-----------------------------------------------------------------------------
void CSoundscapeList::PaintBackground()
{
	//colors
	static Color EnabledColor = Color(100, 100, 100, 200);
	static Color DisabledColor = Color(60, 60, 60, 200);

	//if m_KeyValues then paint the default color
	if (m_Keyvalues)
		SetBgColor(EnabledColor);
	else
		SetBgColor(DisabledColor);

	BaseClass::PaintBackground();
}

//-----------------------------------------------------------------------------
// Purpose: Called on keyboard code pressed
//-----------------------------------------------------------------------------
void CSoundscapeList::OnKeyCodePressed(vgui::KeyCode code)
{
	//check for arrow
	if (code == KEY_UP)
	{
		//find selected item
		for (int i = 0; i < m_MenuButtons.Count(); i++)
		{
			if (m_MenuButtons[i]->m_bIsSelected)
			{
				//check for size and to see if we can select item
				if (i - 1 < 0)
					return;

				//select that item
				GetParent()->OnCommand(m_MenuButtons[i - 1]->GetCommand()->GetString("command"));
				return;
			}
		}
	}

	//check for arrow
	if (code == KEY_DOWN)
	{
		//find selected item
		for (int i = 0; i < m_MenuButtons.Count(); i++)
		{
			if (m_MenuButtons[i]->m_bIsSelected)
			{
				//check for size and to see if we can select item
				if (i + 1 >= m_MenuButtons.Count())
					return;

				//select that item
				GetParent()->OnCommand(m_MenuButtons[i + 1]->GetCommand()->GetString("command"));
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on scroll bar moved
//-----------------------------------------------------------------------------
void CSoundscapeList::ScrollBarMoved(int delta)
{
	int position = m_pSideSlider->GetValue();

	//move everything down (if needed)
	for (int i = 0; i < m_MenuButtons.Count(); i++)
	{
		//make not visible if i < position
		if (i < position)
		{
			m_MenuButtons[i]->SetVisible(false);
			continue;
		}

		m_MenuButtons[i]->SetPos(5, 22 * ((i - position) + 1));
		m_MenuButtons[i]->SetVisible(true);
	}
}




//soundscape data list


#define NEW_PLAYLOOPING_COMMAND "NewLooping"
#define NEW_SOUNDSCAPE_COMMAND "NewSoundscape"
#define NEW_RANDOM_COMMAND "NewRandom"


class CSoundscapeDataList : public CSoundscapeList
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeDataList, CSoundscapeList);

	CSoundscapeDataList(vgui::Panel* parent, const char* name, const char* text, int text_x_pos, int max, int width, int height)
		: CSoundscapeList(parent, name, text, text_x_pos, max, width, height)
	{}

	//override right click functionality
	virtual void OnMouseReleased(vgui::MouseCode code);

	void OnCommand(const char* pszCommand);

private:
	friend class CSoundscapeMaker;
};


//-----------------------------------------------------------------------------
// Purpose: Called when a mouse code is released
//-----------------------------------------------------------------------------
void CSoundscapeDataList::OnMouseReleased(vgui::MouseCode code)
{
	//if no soundscape is selected or mouse code != right then return
	if (code != vgui::MouseCode::MOUSE_RIGHT || !m_Keyvalues)
		return;

	//get cursor pos
	int x, y;
	vgui::surface()->SurfaceGetCursorPos(x, y);

	//create menu
	menu = new vgui::Menu(this, "Menu");
	menu->AddMenuItem("AddLooping", "Add Looping Sound", NEW_PLAYLOOPING_COMMAND, this);
	menu->AddMenuItem("AddSoundscape", "Add Soundscape", NEW_SOUNDSCAPE_COMMAND, this);
	menu->AddMenuItem("AddSoundscape", "Add Random Sounds", NEW_RANDOM_COMMAND, this);

	//add clipboard thing
	if (CurrClipboardData.Count() > 0)
	{
		menu->AddSeparator();
		menu->AddMenuItem("PasteFromClipboard", "Paste", PASTE_FROM_CLIBOARD_COMMAND, this);
		menu->AddMenuItem("OpenClipboard", "Open Clipboard", OPEN_CLIBOARD_COMMAND, this);
	}

	menu->SetBounds(x, y, 200, 50);
	menu->SetVisible(true);
}


//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeDataList::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, NEW_PLAYLOOPING_COMMAND))
	{
		int LoopingNum = 0;
		FOR_EACH_TRUE_SUBKEY(m_Keyvalues, data)
		{
			//store data name
			const char* name = data->GetName();

			//increment variables based on name
			if (!Q_strcasecmp(name, "playlooping"))
				LoopingNum++;
		}

		//add the keyvalues
		KeyValues* kv = new KeyValues("playlooping");
		kv->SetFloat("volume", 1);
		kv->SetInt("pitch", 100);

		//add the keyvalue to both this and the keyvalues
		AddButton("playlooping", "playlooping", CFmtStr("$playlooping%d", LoopingNum + 1), GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeData);

		m_Keyvalues->AddSubKey(kv);

		GetParent()->OnCommand(CFmtStr("$playlooping%d", LoopingNum + 1));

		return;
	}
	else if (!Q_strcmp(pszCommand, NEW_SOUNDSCAPE_COMMAND))
	{
		int SoundscapeNum = 0;
		FOR_EACH_TRUE_SUBKEY(m_Keyvalues, data)
		{
			//store data name
			const char* name = data->GetName();

			//increment variables based on name
			if (!Q_strcasecmp(name, "playsoundscape"))
				SoundscapeNum++;
		}

		//add the keyvalues
		KeyValues* kv = new KeyValues("playsoundscape");
		kv->SetFloat("volume", 1);

		AddButton("playsoundscape", "playsoundscape", CFmtStr("$playsoundscape%d", SoundscapeNum + 1), GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeData);

		//add the keyvalue to both this and the keyvalues
		m_Keyvalues->AddSubKey(kv);

		GetParent()->OnCommand(CFmtStr("$playsoundscape%d", SoundscapeNum + 1));

		return;
	}
	else if (!Q_strcmp(pszCommand, NEW_RANDOM_COMMAND))
	{
		int RandomNum = 0;
		FOR_EACH_TRUE_SUBKEY(m_Keyvalues, data)
		{
			//store data name
			const char* name = data->GetName();

			//increment variables based on name
			if (!Q_strcasecmp(name, "playrandom"))
				RandomNum++;
		}

		//add the keyvalues
		KeyValues* kv = new KeyValues("playrandom");


		kv->SetString("volume", "0.5,0.8");
		kv->SetInt("pitch", 100);
		kv->SetString("time", "10,20");

		AddButton("playrandom", "playrandom", CFmtStr("$playrandom%d", RandomNum + 1), GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeData);

		//make rndwave subkey
		KeyValues* rndwave = new KeyValues("rndwave");
		kv->AddSubKey(rndwave);

		//add the keyvalue to both this and the keyvalues
		m_Keyvalues->AddSubKey(kv);

		//make the parent show the new item
		GetParent()->OnCommand(CFmtStr("$playrandom%d", RandomNum + 1));

		return;
	}
	else if (!Q_strcmp(pszCommand, PASTE_FROM_CLIBOARD_COMMAND))
	{
		int index = CurrClipboardData.Count() - 1;

		const char* type = CurrClipboardData[index]->GetName();

		//get num of that item
		int NumItem = 0;
		FOR_EACH_TRUE_SUBKEY(m_Keyvalues, data)
		{
			//store data name
			const char* name = data->GetName();

			//increment variables based on name
			if (!Q_strcasecmp(name, type))
				NumItem++;
		}

		//add the keyvalues
		KeyValues* kv = new KeyValues(type);
		CurrClipboardData[index]->CopySubkeys(kv);

		AddButton(type, type, CFmtStr("$%s%d", type, NumItem + 1), GetParent(), kv, SoundscapeClipboardType::Type_SoundscapeData);

		//add the keyvalue to both this and the keyvalues
		m_Keyvalues->AddSubKey(kv);

		//make the parent show the new item
		GetParent()->OnCommand(CFmtStr("$%s%d", type, NumItem + 1));
		return;
	}
	else if (!Q_strcmp(pszCommand, OPEN_CLIBOARD_COMMAND))
	{
		if (g_SoundscapeClipboard)
			g_SoundscapeClipboard->DeletePanel();

		g_SoundscapeClipboard = new CSoundscapeClipboard(SoundscapeClipboardType::Type_SoundscapeData);
		return;
	}

	BaseClass::OnCommand(pszCommand);
}


//soundscape rndwave data list


#define NEW_RNDWAVE_WAVE_COMMAND "NewRNDWave"


class CSoundscapeRndwaveList : public CSoundscapeList
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeDataList, CSoundscapeList);

	CSoundscapeRndwaveList(vgui::Panel* parent, const char* name, const char* text, int text_x_pos, int max, int width, int height)
		: CSoundscapeList(parent, name, text, text_x_pos, max, width, height)
	{}

	//override right click functionality
	virtual void OnMouseReleased(vgui::MouseCode code);

	void OnCommand(const char* pszCommand);

private:
	friend class CSoundscapeMaker;
};


//-----------------------------------------------------------------------------
// Purpose: Called when a mouse code is released
//-----------------------------------------------------------------------------
void CSoundscapeRndwaveList::OnMouseReleased(vgui::MouseCode code)
{
	//if no soundscape is selected or mouse code != right then return
	if (code != vgui::MouseCode::MOUSE_RIGHT || !m_Keyvalues)
		return;

	//get cursor pos
	int x, y;
	vgui::surface()->SurfaceGetCursorPos(x, y);

	//create menu
	menu = new vgui::Menu(this, "Menu");
	menu->AddMenuItem("AddRandom", "Add Random Wave", NEW_RNDWAVE_WAVE_COMMAND, this);

	//add clipboard thing
	if (CurrClipboardRandom.Count() > 0)
	{
		menu->AddSeparator();
		menu->AddMenuItem("PasteFromClipboard", "Paste", PASTE_FROM_CLIBOARD_COMMAND, this);
		menu->AddMenuItem("OpenClipboard", "Open Clipboard", OPEN_CLIBOARD_COMMAND, this);
	}

	menu->SetBounds(x, y, 200, 50);
	menu->SetVisible(true);
}


//-----------------------------------------------------------------------------
// Purpose: Called on command
//-----------------------------------------------------------------------------
void CSoundscapeRndwaveList::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, NEW_RNDWAVE_WAVE_COMMAND) && m_Keyvalues)
	{
		//get number of keyvalues
		int num = 0;

		FOR_EACH_VALUE(m_Keyvalues, kv)
			num++;

		KeyValues* add = new KeyValues("wave");
		add->SetString(nullptr, "");

		//add keyvalues and button
		AddButton("Rndwave", "", CFmtStr("$rndwave%d", num + 1), GetParent(), add, SoundscapeClipboardType::Type_SoundscapeRandomWave);

		m_Keyvalues->AddSubKey(add);

		//forward command to parent
		GetParent()->OnCommand(CFmtStr("$rndwave%d", num + 1));

		return;
	}

	else if (!Q_strcmp(pszCommand, PASTE_FROM_CLIBOARD_COMMAND))
	{
		//get number of keyvalues
		int num = 0;

		FOR_EACH_VALUE(m_Keyvalues, kv)
			num++;

		int index = CurrClipboardRandom.Count() - 1;

		const char* text = CurrClipboardRandom[index]->GetString();

		KeyValues* add = new KeyValues("wave");
		add->SetString(nullptr, text);

		//get last / or \ and make the string be that + 1
		char* fslash = Q_strrchr(text, '/');
		char* bslash = Q_strrchr(text, '\\');

		if (fslash > bslash)
			text = fslash + 1;
		else if (bslash > fslash)
			text = bslash + 1;

		//add keyvalues and button
		AddButton("Rndwave", text, CFmtStr("$rndwave%d", num + 1), GetParent(), add, SoundscapeClipboardType::Type_SoundscapeRandomWave);

		m_Keyvalues->AddSubKey(add);

		//forward command to parent
		GetParent()->OnCommand(CFmtStr("$rndwave%d", num + 1));
		return;
	}
	else if (!Q_strcmp(pszCommand, OPEN_CLIBOARD_COMMAND))
	{
		if (g_SoundscapeClipboard)
			g_SoundscapeClipboard->DeletePanel();

		g_SoundscapeClipboard = new CSoundscapeClipboard(SoundscapeClipboardType::Type_SoundscapeRandomWave);
		return;
	}

	BaseClass::OnCommand(pszCommand);
}


//soundscape panel


#define SOUNDSCAPE_PANEL_WIDTH 760
#define SOUNDSCAPE_PANEL_HEIGHT 630

#define NEW_BUTTON_COMMAND "$NewSoundscape"
#define SAVE_BUTTON_COMMAND "$SaveSoundscape"
#define LOAD_BUTTON_COMMAND "$LoadSoundscape"
#define OPTIONS_BUTTON_COMMAND "$ShowOptions"
#define EDIT_BUTTON_COMMAND "$Edit"
#define RESET_BUTTON_COMMAND "$ResetSoundscapes"
#define SOUNDS_LIST_BUTTON_COMMAND "$ShowSoundsList"
#define PLAY_SOUNDSCAPE_COMMAND "$PlaySoundscape"
#define RESET_SOUNDSCAPE_BUTTON_COMMAND "$ResetSoundscape"
#define DELETE_CURRENT_ITEM_COMMAND "$DeleteItem"

//static bool to determin if the soundscape panel should show or not
bool g_ShowSoundscapePanel = false;
bool g_IsPlayingSoundscape = false;

//soundscape maker panel
class CSoundscapeMaker : public vgui::Frame, CAutoGameSystem
{
public:
	DECLARE_CLASS_SIMPLE(CSoundscapeMaker, vgui::Frame)

	CSoundscapeMaker(vgui::VPANEL parent);

	//tick functions
	void OnTick();

	//other functions
	void OnClose();
	void OnCommand(const char* pszCommand);
	void Paste(SoundscapeClipboardType type);

	void PlaySelectedSoundscape();
	void LoadFile(KeyValues* file);

	void OnKeyCodePressed(vgui::KeyCode code);

	void SetSoundText(const char* text);

	//to play the soundscape on map spawn
	void LevelInitPostEntity();

	//sets the keyvalue file
	void Set(const char* buffer);

	//message pointer funcs
	MESSAGE_FUNC_CHARPTR(OnFileSelected, "FileSelected", fullpath);
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", data);

	~CSoundscapeMaker();

public:

	//the soundscape keyvalues file
	KeyValues* m_KeyValues = nullptr;

private:
	void CreateEverything();

private:
	//lists all the soundscapes
	CSoundscapeList* m_SoundscapesList;
	CSoundscapeDataList* m_pDataList;
	CSoundscapeRndwaveList* m_pSoundList;

	//buttons
	vgui::Button* m_ButtonNew = nullptr;
	vgui::Button* m_ButtonSave = nullptr;
	vgui::Button* m_ButtonLoad = nullptr;
	vgui::Button* m_ButtonOptions = nullptr;
	vgui::Button* m_EditButton = nullptr;

	//file load and save dialogs
	vgui::FileOpenDialog* m_FileSave = nullptr;
	vgui::FileOpenDialog* m_FileLoad = nullptr;
	bool m_bWasFileLoad = false;

	//text entry for name
	vgui::TextEntry* m_TextEntryName;

	//combo box for dsp effects
	vgui::ComboBox* m_DspEffects;
	vgui::ComboBox* m_SoundLevels;

	//sound data text entry
	vgui::TextEntry* m_TimeTextEntry;
	vgui::TextEntry* m_VolumeTextEntry;
	vgui::TextEntry* m_PitchTextEntry;
	vgui::TextEntry* m_PositionTextEntry;
	vgui::TextEntry* m_SoundNameTextEntry;

	//play sound button
	vgui::Button* m_SoundNamePlay;

	//play/reset soundscape buttons
	vgui::CheckButton* m_PlaySoundscapeButton;
	vgui::Button* m_ResetSoundscapeButton;
	vgui::Button* m_DeleteCurrentButton;

	//current selected soundscape
	CSoundscapeButton* m_pCurrentSelected = nullptr;

public:
	KeyValues* m_kvCurrSelected = nullptr;

private:
	KeyValues* m_kvCurrSound = nullptr;
	KeyValues* m_kvCurrRndwave = nullptr;

	int m_iCurrRndWave = 0;

	//currently in non randomwave thing
	SoundscapeMode m_iSoundscapeMode = SoundscapeMode::Mode_Random;

	//temporary added soundscapes
	CUtlVector<KeyValues*> m_TmpAddedSoundscapes;
};

//user message hook
void _SoundscapeMaker_Recieve(bf_read& bf);

//-----------------------------------------------------------------------------
// Purpose: Constructor for soundscape maker panel
//-----------------------------------------------------------------------------
CSoundscapeMaker::CSoundscapeMaker(vgui::VPANEL parent)
	: BaseClass(nullptr, "SoundscapeMaker")
{
	static bool bRegistered = false;
	if (!bRegistered)
	{
		usermessages->HookMessage("SoundscapeMaker_Recieve", _SoundscapeMaker_Recieve);
		bRegistered = true;
	}

	//set variables
	m_pCurrentSelected = nullptr;

	SetParent(parent);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);

	SetProportional(false);
	SetTitleBarVisible(true);
	SetMinimizeButtonVisible(false);
	SetMaximizeButtonVisible(false);
	SetCloseButtonVisible(true);
	SetSizeable(false);
	SetMoveable(true);
	SetVisible(g_ShowSoundscapePanel);

	int ScreenWide, ScreenTall;
	vgui::surface()->GetScreenSize(ScreenWide, ScreenTall);

	SetTitle("Soundscape Maker (New File)", true);
	SetSize(SOUNDSCAPE_PANEL_WIDTH, SOUNDSCAPE_PANEL_HEIGHT);
	SetPos((ScreenWide - SOUNDSCAPE_PANEL_WIDTH) / 2, (ScreenTall - SOUNDSCAPE_PANEL_HEIGHT) / 2);



	//add a tick signal for every 50 ms
	vgui::ivgui()->AddTickSignal(GetVPanel(), 50);

	CreateEverything();
}

//-----------------------------------------------------------------------------
// Purpose: Creates everything for this panel
//-----------------------------------------------------------------------------
void CSoundscapeMaker::CreateEverything()
{
	//create the divider that will be the outline for the inside of the panel
	vgui::Divider* PanelOutline = new vgui::Divider(this, "InsideOutline");
	PanelOutline->SetEnabled(false);
	PanelOutline->SetBounds(5, 25, SOUNDSCAPE_PANEL_WIDTH - 10, SOUNDSCAPE_PANEL_HEIGHT - 62);

	//create the buttons
		//create the buttons
	m_ButtonNew = new vgui::Button(this, "NewButton", "New Soundscape File");
	m_ButtonNew->SetVisible(true);
	m_ButtonNew->SetBounds(7, 600, 145, 25);
	m_ButtonNew->SetCommand(NEW_BUTTON_COMMAND);
	m_ButtonNew->SetDepressedSound("ui/buttonclickrelease.wav");

	m_ButtonSave = new vgui::Button(this, "SaveButton", "Save Soundscapes");
	m_ButtonSave->SetVisible(true);
	m_ButtonSave->SetBounds(157, 600, 145, 25);
	m_ButtonSave->SetCommand(SAVE_BUTTON_COMMAND);
	m_ButtonSave->SetDepressedSound("ui/buttonclickrelease.wav");

	m_ButtonLoad = new vgui::Button(this, "LoadButton", "Load Soundscapes");
	m_ButtonLoad->SetVisible(true);
	m_ButtonLoad->SetBounds(307, 600, 145, 25);
	m_ButtonLoad->SetCommand(LOAD_BUTTON_COMMAND);
	m_ButtonLoad->SetDepressedSound("ui/buttonclickrelease.wav");

	m_ButtonOptions = new vgui::Button(this, "OptionsButton", "Show Options Panel");
	m_ButtonOptions->SetVisible(true);
	m_ButtonOptions->SetBounds(457, 600, 145, 25);
	m_ButtonOptions->SetCommand(OPTIONS_BUTTON_COMMAND);
	m_ButtonOptions->SetDepressedSound("ui/buttonclickrelease.wav");

	m_EditButton = new vgui::Button(this, "EditButton", "Show Text Editor");
	m_EditButton->SetVisible(true);
	m_EditButton->SetBounds(607, 600, 145, 25);
	m_EditButton->SetCommand(EDIT_BUTTON_COMMAND);
	m_EditButton->SetDepressedSound("ui/buttonclickrelease.wav");

	//create the soundscapes menu
	m_SoundscapesList = new CSoundscapeList(this, "SoundscapesList", "Soundscapes:", 90, 22, 300, 550);
	m_SoundscapesList->SetBounds(15, 35, 300, 550);
	m_SoundscapesList->SetVisible(true);

	//create data list
	m_pDataList = new CSoundscapeDataList(this, "SoudscapeDataList", "Soundscape Data:", 35, 10, 200, 310);
	m_pDataList->SetBounds(327, 275, 200, 310);
	m_pDataList->SetVisible(true);

	//create sound list
	m_pSoundList = new CSoundscapeRndwaveList(this, "SoudscapeDataList", "Random Sounds:", 40, 10, 200, 310);
	m_pSoundList->SetBounds(542, 275, 200, 310);
	m_pSoundList->SetVisible(true);

	//name text entry
	m_TextEntryName = new vgui::TextEntry(this, "NameTextEntry");
	m_TextEntryName->SetEnabled(false);
	m_TextEntryName->SetBounds(325, 40, 295, 20);
	m_TextEntryName->SetMaximumCharCount(256);

	//dsp effects combo box
	m_DspEffects = new vgui::ComboBox(this, "DspEffects", 20, false);
	m_DspEffects->SetEnabled(false);
	m_DspEffects->SetBounds(325, 65, 295, 20);
	m_DspEffects->SetText("");
	m_DspEffects->AddActionSignalTarget(this);

	for (int i = 0; i < sizeof(g_DspEffects) / sizeof(g_DspEffects[i]); i++)
		m_DspEffects->AddItem(g_DspEffects[i], nullptr);

	//time text entry
	m_TimeTextEntry = new vgui::TextEntry(this, "TimeTextEntry");
	m_TimeTextEntry->SetBounds(325, 90, 295, 20);
	m_TimeTextEntry->SetEnabled(false);
	m_TimeTextEntry->SetVisible(true);

	//volume text entry
	m_VolumeTextEntry = new vgui::TextEntry(this, "VolumeTextEntry");
	m_VolumeTextEntry->SetBounds(325, 115, 295, 20);
	m_VolumeTextEntry->SetEnabled(false);
	m_VolumeTextEntry->SetVisible(true);

	//pitch text entry
	m_PitchTextEntry = new vgui::TextEntry(this, "PitchTextEntry");
	m_PitchTextEntry->SetBounds(325, 140, 295, 20);
	m_PitchTextEntry->SetEnabled(false);
	m_PitchTextEntry->SetVisible(true);

	//position text entry
	m_PositionTextEntry = new vgui::TextEntry(this, "PositionTextEntry");
	m_PositionTextEntry->SetBounds(325, 165, 295, 20);
	m_PositionTextEntry->SetEnabled(false);
	m_PositionTextEntry->SetVisible(true);

	//sound levels
	m_SoundLevels = new vgui::ComboBox(this, "SoundLevels", 20, false);
	m_SoundLevels->SetEnabled(false);
	m_SoundLevels->SetBounds(325, 190, 295, 20);
	m_SoundLevels->SetText("");
	m_SoundLevels->AddActionSignalTarget(this);

	for (int i = 0; i < sizeof(g_SoundLevels) / sizeof(g_SoundLevels[i]); i++)
		m_SoundLevels->AddItem(g_SoundLevels[i], nullptr);

	//sound name
	m_SoundNameTextEntry = new vgui::TextEntry(this, "SoundName");
	m_SoundNameTextEntry->SetBounds(325, 215, 215, 20);
	m_SoundNameTextEntry->SetEnabled(false);
	m_SoundNameTextEntry->SetVisible(true);

	//sound list button
	m_SoundNamePlay = new vgui::Button(this, "SoundPlayButton", "Sounds List");
	m_SoundNamePlay->SetBounds(545, 215, 75, 20);
	m_SoundNamePlay->SetCommand(SOUNDS_LIST_BUTTON_COMMAND);
	m_SoundNamePlay->SetEnabled(false);

	//starts the soundscape
	m_PlaySoundscapeButton = new vgui::CheckButton(this, "PlaySoundscape", "Play Soundscape");
	m_PlaySoundscapeButton->SetBounds(330, 243, 125, 20);
	m_PlaySoundscapeButton->SetCommand(PLAY_SOUNDSCAPE_COMMAND);
	m_PlaySoundscapeButton->SetEnabled(false);
	m_PlaySoundscapeButton->SetSelected(false);

	//reset soundscape button
	m_ResetSoundscapeButton = new vgui::Button(this, "ResetSoundscape", "Restart Soundscape");
	m_ResetSoundscapeButton->SetBounds(465, 243, 125, 20);
	m_ResetSoundscapeButton->SetCommand(RESET_SOUNDSCAPE_BUTTON_COMMAND);
	m_ResetSoundscapeButton->SetEnabled(false);

	//delete this item
	m_DeleteCurrentButton = new vgui::Button(this, "DeleteItem", "Delete Current Item");
	m_DeleteCurrentButton->SetBounds(595, 243, 135, 20);
	m_DeleteCurrentButton->SetCommand(DELETE_CURRENT_ITEM_COMMAND);
	m_DeleteCurrentButton->SetEnabled(false);

	//create the soundscape name text
	vgui::Label* NameLabel = new vgui::Label(this, "NameLabel", "Soundscape Name");
	NameLabel->SetBounds(635, 40, 125, 20);

	//create the soundscape dsp text
	vgui::Label* DspLabel = new vgui::Label(this, "DspLabel", "Soundscape Dsp");
	DspLabel->SetBounds(635, 65, 125, 20);

	//create the soundscape time text
	vgui::Label* TimeLabel = new vgui::Label(this, "TimeLabel", "Sound Time");
	TimeLabel->SetBounds(635, 90, 125, 20);

	//create the soundscape volumn text
	vgui::Label* VolumeLabel = new vgui::Label(this, "VolumeLabel", "Sound Volume");
	VolumeLabel->SetBounds(635, 115, 125, 20);

	//create the soundscape pitch text
	vgui::Label* PitchLabel = new vgui::Label(this, "PitchLabel", "Sound Pitch");
	PitchLabel->SetBounds(635, 140, 125, 20);

	//create the soundscape position text
	vgui::Label* PositionLabel = new vgui::Label(this, "PositionLabel", "Sound Position");
	PositionLabel->SetBounds(635, 165, 125, 20);

	//create the soundscape sound level text
	vgui::Label* SoundLevelLabel = new vgui::Label(this, "SoundLevelLabel", "Sound Level");
	SoundLevelLabel->SetBounds(635, 190, 125, 20);

	//create the soundscape sound name text
	vgui::Label* SoundName = new vgui::Label(this, "SoundName", "Sound Name");
	SoundName->SetBounds(635, 215, 125, 20);

	//create the soundscape keyvalues and load it
	m_KeyValues = new KeyValues("Empty Soundscape");

	LoadFile(m_KeyValues);
}

//-----------------------------------------------------------------------------
// Purpose: Called every tick for the soundscape maker
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnTick()
{
	//set the visibility
	static bool bPrevVisible = g_ShowSoundscapePanel;
	if (g_ShowSoundscapePanel != bPrevVisible)
		SetVisible(g_ShowSoundscapePanel);

	//set the old visibility
	bPrevVisible = g_ShowSoundscapePanel;
}

//-----------------------------------------------------------------------------
// Purpose: Called when the close button is pressed
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnClose()
{
	//hide the other panels
	g_SoundPanel->OnClose();
	g_SettingsPanel->OnClose();
	g_SoundscapeTextPanel->OnClose();

	g_ShowSoundscapePanel = false;
}

//-----------------------------------------------------------------------------
// Purpose: Play the selected soundscape
//-----------------------------------------------------------------------------
void CSoundscapeMaker::PlaySelectedSoundscape()
{
	g_IsPlayingSoundscape = m_PlaySoundscapeButton->IsSelected();
	if (!g_IsPlayingSoundscape)
		return;

	//set debug stuff
	SoundscapePrint(Color(255, 255, 255, 255), "\n\n\n=============== %s %s =================\n\n", m_kvCurrSelected ? "Starting Soundscape: " : "Stopping Current Soundscape", m_kvCurrSelected ? m_kvCurrSelected->GetName() : "");
	g_SoundscapeDebugPanel->m_PanelSoundscapesFadingIn->Clear();

	g_bSSMHack = true;

	//remove all the temporary soundscapes from the soundscape system
	for (int i = 0; i < m_TmpAddedSoundscapes.Count(); i++)
	{
		for (int j = 0; j < g_SoundscapeSystem.m_soundscapes.Count(); j++)
		{
			if (g_SoundscapeSystem.m_soundscapes[j] == m_TmpAddedSoundscapes[i])
			{
				g_SoundscapeSystem.m_soundscapes.Remove(j);
				break;
			}
		}
	}

	m_TmpAddedSoundscapes.RemoveAll();

	//change audio params position
	g_SoundscapeSystem.m_params.localBits = 0x7f;
	for (int i = 0; i < MAX_SOUNDSCAPES - 1; i++)
		g_SoundscapeSystem.m_params.localSound.Set(i, g_SoundscapePositions[i]);


	//if m_kvCurrSelected then add all the "playsoundscape" soundscape keyvalues
	//into the g_SoundscapeSystem.m_soundscapes array
	if (m_kvCurrSelected)
	{
		CUtlVector<const char*> SoundscapeNames;
		FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, subkey)
		{
			//look for playsoundscape file
			if (!Q_strcasecmp(subkey->GetName(), "playsoundscape"))
			{
				const char* name = subkey->GetString("name", nullptr);
				if (!name || !name[0] || SoundscapeNames.Find(name) != SoundscapeNames.InvalidIndex())
					continue;

				SoundscapeNames.AddToTail(name);
			}
		}

		//now look for each keyvalue
		for (int i = 0; i < SoundscapeNames.Count(); i++)
		{
			for (KeyValues* subkey = m_KeyValues; subkey != nullptr; subkey = subkey->GetNextTrueSubKey())
			{
				//look for playsoundscape file
				if (!Q_strcmp(subkey->GetName(), SoundscapeNames[i]))
				{
					//add it to the soundscape system
					m_TmpAddedSoundscapes.AddToTail(subkey);
					g_SoundscapeSystem.m_soundscapes.AddToTail(subkey);
				}
			}
		}
	}

	//stop all sounds
	enginesound->StopAllSounds(true);

	//stop the current soundscape and start a new soundscape
	g_SoundscapeSystem.StartNewSoundscape(nullptr);
	g_SoundscapeSystem.StartNewSoundscape(m_kvCurrSelected);

	//start debug graphs
	g_SoundscapeDebugPanel->m_PanelSoundscapesFadingIn->Start();

	g_bSSMHack = false;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a button or something else gets pressed
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnCommand(const char* pszCommand)
{
	//check for close command first
	if (!Q_strcmp(pszCommand, "Close"))
	{
		BaseClass::OnCommand(pszCommand);
		return;
	}

	//check for the save button command
	else if (!Q_strcmp(pszCommand, SAVE_BUTTON_COMMAND))
	{
		//initalize the file save dialog
		if (!m_FileSave)
		{
			//get the current game directory
			char buf[512];
			filesystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));

			//create the save dialog
			m_FileSave = new vgui::FileOpenDialog(this, "Save Soundscape File", false);
			m_FileSave->AddFilter("*.txt", "Soundscape Text File", true);
			m_FileSave->AddFilter("*.*", "All Files (*.*)", false);
			m_FileSave->SetStartDirectory(buf);
			m_FileSave->AddActionSignalTarget(this);
		}

		//show the dialog
		m_FileSave->DoModal(false);
		m_FileSave->Activate();

		//file wasnt loadad
		m_bWasFileLoad = false;

		return;
	}

	//check for load button command
	else if (!Q_strcmp(pszCommand, LOAD_BUTTON_COMMAND))
	{
		//initalize the file save dialog
		if (!m_FileLoad)
		{
			//get the current game directory
			char buf[512];
			filesystem->RelativePathToFullPath("scripts", "MOD", buf, sizeof(buf));

			//create the load dialog
			m_FileLoad = new vgui::FileOpenDialog(this, "Load Soundscape File", true);
			m_FileLoad->AddFilter("*.txt", "Soundscape Text File", true);
			m_FileLoad->AddFilter("*.*", "All Files (*.*)", false);
			m_FileLoad->SetStartDirectory(buf);
			m_FileLoad->AddActionSignalTarget(this);
		}

		//show the file load dialog
		m_FileLoad->DoModal(false);
		m_FileLoad->Activate();

		//file was loadad
		m_bWasFileLoad = true;

		return;
	}

	//check for options panel button
	else if (!Q_strcmp(pszCommand, OPTIONS_BUTTON_COMMAND))
	{
		g_SettingsPanel->SetVisible(true);
		g_SettingsPanel->MoveToFront();
		g_SettingsPanel->RequestFocus();
		return;
	}

	//check for edit panel button
	else if (!Q_strcmp(pszCommand, EDIT_BUTTON_COMMAND))
	{
		g_SoundscapeTextPanel->SetVisible(true);
		g_SoundscapeTextPanel->MoveToFront();
		g_SoundscapeTextPanel->RequestFocus();
		g_SoundscapeTextPanel->Set(m_KeyValues);
		return;
	}

	//check for new soundscape
	else if (!Q_strcmp(pszCommand, NEW_BUTTON_COMMAND))
	{
		//make sure you want to create a new soundscape file
		vgui::QueryBox* popup = new vgui::QueryBox("New File?", "Are you sure you want to create a new soundscape file?", this);
		popup->SetOKCommand(new KeyValues("Command", "command", RESET_BUTTON_COMMAND));
		popup->SetCancelButtonVisible(false);
		popup->AddActionSignalTarget(this);
		popup->DoModal(this);

		return;
	}

	//check for reset soundscape
	else if (!Q_strcmp(pszCommand, RESET_BUTTON_COMMAND))
	{
		m_kvCurrSelected = nullptr;

		//stop all soundscapes before deleting the old soundscapes
		if (g_IsPlayingSoundscape)
			PlaySelectedSoundscape();

		m_KeyValues->deleteThis();
		m_KeyValues = new KeyValues("Empty Soundscape");

		//reset title
		SetTitle("Soundscape Maker (New File)", true);

		LoadFile(m_KeyValues);
		return;
	}

	//check for play sound
	else if (!Q_strcmp(pszCommand, SOUNDS_LIST_BUTTON_COMMAND))
	{
		//initalize the sounds
		static bool g_SoundPanelInitalized = false;
		if (!g_SoundPanelInitalized)
		{
			g_SoundPanelInitalized = true;
			g_SoundPanel->InitalizeSounds();
		}

		//get sound text entry name
		char buf[512];
		m_SoundNameTextEntry->GetText(buf, sizeof(buf));

		//check the current mode
		if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
		{
			g_SoundPanel->SetIsUsingSoundPanel(false);

			//load all the temporary soundscapes
			CUtlVector<const char*> OtherSoundscapes;
			for (KeyValues* curr = m_KeyValues; curr; curr = curr->GetNextKey())
			{
				if (curr == m_kvCurrSelected)
					continue;

				OtherSoundscapes.AddToTail(curr->GetName());
			}

			g_SoundPanel->InitalizeSoundscapes(OtherSoundscapes);
		}
		else
		{
			g_SoundPanel->SetIsUsingSoundPanel(true);

			//look for item with same name
			for (int i = 0; i < g_SoundDirectories.Count(); i++)
			{
				if (!Q_strcmp(buf, g_SoundDirectories[i]))
				{
					//select item
					g_SoundPanel->m_SoundsList->ActivateItem(i);
					g_SoundPanel->m_SoundsList->SetText(buf);

					break;
				}
			}
		}

		g_SoundPanel->SetVisible(true);
		g_SoundPanel->MoveToFront();
		g_SoundPanel->RequestFocus();
		return;
	}

	//check for play soundscape
	else if (!Q_strcmp(pszCommand, PLAY_SOUNDSCAPE_COMMAND))
	{
		if (m_PlaySoundscapeButton->IsSelected())
		{
			//enable the reset soundscape button
			m_ResetSoundscapeButton->SetEnabled(true);

			//play the soundscape
			PlaySelectedSoundscape();
		}
		else
		{
			//disable the reset soundscape button
			m_ResetSoundscapeButton->SetEnabled(false);

			g_IsPlayingSoundscape = false;

			//stop all sounds and soundscapes
			enginesound->StopAllSounds(true);
			g_SoundscapeSystem.StartNewSoundscape(nullptr);
		}

		return;
	}

	//check for play soundscape
	else if (!Q_strcmp(pszCommand, RESET_SOUNDSCAPE_BUTTON_COMMAND))
	{
		PlaySelectedSoundscape();
		return;
	}


	//check for delete item
	else if (!Q_strcmp(pszCommand, DELETE_CURRENT_ITEM_COMMAND))
	{
		//check for current rndwave
		if (m_kvCurrRndwave && m_SoundNameTextEntry->IsEnabled())
		{
			if (!m_kvCurrRndwave || m_iCurrRndWave <= 0)
				return;

			//get the keyvalues by the index
			int curr = 0;
			KeyValues* prev = nullptr;

			FOR_EACH_VALUE(m_kvCurrRndwave, keyvalues)
			{
				if (++curr == m_iCurrRndWave)
				{
					//delete
					if (prev)
						prev->SetNextKey(keyvalues->GetNextValue());
					else
					{
						m_kvCurrRndwave->m_pSub = keyvalues->GetNextValue();
						m_iCurrRndWave = -1;
					}

					curr = curr - 1;

					keyvalues->SetNextKey(nullptr);
					keyvalues->deleteThis();
					break;
				}



				prev = keyvalues;
			}

			//reset everything
			m_SoundNameTextEntry->SetText("");
			m_SoundNameTextEntry->SetEnabled(false);
			m_SoundNamePlay->SetEnabled(false);

			//store vector
			auto& vec = m_pSoundList->m_MenuButtons;

			//remove it
			delete vec[curr];
			vec.Remove(curr);

			//move everything down
			m_pSoundList->m_iCurrentY = m_pSoundList->m_iCurrentY - 22;
			m_pSoundList->m_Keyvalues = m_kvCurrRndwave;

			if (vec.Count() >= m_pSoundList->m_iMax)
			{
				m_pSoundList->OnMouseWheeled(1);

				int min, max;
				m_pSoundList->m_pSideSlider->GetRange(min, max);
				m_pSoundList->m_pSideSlider->SetRange(0, max - 1);
			}

			for (int i = curr; i < vec.Count(); i++)
			{
				//move everything down
				int x, y = 0;
				vec[i]->GetPos(x, y);
				vec[i]->SetPos(x, y - 22);
			}

			//reset every command
			int WaveAmount = 0;
			for (int i = 0; i < vec.Count(); i++)
			{
				//store data name
				const char* name = vec[i]->GetCommand()->GetString("command");

				//increment variables based on name
				if (Q_stristr(name, "$rndwave") == name)
				{
					WaveAmount++;
					vec[i]->SetCommand(CFmtStr("$rndwave%d", WaveAmount));
				}
			}

			//bounds check
			if (vec.Count() <= 0)
			{
				m_kvCurrRndwave = nullptr;
				m_pSoundList->m_Keyvalues = nullptr;

				//restart soundscape
				PlaySelectedSoundscape();

				return;
			}

			//select next item
			if (m_iCurrRndWave <= vec.Count())
				OnCommand(CFmtStr("$rndwave%d", curr + 1));
			else
				OnCommand(CFmtStr("$rndwave%d", curr));
		}
		else if (m_kvCurrSound)
		{
			//find keyvalue with same pointer and get the index
			int tmpindex = 0;
			int index = -1;

			KeyValues* prev = nullptr;
			FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, keyvalues)
			{
				if (m_kvCurrSound == keyvalues)
				{
					//remove it
					if (prev)
						prev->SetNextKey(keyvalues->GetNextTrueSubKey());
					else
						m_kvCurrSelected->m_pSub = keyvalues->GetNextTrueSubKey();

					keyvalues->SetNextKey(nullptr);
					keyvalues->deleteThis();

					//get index
					index = tmpindex;
					break;
				}

				prev = keyvalues;

				//increment
				tmpindex++;
			}

			//error
			if (index == -1)
				return;

			//store vector
			auto& vec = m_pDataList->m_MenuButtons;

			//remove it
			delete vec[index];
			vec.Remove(index);

			//move everything down
			m_pDataList->m_iCurrentY = m_pDataList->m_iCurrentY - 22;
			m_pDataList->m_Keyvalues = m_kvCurrSelected;

			for (int i = index; i < vec.Count(); i++)
			{
				//move everything down
				int x, y = 0;
				vec[i]->GetPos(x, y);
				vec[i]->SetPos(x, y - 22);
			}

			if (vec.Count() >= m_pDataList->m_iMax)
			{
				m_pDataList->OnMouseWheeled(1);

				int min, max;
				m_pDataList->m_pSideSlider->GetRange(min, max);

				if (max > 0)
					m_pDataList->m_pSideSlider->SetRange(0, max - 1);
				else
					m_pDataList->m_pSideSlider->SetRange(0, 0);
			}

			//reset the names of each button
			int RandomNum = 0;
			int LoopingNum = 0;
			int SoundscapeNum = 0;

			//change the commands of the buttons
			for (int i = 0; i < vec.Count(); i++)
			{
				//store data name
				const char* name = vec[i]->GetCommand()->GetString("command");

				//increment variables based on name
				if (Q_stristr(name, "$playrandom") == name)
				{
					RandomNum++;
					vec[i]->SetCommand(CFmtStr("$playrandom%d", RandomNum));
				}

				if (Q_stristr(name, "$playlooping") == name)
				{
					LoopingNum++;
					vec[i]->SetCommand(CFmtStr("$playlooping%d", LoopingNum));
				}

				if (Q_stristr(name, "$playsoundscape") == name)
				{
					SoundscapeNum++;
					vec[i]->SetCommand(CFmtStr("$playsoundscape%d", SoundscapeNum));
				}
			}

			//reset everything
			m_SoundLevels->SetText("");
			m_SoundNameTextEntry->SetText("");
			m_TimeTextEntry->SetText("");
			m_PitchTextEntry->SetText("");
			m_PositionTextEntry->SetText("");
			m_VolumeTextEntry->SetText("");

			m_SoundLevels->SetEnabled(false);
			m_SoundNameTextEntry->SetEnabled(false);
			m_TimeTextEntry->SetEnabled(false);
			m_PitchTextEntry->SetEnabled(false);
			m_PositionTextEntry->SetEnabled(false);
			m_VolumeTextEntry->SetEnabled(false);
			m_SoundNamePlay->SetEnabled(false);

			m_pSoundList->Clear();

			m_kvCurrSound = nullptr;
			m_pSoundList->m_Keyvalues = nullptr;

			//bounds checking
			if (index >= vec.Count())
				index = vec.Count() - 1; // fix bounds more safely

			//select the button
			if (index >= 0)
				OnCommand(vec[index]->GetCommand()->GetString("command"));
		}
		else if (m_kvCurrSelected)
		{
			if (m_KeyValues == m_kvCurrSelected)
			{
				//play an error sound
				vgui::surface()->PlaySound("resource/warning.wav");

				//show an error
				vgui::QueryBox* popup = new vgui::QueryBox("Error", "Can not delete base soundscape!", this);
				popup->SetOKButtonText("Ok");
				popup->SetCancelButtonVisible(false);
				popup->AddActionSignalTarget(this);
				popup->DoModal(this);

				return;
			}

			//find keyvalue with same pointer and get the index
			int tmpindex = 0;
			int index = -1;

			KeyValues* prev = nullptr;
			for (KeyValues* keyvalues = m_KeyValues; keyvalues != nullptr; keyvalues = keyvalues->GetNextTrueSubKey())
			{
				if (m_kvCurrSelected == keyvalues)
				{
					//remove it
					if (!prev)
						break;

					prev->SetNextKey(keyvalues->GetNextTrueSubKey());
					keyvalues->SetNextKey(nullptr);
					keyvalues->deleteThis();

					//get index
					index = tmpindex;
					break;
				}

				prev = keyvalues;

				//increment
				tmpindex++;
			}

			//error
			if (index == -1)
				return;

			//store vector
			auto& vec = m_SoundscapesList->m_MenuButtons;

			//remove it
			delete vec[index];
			vec.Remove(index);

			//move everything down
			m_SoundscapesList->m_iCurrentY = m_SoundscapesList->m_iCurrentY - 22;

			for (int i = index; i < vec.Count(); i++)
			{
				//move everything down
				int x, y = 0;
				vec[i]->GetPos(x, y);
				vec[i]->SetPos(x, y - 22);
			}

			if (vec.Count() >= m_SoundscapesList->m_iMax)
			{
				m_SoundscapesList->OnMouseWheeled(1);

				int min, max;
				m_SoundscapesList->m_pSideSlider->GetRange(min, max);
				m_SoundscapesList->m_pSideSlider->SetRange(0, max - 1);
			}

			//reset everything
			m_DspEffects->SetText("");
			m_SoundLevels->SetText("");
			m_TextEntryName->SetText("");
			m_SoundNameTextEntry->SetText("");
			m_TimeTextEntry->SetText("");
			m_PitchTextEntry->SetText("");
			m_PositionTextEntry->SetText("");
			m_VolumeTextEntry->SetText("");

			m_DspEffects->SetEnabled(false);
			m_SoundLevels->SetEnabled(false);
			m_TextEntryName->SetEnabled(false);
			m_SoundNameTextEntry->SetEnabled(false);
			m_TimeTextEntry->SetEnabled(false);
			m_PitchTextEntry->SetEnabled(false);
			m_PositionTextEntry->SetEnabled(false);
			m_VolumeTextEntry->SetEnabled(false);
			m_SoundNamePlay->SetEnabled(false);

			m_pDataList->Clear();
			m_pSoundList->Clear();

			m_kvCurrSound = nullptr;
			m_pDataList->m_Keyvalues = nullptr;

			//go to next soundscape
			if (!prev)
			{
				//restart soundscape
				PlaySelectedSoundscape();
				return;
			}

			if (prev->GetNextTrueSubKey())
				OnCommand(prev->GetNextTrueSubKey()->GetName());
			else
				OnCommand(prev->GetName());
		}

		//restart soundscape
		PlaySelectedSoundscape();
		return;
	}

	//check for "playrandom", "playsoundscape" or "playlooping"
	if (Q_stristr(pszCommand, "$playrandom") == pszCommand)
	{
		//get the selected number
		char* str_number = (char*)(pszCommand + 11);
		int number = atoi(str_number);
		if (number != 0)
		{
			//look for button with same command
			auto& vec = m_pDataList->m_MenuButtons;
			for (int i = 0; i < vec.Count(); i++)
			{
				//if the button doesnt have the same command then de-select it. else select it
				if (!Q_strcmp(vec[i]->GetCommand()->GetString("command"), pszCommand))
					vec[i]->m_bIsSelected = true;
				else
					vec[i]->m_bIsSelected = false;
			}


			//clear the m_pSoundList
			m_pSoundList->Clear();
			m_pSoundList->m_Keyvalues = nullptr;

			//store variables
			KeyValues* data = nullptr;
			int curr = 0;

			//get subkey
			FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, sounds)
			{
				if (Q_strcasecmp(sounds->GetName(), "playrandom"))
					continue;

				if (++curr == number)
				{
					data = sounds;
					break;
				}
			}

			//no data
			if (!data)
				return;

			m_kvCurrSound = data;
			m_kvCurrRndwave = nullptr;

			//set the random times
			m_TimeTextEntry->SetText(data->GetString("time", "10,20"));
			m_VolumeTextEntry->SetText(data->GetString("volume", "0.5,0.8"));
			m_PitchTextEntry->SetText(data->GetString("pitch", "100"));
			m_PositionTextEntry->SetText(data->GetString("position", ""));
			m_SoundNameTextEntry->SetText("");

			//get snd level index
			int index = 8;	//8 = SNDLVL_NORM
			const char* name = data->GetString("soundlevel", nullptr);

			//check for the name
			if (name)
			{

				//loop through the sound levels to find the right one
				for (int i = 0; i < sizeof(g_SoundLevels) / sizeof(g_SoundLevels[i]); i++)
				{
					if (!Q_strcmp(name, g_SoundLevels[i]))
					{
						index = i;
						break;
					}
				}
			}

			//select the index
			m_SoundLevels->ActivateItem(index);

			//enable the text entries
			m_TimeTextEntry->SetEnabled(true);
			m_VolumeTextEntry->SetEnabled(true);
			m_PitchTextEntry->SetEnabled(true);
			m_PositionTextEntry->SetEnabled(true);
			m_SoundLevels->SetEnabled(true);
			m_SoundNameTextEntry->SetEnabled(false);
			m_SoundNamePlay->SetEnabled(false);

			g_SoundPanel->OnCommand(SOUND_LIST_STOP_COMMAND);
			g_SoundPanel->SetVisible(false);

			//check for randomwave subkey
			if ((data = data->FindKey("rndwave")) == nullptr)
				return;

			m_kvCurrRndwave = data;
			m_pSoundList->m_Keyvalues = data;

			//add all the data
			int i = 0;
			FOR_EACH_VALUE(data, sound)
			{
				const char* name = sound->GetName();

				//get real text
				const char* text = sound->GetString();

				//get last / or \ and make the string be that + 1
				char* fslash = Q_strrchr(text, '/');
				char* bslash = Q_strrchr(text, '\\');

				//no forward slash and no back slash
				if (!fslash && !bslash)
				{
					text = text;
				}
				else
				{
					if (fslash > bslash)
						text = fslash + 1;

					else if (bslash > fslash)
						text = bslash + 1;
				}

				m_pSoundList->AddButton(name, text, CFmtStr("$rndwave%d", ++i), this, sound, SoundscapeClipboardType::Type_SoundscapeRandomWave);
			}

			m_iSoundscapeMode = SoundscapeMode::Mode_Random;
			return;
		}
	}
	else if (Q_stristr(pszCommand, "$playlooping") == pszCommand)
	{
		//get the selected number
		char* str_number = (char*)(pszCommand + 12);
		int number = atoi(str_number);
		if (number != 0)
		{
			//look for button with same command
			auto& vec = m_pDataList->m_MenuButtons;
			for (int i = 0; i < vec.Count(); i++)
			{
				//if the button doesnt have the same command then de-select it. else select it
				if (!Q_strcmp(vec[i]->GetCommand()->GetString("command"), pszCommand))
					vec[i]->m_bIsSelected = true;
				else
					vec[i]->m_bIsSelected = false;
			}


			//clear the m_pSoundList
			m_pSoundList->Clear();
			m_pSoundList->m_Keyvalues = nullptr;

			//store variables
			KeyValues* data = nullptr;
			int curr = 0;

			//get subkey
			FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, sounds)
			{
				if (Q_strcasecmp(sounds->GetName(), "playlooping"))
					continue;

				if (++curr == number)
				{
					data = sounds;
					break;
				}
			}

			//no data
			if (!data)
				return;

			m_kvCurrSound = data;
			m_kvCurrRndwave = nullptr;

			//set the random times
			m_TimeTextEntry->SetText("");
			m_VolumeTextEntry->SetText(data->GetString("volume", "1"));
			m_PitchTextEntry->SetText(data->GetString("pitch", "100"));
			m_PositionTextEntry->SetText(data->GetString("position", ""));
			m_SoundNameTextEntry->SetText(data->GetString("wave", ""));

			//get snd level index
			int index = 8;	//8 = SNDLVL_NORM
			const char* name = data->GetString("soundlevel", nullptr);

			//check for the name
			if (name)
			{

				//loop through the sound levels to find the right one
				for (int i = 0; i < sizeof(g_SoundLevels) / sizeof(g_SoundLevels[i]); i++)
				{
					if (!Q_strcmp(name, g_SoundLevels[i]))
					{
						index = i;
						break;
					}
				}
			}

			//select the index
			m_SoundLevels->ActivateItem(index);

			//enable the text entries
			m_TimeTextEntry->SetEnabled(false);
			m_VolumeTextEntry->SetEnabled(true);
			m_PitchTextEntry->SetEnabled(true);
			m_PositionTextEntry->SetEnabled(true);
			m_SoundLevels->SetEnabled(true);
			m_SoundNameTextEntry->SetEnabled(true);
			m_SoundNamePlay->SetEnabled(true);
			g_SoundPanel->SetVisible(false);

			m_iSoundscapeMode = SoundscapeMode::Mode_Looping;
			return;
		}
	}
	else if (Q_stristr(pszCommand, "$playsoundscape") == pszCommand)
	{
		//get the selected number
		char* str_number = (char*)(pszCommand + 15);
		int number = atoi(str_number);
		if (number != 0)
		{
			//look for button with same command
			auto& vec = m_pDataList->m_MenuButtons;
			for (int i = 0; i < vec.Count(); i++)
			{
				//if the button doesnt have the same command then de-select it. else select it
				if (!Q_strcmp(vec[i]->GetCommand()->GetString("command"), pszCommand))
					vec[i]->m_bIsSelected = true;
				else
					vec[i]->m_bIsSelected = false;
			}


			//clear the m_pSoundList
			m_pSoundList->Clear();
			m_pSoundList->m_Keyvalues = nullptr;

			//store variables
			KeyValues* data = nullptr;
			int curr = 0;

			//get subkey
			FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, sounds)
			{
				if (Q_strcasecmp(sounds->GetName(), "playsoundscape"))
					continue;

				if (++curr == number)
				{
					data = sounds;
					break;
				}
			}

			//no data
			if (!data)
				return;

			m_kvCurrSound = data;
			m_kvCurrRndwave = nullptr;

			//set the random times
			m_TimeTextEntry->SetText("");
			m_VolumeTextEntry->SetText(data->GetString("volume", "1"));
			m_PositionTextEntry->SetText(data->GetString("positionoverride", ""));
			m_SoundNameTextEntry->SetText(data->GetString("name", ""));
			m_PitchTextEntry->SetText("");

			//get snd level index
			int index = 8;	//8 = SNDLVL_NORM
			const char* name = data->GetString("soundlevel", nullptr);

			//check for the name
			if (name)
			{

				//loop through the sound levels to find the right one
				for (int i = 0; i < sizeof(g_SoundLevels) / sizeof(g_SoundLevels[i]); i++)
				{
					if (!Q_strcmp(name, g_SoundLevels[i]))
					{
						index = i;
						break;
					}
				}
			}

			//select the index
			m_SoundLevels->ActivateItem(index);

			//enable the text entries
			m_TimeTextEntry->SetEnabled(true);
			m_VolumeTextEntry->SetEnabled(true);
			m_PitchTextEntry->SetEnabled(false);
			m_PositionTextEntry->SetEnabled(true);
			m_SoundLevels->SetEnabled(true);
			m_SoundNameTextEntry->SetEnabled(true);
			m_TimeTextEntry->SetEnabled(false);
			m_SoundNamePlay->SetEnabled(true);

			g_SoundPanel->OnCommand(SOUND_LIST_STOP_COMMAND);
			g_SoundPanel->SetVisible(false);

			m_iSoundscapeMode = SoundscapeMode::Mode_Soundscape;
			return;
		}
	}
	else if (Q_stristr(pszCommand, "$rndwave") == pszCommand)
	{
		if (!m_kvCurrRndwave)
			return;

		//get the selected number
		char* str_number = (char*)(pszCommand + 8);
		m_iCurrRndWave = atoi(str_number);
		if (m_iCurrRndWave != 0)
		{
			//look for button with same command
			auto& vec = m_pSoundList->m_MenuButtons;
			for (int i = 0; i < vec.Count(); i++)
			{
				//if the button doesnt have the same command then de-select it. else select it
				if (!Q_strcmp(vec[i]->GetCommand()->GetString("command"), pszCommand))
					vec[i]->m_bIsSelected = true;
				else
					vec[i]->m_bIsSelected = false;
			}


			int i = 0;

			//get value
			KeyValues* curr = nullptr;
			FOR_EACH_VALUE(m_kvCurrRndwave, wave)
			{
				if (++i == m_iCurrRndWave)
				{
					curr = wave;
					break;
				}
			}

			//if no curr then throw an error
			if (!curr)
			{
				//play an error sound
				vgui::surface()->PlaySound("resource/warning.wav");

				//show error
				char buf[1028];
				Q_snprintf(buf, sizeof(buf), "Failed to get rndwave '%d' for subkey \"%s\"\nfor current soundscape file!", i, m_kvCurrSelected->GetName());

				//show an error
				vgui::QueryBox* popup = new vgui::QueryBox("Error", buf, this);
				popup->SetOKButtonText("Ok");
				popup->SetCancelButtonVisible(false);
				popup->AddActionSignalTarget(this);
				popup->DoModal(this);
				return;
			}

			m_SoundNameTextEntry->SetEnabled(true);
			m_SoundNameTextEntry->SetText(curr->GetString());

			m_SoundNamePlay->SetEnabled(true);

			m_iSoundscapeMode = SoundscapeMode::Mode_Random;
			return;
		}
	}

	//look for button with the same name as the command
	{
		//store vars
		CUtlVector<CSoundscapeButton*>& array = m_SoundscapesList->m_MenuButtons;

		//de-select button
		if (m_pCurrentSelected)
			m_pCurrentSelected->m_bIsSelected = false;

		//check for name
		for (int i = 0; i < m_SoundscapesList->m_MenuButtons.Size(); i++)
		{
			//check button name
			if (!Q_strcmp(array[i]->GetCommand()->GetString("command"), pszCommand))
			{
				//found it
				m_pCurrentSelected = array[i];
				break;
			}
		}

		//set needed stuff
		if (m_pCurrentSelected)
		{
			//select button
			m_pCurrentSelected->m_bIsSelected = true;
			m_DeleteCurrentButton->SetEnabled(false);

			//reset the selected kv
			m_kvCurrSelected = nullptr;

			//find selected keyvalues
			for (KeyValues* kv = m_KeyValues; kv != nullptr; kv = kv->GetNextTrueSubKey())
			{
				if (!Q_strcmp(kv->GetName(), pszCommand))
				{
					m_kvCurrSelected = kv;
					break;
				}
			}

			//set 
			m_kvCurrSound = nullptr;
			m_kvCurrRndwave = nullptr;

			m_TimeTextEntry->SetEnabled(false);
			m_TimeTextEntry->SetText("");

			m_VolumeTextEntry->SetEnabled(false);
			m_VolumeTextEntry->SetText("");

			m_PitchTextEntry->SetEnabled(false);
			m_PitchTextEntry->SetText("");

			m_PositionTextEntry->SetEnabled(false);
			m_PositionTextEntry->SetText("");

			m_SoundLevels->SetEnabled(false);
			m_SoundLevels->SetText("");

			m_SoundNameTextEntry->SetEnabled(false);
			m_SoundNameTextEntry->SetText("");

			m_SoundNamePlay->SetEnabled(false);

			if (g_SoundPanel)
			{
				g_SoundPanel->OnCommand(SOUND_LIST_STOP_COMMAND);
				g_SoundPanel->SetVisible(false);
			}

			//check for current keyvalues. should never bee nullptr but could be
			if (!m_kvCurrSelected)
			{
				//play an error sound
				vgui::surface()->PlaySound("resource/warning.wav");

				//show error
				char buf[1028];
				Q_snprintf(buf, sizeof(buf), "Failed to find KeyValue subkey \"%s\"\nfor current soundscape file!", pszCommand);

				//show an error
				vgui::QueryBox* popup = new vgui::QueryBox("Error", buf, this);
				popup->SetOKButtonText("Ok");
				popup->SetCancelButtonVisible(false);
				popup->AddActionSignalTarget(this);
				popup->DoModal(this);

				//reset vars
				m_pCurrentSelected = nullptr;

				m_TextEntryName->SetEnabled(false);
				m_TextEntryName->SetText("");

				m_DspEffects->SetEnabled(false);
				m_DspEffects->SetText("");
				return;
			}

			if (g_IsPlayingSoundscape)
				PlaySelectedSoundscape();

			m_DeleteCurrentButton->SetEnabled(true);

			//set current soundscape name
			m_TextEntryName->SetText(pszCommand);
			m_TextEntryName->SetEnabled(true);
			m_pDataList->m_Keyvalues = m_kvCurrSelected;

			//set dsp effect
			int dsp = Clamp<int>(m_kvCurrSelected->GetInt("dsp"), 0, 29);

			m_PlaySoundscapeButton->SetEnabled(true);

			m_DspEffects->SetEnabled(true);
			m_DspEffects->ActivateItem(dsp);

			//clear these
			m_pDataList->Clear();
			m_pSoundList->Clear();
			m_pSoundList->m_Keyvalues = nullptr;

			//set variables
			int RandomNum = 0;
			int LoopingNum = 0;
			int SoundscapeNum = 0;

			FOR_EACH_TRUE_SUBKEY(m_kvCurrSelected, data)
			{
				//store data name
				const char* name = data->GetName();

				//increment variables based on name
				if (!Q_strcasecmp(name, "playrandom"))
				{
					RandomNum++;
					m_pDataList->AddButton(data->GetName(), data->GetName(), CFmtStr("$playrandom%d", RandomNum), this, data, SoundscapeClipboardType::Type_SoundscapeData);
				}

				if (!Q_strcasecmp(name, "playlooping"))
				{
					LoopingNum++;
					m_pDataList->AddButton(data->GetName(), data->GetName(), CFmtStr("$playlooping%d", LoopingNum), this, data, SoundscapeClipboardType::Type_SoundscapeData);
				}

				if (!Q_strcasecmp(name, "playsoundscape"))
				{
					SoundscapeNum++;
					m_pDataList->AddButton(data->GetName(), data->GetName(), CFmtStr("$playsoundscape%d", SoundscapeNum), this, data, SoundscapeClipboardType::Type_SoundscapeData);
				}
			}
		}
	}

	BaseClass::OnCommand(pszCommand);
}

//-----------------------------------------------------------------------------
// Purpose: Paste item from clipboard
//-----------------------------------------------------------------------------
void CSoundscapeMaker::Paste(SoundscapeClipboardType type)
{
	switch (type)
	{
	case SoundscapeClipboardType::Type_SoundscapeName:
		m_SoundscapesList->OnCommand(PASTE_FROM_CLIBOARD_COMMAND);
		break;
	case SoundscapeClipboardType::Type_SoundscapeData:
		m_pDataList->OnCommand(PASTE_FROM_CLIBOARD_COMMAND);
		break;
	case SoundscapeClipboardType::Type_SoundscapeRandomWave:
		m_pSoundList->OnCommand(PASTE_FROM_CLIBOARD_COMMAND);
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Function to recursivly write keyvalues to keyvalue files. the keyvalues
//			class does have a function to do this BUT this function writes every single
//			item one after another. this function does that but writes the keys
//			first then the subkeys so the order is good.
//-----------------------------------------------------------------------------
void RecursivlyWriteKeyvalues(KeyValues* prev, CUtlBuffer& buffer, int& indent)
{
	//write \t indent
	for (int i = 0; i < indent; i++)
		buffer.PutChar('\t');

	//write name
	buffer.PutChar('"');
	buffer.PutString(prev->GetName());
	buffer.PutString("\"\n");

	//write {
	for (int i = 0; i < indent; i++)
		buffer.PutChar('\t');

	buffer.PutString("{\n");

	//increment indent
	indent++;

	//write all the keys first
	FOR_EACH_VALUE(prev, value)
	{
		for (int i = 0; i < indent; i++)
			buffer.PutChar('\t');

		//write name and value
		buffer.PutChar('"');
		buffer.PutString(value->GetName());
		buffer.PutString("\"\t");

		buffer.PutChar('"');
		buffer.PutString(value->GetString());
		buffer.PutString("\"\n");
	}

	//write all the subkeys now
	FOR_EACH_TRUE_SUBKEY(prev, value)
	{
		//increment indent
		RecursivlyWriteKeyvalues(value, buffer, indent);

		if (value->GetNextTrueSubKey())
			buffer.PutChar('\n');
	}

	//decrement indent
	indent--;

	//write ending }
	for (int i = 0; i < indent; i++)
		buffer.PutChar('\t');

	buffer.PutString("}\n");
}

//-----------------------------------------------------------------------------
// Purpose: Called when a file gets opened/closed
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnFileSelected(const char* pszFileName)
{
	//check for null or empty string
	if (!pszFileName || pszFileName[0] == '\0')
		return;

	//check for file save
	if (!m_bWasFileLoad)
	{
		//save the file
		if (m_KeyValues)
		{
			//write everything into a buffer
			CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
			buf.PutString("//------------------------------------------------------------------------------------\n");
			buf.PutString("//\n");
			buf.PutString("// Auto-generated soundscape file created with modbases soundscape tool'\n");
			buf.PutString("//\n");
			buf.PutString("//------------------------------------------------------------------------------------\n");

			//now write the keyvalues
			KeyValues* pCurrent = m_KeyValues;
			while (pCurrent)
			{
				int indent = 0;
				RecursivlyWriteKeyvalues(pCurrent, buf, indent);

				//put a newline
				if (pCurrent->GetNextTrueSubKey())
					buf.PutChar('\n');

				//get next
				pCurrent = pCurrent->GetNextTrueSubKey();
			}

			if (!g_pFullFileSystem->WriteFile(pszFileName, "MOD", buf))
			{
				//play an error sound
				vgui::surface()->PlaySound("resource/warning.wav");

				//get the error first
				char buf[1028];
				Q_snprintf(buf, sizeof(buf), "Failed to save soundscape to file \"%s\"", pszFileName);

				//show an error
				vgui::QueryBox* popup = new vgui::QueryBox("Error", buf, this);
				popup->SetOKButtonText("Ok");
				popup->SetCancelButtonVisible(false);
				popup->AddActionSignalTarget(this);
				popup->DoModal(this);
				return;
			}
		}


		//store vars
		const char* last = pszFileName;
		const char* tmp = nullptr;

		//get the last /
		while ((last = Q_strstr(last, "\\")) != nullptr)
			tmp = ++last; //move past the backslash

		//check tmp
		if (!tmp || !*tmp)
			tmp = pszFileName;

		//set new title
		char buf[1028];
		Q_snprintf(buf, sizeof(buf), "Soundscape Maker (%s)", tmp);

		SetTitle(buf, true);

		//create copy of pszFileName
		char manifest[1028];
		Q_strncpy(manifest, pszFileName, sizeof(manifest));

		//get last /
		char* lastSlash = Q_strrchr(manifest, '\\');
		if (lastSlash == nullptr || *lastSlash == '\0')
			return;

		//append 'soundscapes_manifest.txt'
		lastSlash[1] = '\0';
		strcat(manifest, "soundscapes_manifest.txt");

		//see if we can open manifest file
		KeyValues* man_file = new KeyValues("manifest");
		if (!man_file->LoadFromFile(g_pFullFileSystem, manifest))
		{
			//cant open manifest file
			man_file->deleteThis();
			return;
		}

		//get real filename
		pszFileName = Q_strrchr(pszFileName, '\\');
		if (!pszFileName || !*pszFileName)
		{
			man_file->deleteThis();
			return;
		}

		pszFileName = pszFileName + 1;

		//create name to be added to the manifest file
		char add_file[1028];
		Q_snprintf(add_file, sizeof(add_file), "scripts/%s", pszFileName);

		//add filename to manifest file if not found
		FOR_EACH_VALUE(man_file, value)
		{
			if (!Q_strcmp(value->GetString(), add_file))
			{
				man_file->deleteThis();
				return;
			}
		}


		//add to manifest file
		KeyValues* kv = new KeyValues("file");
		kv->SetString(nullptr, add_file);
		man_file->AddSubKey(kv);

		//write to file
		man_file->SaveToFile(g_pFullFileSystem, manifest);

		man_file->deleteThis();
		return;
	}

	//try and load the keyvalues file first
	KeyValues* temp = new KeyValues("SoundscapeFile");
	if (!temp->LoadFromFile(filesystem, pszFileName))
	{
		//play an error sound
		vgui::surface()->PlaySound("resource/warning.wav");

		//get the error first
		char buf[1028];
		Q_snprintf(buf, sizeof(buf), "Failed to open keyvalues file \"%s\"", pszFileName);

		//show an error
		vgui::QueryBox* popup = new vgui::QueryBox("Error", buf, this);
		popup->SetOKButtonText("Ok");
		popup->SetCancelButtonVisible(false);
		popup->AddActionSignalTarget(this);
		popup->DoModal(this);

		temp->deleteThis();
		return;
	}

	//set the new title
	{
		//store vars
		const char* last = pszFileName;
		const char* tmp = nullptr;

		//get the last /
		while ((last = Q_strstr(last, "\\")) != nullptr)
			tmp = ++last; //move past the backslash

		//check tmp
		if (!tmp || !*tmp)
			tmp = pszFileName;

		//create the new new title
		char buf[1028];
		Q_snprintf(buf, sizeof(buf), "Soundscape Maker (%s)", tmp);

		SetTitle(buf, true);
	}

	//stop all soundscapes before deleting the old soundscapes
	m_kvCurrSelected = nullptr;

	if (g_IsPlayingSoundscape)
		PlaySelectedSoundscape();

	//delete and set the old keyvalues
	if (m_KeyValues)
		m_KeyValues->deleteThis();

	m_KeyValues = temp;

	//load the file
	LoadFile(m_KeyValues);
}

//-----------------------------------------------------------------------------
// Purpose: Called when a text thing changes
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnTextChanged(KeyValues* keyvalues)
{
	//check for these things
	if (!m_pCurrentSelected || !m_kvCurrSelected)
		return;

	//check to see if the current focus is the text text entry
	if (m_TextEntryName->HasFocus())
	{
		//get text
		char buf[50];
		m_TextEntryName->GetText(buf, sizeof(buf));

		//set current text and keyvalue name
		m_kvCurrSelected->SetName(buf);
		m_pCurrentSelected->SetText(buf);
		m_pCurrentSelected->SetCommand(buf);
		return;
	}

	//set dsp
	if (Clamp<int>(m_DspEffects->GetActiveItem(), 0, SIZE_OF_ARRAY(g_DspEffects)) != 0)
		m_kvCurrSelected->SetInt("dsp", Clamp<int>(m_DspEffects->GetActiveItem() - 1, 0, SIZE_OF_ARRAY(g_DspEffects) - 1));
	else
		m_kvCurrSelected->RemoveSubKey(m_kvCurrSelected->FindKey("dsp"));

	//if the m_kvCurrSound is nullptr then dont do the rest of the stuff
	if (!m_kvCurrSound)
		return;

	//set the curr sound and stuff
	if (m_TimeTextEntry->HasFocus())
	{
		//get text
		char buf[38];
		m_TimeTextEntry->GetText(buf, sizeof(buf));

		m_kvCurrSound->SetString("time", buf);
		return;
	}
	else if (m_VolumeTextEntry->HasFocus())
	{
		//get text
		char buf[38];
		m_VolumeTextEntry->GetText(buf, sizeof(buf));

		m_kvCurrSound->SetString("volume", buf);
		return;
	}
	else if (m_PitchTextEntry->HasFocus())
	{
		//dont add to soundscaep
		if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
			return;

		//get text
		char buf[38];
		m_PitchTextEntry->GetText(buf, sizeof(buf));

		m_kvCurrSound->SetString("pitch", buf);
		return;
	}
	else if (m_PositionTextEntry->HasFocus())
	{
		//get text
		char buf[38];
		m_PositionTextEntry->GetText(buf, sizeof(buf));

		//if the string is empty then remove the position instead
		if (!buf[0])
		{
			if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
				m_kvCurrSound->RemoveSubKey(m_kvCurrSound->FindKey("positionoverride"));
			else
				m_kvCurrSound->RemoveSubKey(m_kvCurrSound->FindKey("position"));
		}
		else
		{
			if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
				m_kvCurrSound->SetString("positionoverride", buf);
			else
				m_kvCurrSound->SetString("position", buf);

		}

		return;
	}

	//get the sound level amount
	int sndlevel = Clamp<int>(m_SoundLevels->GetActiveItem(), 0, 20);
	m_kvCurrSound->SetString("soundlevel", g_SoundLevels[sndlevel]);

	//set soundscape name/wave
	if (m_SoundNameTextEntry->HasFocus())
	{
		//get text
		char buf[512];
		m_SoundNameTextEntry->GetText(buf, sizeof(buf));

		if (m_iSoundscapeMode == SoundscapeMode::Mode_Looping)
			m_kvCurrSound->SetString("wave", buf);
		else if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
			m_kvCurrSound->SetString("name", buf);
		else if (m_iSoundscapeMode == SoundscapeMode::Mode_Random && m_kvCurrRndwave)
		{
			//get value
			int i = 0;

			FOR_EACH_VALUE(m_kvCurrRndwave, wave)
			{
				if (++i == m_iCurrRndWave)
				{
					wave->SetStringValue(buf);

					//set text on the sounds panel
					vgui::Button* button = m_pSoundList->m_MenuButtons[i - 1];
					if (button)
					{
						//get last / or \ and make the string be that + 1
						char* fslash = Q_strrchr(buf, '/');
						char* bslash = Q_strrchr(buf, '\\');

						//no forward slash and no back slash
						if (!fslash && !bslash)
						{
							button->SetText(buf);
							return;
						}

						if (fslash > bslash)
						{
							button->SetText(fslash + 1);
							return;
						}

						else if (bslash > fslash)
						{
							button->SetText(bslash + 1);
							return;
						}
					}

					break;
				}
			}
		}

		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loads the keyvalues
//-----------------------------------------------------------------------------
void CSoundscapeMaker::LoadFile(KeyValues* file)
{
	//clear all the text's
	m_TextEntryName->SetEnabled(false);
	m_TextEntryName->SetText("");

	m_DspEffects->SetEnabled(false);
	m_DspEffects->SetText("");

	m_TimeTextEntry->SetEnabled(false);
	m_TimeTextEntry->SetText("");

	m_VolumeTextEntry->SetEnabled(false);
	m_VolumeTextEntry->SetText("");

	m_PitchTextEntry->SetEnabled(false);
	m_PitchTextEntry->SetText("");

	m_PositionTextEntry->SetEnabled(false);
	m_PositionTextEntry->SetText("");

	m_SoundNameTextEntry->SetEnabled(false);
	m_SoundNameTextEntry->SetText("");

	m_SoundNamePlay->SetEnabled(false);

	if (g_SoundPanel)
	{
		g_SoundPanel->OnCommand(SOUND_LIST_STOP_COMMAND);
		g_SoundPanel->SetVisible(false);
	}

	m_PlaySoundscapeButton->SetEnabled(false);

	if (g_IsPlayingSoundscape)
		m_PlaySoundscapeButton->SetSelected(false);

	m_ResetSoundscapeButton->SetEnabled(false);
	m_DeleteCurrentButton->SetEnabled(false);

	//clear current file
	m_pCurrentSelected = nullptr;
	m_kvCurrSelected = nullptr;
	m_kvCurrSound = nullptr;
	m_kvCurrRndwave = nullptr;

	//clear the menu items
	m_SoundscapesList->Clear();
	m_pSoundList->Clear();
	m_pDataList->Clear();

	m_SoundscapesList->m_Keyvalues = file;
	m_pDataList->m_Keyvalues = nullptr;
	m_pSoundList->m_Keyvalues = nullptr;

	g_IsPlayingSoundscape = false;

	//temp soundscapes list
	CUtlVector<const char*> Added;

	//add all the menu items
	for (KeyValues* soundscape = file; soundscape != nullptr; soundscape = soundscape->GetNextTrueSubKey())
	{
		//add the menu buttons
		const char* name = soundscape->GetName();

		//check for the soundscape first
		if (Added.Find(name) != Added.InvalidIndex())
		{
			ConWarning("CSoundscapePanel: Failed to add repeated soundscape '%s'\n", name);
			continue;
		}

		Added.AddToTail(name);
		m_SoundscapesList->AddButton(name, name, name, this, soundscape, SoundscapeClipboardType::Type_SoundscapeName);
	}

	m_SoundscapesList->m_pSideSlider->SetValue(0);
	m_SoundscapesList->ScrollBarMoved(0);

	//
	OnCommand(file->GetName());
}

//-----------------------------------------------------------------------------
// Purpose: Sets the sounds text
//-----------------------------------------------------------------------------
void CSoundscapeMaker::SetSoundText(const char* text)
{
	m_SoundNameTextEntry->SetText(text);

	//set soundscape name/wave
	if (m_iSoundscapeMode != SoundscapeMode::Mode_Random)
	{
		if (m_iSoundscapeMode == SoundscapeMode::Mode_Soundscape)
			m_kvCurrSound->SetString("name", text);
		else
			m_kvCurrSound->SetString("wave", text);
	}
	else
	{
		//get value
		int i = 0;

		FOR_EACH_VALUE(m_kvCurrRndwave, wave)
		{
			if (++i == m_iCurrRndWave)
			{
				wave->SetStringValue(text);

				//set text on the sounds panel
				vgui::Button* button = m_pSoundList->m_MenuButtons[i - 1];
				if (button)
				{
					//get last / or \ and make the string be that + 1
					char* fslash = Q_strrchr(text, '/');
					char* bslash = Q_strrchr(text, '\\');

					//no forward slash and no back slash
					if (!fslash && !bslash)
					{
						button->SetText(text);
						return;
					}

					if (fslash > bslash)
					{
						button->SetText(fslash + 1);
						return;
					}

					else if (bslash > fslash)
					{
						button->SetText(bslash + 1);
						return;
					}
				}

				break;
			}
		}
	}

	return;
}

//-----------------------------------------------------------------------------
// Purpose: Called when a keyboard key is pressed
//-----------------------------------------------------------------------------
void CSoundscapeMaker::OnKeyCodePressed(vgui::KeyCode code)
{
	//check for ctrl o or ctrl s
	if (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LCONTROL) ||
		vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RCONTROL))
	{
		if (code == vgui::KeyCode::KEY_O)
			OnCommand(LOAD_BUTTON_COMMAND);
		else if (code == vgui::KeyCode::KEY_S)
			OnCommand(SAVE_BUTTON_COMMAND);
		else if (code == vgui::KeyCode::KEY_N)
			OnCommand(NEW_BUTTON_COMMAND);
		else if (code == vgui::KeyCode::KEY_P)
		{
			//show settings
			g_SettingsPanel->SetVisible(true);
			g_SettingsPanel->RequestFocus();
			g_SettingsPanel->MoveToFront();
		}

		else if (code == vgui::KeyCode::KEY_D)
			OnCommand(DELETE_CURRENT_ITEM_COMMAND);

		//check for ctrl+alt+a
		else if (code == vgui::KeyCode::KEY_A && (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LALT) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RALT)) && m_pSoundList && m_pSoundList->m_Keyvalues)
			m_pSoundList->OnCommand(NEW_RNDWAVE_WAVE_COMMAND);

		//check for just ctrl+shift+a
		else if (code == vgui::KeyCode::KEY_A && (vgui::input()->IsKeyDown(vgui::KeyCode::KEY_LSHIFT) || vgui::input()->IsKeyDown(vgui::KeyCode::KEY_RSHIFT)) && m_SoundscapesList)
			m_SoundscapesList->OnCommand(ADD_SOUNDSCAPE_COMMAND);

		return;
	}

	//check for arrow keys
	if (code == KEY_DOWN || code == KEY_UP)
	{
		if (m_kvCurrRndwave)
		{
			m_pSoundList->OnKeyCodePressed(code);
		}
		else if (m_kvCurrSelected && m_pDataList->m_MenuButtons.Count())
		{
			m_pDataList->OnKeyCodePressed(code);
		}
		else
		{
			m_SoundscapesList->OnKeyCodePressed(code);
		}

		return;
	}

	//get key bound to this
	const char* key = engine->Key_LookupBinding("modbase_soundscape_panel");
	if (!key)
		return;

	//convert the key to a keyboard code
	const char* keystring = KeyCodeToString(code);

	//remove the KEY_ if found
	if (Q_strstr(keystring, "KEY_") == keystring)
		keystring = keystring + 4;

	//check both strings
	if (!Q_strcasecmp(key, keystring))
		OnClose();
}

//-----------------------------------------------------------------------------
// Purpose: Starts the soundscape on map spawn
//-----------------------------------------------------------------------------
void CSoundscapeMaker::LevelInitPostEntity()
{
	if (g_IsPlayingSoundscape)
		PlaySelectedSoundscape();
}

//-----------------------------------------------------------------------------
// Purpose: Sets keyvalues from text
//-----------------------------------------------------------------------------
void CSoundscapeMaker::Set(const char* buffer)
{
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
	buf.PutString(buffer);

	//try and load the keyvalues file first
	KeyValues* temp = new KeyValues("SoundscapeFile");
	if (!temp->LoadFromBuffer("Text Editor Panel Buffer", buf))
	{
		//play an error sound
		vgui::surface()->PlaySound("resource/warning.wav");

		//show an error
		vgui::QueryBox* popup = new vgui::QueryBox("Error", "Failed to open keyvalues data from \"Text Editor Panel\"", this);
		popup->SetOKButtonText("Ok");
		popup->SetCancelButtonVisible(false);
		popup->AddActionSignalTarget(this);
		popup->DoModal(this);

		temp->deleteThis();
		return;
	}

	//stop all soundscapes before deleting the old soundscapes
	m_kvCurrSelected = nullptr;

	if (g_IsPlayingSoundscape)
		PlaySelectedSoundscape();

	//delete and set the old keyvalues
	if (m_KeyValues)
		m_KeyValues->deleteThis();

	m_KeyValues = temp;

	//load the file
	LoadFile(m_KeyValues);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor for soundscape maker panel
//-----------------------------------------------------------------------------
CSoundscapeMaker::~CSoundscapeMaker()
{
	//delete the keyvalue files if needed
	if (m_KeyValues)
		m_KeyValues->deleteThis();
}

//static panel instance
static CSoundscapeMaker* g_SSMakerPanel = nullptr;

//interface class
class CSoundscapeMakerInterface : public ISoundscapeMaker
{
public:
	void Create(vgui::VPANEL parent)
	{
		g_SSMakerPanel = new CSoundscapeMaker(parent);
		g_SoundPanel = new CSoundListPanel(parent, "SoundscapeSoundListPanel");
		g_SettingsPanel = new CSoundscapeSettingsPanel(parent, "SoundscapeSettingsPanel");
		g_SoundscapeTextPanel = new CSoundscapeTextPanel(parent, "SoundscapeTextPanel");
		g_SoundscapeDebugPanel = new CSoundscapeDebugPanel(parent, "SoundscapeDebugPanel");
	}

	void SetVisible(bool bVisible)
	{
		if (g_SSMakerPanel)
			g_SSMakerPanel->SetVisible(bVisible);
	}

	void Destroy()
	{
		if (g_SSMakerPanel)
			g_SSMakerPanel->DeletePanel();

		if (g_SoundPanel)
			g_SoundPanel->DeletePanel();

		if (g_SettingsPanel)
			g_SettingsPanel->DeletePanel();

		if (g_SoundscapeTextPanel)
			g_SoundscapeTextPanel->DeletePanel();

		if (g_SoundscapeDebugPanel)
			g_SoundscapeDebugPanel->DeletePanel();

		if (g_SoundscapeClipboard)
			g_SoundscapeClipboard->DeletePanel();

		g_SSMakerPanel = nullptr;
		g_SoundPanel = nullptr;
		g_SettingsPanel = nullptr;
		g_SoundscapeTextPanel = nullptr;
		g_SoundscapeDebugPanel = nullptr;
		g_SoundscapeClipboard = nullptr;
	}

	void SetSoundText(const char* text)
	{
		if (!g_SSMakerPanel)
			return;

		g_SSMakerPanel->SetSoundText(text);
		g_SSMakerPanel->RequestFocus();
		g_SSMakerPanel->MoveToFront();
	}

	void SetAllVisible(bool bVisible)
	{
		g_ShowSoundscapePanel = false;

		if (g_SSMakerPanel)
			g_SSMakerPanel->SetVisible(bVisible);

		if (g_SoundPanel)
			g_SoundPanel->SetVisible(bVisible);

		if (g_SettingsPanel)
			g_SettingsPanel->SetVisible(bVisible);

		if (g_SoundscapeTextPanel)
			g_SoundscapeTextPanel->SetVisible(bVisible);

		if (g_SoundscapeDebugPanel)
			g_SoundscapeDebugPanel->SetVisible(bVisible);
	}

	void SetBuffer(const char* text)
	{
		if (g_SSMakerPanel)
			g_SSMakerPanel->Set(text);
	}

	KeyValues* GetPanelFile()
	{
		return g_SSMakerPanel->m_KeyValues;
	}

	KeyValues* GetPanelSelected()
	{
		return g_SSMakerPanel->m_kvCurrSelected;
	}

	void PasteFromClipboard(int type)
	{
		g_SSMakerPanel->Paste((SoundscapeClipboardType)type);
	}
};

CSoundscapeMakerInterface SoundscapeMaker;
ISoundscapeMaker* g_SoundscapeMaker = &SoundscapeMaker;

//-----------------------------------------------------------------------------
// Purpose: User message hook function for setting/getting soundscape position
//-----------------------------------------------------------------------------
void _SoundscapeMaker_Recieve(bf_read& bf)
{
	//show the soundscape panel
	g_ShowSoundscapePanel = true;
	g_SSMakerPanel->SetVisible(true);

	//show settings panel
	g_SettingsPanel->SetVisible(true);

	//get the stuff
	byte index = bf.ReadByte();
	Vector pos;
	bf.ReadBitVec3Coord(pos);

	//if index == -1 (255) then that means the message was canceled
	if (index == 255)
		return;

	//clamp index and get pos
	index = Clamp<int>(index, 0, MAX_SOUNDSCAPES - 1);

	//send message to settings
	g_SettingsPanel->SetItem(index, pos);
}

//-----------------------------------------------------------------------------
// Purpose: Command to toggle the soundscape panel
//-----------------------------------------------------------------------------
CON_COMMAND(modbase_soundscape_panel, "Toggles the modebase soundscape panel")
{
	g_ShowSoundscapePanel = !g_ShowSoundscapePanel;
	SoundscapeMaker.SetVisible(g_ShowSoundscapePanel);

	//tell player to stop soundscape mode
	static ConCommand* cc = cvar->FindCommand("__ss_maker_stop");
	if (cc)
		cc->Dispatch({});
}