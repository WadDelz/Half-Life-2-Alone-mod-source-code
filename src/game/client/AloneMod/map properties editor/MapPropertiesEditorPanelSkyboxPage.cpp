#include "cbase.h"
#include "MapPropertiesEditorPanelSkyboxPage.h"
#include "MapPropertiesEditorMenuPanel.h"

//-------------------------------------------------------------------------------------------------------
// Purpose: Constructor for the skybox filter page
//-------------------------------------------------------------------------------------------------------
CMapPropertiesPanelSkyboxFiltersPage::CMapPropertiesPanelSkyboxFiltersPage(Panel* parent, const char* name) : BaseClass(parent, name, "resource/panels/MapPropertiesEditor/SkyboxFilterPage.res")
{
	//skybox
	{
		//make the background brush
		m_SkyboxBackground = new ImagePanel(this, "SkyboxBackground");

		//foreground brush
		m_SkyboxForeground = new CStretchingImage(this, "SkyboxForeground");

		//skies combo box
		m_SkyboxNames = new CMapPropertiesEditorComboBox(this, "SkyboxComboBox", 14, false);
	}

	//post processing
	{
		//intensity combo box
		m_FilterComboBox = new CMapPropertiesEditorComboBox(this, "FilterComboBox", 14, false);

		//intensity text
		m_FilterIntensityText = new Label(this, "FilterIntensitySlider", "");

		//intensity slider
		m_FilterIntensitySlider = new CMapPropertiesPanelSlider(this, "FilterIntensitySlider");
	}

	//cloud settings
	{
		m_CloudButton = new CMapPropertiesPanelButton(this, "CloudButton", "");
		m_CloudButton->SetCommand(COMMAND_CHANGE_CLOUDS_COLOR);
		m_CloudButton->SetAttatchedColor(&m_CloudColor);
		m_CloudButton->SetPasteCommand("amod_clouds_color %d %d %d %d");

		m_CloudColor.SetColor(255, 255, 255, 255);
	}

	//bloom
	{
		//enable check button
		m_EnableBloomCheckButton = new CheckButton(this, "EnableBloom", "Enable Bloom");
		m_EnableBloomCheckButton->SetBounds(245, 488, 400, 18);

		//bloom amount slider
		m_BloomScaleSlider = new CMapPropertiesPanelSlider(this, "BloomScaleSlider");

		//bloom scale amount text
		m_BloomScaleText = new Label(this, "BloomScaleText", "");

		//bloom scalar slider
		m_BloomScalarSlider = new CMapPropertiesPanelSlider(this, "BloomSlider");

		//bloom scalar amount text
		m_BloomScalarText = new Label(this, "BloomScalarText", "");
	}

	//perform layout to set the range sliders and such
	PerformLayout();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Formats the image filepath
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::FormatImage(const char* input, char* output, int outsize)
{
	//failsafe
	Q_strncpy(output, CFmtStr("../skybox/%sup", input), outsize);

	//load the panels/sky path
	KeyValuesAD keyvalues(new KeyValues("SkyPanel"));
	if (!keyvalues->LoadFromFile(filesystem, "resource/panels/skypanel.txt"))
		return;

	KeyValues* timekey = keyvalues->FindKey(m_bNightTimeMode ? "Night" : "Day");
	if (!timekey)
		return;

	//go through each key
	FOR_EACH_VALUE(timekey, value)
	{
		//check for % to change the direction the skybox displays
		char temp[512];
		Q_strncpy(temp, value->GetString(), sizeof(temp));
		const char* current = temp;

		//look for percent
		char* percent = Q_strstr(temp, "%");
		if (percent)
		{
			*percent = '\0';
			percent++;
		}
		else
			percent = "up";

		//check
		if (!Q_stricmp(current, input))
		{
			Q_strncpy(output, CFmtStr("../skybox/%s%s", current, percent), outsize);
			return;
		}
	}
}

//HACK
static bool s_ShouldUpdateSkyboxAndFilter = false;
void HACKShouldUpdateSkyboxAndFilter()
{
	s_ShouldUpdateSkyboxAndFilter = true;
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on think
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::Update()
{
	//color correction
	{
		//set intensity text
		m_FilterIntensityText->SetText(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));

		//convars
		static ConVar* intensity_var = cvar->FindVar("amod_trigger_filterintensity");

		//set the convar value
		float value = (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax();
		intensity_var->SetValue(value);
	}

	//clouds color
	{
		static ConVar* amod_clouds_color = cvar->FindVar("amod_clouds_color");

		//check if the clouds color already matches our color. If so then dont set the value
		const char* color = CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a());
		if (Q_stricmp(amod_clouds_color->GetString(), color))
			amod_clouds_color->SetValue(color);
	}

	//bloom
	{
		static ConVar* mat_force_bloom = cvar->FindVar("mat_force_bloom");
		static ConVar* mat_bloomscale = cvar->FindVar("mat_bloomscale");
		static ConVar* mat_bloom_scalefactor_scalar = cvar->FindVar("mat_bloom_scalefactor_scalar");

		//set the convar values
		mat_force_bloom->SetValue(m_EnableBloomCheckButton->IsSelected());
		mat_bloomscale->SetValue(m_BloomScaleSlider->GetValue());
		mat_bloom_scalefactor_scalar->SetValue((float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR);

		//set text
		m_BloomScaleText->SetText(CFmtStr("%d", mat_bloomscale->GetInt()));
		m_BloomScalarText->SetText(CFmtStr("%.3f", mat_bloom_scalefactor_scalar->GetFloat()));

		//set enabled state
		m_BloomScaleSlider->SetEnabled(m_EnableBloomCheckButton->IsSelected());
		m_BloomScalarSlider->SetEnabled(m_EnableBloomCheckButton->IsSelected());
	}

	//skybox and filter if needed
	if (s_ShouldUpdateSkyboxAndFilter)
	{
		//make the keyvalues
		KeyValuesAD keyvalues("OnTextChanged");

		//update the skybox names
		keyvalues->SetPtr("Panel", m_SkyboxNames);
		OnTextChanged(keyvalues);

		//update the filter names
		keyvalues->SetPtr("Panel", m_FilterComboBox);
		OnTextChanged(keyvalues);

		s_ShouldUpdateSkyboxAndFilter = false;
	}

	BaseClass::Update();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel paint
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	surface()->DrawSetColor(m_CloudColor);
	surface()->DrawFilledRect(m_CloudColorRect.x, m_CloudColorRect.y, m_CloudColorRect.x + m_CloudColorRect.width, m_CloudColorRect.y + m_CloudColorRect.height);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel layout set
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::PerformLayout()
{
	BaseClass::PerformLayout();

	//set the skybox panel
	{
		//skybox background
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxBackground"), m_SkyboxBackground);

		//skybox foreground
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxForeground"), m_SkyboxForeground);

		//skybox combo box
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("SkyboxNames"), m_SkyboxNames);
	}

	//post processing
	{
		//skybox background
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensityComboBox"), m_FilterComboBox);

		//skybox foreground
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensityText"), m_FilterIntensityText);

		//skybox combo box
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("FilterIntensitySlider"), m_FilterIntensitySlider);
	}

	//cloud
	{
		//cloud button
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("CloudButton"), m_CloudButton);

		//get the proportionate value for the cloud fog color draw rect
		sscanf(m_KeyValuesFile->GetString("CloudColorRect"), "%d %d %d %d", &m_CloudColorRect.x, &m_CloudColorRect.y, &m_CloudColorRect.width, &m_CloudColorRect.height);
	}

	//bloom
	{

		//bloom enabled
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomEnabledButton"), m_EnableBloomCheckButton);

		//bloom scale
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScaleSlider"), m_BloomScaleSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScaleText"), m_BloomScaleText);

		//bloom scalar
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScalarSlider"), m_BloomScalarSlider);
		ApplySettingsToPanel(m_KeyValuesFile->FindKey("BloomScalarText"), m_BloomScalarText);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on command
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnCommand(const char* pszCommand)
{
	//called on command
	if (!Q_stricmp(pszCommand, COMMAND_CHANGE_CLOUDS_COLOR))
	{
		//create the skybox fog color dialog
		m_ColorPicker = new CColorPicker(GetVPanel());
		m_ColorPicker->SetTitle("#MapProperties_SkyboxPage_ColorPicker_CloudsColor", true);
		m_ColorPicker->SetUsesAlpha(true);
		m_ColorPicker->SetColor(m_CloudColor);
		m_ColorPicker->DoModal();
	}

	BaseClass::OnCommand(pszCommand);
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when text is changed
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnTextChanged(KeyValues* data)
{
	if (data->GetPtr("panel") == m_SkyboxNames)
	{
		//HACK: if you use the arrow keys to navigate, the active item will NOT get set. Fix this
		char string[128];
		m_SkyboxNames->GetText(string, sizeof(string));

		//go through all the items
		for (int i = 0; i < m_SkyboxNames->GetItemCount(); i++)
		{
			//compare with that items string
			char temp[128];
			m_SkyboxNames->GetItemText(i, temp, sizeof(temp));
			if (!Q_stricmp(string, temp))
			{
				m_SkyboxNames->ActivateItem(i);
				break;
			}
		}

		//get the image
		char image[512];
		FormatImage(m_SkyboxNames->GetActiveItemUserData()->GetName(), image, sizeof(image));

		//set the image
		m_SkyboxForeground->SetImage(image);

		//set the sv_skyname convar
		ConVarRef("sv_skyname").SetValue(m_SkyboxNames->GetActiveItemUserData()->GetName());

		//add a step
		AddUndo_SetComboBox(m_SkyboxNames, m_PrevSkyboxNamesValue);
		m_PrevSkyboxNamesValue = m_SkyboxNames->GetActiveItem();
	}

	//check for epic filter combo box
	else if (data->GetPtr("panel") == m_FilterComboBox)
	{
		//HACK: if you use the arrow keys to navigate, the active item will NOT get set. Fix this
		char string[128];
		m_FilterComboBox->GetText(string, sizeof(string));

		//go through all the items
		for (int i = 0; i < m_FilterComboBox->GetItemCount(); i++)
		{
			//compare with that items string
			char temp[128];
			m_FilterComboBox->GetItemText(i, temp, sizeof(temp));
			if (!Q_stricmp(string, temp))
			{
				m_FilterComboBox->ActivateItem(i);
				break;
			}
		}

		//set amod_filter_filename
		ConVarRef amod_filter_filename("amod_trigger_filtername");
		amod_filter_filename.SetValue(m_FilterComboBox->GetActiveItemUserData()->GetName());

		//add a step
		AddUndo_SetComboBox(m_FilterComboBox, m_PrevFilterComboBoxValue);
		m_PrevFilterComboBoxValue = m_FilterComboBox->GetActiveItem();
	}
}

//-------------------------------------------------------------------------------------------------------
// Purpose: Called when a color gets selected
//-------------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::OnColorSelected(KeyValues* data)
{
	//close the color picker
	m_ColorPicker->Close();
	m_ColorPicker = nullptr;

	//add an undo step
	AddUndo_SetColor(&m_CloudColor, m_CloudColor._color, "amod_clouds_color %d %d %d %d");

	//get the color
	Color color(data->GetInt("r"), data->GetInt("g"), data->GetInt("b"), data->GetInt("a"));
	m_CloudColor = color;

	//call base func
	BaseClass::OnColorSelected(data);
}

//----------------------------------------------------------------------------------------------------
// Purpose: Initalizes the skybox, filter and cloud color settings
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::InitSkyboxAndFilter(MapTimeInfo_t& info, bool IsNightPage)
{
	m_bNightTimeMode = IsNightPage;

	{
		const char* skybox = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.Skybox) : StringFromMapTimeStringTableIndex(info.DayInfo.Skybox);

		//get the image
		char image[512];
		FormatImage(skybox, image, sizeof(image));

		//set the image
		m_SkyboxForeground->SetImage(image);

		//set the skybox combo box
		m_SkyboxNames->RemoveAll();
		do
		{
			//load keyvalues file
			KeyValuesAD keyvalues(new KeyValues("SkyPanel"));
			if (!keyvalues->LoadFromFile(filesystem, "resource/panels/skypanel.txt"))
				break;

			KeyValues* timekey = keyvalues->FindKey(IsNightPage ? "Night" : "Day");
			if (!timekey)
				break;

			//go through each key
			int index = -1, current = 0;
			FOR_EACH_VALUE(timekey, value)
			{
				//remove percent sigh
				char temp[512];
				Q_strncpy(temp, value->GetString(), sizeof(temp));
				char* percent = Q_strstr(temp, "%");
				if (percent)
					*percent = '\0';

				//add the item
				m_SkyboxNames->AddItem(value->GetName(), new KeyValues(temp));

				//check skybox
				if (!Q_stricmp(skybox, temp) && index == -1)
					index = current;

				current++;
			}

			//active the current index
			m_SkyboxNames->ActivateItem(index == -1 ? 0 : index);
			m_PrevSkyboxNamesValue = index == -1 ? 0 : index;

		} while (false);


		//make SURE the current skybox is added
		bool added = false;
		for (int i = 0; i < m_SkyboxNames->GetItemCount(); i++)
		{
			if (!Q_stricmp(skybox, m_SkyboxNames->GetItemUserData(i)->GetName()))
			{
				added = true;
				break;
			}
		}

		if (!added)
		{
			m_SkyboxNames->AddItem(skybox, new KeyValues(skybox));
			m_SkyboxNames->ActivateItem(m_SkyboxNames->GetItemCount() - 1);
		}
	}

	//post processing
	{
		const char* filtername = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.FilterName) : StringFromMapTimeStringTableIndex(info.DayInfo.FilterName);

		//get the stuff
		int index = 0, current = 0;

		//go through each filter in the scripts/colorcorrection/* directory
		FileFindHandle_t find;
		const char* first = filesystem->FindFirst("scripts/colorcorrection/*.raw", &find);
		while (first)
		{
			//check for . and ..
			if (!Q_stricmp(first, ".") || !Q_stricmp(first, ".."))
			{
				first = filesystem->FindNext(find);
				continue;
			}

			//check the extention
			const char* ext = Q_GetFileExtension(first);
			if (Q_stricmp(ext, "raw"))
			{
				first = filesystem->FindNext(find);
				continue;
			}

			//add the item
			const char* cc_string = CFmtStr("scripts/colorcorrection/%s", first);
			m_FilterComboBox->AddItem(first, new KeyValues(cc_string));

			//add to combo box
			if (!Q_stricmp(cc_string, filtername))
				index = current;

			first = filesystem->FindNext(find);

			current++;
		}

		//active the current index
		m_FilterComboBox->ActivateItem(index);
		m_PrevFilterComboBoxValue = index;

		filesystem->FindClose(find);

		//set the slider
		float filtervalue = atof(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.FilterIntensity : info.DayInfo.FilterIntensity));
		m_FilterIntensitySlider->SetValue(int(filtervalue * m_FilterIntensitySlider->GetMax()));
	}

	//clouds
	{
		int cloudcolor[4] = { 0,0,0,0 };
		UTIL_StringToIntArray(cloudcolor, 4, StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.CloudsColor : info.DayInfo.CloudsColor));
		m_CloudColor.SetColor(cloudcolor[0], cloudcolor[1], cloudcolor[2], cloudcolor[3]);
	}


	//bloom
	{
		m_EnableBloomCheckButton->SetSelected(IsNightPage ? info.NightInfo.BloomEnabled : info.DayInfo.BloomEnabled);
		m_BloomScaleSlider->SetValue(IsNightPage ? info.NightInfo.BloomScale : info.DayInfo.BloomScale);
		m_BloomScalarSlider->SetValue((IsNightPage ? info.NightInfo.BloomScalarFactor : info.DayInfo.BloomScalarFactor) * BLOOM_SCALAR_DIVISOR);
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Gets the fog info
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::GetSkyboxFilterInfo(MapTimeInfo_t& info)
{
	//set the skybox
	do
	{
		//set the value
		if (m_bNightTimeMode)
			info.NightInfo.Skybox = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());
		else
			info.DayInfo.Skybox = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());

	} while (false);


	//set the color correction
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.FilterName = StringToMapTimeStringTableIndex(m_FilterComboBox->GetActiveItemUserData()->GetName());
			info.NightInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));
		}
		else
		{
			info.DayInfo.FilterName = StringToMapTimeStringTableIndex(m_FilterComboBox->GetActiveItemUserData()->GetName());
			info.DayInfo.FilterIntensity = StringToMapTimeStringTableIndex(CFmtStr("%.2f", (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax()));
		}

	} while (false);

	//set the clouds color
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a()));
		}
		else
		{
			info.DayInfo.CloudsColor = StringToMapTimeStringTableIndex(CFmtStr("%d %d %d %d", m_CloudColor.r(), m_CloudColor.g(), m_CloudColor.b(), m_CloudColor.a()));
		}

	} while (false);

	//set the bloom
	do
	{
		//set the value
		if (m_bNightTimeMode)
		{
			info.NightInfo.BloomEnabled = m_EnableBloomCheckButton->IsSelected();
			info.NightInfo.BloomScale = m_BloomScaleSlider->GetValue();
			info.NightInfo.BloomScalarFactor = (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR;
		}
		else
		{
			info.DayInfo.BloomEnabled = m_EnableBloomCheckButton->IsSelected();
			info.DayInfo.BloomScale = m_BloomScaleSlider->GetValue();
			info.DayInfo.BloomScalarFactor = (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR;
		}

	} while (false);
}
