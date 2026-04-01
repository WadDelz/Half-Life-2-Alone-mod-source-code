//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============// 
// 
// Purpose: Saturation shader 
// 
//=============================================================================// 
#include "BaseVSShader.h" 
#include "convar.h" 
#include "screenspaceeffect_vs20.inc" 
#include "saturation_ps20.inc" 

//saturation vars
static ConVar amod_saturation_amount("amod_saturation_amount", "1.4");

//actual shader
BEGIN_VS_SHADER_FLAGS(SATURATION, "Screenspace Saturation", SHADER_NOT_EDITABLE) 
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

			int fmt = VERTEX_POSITION; 
			pShaderShadow->VertexShaderVertexFormat(fmt, 1, 0, 0); 
			
			DECLARE_STATIC_VERTEX_SHADER(screenspaceeffect_vs20); 
			SET_STATIC_VERTEX_SHADER(screenspaceeffect_vs20); 

			DECLARE_STATIC_PIXEL_SHADER(saturation_ps20); 
			SET_STATIC_PIXEL_SHADER(saturation_ps20); 
		} 
	
		//dynamic state
		DYNAMIC_STATE 
		{ 
			pShaderAPI->BindStandardTexture(SHADER_SAMPLER0, TEXTURE_FRAME_BUFFER_FULL_TEXTURE_0); 
			
			DECLARE_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20); 
			SET_DYNAMIC_VERTEX_SHADER(screenspaceeffect_vs20); 
			
			float sat[4]; 
			sat[0] = amod_saturation_amount.GetFloat(); 
			sat[1] = 0.0f; 
			sat[2] = 0.0f; 
			sat[3] = 0.0f; 
			
			pShaderAPI->SetPixelShaderConstant(0, sat, 1); 
		} 
		
		Draw(); 
	} 
END_SHADER