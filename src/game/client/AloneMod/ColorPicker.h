#ifndef __COLORPICKER_H
#define __COLORPICKER_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>

//vgui color picker window
class CColorPicker : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CColorPicker, Frame);
public:
	CColorPicker(vgui::VPANEL parent);
	~CColorPicker();

	//layout
	virtual void PerformLayout();

	//paint 
	virtual void Paint();

	//message/command funcs
	MESSAGE_FUNC(OnTextChanged, "TextChanged");
	virtual void OnCommand(const char* command);

	//color
	void SetUsesAlpha(bool uses);
	void SetDefaultColor(int r, int g, int b, int a);
	void SetColor(int r, int g, int b, int a) { SetColor(Color(r, g, b, a)); }
	void SetColor(Color color);
private:
	vgui::Label* m_labels[4];
	vgui::TextEntry* m_entries[4];
	vgui::Button* m_colorButtons[10];
	vgui::Button* m_selectButton;
	vgui::Button* m_setDefaultButton;
	vgui::Label* m_commonColorsLabel;

	//colors
	Color m_CurrentColor;
	Color m_DefaultColor;

	//TODO: FIX FOCUS ISSUE

	//for proportionate scaling
	int m_previewX;
	int m_previewY;
	int m_previewSize;
	
	//do we use alpha?
	bool m_bUseAlpha = true;

	//internal setters
	void UpdateTextEntries();

	//next close time
	float m_CloseTime = 0.0f;
	bool m_ShouldClose = false;
};

//all color pickers should be modals. This is the current modal
extern CColorPicker* s_ColorPickerModal;
extern bool g_bShouldSetColorPicker;

#endif