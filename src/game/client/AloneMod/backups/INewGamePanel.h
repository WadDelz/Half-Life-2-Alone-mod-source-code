class NewGamePanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Init(bool hl1panel) = 0;
	virtual void		Destroy(void) = 0;
	virtual void		Activate(void) = 0;
};

extern NewGamePanel* newgamepanel;