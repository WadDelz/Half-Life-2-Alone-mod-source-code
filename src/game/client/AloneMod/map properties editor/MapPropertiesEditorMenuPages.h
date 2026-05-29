#ifndef __MAPPROPERTIESEDITORMENUPAGE_H
#define __MAPPROPERTIESEDITORMENUPAGE_H

#include "MapPropertiesEditorPanel.h"
#include <vgui_controls/ScrollBar.h>

#define ENABLE_MAP_PREFIX "EnableTimeForMap"
#define FLIP_MAP_PREFIX "SetShouldFlipTimeForMap"
#define MODIFY_MAP_PREFIX "OpenModifyMapWindow"
#define COPY_MAP_STATE_PREFIX "CopyMapState"
#define PASTE_MAP_STATE_PREFIX "PasteMapState"

//main map properties editor night page
class CMapPropertiesEditorPageBase : public PropertyPage
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorPageBase, PropertyPage);
public:
	CMapPropertiesEditorPageBase(Panel* parent, const char* name, bool IsNightPage);

	//layout
	void PerformLayout();

	//scroll wheel
	void OnMouseWheeled(int delta);

	//clear/add functions
	void Populate(CUtlVector<MapTimeInfo_t>& base);
	void Clear();

	void ApplySchemeSettings(IScheme* settings);

	//message funcs
	MESSAGE_FUNC_PARAMS(OnTextChanged, "TextChanged", kv);
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");
	MESSAGE_FUNC(OnApplyPageSetting, "ApplyPageSetting");
	MESSAGE_FUNC(OnMapPropertiesPanelClosed, "MapPropertiesPanelClosed");
	MESSAGE_FUNC_PARAMS(OnCommand, "Command", data);
private:
	//is this for the day or night page
	bool m_bIsNightPage;

	//top combo box
	class CMapPropertiesEditorComboBox* m_FileList;

	//scroll bar
	ScrollBar* m_ScrollBar;

	//list of items
	CUtlVector<Divider*> m_MapList;

	//current selected page
	int m_CurrentPage;

	//text font
	HFont m_MapTextFont = INVALID_FONT;
};




//main map properties editor night page
class CMapPropertiesEditorNightPage : public CMapPropertiesEditorPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorNightPage, CMapPropertiesEditorPageBase);
public:
	CMapPropertiesEditorNightPage(Panel* parent) : BaseClass(parent, "MapPropertiesDayPage", true) {}
};




//main map properties editor day page
class CMapPropertiesEditorDayPage : public CMapPropertiesEditorPageBase
{
	DECLARE_CLASS_SIMPLE(CMapPropertiesEditorDayPage, CMapPropertiesEditorPageBase);
public:
	CMapPropertiesEditorDayPage(Panel* parent) : BaseClass(parent, "MapPropertiesDayPage", false) {}
};



#endif  //__MAPPROPERTIESEDITORMENUPAGE_H