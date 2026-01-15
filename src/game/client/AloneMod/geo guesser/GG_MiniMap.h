//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Minimap for geo-guesser game page
//
// $NoKeywords: $
//
//=================================================================================//
#ifndef _GG_MINIMAP_H
#define _GG_MINIMAP_H

#ifdef WIN32
#pragma once
#endif

//vgui headers
#include "vgui_controls/Divider.h"
#include "vgui_controls/Image.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "GG_MainPanel.h"

//quick image class that streatches the image
class CStretchingImage : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CStretchingImage, Panel);
public:
	struct ImageHandle_t
	{
		char image[512];
		int id;
		int refcount;
	};
public:
	CStretchingImage(vgui::Panel* parent, const char* name) : BaseClass(parent, name) { m_X = m_Y = m_W = m_H = 0; }
	~CStretchingImage();

	//override set bounds func
	virtual void PerformLayout();

	//image funcs
	virtual void SetImage(const char* filename);

	//paints the panel
	virtual void Paint();

	//kb/mouse
	void OnMousePressed(vgui::MouseCode code) { GetParent()->OnMousePressed(code); };
	void OnMouseReleased(vgui::MouseCode code) { GetParent()->OnMouseReleased(code); };
	void OnCursorMoved(int x, int y) { GetParent()->OnCursorMoved(x, y); };

	//x y w and h
	int m_X, m_Y, m_W, m_H;
private:
	ImageHandle_t* m_Handle = nullptr;
};


//map list panel
class CGG_MiniMap : public vgui::Divider
{
	DECLARE_CLASS_SIMPLE(CGG_MiniMap, vgui::Divider)
public:
	//constructor
	CGG_MiniMap(CGG_MainPanel* parent, const char* name);

	//sets the map image
	void SetImage(const char* image);

	//override bounds function
	void SetBounds(int x, int y, int w, int h) override;
	void PerformLayout() override;

	//mouse/keyboard funcs
	void OnMouseWheeled(int delta) override;

	void OnMousePressed(vgui::MouseCode code) override;
	void OnMouseReleased(vgui::MouseCode code) override;
	void OnCursorMoved(int x, int y) override;

	//reset
	void Reset();
	void ResetImageBounds();

	//sets if we can mark
	void SetCanMark(bool can) { m_bCanMark = can; }

	//mark/pin/points funcs
	bool IsMarked();
	void SetActuallPosMarker(const Vector2D& position);
	int GetPointsValue(const Vector2D& other);
	void RemovePinMarker();

	//think func
	void OnThink();
private:
	//images
	CStretchingImage* m_MapImage;
	CStretchingImage* m_PinImage;
	CStretchingImage* m_ActuallPosImage;
	vgui::Label* m_DistanceLabel;

	//zoom stuff
	float m_fZoom;
	int m_iOffsetX;
	int m_iOffsetY;

	//for dragging
	bool m_bDragging;
	int  m_iDragStartX;
	int  m_iDragStartY;
	int  m_iMapStartX;
	int  m_iMapStartY;

	//marking
	Vector2D m_Position;        // pixel position of the mark
	bool m_bHasMark;            // whether a mark exists

	Vector2D m_ActuallPos;        // pixel position of the mark
	bool m_bHasActuallMark;     // whether the actuall position pin should show

	//can we mark?
	bool m_bCanMark = true;

	//position multiplyer
	float m_PosMultiplyerX;
	float m_PosMultiplyerY;
};

#endif //_GG_MINIMAP_H