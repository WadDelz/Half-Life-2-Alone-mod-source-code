//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: taken from leaked engine code (engine/gl_warp.cpp) sorry valve :)
//			used for dynamic skybox changing.
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "utlvector.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "tier2/tier2.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier0/vprof.h"
#include "view.h"
#include "DynamicSky.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SQRT3INV		(0.57735f)		// a little less than 1 / sqrt(3)

static IMaterial* skyboxMaterials[6] = { NULL, NULL, NULL, NULL, NULL, NULL };

// 1 = s, 2 = t, 3 = 2048
int	st_to_vec[6][3] =
{
	{3,-1,2},
	{-3,1,2},

	{1,3,2},
	{-1,-3,2},

	{-2,-1,3},
	{2,-1,-3}
};

int	skytexorder[6] = { 0,2,1,3,4,5 };
#define SIGN(d)	((d)<0?-1:1)
static int		gFakePlaneType[6] = { 1,-1,2,-2,3,-3 };

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ModBase_UnloadSkys(void)
{
	int i;

	for (i = 0; i < 6; i++)
	{
		if (skyboxMaterials[i])
		{
			skyboxMaterials[i]->DecrementReferenceCount();
			skyboxMaterials[i] = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
static bool ModBase_LoadNamedSkys(const char* skyname)
{
	char		name[512];
	IMaterial* skies[6];
	bool		success = true;
	char* skyboxsuffix[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

	bool bUseDx8Skyboxes = (g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90);
	for (int i = 0; i < 6; i++)
	{
		skies[i] = NULL;
		if (bUseDx8Skyboxes)
		{
			Q_snprintf(name, sizeof(name), "skybox/%s_dx80%s", skyname, skyboxsuffix[i]);
			skies[i] = materials->FindMaterial(name, TEXTURE_GROUP_SKYBOX, false);
			if (IsErrorMaterial(skies[i]))
			{
				skies[i] = NULL;
			}
		}

		if (skies[i] == NULL)
		{
			Q_snprintf(name, sizeof(name), "skybox/%s%s", skyname, skyboxsuffix[i]);
			skies[i] = materials->FindMaterial(name, TEXTURE_GROUP_SKYBOX);
		}
		if (!IsErrorMaterial(skies[i]))
			continue;

		success = false;
		break;
	}

	if (!success)
	{
		return false;
	}

	// Increment references
	for (int i = 0; i < 6; i++)
	{
		// Unload any old skybox
		if (skyboxMaterials[i])
		{
			skyboxMaterials[i]->DecrementReferenceCount();
			skyboxMaterials[i] = NULL;
		}

		// Use the new one
		assert(skies[i]);
		skyboxMaterials[i] = skies[i];
		skyboxMaterials[i]->IncrementReferenceCount();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ModBase_LoadSkys(void)
{
	bool success = true;

	char requestedsky[128];

	ConVarRef skyname("sv_skyname");
	if (skyname.IsValid())
	{
		Q_strncpy(requestedsky, skyname.GetString(), sizeof(requestedsky));
	}
	else
	{
		ConDMsg("Unable to find skyname ConVar!!!\n");
		return;
	}

	// See if user's sky will work
	if (!ModBase_LoadNamedSkys(requestedsky))
	{
		// Assume failure
		success = false;

		// See if user requested other than the default
		if (Q_stricmp(requestedsky, "sky_day01_01"))
		{
			// Try the default
			skyname.SetValue("sky_day01_01");

			// See if we could load that one now
			if (ModBase_LoadNamedSkys(skyname.GetString()))
			{
				ConDMsg("Unable to load sky %s, but successfully loaded %s\n", requestedsky, skyname.GetString());
				success = true;
			}
		}
	}

	if (!success)
	{
		ConDMsg("Unable to load sky %s\n", requestedsky);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#pragma warning (disable : 4701)
//static void MakeSkyVec(float s, float t, int axis, float zFar, Vector& position, Vector2D& texCoord)
//{
//	Vector		v, b;
//	int			j, k;
//	float		width;
//
//	width = zFar * SQRT3INV;
//
//	if (s < -1)
//		s = -1;
//	else if (s > 1)
//		s = 1;
//	if (t < -1)
//		t = -1;
//	else if (t > 1)
//		t = 1;
//
//	b[0] = s * width;
//	b[1] = t * width;
//	b[2] = width;
//
//	for (j = 0; j < 3; j++)
//	{
//		k = st_to_vec[axis][j];
//		if (k < 0)
//			v[j] = -b[-k - 1];
//		else
//			v[j] = b[k - 1];
//		v[j] += CurrentViewOrigin()[j];
//	}
//
//	// avoid bilerp seam
//	s = (s + 1) * 0.5;
//	t = (t + 1) * 0.5;
//
//	if (s < 1.0 / 512)
//		s = 1.0 / 512;
//	else if (s > 511.0 / 512)
//		s = 511.0 / 512;
//	if (t < 1.0 / 512)
//		t = 1.0 / 512;
//	else if (t > 511.0 / 512)
//		t = 511.0 / 512;
//	
//	t = 1.0 - t;
//	VectorCopy(v, position);
//	texCoord[0] = s;
//	texCoord[1] = t;
//}

static void MakeSkyVec(float s, float t, int axis, float zFar, Vector& position, Vector2D& texCoord, const QAngle& angles)
{
	Vector v, b;
	float width = zFar * SQRT3INV;

	s = clamp(s, -1.0f, 1.0f);
	t = clamp(t, -1.0f, 1.0f);

	b[0] = s * width;
	b[1] = t * width;
	b[2] = width;

	for (int j = 0; j < 3; j++)
	{
		int k = st_to_vec[axis][j];
		v[j] = (k < 0) ? -b[-k - 1] : b[k - 1];
	}

	// Apply rotation
	Vector rotated;
	VectorRotate(v, angles, rotated);
	position = rotated + CurrentViewOrigin();

	// Adjust texture coordinates
	s = (s + 1) * 0.5f;
	t = (t + 1) * 0.5f;
	s = clamp(s, 1.0f / 512, 511.0f / 512);
	t = clamp(1.0f - t, 1.0f / 512, 511.0f / 512);

	texCoord[0] = s;
	texCoord[1] = t;
}
#pragma warning (default : 4701)

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ModBase_DrawSkyBox(float zFar, int nDrawFlags)
{
	VPROF("ModBase_DrawSkyBox");

	static ConVarRef r_drawskybox("r_drawskybox");
	static ConVarRef mat_loadtextures("mat_loadtextures");
	if (!r_drawskybox.GetInt() || !mat_loadtextures.GetInt())
		return;

	CMatRenderContextPtr pRenderContext(materials);

	for (int i = 0; i < 6; i++, nDrawFlags >>= 1)
	{
		if (!(nDrawFlags & 1))
			continue;

		Vector positionArray[4];
		Vector2D texCoordArray[4];

		if (skyboxMaterials[skytexorder[i]])
		{
			pRenderContext->Bind(skyboxMaterials[skytexorder[i]]);

			MakeSkyVec(-1.0f, -1.0f, i, zFar, positionArray[0], texCoordArray[0], g_PModBase_DynamicSkybox_Angle);
			MakeSkyVec(-1.0f, 1.0f, i, zFar, positionArray[1], texCoordArray[1], g_PModBase_DynamicSkybox_Angle);
			MakeSkyVec(1.0f, 1.0f, i, zFar, positionArray[2], texCoordArray[2], g_PModBase_DynamicSkybox_Angle);
			MakeSkyVec(1.0f, -1.0f, i, zFar, positionArray[3], texCoordArray[3], g_PModBase_DynamicSkybox_Angle);

			IMesh* pMesh = pRenderContext->GetDynamicMesh();
			CMeshBuilder meshBuilder;
			meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

			for (int j = 0; j < 4; ++j)
			{
				meshBuilder.Position3fv(positionArray[j].Base());
				meshBuilder.TexCoord2fv(0, texCoordArray[j].Base());
				meshBuilder.AdvanceVertex();
			}

			meshBuilder.End();
			pMesh->Draw();
		}
	}
}

//boolean used to see if the mod uses default skybox system or dynamic skybox system
bool g_PModBase_DynamicSkybox_bUse = false;
QAngle g_PModBase_DynamicSkybox_Angle = QAngle(0, 0, 0);	//the angle of the skybox