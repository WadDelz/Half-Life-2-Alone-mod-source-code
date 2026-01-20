//======== Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: modbase dynamic sky
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef __DYNAMICSKY_H
#define __DYNAMICSKY_H

#ifdef _WIN32
#pragma once
#endif

#define USES_DYNAMIC_SKY 1
#define DRAW_USES_DYNAMIC_SKY !USES_DYNAMIC_SKY_NEWFUNC && USES_DYNAMIC_SKY

#ifdef LINUX
#define USES_DYNAMIC_SKY_NEWFUNC 1							//im going to try out the new system i created that actually calls engine code instead of engine code coppied to the client. If it goes to shit then i will set this to 0.
#endif

void ModBase_UnloadSkys(void);
void ModBase_LoadSkys(void);
void ModBase_DrawSkyBox(float zFar, int nDrawFlags = 0x3F);

//boolean used to see if the mod uses default skybox system or dynamic skybox system
extern bool g_PModBase_DynamicSkybox_bUse;
extern QAngle g_PModBase_DynamicSkybox_Angle;

#endif