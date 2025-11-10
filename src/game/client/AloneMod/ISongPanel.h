class SongPanel
{
public:
	virtual void		Create(vgui::VPANEL parent) = 0;
	virtual void		Destroy(void) = 0;
	virtual void		Activate(void) = 0;
	virtual void		OnLevelChange(void) = 0;
	virtual void		OnLevelShutdown(void) = 0;
	virtual void		StartPlaylist(void) = 0;
	virtual void		OnNewLevel(void) = 0;
	virtual void		AddNotification(const char* message) = 0;
	virtual void		SavePlaylistToFile(const char* filename) = 0;
	virtual void		LoadPlaylistFromFile(const char* filename) = 0;
};

extern SongPanel* songpanel;