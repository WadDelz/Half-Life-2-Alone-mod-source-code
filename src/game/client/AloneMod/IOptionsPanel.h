#ifndef __AMODOPTIONSPANEL_H
#define __AMODOPTIONSPANEL_H

#pragma once

#include "vgui_controls/Panel.h"

using namespace vgui;

//options panel interface
class OptionsPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
	virtual void		Activate(void) = 0;
};
extern OptionsPanel* optionspanel;

//credits panel interfece
class AmodCreditsPanel
{
public:
	virtual void Open(Panel* parent) = 0;
	virtual void ClearPanel() = 0;
	virtual void Close() = 0;
};
extern AmodCreditsPanel* creditspanel;

//weather panel interface
class AmodWeatherPanel
{
public:
	virtual void Open() = 0;
	virtual void ClearPanel() = 0;
	virtual void Close() = 0;
};
extern AmodWeatherPanel* weatherpanel;

//skybox panel interface
class AmodSkyboxPanel
{
public:
	virtual void Open(Panel* parent) = 0;
	virtual void ClearPanel() = 0;
	virtual void Close() = 0;
};
extern AmodSkyboxPanel* skyboxpanel;

#endif //__AMODOPTIONSPANEL_H