//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VGUI panel which can play back video, in-engine
//
//=============================================================================

#include "cbase.h"
#include <vgui/IVGui.h>
#include "vgui/IInput.h"
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui_video.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//only 1 video panel allowed at a time
VideoPanel* g_VideoPanelSingleton = nullptr;

VideoPanel::VideoPanel(unsigned int nXPos, unsigned int nYPos, unsigned int nHeight, unsigned int nWidth, bool allowAlternateMedia, bool bAllowEscape) :
	BaseClass(NULL, "VideoPanel"),
	m_VideoMaterial(NULL),
	m_nPlaybackWidth(0),
	m_nPlaybackHeight(0),
	m_bAllowAlternateMedia(allowAlternateMedia),
	m_bAllowEscape(bAllowEscape)
{
	SetParent(enginevgui->GetPanel(PANEL_CLIENTDLL_TOOLS));
	SetBuildModeEditable(false);
	SetVisible(false);
	SetPaintEnabled(false);
	SetProportional(true);
	SetKeyBoardInputEnabled(true);
	SetPaintBorderEnabled(false);

	//clear the exit command
	SetExitCommand("");

	// Set us up
	SetSize(nWidth, nHeight);
	SetPos(nXPos, nYPos);
	SetZPos(100);
}

//-----------------------------------------------------------------------------
// Properly shutdown out materials
//-----------------------------------------------------------------------------
VideoPanel::~VideoPanel(void)
{
	//notify of end movie playback
	enginesound->NotifyEndMoviePlayback();

	//release the video
	ReleaseVideo();

	//clear g_VideoPanelSingleton
	g_VideoPanelSingleton = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Begins playback of a movie
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel::BeginPlayback(const char* pFilename)
{
	//set visible and enabled
	SetVisible(true);
	SetPaintEnabled(true);
	RequestFocus();
	MoveToFront();

	// Need working video services
	if (g_pVideo == NULL)
		return false;

	//check for old g_VideoPanelSingleton
	if (g_VideoPanelSingleton)
	{
		g_VideoPanelSingleton->OnVideoOver();
		delete g_VideoPanelSingleton;
		g_VideoPanelSingleton = nullptr;
	}

	// Destroy any previously allocated video
	if (m_VideoMaterial != NULL)
	{
		g_pVideo->DestroyVideoMaterial(m_VideoMaterial);
		m_VideoMaterial = NULL;
	}

	//notify of movie playback
	enginesound->NotifyBeginMoviePlayback();

	// Create new Video material
	m_VideoMaterial = g_pVideo->CreateVideoMaterial("VideoMaterial", pFilename, "GAME",
		VideoPlaybackFlags::DEFAULT_MATERIAL_OPTIONS,
		VideoSystem::DETERMINE_FROM_FILE_EXTENSION, m_bAllowAlternateMedia);
	if (m_VideoMaterial == NULL)
		return false;

	int nWidth, nHeight;
	m_VideoMaterial->GetVideoImageSize(&nWidth, &nHeight);
	m_VideoMaterial->GetVideoTexCoordRange(&m_flU, &m_flV);
	m_pMaterial = m_VideoMaterial->GetMaterial();

	//set g_VideoPanelSingleton to this
	g_VideoPanelSingleton = this;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::ReleaseVideo(void)
{
	// Destroy any previously allocated video
	// Shut down this video, destroy the video material
	if (g_pVideo != nullptr && m_VideoMaterial != nullptr)
	{
		g_pVideo->DestroyVideoMaterial(m_VideoMaterial);
		m_VideoMaterial = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::OnVideoOver()
{
	// Fire an exit command if we're asked to do so
	if (m_szExitCommand && m_szExitCommand[0])
	{
		engine->ClientCmd(m_szExitCommand);
	}

	//set this panel's visibility
	SetVisible(false);
	SetPaintEnabled(false);

	//mark this panel for DELETION
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::DoModal(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Handle keys that should cause us to close
//-----------------------------------------------------------------------------
void VideoPanel::HandleKeyPress(vgui::KeyCode code)
{
	//see if we are allowed to escape or not
	if (!m_bAllowEscape)
		return;

	//check for these keys
	if (code == KEY_SPACE || code == KEY_ENTER || code == KEY_W || code == KEY_A || code == KEY_S || code == KEY_D)
	{
		//close the panel
		OnVideoOver();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VideoPanel::GetPanelPos(int& xpos, int& ypos)
{
	vgui::ipanel()->GetAbsPos(GetVPanel(), xpos, ypos);
}

//-----------------------------------------------------------------------------
// Purpose: Update and draw the frame
//-----------------------------------------------------------------------------
void VideoPanel::Paint(void)
{
	//check for video material
	if (m_VideoMaterial == NULL)
		return;

	//check for more updates
	if (m_VideoMaterial->Update() == false)
	{
		OnVideoOver();
		return;
	}

	//set playback width and height
	m_nPlaybackHeight = ScreenHeight() + 2;
	m_nPlaybackWidth = ScreenWidth() + 2;

	// Sit in the "center"
	int xpos, ypos;
	GetPanelPos(xpos, ypos);

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext(materials);

	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind(m_pMaterial, NULL);

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh(true);
	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

	float flLeftX = xpos;
	float flRightX = xpos + (m_nPlaybackWidth - 1);

	float flTopY = ypos;
	float flBottomY = ypos + (m_nPlaybackHeight - 1);

	// Map our UVs to cut out just the portion of the video we're interested in
	float flLeftU = 0.0f;
	float flTopV = 0.0f;

	// We need to subtract off a pixel to make sure we don't bleed
	float flRightU = m_flU - (1.0f / (float)m_nPlaybackWidth);
	float flBottomV = m_flV - (1.0f / (float)m_nPlaybackHeight);

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport(vx, vy, vw, vh);

	// map from screen pixel coords to -1..1
	flRightX = FLerp(-1, 1, 0, vw, flRightX);
	flLeftX = FLerp(-1, 1, 0, vw, flLeftX);
	flTopY = FLerp(1, -1, 0, vh, flTopY);
	flBottomY = FLerp(1, -1, 0, vh, flBottomY);

	float alpha = ((float)GetFgColor()[3] / 255.0f);

	for (int corner = 0; corner < 4; corner++)
	{
		bool bLeft = (corner == 0) || (corner == 3);
		meshBuilder.Position3f((bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f);
		meshBuilder.Normal3f(0.0f, 0.0f, 1.0f);
		meshBuilder.TexCoord2f(0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV);
		meshBuilder.TangentS3f(0.0f, 1.0f, 0.0f);
		meshBuilder.TangentT3f(1.0f, 0.0f, 0.0f);
		meshBuilder.Color4f(1.0f, 1.0f, 1.0f, alpha);
		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode(MATERIAL_VIEW);
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode(MATERIAL_PROJECTION);
	pRenderContext->PopMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: Create and playback a video in a panel
// Input  : nWidth - Width of panel (in pixels) 
//			nHeight - Height of panel
//			*pVideoFilename - Name of the file to play
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool VideoPanel_Create(unsigned int nXPos, unsigned int nYPos,
	unsigned int nWidth, unsigned int nHeight,
	const char* pVideoFilename,
	const char* pExitCommand, /*= NULL*/
	bool bCanEscape /*= true */)
{
	// Create the base video panel
	VideoPanel* pVideoPanel = new VideoPanel(nXPos, nYPos, nHeight, nWidth, false, bCanEscape);
	if (pVideoPanel == NULL)
		return false;

	// Set the command we'll call (if any) when the video is interrupted or completes
	pVideoPanel->SetExitCommand(pExitCommand);

	// Start it going
	if (pVideoPanel->BeginPlayback(pVideoFilename) == false)
	{
		delete pVideoPanel;
		return false;
	}

	// Take control
	pVideoPanel->DoModal();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback (Debug) -
//  user must include file extension
//-----------------------------------------------------------------------------
extern ConVar amod_new_ending;

CON_COMMAND(playvideo, "Plays a video: <filename> [width height]")
{
	if (args.ArgC() < 2)
		return;

	const char* movie = args.Arg(1);
	if (!Q_strcmp(movie, "Amod_OutroVideo") && amod_new_ending.GetBool())
		movie = "Amod_OutroVideo2";

	unsigned int nScreenWidth = Q_atoi(args[2]);
	unsigned int nScreenHeight = Q_atoi(args[3]);

	char strFullpath[MAX_PATH];
	Q_strncpy(strFullpath, "media/", MAX_PATH);	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension(movie, strFilename, MAX_PATH);
	Q_strncat(strFullpath, movie, MAX_PATH);

	if (nScreenWidth == 0)
	{
		nScreenWidth = ScreenWidth();
	}

	if (nScreenHeight == 0)
	{
		nScreenHeight = ScreenHeight();
	}

	char command[256] = { 0 };
	if (!Q_strcmp(args.Arg(1), "Amod_OutroVideo"))
	{
		vgui::surface()->SurfaceSetCursorPos(9999, 9999);
		Q_strcpy(command, "ent_fire logic_ending_credits trigger");
	}
	// Create the panel and go!
	if (VideoPanel_Create(0, 0, nScreenWidth, nScreenHeight, strFullpath, command, false) == false)
	{
		Warning("Unable to play video: %s\n", strFullpath);
		engine->ExecuteClientCmd(command);
	}
	else
	{
		engine->ClientCmd("gameui_hide");
	}
}

CON_COMMAND(amod_playvideo, "Plays a video: <filename> <command after finished> [width height]")
{
	if (args.ArgC() < 2)
		return;

	const char* movie = args.Arg(1);

	unsigned int nScreenWidth = Q_atoi(args[3]);
	unsigned int nScreenHeight = Q_atoi(args[4]);

	char strFullpath[MAX_PATH];
	Q_strncpy(strFullpath, "media/", MAX_PATH);	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension(movie, strFilename, MAX_PATH);
	Q_strncat(strFullpath, movie, MAX_PATH);

	if (nScreenWidth == 0)
	{
		nScreenWidth = ScreenWidth();
	}

	if (nScreenHeight == 0)
	{
		nScreenHeight = ScreenHeight();
	}

	char command[512];
	Q_snprintf(command, sizeof(command), "%s", args.Arg(2));

	// Create the panel and go!
	if (VideoPanel_Create(0, 0, nScreenWidth, nScreenHeight, strFullpath, command) == false)
	{
		Warning("Unable to play video: %s\n", strFullpath);
		engine->ExecuteClientCmd(command);
	}
	else
	{
		//hide the gameui
		engine->ClientCmd("gameui_hide");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Used to launch a video playback and fire a command on completion
//-----------------------------------------------------------------------------

CON_COMMAND(playvideo_exitcommand, "Plays a video and fires and exit command when it is stopped or finishes: <filename> <exit command>")
{
	if (args.ArgC() < 2)
		return;

	unsigned int nScreenWidth = ScreenWidth();
	unsigned int nScreenHeight = ScreenHeight();

	char strFullpath[MAX_PATH];
	Q_strncpy(strFullpath, "media/", MAX_PATH);	// Assume we must play out of the media directory
	char strFilename[MAX_PATH];
	Q_StripExtension(args[1], strFilename, MAX_PATH);
	Q_strncat(strFullpath, args[1], MAX_PATH);

	char* pExitCommand = Q_strstr(args.GetCommandString(), args[2]);

	// Create the panel and go!
	if (VideoPanel_Create(0, 0, nScreenWidth, nScreenHeight, strFullpath, pExitCommand) == false)
	{
		Warning("Unable to play video: %s\n", strFullpath);
		engine->ClientCmd(pExitCommand);
	}
}

//system to delete the movie on level shutdown
class CAutoVideoPanelRemoverSystem : public CAutoGameSystem
{
	//called on level shutdown
	void LevelShutdownPreEntity()
	{
		//check for panel
		if (g_VideoPanelSingleton)
			g_VideoPanelSingleton->OnVideoOver();
	}
};
static CAutoVideoPanelRemoverSystem g_VideoRemoverSystem;