#include "cbase.h"
#include "ColorPicker.h"
#include "fmtstr.h"
#include <vgui/ISurface.h>

using namespace vgui;

//common colors
static const char* s_CommonColorNames[10] =
{
	"Random Color", "Red", "Green", "Blue", "Yellow", "Cyan",
	"Magenta", "White/Default", "Black/Transparent", "Orange"
};

//common color presets
static Color s_CommonColorPresets[10] = {
		Color() /*uses random color*/, Color(255, 0, 0, 255), Color(0, 255, 0, 255), Color(0, 0, 255, 255), Color(255, 255, 0, 255), Color(0, 255, 255, 255),
		Color(255, 0, 255, 255), Color(255, 255, 255, 255), Color(0, 0, 0, 255), Color(255, 165, 0, 255)
};

static const char* const s_CommonColorCmdPrefix = "PresetColor_";

//color picker modal singleton
CColorPicker* s_ColorPickerModal = nullptr;
bool g_bShouldSetColorPicker = false;				//should we get the color of the pixel at the mouse position for CViewRender::RenderView(...)

//-----------------------------------------------------------------------
// Purpose: Color picker window constructor
//-----------------------------------------------------------------------
CColorPicker::CColorPicker(vgui::VPANEL parent) : BaseClass(nullptr, "CColorPicker")
{
	s_ColorPickerModal = this;

	//parent funcs
	SetParent(parent);
	SetSize(630, 290);
	SetMinimumSize(630, 290);
	SetTitle("RGBA Color Picker", true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetDeleteSelfOnClose(true);
	SetSizeable(true);
	SetVisible(true);
	MoveToCenterOfScreen();

	//init the colors
	m_CurrentColor.SetColor(255, 255, 255, 255);
	m_DefaultColor.SetColor(255, 255, 255, 255);

	//create the labels + text entries
	const char* labelNames[4] = { "R:", "G:", "B:", "A:" };
	for (int i = 0; i < 4; i++)
	{
		m_labels[i] = new Label(this, labelNames[i], labelNames[i]);
		m_entries[i] = new TextEntry(this, "entry");
		m_entries[i]->AddActionSignalTarget(this);
		m_entries[i]->SetMaximumCharCount(3);
	}

	//create the common colors
	m_commonColorsLabel = new Label(this, "CommonColorsLabel", "Common Colors");
	for (int i = 0; i < 10; i++)
	{
		m_colorButtons[i] = new Button(this, s_CommonColorNames[i], s_CommonColorNames[i]);
		m_colorButtons[i]->SetCommand(CFmtStr("%s%d", s_CommonColorCmdPrefix, i));
	}

	//create the select + set color things
	m_selectButton = new Button(this, "SelectColor", "Select Color");
	m_selectButton->SetCommand("select_color");

	m_setDefaultButton = new Button(this, "SetDefaultColor", "Set Default");
	m_setDefaultButton->SetCommand("set_default_color");

	//update the text entries
	UpdateTextEntries();
}

//-----------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------
CColorPicker::~CColorPicker()
{
	s_ColorPickerModal = nullptr;
}

//-----------------------------------------------------------------------
// Purpose: Performs the layout for all the elements
//-----------------------------------------------------------------------
void CColorPicker::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize(wide, tall);

	const int padding = 10;
	const int titleOffset = 24;

	int contentTop = titleOffset + padding;
	int contentBottom = tall - padding;

	int buttonHeight = 26;

	int previewSize = (contentBottom - contentTop) - (buttonHeight + padding);
	if (previewSize > wide / 3)
		previewSize = wide / 3;

	m_previewX = padding;
	m_previewY = contentTop;
	m_previewSize = previewSize;

	int middleX = m_previewX + previewSize + padding;
	int middleWidth = 90;

	int rowHeight = 24;
	int entryWidth = 50;

	for (int i = 0; i < 4; i++)
	{
		int y = contentTop + i * (rowHeight + 6);

		m_labels[i]->SetBounds(middleX, y, 20, rowHeight);
		m_entries[i]->SetBounds(middleX + 24, y, entryWidth, rowHeight);
	}

	int buttonColumnX = middleX + middleWidth + padding;
	int buttonColumnWidth = wide - buttonColumnX - padding;

	m_commonColorsLabel->SetBounds(
		buttonColumnX,
		contentTop - 18,
		buttonColumnWidth - 20,
		16
	);

	int presetHeight = 18;

	for (int i = 0; i < 10; i++)
	{
		m_colorButtons[i]->SetBounds(
			buttonColumnX,
			contentTop + i * (presetHeight + 4),
			buttonColumnWidth,
			presetHeight
		);
	}

	int bottomY = tall - buttonHeight - padding;

	m_selectButton->SetBounds(
		padding,
		bottomY,
		buttonColumnX - (padding * 2),
		buttonHeight
	);

	m_setDefaultButton->SetBounds(
		buttonColumnX,
		bottomY,
		buttonColumnWidth,
		buttonHeight
	);
}

