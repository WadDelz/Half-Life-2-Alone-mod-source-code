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

void ModBase_UnloadSkys(void);
void ModBase_LoadSkys(void);
void ModBase_DrawSkyBox(float zFar, int nDrawFlags = 0x3F);


//boolean used to see if the mod uses default skybox system or dynamic skybox system
extern bool g_PModBase_DynamicSkybox_bUse;
extern QAngle g_PModBase_DynamicSkybox_Angle;

#endif