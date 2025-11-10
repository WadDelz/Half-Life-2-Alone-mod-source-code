//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: a vgui panel that allows you to create and test soundscapes in game
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __VGUI_SOUNDSCAPE_MAKER_H
#define __VGUI_SOUNDSCAPE_MAKER_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui/VGUI.h"

//interface for soundscape maker panel
class ISoundscapeMaker
{
public:
	virtual void Create(vgui::VPANEL parent) = 0;
	virtual void Destroy() = 0;
	virtual void SetSoundText(const char* text) = 0;
	virtual void SetAllVisible(bool bVisible) = 0;
	virtual void SetBuffer(const char* text) = 0;
	virtual KeyValues* GetPanelFile() = 0;
	virtual KeyValues* GetPanelSelected() = 0;
	virtual void PasteFromClipboard(int type) = 0;
};

extern ISoundscapeMaker* g_SoundscapeMaker;

#endif //__VGUI_SOUNDSCAPE_MAKER_H