#ifndef __INEWGAMEPANEL
#define __INEWGAMEPANEL

class INewGamePanel
{
public:
	virtual void Create(vgui::VPANEL parent) = 0;
	virtual void Destroy(void) = 0;
	virtual void DoDayCheck() = 0;
};

extern INewGamePanel* newgamepanel;

#endif //__INEWGAMEPANEL