//-----------------------------------------------------------------------
// Purpose: Paints the color picker window
//-----------------------------------------------------------------------
void CColorPicker::Paint()
{
	BaseClass::Paint();

	//paint the colored square
	surface()->DrawSetColor(m_CurrentColor.r(), m_CurrentColor.g(), m_CurrentColor.b(), m_CurrentColor.a());
	surface()->DrawFilledRect(m_previewX, m_previewY, m_previewX + m_previewSize, m_previewY + m_previewSize);
}

//-----------------------------------------------------------------------
// Purpose: Paints the color picker window
//-----------------------------------------------------------------------
void CColorPicker::OnTextChanged()
{
	//get the color text entries
	char buf[32];
	m_entries[0]->GetText(buf, sizeof(buf));
	int r = Clamp(atoi(buf), 0, 255);

	m_entries[1]->GetText(buf, sizeof(buf));
	int g = Clamp(atoi(buf), 0, 255);

	m_entries[2]->GetText(buf, sizeof(buf));
	int b = Clamp(atoi(buf), 0, 255);

	m_entries[3]->GetText(buf, sizeof(buf));
	int a = Clamp(atoi(buf), 0, 255);

	//set the color + repaint
	m_CurrentColor.SetColor(r, g, b, a);
	Repaint();
}

//-----------------------------------------------------------------------
// Purpose: Paints the color picker window
//-----------------------------------------------------------------------
void CColorPicker::OnCommand(const char* command)
{
	//check for select color command
	if (!Q_stricmp(command, "select_color"))
	{
		KeyValues* kv = new KeyValues("ColorSelected");
		kv->SetInt("r", m_CurrentColor.r());
		kv->SetInt("g", m_CurrentColor.g());
		kv->SetInt("b", m_CurrentColor.b());
		kv->SetInt("a", m_CurrentColor.a());
		PostActionSignal(kv);
		return;
	}
	
	//set the default color
	else if (!Q_stricmp(command, "set_default_color"))
	{
		SetColor(m_DefaultColor[0], m_DefaultColor[1], m_DefaultColor[2], m_DefaultColor[3]);
		return;
	}

	//check the color preset prefixx
	else if (Q_stristr(command, s_CommonColorCmdPrefix))
	{
		//get the color
		int index = atoi(command + strlen(s_CommonColorCmdPrefix));
		Color color = s_CommonColorPresets[index];
		if (index == 0)
			color = Color(random->RandomInt(0, 255), random->RandomInt(0, 255), random->RandomInt(0, 255), (unsigned char)(255.0f * (1.0f - random->RandomFloatExp(0.0f, 1.0f, 5.0f))));

		//do we use alpha?
		if (!m_bUseAlpha)
			color._color[3] = 255;

		//set the color
		SetColor(color);
	}

	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------
// Purpose: Sets if this uses alpha or not
//-----------------------------------------------------------------------
void CColorPicker::SetUsesAlpha(bool uses)
{
	m_bUseAlpha = uses;

	//update
	if (uses)
	{
		m_entries[3]->SetEnabled(true);
	}
	else
	{
		m_entries[3]->SetEnabled(false);
		m_entries[3]->SetText("255");
	}
}

//-----------------------------------------------------------------------
// Purpose: Sets the default color
//-----------------------------------------------------------------------
void CColorPicker::SetDefaultColor(int r, int g, int b, int a)
{
	m_DefaultColor[0] = r;
	m_DefaultColor[1] = g;
	m_DefaultColor[2] = b;
	m_DefaultColor[3] = m_bUseAlpha ? a : 255;
}

//-----------------------------------------------------------------------
// Purpose: Sets the color
//-----------------------------------------------------------------------
void CColorPicker::SetColor(Color color)
{
	//set then update
	m_CurrentColor = color;

	UpdateTextEntries();
	Repaint();
}

//-----------------------------------------------------------------------
// Purpose: Update the color picker text entries
//-----------------------------------------------------------------------
void CColorPicker::UpdateTextEntries()
{
	char buf[8];

	Q_snprintf(buf, sizeof(buf), "%d", m_CurrentColor.r());
	m_entries[0]->SetText(buf);

	Q_snprintf(buf, sizeof(buf), "%d", m_CurrentColor.g());
	m_entries[1]->SetText(buf);

	Q_snprintf(buf, sizeof(buf), "%d", m_CurrentColor.b());
	m_entries[2]->SetText(buf);

	Q_snprintf(buf, sizeof(buf), "%d", m_CurrentColor.a());
	m_entries[3]->SetText(buf);
}