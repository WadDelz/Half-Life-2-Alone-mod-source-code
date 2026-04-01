//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Lens Dirt overlay shader
//
//=============================================================================//
#include "BaseVSShader.h"
#include "convar.h"
#include "screenspaceeffect_vs20.inc"
#include "lensdirt_ps20.inc"

//lens dirt vars
static ConVar amod_lensdirt_intensity("amod_lensdirt_intensity", "1.0");
static ConVar amod_lensdirt_alpha("amod_lensdirt_alpha", "0.775");

//actual shader
BEGIN_VS_SHADER_FLAGS(LENSDIRT, "Lens Dirt Overlay", SHADER_NOT_EDITABLE)
    BEGIN_SHADER_PARAMS
    END_SHADER_PARAMS
    
    //init params func
    SHADER_INIT_PARAMS()
    {
        SET_FLAGS2(MATERIAL_VAR2_NEEDS_FULL_FRAME_BUFFER_TEXTURE);
    }
    
	//init func
    SHADER_INIT
    {
        LoadTexture(BASETEXTURE);
    }
    
	//fallback func
    SHADER_FALLBACK
    {
        if (g_pHardwareConfig->GetDXSupportLevel() < 90)
            return "Wireframe";
        return 0;
    }
    
	//drawing func
    SHADER_DRAW
    {
        //shadow state
        SHADOW_STATE
        {
            pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
            pShaderShadow->EnableTexture(SHADER_SAMPLER1, true);
    
            int fmt = VERTEX_POSITION;
            pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);
    
            DECLARE_STATIC_VERTEX_SHADER(screenspaceeffect_vs20);
            SET_STATIC_VERTEX_SHADER(screenspaceeffect_vs20);
    
            DECLARE_STATIC_PIXEL_SHADER(lensdirt_ps20);
            SET_STATIC_PIXEL_SHADER(lensdirt_ps20);
        }
    
        //dynamic state
        DYNAMIC_STATE
        {
            pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);
            BindTexture(SHADER_SAMPLER1, BASETEXTURE);
    
            DECLARE_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20);
            SET_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20);
    
            float params[4];
            params[0] = amod_lensdirt_intensity.GetFloat();
            params[1] = amod_lensdirt_alpha.GetFloat();
            params[2] = 0.0f;
            params[3] = 0.0f;
    
            pShaderAPI->SetPixelShaderConstant(0, params, 1);
        }
    
        Draw();
    }
END_SHADER