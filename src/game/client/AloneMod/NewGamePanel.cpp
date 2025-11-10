// aint no way i got this to work
// i had to make the new "NewGamePanel" because only 32 chapters are alowed on the base hl2 new game panel at a time
#include "cbase.h"
#include "fmtstr.h"
#include "filesystem.h"
#include "INewGamePanel.h"
#include <vgui/IVGui.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Divider.h>
#include <vgui_controls/CheckButton.h>
#include <ienginevgui.h>
#include <vgui/ISurface.h>
#include "fmtstr.h"
#include "AloneMod/Amod_SharedDefs.h"

using namespace vgui;

ConVar cl_drawnewgamepanel("cl_drawnewgamepanel", "0", FCVAR_CLIENTDLL | FCVAR_HIDDEN, "Sets the state of the alone mod New Game panel");

static bool bUsingHl1Panel = false;

#if !AMOD_DAYTIME_EDITION
extern ConVar amod_day;

#define IsDay(a) (!bUsingHl1Panel && amod_day.GetBool() && (ButtonExp + a > 48 && ButtonExp + a < 69))
#else
#define IsDay(a) false
#endif

class CNewGamePanel : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CNewGamePanel, vgui::Frame);

	CNewGamePanel(vgui::VPANEL parent, bool hl1panel);
	~CNewGamePanel() {};

	void DoDayCheck();

	void OnClose();
protected:
	//VGUI overrides:
	virtual void OnTick();
	virtual void OnCommand(const char* pcCommand);

private:
	int ButtonExp = 0;
	Button* chapter1b, * chapter2b, * chapter3b, * next, * prev;
	Label* chapter1l, * chapter2l, * chapter3l;
	ImagePanel* ImageC1, * ImageC2, * ImageC3;

	CUtlVector<const char*> ChapterNames;
	void Init(bool hl1panel);
};

CNewGamePanel::CNewGamePanel(vgui::VPANEL parent, bool hl1panel)
	: BaseClass(NULL, "NewGamePanel")
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
	SetVisible(true);

	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));

	vgui::ivgui()->AddTickSignal(GetVPanel(), 50);

	Init(hl1panel);
}

void CNewGamePanel::Init(bool hl1panel)
{
	KeyValues* kv = new KeyValues("");
	kv->LoadFromFile(filesystem, "resource/HL2_AloneMod_english.txt", "MOD");
	KeyValues* sub = kv->GetFirstTrueSubKey();

	if (!hl1panel)
	{
		for (int i = 1; i <= 101; i++)
		{
			if (!filesystem->FileExists(CFmtStr("cfg/hl2/chapter%d.cfg", i)))
				break;

			ChapterNames.AddToTail(sub->GetString(CFmtStr("HL2_Chapter%d_Title", i)));
		}
	}
	else
	{
		for (int i = 1; i <= 101; i++)
		{
			if (!filesystem->FileExists(CFmtStr("cfg/hl1/chapter%d.cfg", i)))
				break;

			ChapterNames.AddToTail(sub->GetString(CFmtStr("HL1_Chapter%d_Title", i)));
		}
	}

	SetTitle("Chapter Select", false);
	SetSize(520, 193);

	int screenWidth, screenHeight;
	vgui::surface()->GetScreenSize(screenWidth, screenHeight);

	int xPos = (screenWidth - 520) / 2;
	int yPos = (screenHeight - 193) / 2;

	SetPos(xPos, yPos);

	ImageC1 = new ImagePanel(this, "ImageC1");
	ImageC1->SetBounds(10, 40, 160, 90);
	ImageC1->SetImage(CFmtStr("chapters/%s/chapter1", bUsingHl1Panel ? "hl1" : "hl2"));

	ImageC2 = new ImagePanel(this, "ImageC2");
	ImageC2->SetBounds(180, 40, 160, 90);
	ImageC2->SetImage(CFmtStr("chapters/%s/chapter2", bUsingHl1Panel ? "hl1" : "hl2"));

	ImageC3 = new ImagePanel(this, "ImageC2");
	ImageC3->SetBounds(350, 40, 160, 90);
	ImageC3->SetImage(CFmtStr("chapters/%s/chapter3", bUsingHl1Panel ? "hl1" : "hl2"));

	chapter1l = new Label(this, "Chapter1l", ChapterNames[0]);
	chapter1l->SetBounds(10, 20, 170, 20);

	chapter2l = new Label(this, "Chapter2l", ChapterNames[1]);
	chapter2l->SetBounds(180, 20, 165, 20);

	chapter3l = new Label(this, "Chapter3l", ChapterNames[2]);
	chapter3l->SetBounds(350, 20, 165, 20);

	//Divider* div1 = new Divider(this, "div1");
	//div1->SetBounds(5, 23, 510, 2);

	prev = new Button(this, "prev", "Previous");
	prev->SetBounds(10, 165, 80, 25);
	prev->SetCommand("Prev");
	prev->SetEnabled(false);

	next = new Button(this, "next", "Next");
	next->SetBounds(430, 165, 80, 25);
	next->SetCommand("Next");

	Divider* div2 = new Divider(this, "div1");
	div2->SetBounds(5, 158, 510, 2);

	chapter1b = new Button(this, "Chapter1b", "Load Chapter 1");
	chapter1b->SetBounds(10, 135, 160, 20);
	chapter1b->SetCommand("chapter1");

	chapter2b = new Button(this, "Chapter2b", "Load Chapter 2");
	chapter2b->SetBounds(180, 135, 160, 20);
	chapter2b->SetCommand("chapter2");

	chapter3b = new Button(this, "Chapter3b", "Load Chapter 3");
	chapter3b->SetBounds(350, 135, 160, 20);
	chapter3b->SetCommand("chapter3");

	//oldpanelbutfornew = new CheckButton(this, "oldbuttonfornewpanel", "Use Other Panel");
	//oldpanelbutfornew->SetCommand("OBPress");
	//oldpanelbutfornew->SetBounds(186, 165, 150, 25);
}

