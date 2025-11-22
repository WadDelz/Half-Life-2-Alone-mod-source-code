//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Minimap for geo-guesser game page
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_MiniMap.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "fmtstr.h"

//---------------------------------------------------------------------------------
// Purpose: Constructor for options page mini map
//---------------------------------------------------------------------------------
CGG_MiniMap::CGG_MiniMap(CGG_MainPanel* parent, const char* name)
	: BaseClass(parent, name)
{
	//create the map image
	m_MapImage = new CStretchingImage(this, "Map");

	//make the pin image
	m_PinImage = new CStretchingImage(this, "Pin");
	m_PinImage->SetImage("geo_guesser/pin");
	
	//make the actuall pos image
	m_ActuallPosImage = new CStretchingImage(this, "Pin");
	m_ActuallPosImage->SetImage("geo_guesser/pin_actuall");

	//create the distance label
	m_DistanceLabel = new vgui::Label(this, "DistanceLabel", "Distance = 0m");

	//reset variables
	Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Sets the minimap image
//---------------------------------------------------------------------------------
void CGG_MiniMap::SetImage(const char* image)
{
	//set the image
	m_MapImage->SetImage(image);

	//reset the image bounds
	m_MapImage->SetBounds(0, 0, GetWide(), GetTall());

	//reset variables
	Reset();
}

//---------------------------------------------------------------------------------
// Purpose: Sets the minimap image
//---------------------------------------------------------------------------------
void CGG_MiniMap::SetBounds(int x, int y, int w, int h)
{
	//call base function
	BaseClass::SetBounds(x, y, w, h);

	//reset the image bounds
	m_MapImage->SetBounds(0, 0, w, h);
}

//---------------------------------------------------------------------------------
// Purpose: Resets the map bounds
//---------------------------------------------------------------------------------
void CGG_MiniMap::ResetImageBounds()
{
	//reset the image bounds
	m_MapImage->SetBounds(0, 0, GetWide(), GetTall());

	//reset zoom
	m_fZoom = 1.0f;
	m_iOffsetX = 0;
	m_iOffsetY = 0;
}

//---------------------------------------------------------------------------------
// Purpose: Removes the pin marker
//---------------------------------------------------------------------------------
void CGG_MiniMap::RemovePinMarker()
{
	//no marking
	m_bHasMark = false;
	m_Position.Init(0, 0);
	m_PinImage->SetBounds(0, 0, 0, 0);
}

ConVar gg_minimap_scroll_multiplyer("gg_minimap_scroll_multiplyer", "5");
ConVar gg_minimap_pin_size("gg_minimap_pin_size", "20");

//---------------------------------------------------------------------------------
// Purpose: Called on mouse press
//---------------------------------------------------------------------------------
void CGG_MiniMap::OnMousePressed(vgui::MouseCode code)
{
	//must be left click
	if (code == MOUSE_LEFT)
		m_bDragging = true;

	//see if we are dragging
	if (m_bDragging)
	{
		//set the mouse capture to this
		vgui::input()->SetMouseCapture(GetVPanel());

		//get the cursor pos for the offset
		vgui::surface()->SurfaceGetCursorPos(m_iDragStartX, m_iDragStartY);
		m_iMapStartX = m_iOffsetX;
		m_iMapStartY = m_iOffsetY;
	}

	BaseClass::OnMousePressed(code);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse release
//---------------------------------------------------------------------------------
void CGG_MiniMap::OnMouseReleased(vgui::MouseCode code)
{
	//see if we are dragging
	if (code == MOUSE_LEFT)
	{
		//set the mouse capture to this
		vgui::input()->SetMouseCapture(NULL);

		//stop dragging
		m_bDragging = false;
	}

	//check for middle mouse button
	if (code == MOUSE_MIDDLE)
	{
		ResetImageBounds();
		return;
	}

	//see if right click and can mark
	if (code == MOUSE_RIGHT && m_bCanMark)
	{
		//get mouse cursor in panel space
		int mx, my;
		vgui::surface()->SurfaceGetCursorPos(mx, my);
		ScreenToLocal(mx, my);

		//convert panel coords to image coords (consider zoom & offset)
		m_Position.x = (float)(mx - m_iOffsetX) / m_fZoom;
		m_Position.y = (float)(my - m_iOffsetY) / m_fZoom;

		//debug
		ConDMsg("DEBUG MINIMAP: %d %d\n", (int)m_Position.x, (int)m_Position.y);

		//flag that we have a mark
		m_bHasMark = true;
	}

	BaseClass::OnMouseReleased(code);
}

//---------------------------------------------------------------------------------
// Purpose: Called on mouse move
//---------------------------------------------------------------------------------
void CGG_MiniMap::OnCursorMoved(int x, int y)
{
	//see if we are dragging
	if (m_bDragging)
	{
		//get cursor pos
		int cx, cy;
		vgui::surface()->SurfaceGetCursorPos(cx, cy);

		//get the offsets and cursor positions
		int dx = cx - m_iDragStartX;
		int dy = cy - m_iDragStartY;

		//get offsets
		m_iOffsetX = m_iMapStartX + dx;
		m_iOffsetY = m_iMapStartY + dy;
		
		//reset positions
		InvalidateLayout();
	}

	BaseClass::OnCursorMoved(x, y);
}

//---------------------------------------------------------------------------------
// Purpose: Resets the mini map variables
//---------------------------------------------------------------------------------
void CGG_MiniMap::Reset()
{
	//reset the zoom stuff
	m_fZoom = 1.0f; //default zoom
	m_iOffsetX = 0;
	m_iOffsetY = 0;

	//reset dragging stuff
	m_bDragging = false;
	m_iDragStartX = 0;
	m_iDragStartY = 0;
	m_iMapStartX = 0;
	m_iMapStartY = 0;

	//set marking pos
	m_Position.Init(0, 0);
	m_bHasMark = false;
	m_ActuallPos.Init(0, 0);
	m_bHasActuallMark = false;

	//set the images
	m_PinImage->SetBounds(0, 0, 0, 0);
	m_ActuallPosImage->SetBounds(0, 0, 0, 0);
	m_DistanceLabel->SetBounds(0, 0, 0, 0);
}

//---------------------------------------------------------------------------------
// Purpose: Zoom the minimap based on mouse wheel! Zooms toward the cursor.
//---------------------------------------------------------------------------------
void CGG_MiniMap::OnMouseWheeled(int delta)
{
	//dont zoom if dragging
	if (m_bDragging)
		return;

	//get mouse position in panel space
	int mx, my;
	vgui::surface()->SurfaceGetCursorPos(mx, my);
	ScreenToLocal(mx, my);

	//previous zoom
	float oldZoom = m_fZoom;

	//base exponential speed
	float base = 0.10f * gg_minimap_scroll_multiplyer.GetInt();

	//apply exponential change
	if (delta > 0)
		m_fZoom *= (1.0f + base);

	if (delta < 0)
		m_fZoom /= (1.0f + base);

	//clamp zoom (minimum 1x, maximum 5000x)
	if (m_fZoom < 1.0f)
		m_fZoom = 1.0f;

	if (m_fZoom > 100)
		m_fZoom = 100;

	//calculate ratio AFTER clamping
	float ratio = m_fZoom / oldZoom;

	//reposition so mouse stays on same map point
	m_iOffsetX = (int)(mx - (mx - m_iOffsetX) * ratio);
	m_iOffsetY = (int)(my - (my - m_iOffsetY) * ratio);

	//apply layout
	InvalidateLayout();
}

//---------------------------------------------------------------------------------
// Purpose: Returns if the map has been marked yet
//---------------------------------------------------------------------------------
bool CGG_MiniMap::IsMarked()
{
	return m_bHasMark;
}

//---------------------------------------------------------------------------------
// Purpose: Returns if the map has been marked yet
//---------------------------------------------------------------------------------
void CGG_MiniMap::SetActuallPosMarker(const Vector2D& position)
{
	//set marker
	m_bHasActuallMark = true;
	m_ActuallPos = position;

	//set text
	m_DistanceLabel->SetText(CFmtStr("Distance = %d", (int)m_Position.DistTo(m_ActuallPos)));
}

ConVar gg_pin_maxpoints("gg_pin_maxpoints", "1000");
ConVar gg_pin_maxpoints_falloff("gg_pin_maxpoints_falloff", "10");
ConVar gg_pin_maxdistance_maxdistance("gg_pin_maxdistance_maxdistance", "175");

//---------------------------------------------------------------------------------
// Purpose: Gets the points value of the current marker and the actuall goal
//---------------------------------------------------------------------------------
int CGG_MiniMap::GetPointsValue(const Vector2D& position)
{
	//if no marker then return 0
	if (!m_bHasMark)
		return 0;

	//get the distance of the 2 positions
	int dist = position.DistTo(m_Position);

	//subtract any falloff if needed (optional)
	//dist -= gg_pin_maxdistance_falloff.GetInt();
	int points = 0;

	//if distance is very close, full points
	if (dist < gg_pin_maxpoints_falloff.GetInt())
		points = gg_pin_maxpoints.GetInt();

	//if distance is too far, zero points
	else if (dist > gg_pin_maxdistance_maxdistance.GetInt())
		points = 0;

	//otherwise, linear interpolation between max and zero
	else
	{
		float t = (float)(dist - gg_pin_maxpoints_falloff.GetInt()) / (gg_pin_maxdistance_maxdistance.GetInt() - gg_pin_maxpoints_falloff.GetInt()); // scale 50..500 → 0..1
		points = (int)(gg_pin_maxpoints.GetInt() * (1.0f - t));
	}

	return clamp(points, 0, gg_pin_maxpoints.GetInt());
}

//---------------------------------------------------------------------------------
// Purpose: Paints the panel
//---------------------------------------------------------------------------------
void CGG_MiniMap::OnThink()
{
	BaseClass::OnThink();

	if (m_bHasMark)
	{
		//convert stored pixel position to screen coords
		int markX = (int)(m_Position.x * m_fZoom + m_iOffsetX);
		int markY = (int)(m_Position.y * m_fZoom + m_iOffsetY);

		//get the size
		int size = gg_minimap_pin_size.GetInt();

		//draw a simple red square 6x6 pixels at the mark
		m_PinImage->SetBounds(markX - size / 2, markY - size / 2, size, size);
	}

	if (m_bHasActuallMark)
	{
		//convert stored pixel position to screen coords
		int markX = (int)(m_ActuallPos.x * m_fZoom + m_iOffsetX);
		int markY = (int)(m_ActuallPos.y * m_fZoom + m_iOffsetY);

		//get the size
		int size = gg_minimap_pin_size.GetInt();

		//draw a simple red square 6x6 pixels at the mark
		m_ActuallPosImage->SetBounds(markX - size / 2, markY - size / 2, size, size);

		if (m_bHasMark)
		{
			//convert m_Position to screen coords as well
			int posX = (int)(m_Position.x * m_fZoom + m_iOffsetX);
			int posY = (int)(m_Position.y * m_fZoom + m_iOffsetY);

			//calculate the midpoint in screen space
			int textX = (posX + markX) / 2;
			int textY = (posY + markY) / 2;

			//set label bounds
			m_DistanceLabel->SetBounds(textX, textY, 115, 20);
		}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called when the panel re-sizes
//---------------------------------------------------------------------------------
void CGG_MiniMap::PerformLayout()
{
	BaseClass::PerformLayout();

	int pw = GetWide();
	int ph = GetTall();

	int mw = (int)(pw * m_fZoom);
	int mh = (int)(ph * m_fZoom);

	if (mw < pw)
		mw = pw;

	if (mh < ph)
		mh = ph;

	if (m_iOffsetX > 0)
		m_iOffsetX = 0;

	if (m_iOffsetY > 0)
		m_iOffsetY = 0;

	if (m_iOffsetX + mw < pw)
		m_iOffsetX = pw - mw;

	if (m_iOffsetY + mh < ph)
		m_iOffsetY = ph - mh;

	m_MapImage->SetBounds(m_iOffsetX, m_iOffsetY, mw, mh);
}