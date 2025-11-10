//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Hit Marker
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_hitmarker.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "fmtstr.h"

// VGUI panel includes
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(CHudHitmarker);
DECLARE_HUD_MESSAGE(CHudHitmarker, ShowHitmarker);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHitmarker::CHudHitmarker(const char* pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudHitmarker")
{
	vgui::Panel* pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// Hitmarker will not show when the player is dead
	SetHiddenBits(HIDEHUD_PLAYERDEAD);

	int screenWide, screenTall;
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Init()
{
	HOOK_HUD_MESSAGE(CHudHitmarker, ShowHitmarker);

	SetAlpha(0);
	m_bHitmarkerShow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Reset()
{
	SetAlpha(0);
	m_bHitmarkerShow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::ApplySchemeSettings(vgui::IScheme* scheme)
{
	m_hFont = vgui::surface()->CreateFont();
	vgui::surface()->SetFontGlyphSet(m_hFont, "Tahoma", 30, 400, 0, 0, vgui::ISurface::FONTFLAG_OUTLINE);

	BaseClass::ApplySchemeSettings(scheme);

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHitmarker::ShouldDraw(void)
{
	return (m_bHitmarkerShow && CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// Purpose: Helper function
//-----------------------------------------------------------------------------
const wchar_t* ConvertToWide(const char* input)
{
	static wchar_t wide[32];
	mbstowcs(wide, input, 32);
	return wide;
}

int ScreenTransform(const Vector& point, Vector& screen);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Paint(void)
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (m_bHitmarkerShow)
	{
		//get position on screen
		Vector screenPos;
		int result = ScreenTransform(m_Vec, screenPos);

		// 1 means behind the camera
		if (result == 1)
			return;

		//get pos
		int x = (int)(0.5f * (1.0f + screenPos.x) * ScreenWidth());
		int y = (int)(0.5f * (1.0f - screenPos.y) * ScreenHeight());

		vgui::surface()->DrawSetColor(m_HitmarkerColor);
		vgui::surface()->DrawLine(x - 6, y - 5, x - 11, y - 10);
		vgui::surface()->DrawLine(x + 5, y - 5, x + 10, y - 10);
		vgui::surface()->DrawLine(x - 6, y + 5, x - 11, y + 10);
		vgui::surface()->DrawLine(x + 5, y + 5, x + 10, y + 10);

		//set print stuff
		vgui::surface()->DrawSetTextFont(m_hFont);
		vgui::surface()->DrawSetTextColor(m_HitmarkerColor);
		vgui::surface()->DrawSetTextPos(x, y);

		//print text
		const wchar_t* text = ConvertToWide(CFmtStr("%d", m_iDamage)); // Text to display
		vgui::surface()->DrawPrintText(text, wcslen(text)); // Draw the text
	}
}

ConVar cl_show_hitmarker("cl_show_hitmarker", "0");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::MsgFunc_ShowHitmarker(bf_read& msg)
{
	//get the stuff needed
	m_bHitmarkerShow = msg.ReadByte();
	bool alive = msg.ReadByte();
	m_iDamage = msg.ReadByte();
	msg.ReadBitVec3Coord(m_Vec);

	if (!cl_show_hitmarker.GetBool())
		return;

	//check for alive or not
	if (alive)
		m_HitmarkerColor.SetColor(250, 235, 235, 255);
	else
		m_HitmarkerColor.SetColor(250, 10, 10, 255);

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HitMarkerShow");
}