class CNGPanellInterface : public NewGamePanel
{
private:
	CNewGamePanel* NPanel;
public:
	CNGPanellInterface()
	{
		NPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		NPanel = new CNewGamePanel(parent, false);
	}
	void Init(bool hl1panel)
	{
		bUsingHl1Panel = hl1panel;

		if (NPanel)
			NPanel->DeletePanel();

		NPanel = new CNewGamePanel(enginevgui->GetPanel(VGuiPanel_t::PANEL_GAMEUIDLL), hl1panel);
		NPanel->SetVisible(true);
		NPanel->RequestFocus();
		NPanel->MoveToFront();
	}
	void Destroy()
	{
		if (NPanel)
		{
			NPanel->SetParent((vgui::Panel*)NULL);
			delete NPanel;
			NPanel = nullptr;
		}
	}
	void Activate(void)
	{
		if (NPanel)
		{
			NPanel->Activate();
			NPanel->MoveToFront();
		}
	}

	void DayCheck()
	{
		if (NPanel)
		{
			NPanel->DoDayCheck();
		}
	}
};
static CNGPanellInterface g_NGanel;
NewGamePanel* newgamepanel = (NewGamePanel*)&g_NGanel;

void CNewGamePanel::OnTick()
{
	BaseClass::OnTick();
	SetVisible(cl_drawnewgamepanel.GetBool());
}

CON_COMMAND_F(ToggleNewGamePanel, "Toggles The Alone Mod New Game Panel", FCVAR_HIDDEN)
{
	if (bUsingHl1Panel)
	{
		bUsingHl1Panel = false;
		newgamepanel->Init(false);
		cl_drawnewgamepanel.SetValue(true);
		return;
	}

	cl_drawnewgamepanel.SetValue(!cl_drawnewgamepanel.GetBool());
	newgamepanel->Activate();
};

CON_COMMAND_F(Togglehl1NewGamePanel, "Toggles The Alone Mod hl1 New Game Panel", FCVAR_HIDDEN)
{
	if (!bUsingHl1Panel)
	{
		bUsingHl1Panel = true;
		newgamepanel->Init(true);
		return;
	}

	cl_drawnewgamepanel.SetValue(!cl_drawnewgamepanel.GetBool());
	newgamepanel->Activate();
};

void CNewGamePanel::OnClose()
{
	cl_drawnewgamepanel.SetValue(0);
}

