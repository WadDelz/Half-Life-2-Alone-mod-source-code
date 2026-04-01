//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============// 
// 
// Purpose: Blur overlay shader 
// 
//=============================================================================// 
#include "BaseVSShader.h"
#include "convar.h"
#include "screenspaceeffect_vs20.inc"
#include "blur_ps20.inc"

//blur vars
static ConVar amod_blur_amount("amod_blur_amount", "0.5");

//actual shader
BEGIN_VS_SHADER_FLAGS(BLUR, "Screenspace Blur", SHADER_NOT_EDITABLE)
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
    }
    
	//fallback func
    SHADER_FALLBACK
    {
        if (g_pHardwareConfig->GetDXSupportLevel() < 90)
        {
            Assert(0);
            return "Wireframe";
        }
        return 0;
    }
    
	//drawing func
    SHADER_DRAW
    {
		//shadow state
        SHADOW_STATE
        {
            pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
    
            int fmt = VERTEX_POSITION;
            pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0);
    
            DECLARE_STATIC_VERTEX_SHADER(screenspaceeffect_vs20);
            SET_STATIC_VERTEX_SHADER(screenspaceeffect_vs20);
    
            DECLARE_STATIC_PIXEL_SHADER(blur_ps20);
            SET_STATIC_PIXEL_SHADER(blur_ps20);
        }
    
        //dynamic state
        DYNAMIC_STATE
        {
            pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0);
    
            DECLARE_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20);
            SET_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20);
    
            float blur[4] = { amod_blur_amount.GetFloat(), 0, 0, 0 };
            pShaderAPI->SetPixelShaderConstant(0, blur, 1);
        }
    
        Draw();
    }
END_SHADER