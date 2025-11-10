//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#ifndef VGUI_VIDEO_H
#define VGUI_VIDEO_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/EditablePanel.h>

//#define QUICKTIME_VIDEO
//#define BINK_VIDEO

#include "video/ivideoservices.h"


class VideoPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( VideoPanel, vgui::Panel);
public:

	VideoPanel( unsigned int nXPos, unsigned int nYPos, unsigned int nHeight, unsigned int nWidth, bool allowAlternateMedia = true, bool bAllowEscape = true);

	virtual ~VideoPanel( void );

	virtual bool BeginPlayback(const char* pFilename);
	virtual void ReleaseVideo();

	virtual void HandleKeyPress( vgui::KeyCode code );

	virtual void OnVideoOver();

	virtual void Paint(void);
	virtual void DoModal(void);
	virtual void GetPanelPos( int &xpos, int &ypos );

	void SetExitCommand( const char *pExitCommand )
	{
		if ( pExitCommand && pExitCommand[0] )
		{
			Q_strncpy( m_szExitCommand, pExitCommand, MAX_PATH );
		}
		else
		{
			m_szExitCommand[0] = '\0';
		}
	}

protected:

	virtual void OnTick( void ) { BaseClass::OnTick(); }
	virtual void OnCommand( const char *pcCommand ) { BaseClass::OnCommand( pcCommand ); }

protected:
	IVideoMaterial *m_VideoMaterial;
	
	IMaterial		*m_pMaterial;
	int				m_nPlaybackHeight;			// Calculated to address ratio changes
	int				m_nPlaybackWidth;
	char			m_szExitCommand[MAX_PATH];	// This call is fired at the engine when the video finishes or is interrupted

	float			m_flU;	// U,V ranges for video on its sheet
	float			m_flV;

	bool			m_bAllowAlternateMedia;

	bool			m_bAllowEscape;
};

//video panel singleton
extern VideoPanel* g_VideoPanelSingleton;

// Creates a VGUI panel which plays a video and executes a client command at its finish (if specified)
extern bool VideoPanel_Create( unsigned int nXPos, unsigned int nYPos, 
							   unsigned int nWidth, unsigned int nHeight, 
							   const char *pVideoFilename, 
							   const char *pExitCommand = NULL,
							   bool bCanEscape = true);

#endif // VGUI_VIDEO_H