//i have absolutly 0 idea how to use % so i just did this
void CNewGamePanel::OnCommand(const char* pcCommand)
{
	static ConVar* commentary = cvar->FindVar("commentary");

	BaseClass::OnCommand(pcCommand);

	if (!Q_strcmp(pcCommand, "Prev"))
	{
		if (ButtonExp == 0)
			return;

		chapter2b->SetSize(160, 20);
		chapter3b->SetSize(160, 20);

		ImageC2->SetSize(160, 90);
		ImageC3->SetSize(160, 90);

		chapter1b->SetText(CFmtStr("Load Chapter %d", ButtonExp - 2));
		chapter2b->SetText(CFmtStr("Load Chapter %d", ButtonExp - 1));
		chapter3b->SetText(CFmtStr("Load Chapter %d", ButtonExp));

		next->SetEnabled(true);

		if (ButtonExp == 3)
		{
			prev->SetEnabled(false);
		}

		next->SetEnabled(true);
		ButtonExp -= 3;
		chapter1l->SetText(ChapterNames[ButtonExp]);
		ImageC1->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 1, IsDay(1) ? "_day" : ""));

		if (ChapterNames.Size() > ButtonExp + 1)
		{
			chapter2l->SetText(ChapterNames[ButtonExp + 1]);
			ImageC2->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 2, IsDay(2) ? "_day" : ""));
		}
		else
		{
			chapter2l->SetText("");
		}

		if (ChapterNames.Size() > ButtonExp + 2)
		{
			chapter3l->SetText(ChapterNames[ButtonExp + 2]);
			ImageC3->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 3, IsDay(3) ? "_day" : ""));
		}
		else
		{
			chapter3l->SetText("");
		}
	}
	else if (!Q_strcmp(pcCommand, "Next"))
	{
		prev->SetEnabled(true);

		ButtonExp += 3;
		if (ButtonExp + 3 >= ChapterNames.Size())
			next->SetEnabled(false);

		chapter1b->SetText(CFmtStr("Load Chapter %d", ButtonExp + 1));

		if (ButtonExp + 1 < ChapterNames.Size())
		{
			chapter2b->SetText(CFmtStr("Load Chapter %d", ButtonExp + 2));
			if (ButtonExp + 2 < ChapterNames.Size())
			{
				chapter3b->SetText(CFmtStr("Load Chapter %d", ButtonExp + 3));
			}
			else
			{
				chapter3b->SetSize(0, 0);
				ImageC3->SetSize(0, 0);
			}
		}
		else
		{
			chapter2b->SetSize(0, 0);
			chapter3b->SetSize(0, 0);
			ImageC2->SetSize(0, 0);
			ImageC3->SetSize(0, 0);
		}

		chapter1l->SetText(ChapterNames[ButtonExp]);
		ImageC1->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 1, IsDay(1) ? "_day" : ""));

		if (ChapterNames.Size() > ButtonExp + 1)
		{
			chapter2l->SetText(ChapterNames[ButtonExp + 1]);
			ImageC2->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 2, IsDay(2) ? "_day" : ""));
		}
		else
		{
			chapter2l->SetText("");
		}

		if (ChapterNames.Size() > ButtonExp + 2)
		{
			chapter3l->SetText(ChapterNames[ButtonExp + 2]);
			ImageC3->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 3, IsDay(3) ? "_day" : ""));
		}
		else
		{
			chapter3l->SetText("");
		}
	}
	else if (!Q_strcmp(pcCommand, "chapter1"))
	{
		if (commentary) commentary->SetValue(0);
		engine->ClientCmd(CFmtStr("exec %s/chapter%d", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 1));
	}
	else if (!Q_strcmp(pcCommand, "chapter2"))
	{
		if (commentary) commentary->SetValue(0);
		engine->ClientCmd(CFmtStr("exec %s/chapter%d", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 2));
	}
	else if (!Q_strcmp(pcCommand, "chapter3"))
	{
		if (commentary) commentary->SetValue(0);
		engine->ClientCmd(CFmtStr("exec %s/chapter%d", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 3));
	}
}

void CNewGamePanel::DoDayCheck()
{
	if (bUsingHl1Panel)
		return;

	ImageC1->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 1, IsDay(1) ? "_day" : ""));

	if (ChapterNames.Size() > ButtonExp + 1)
		ImageC2->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 2, IsDay(2) ? "_day" : ""));

	if (ChapterNames.Size() > ButtonExp + 2)
		ImageC3->SetImage(CFmtStr1024("chapters/%s/chapter%d%s", bUsingHl1Panel ? "hl1" : "hl2", ButtonExp + 3, IsDay(3) ? "_day" : ""));
}

void AmodDayChangeCallback(IConVar* var, const char*, float)
{
	g_NGanel.DayCheck();

	ConMsg("%d %d %d %s\n", engine->IsInGame(), !engine->IsLevelMainMenuBackground(), IsCityMap(szMapName), szMapName);

	if (engine->IsInGame() && !engine->IsLevelMainMenuBackground() && IsCityMap(szMapName))
		engine->ExecuteClientCmd("save tmp_day001; load tmp_day001");
	else
	{
		if (engine->IsInGame() && engine->IsLevelMainMenuBackground() && !Q_strcmp(szMapName, "background06_d"))
			engine->ExecuteClientCmd("map_background background06_d");
		else if (engine->IsInGame() && engine->IsLevelMainMenuBackground() && !Q_strcmp(szMapName, "background07_d"))
			engine->ExecuteClientCmd("map_background background06_d");
	}

#if !AMOD_DAYTIME_EDITION
	if (!amod_day.GetBool())
		engine->ClientCmd_Unrestricted("tf1; alias Amod_ToggleFilter tf2");
#endif
}

void AmodDayRavChangeCallback(IConVar* var, const char*, float)
{
	g_NGanel.DayCheck();

	if (engine->IsInGame() && !engine->IsLevelMainMenuBackground() && Q_strcmp(szMapName, "d1_town_05_d") && Q_strstr(szMapName, "d1_town_"))
		engine->ExecuteClientCmd("save tmp_day001; load tmp_day001");

#if !AMOD_DAYTIME_EDITION
	if (!amod_day.GetBool())
		engine->ClientCmd_Unrestricted("tf1; alias Amod_ToggleFilter tf2");
#endif
}