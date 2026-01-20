#include "cbase.h"
#include "MapPropertiesEditorPanelSkyboxPage.h"

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
		m_SkyboxNames = new ComboBox(this, "SkyboxComboBox", 14, false);
	}

	//post processing
	{
		//intensity combo box
		m_FilterComboBox = new ComboBox(this, "FilterComboBox", 14, false);

		//intensity text
		m_FilterIntensityText = new Label(this, "FilterIntensitySlider", "");

		//intensity slider
		m_FilterIntensitySlider = new CMapPropertiesPanelSlider(this, "FilterIntensitySlider");
	}

	//cloud settings
	{
		m_CloudButton = new CMapPropertiesPanelButton(this, "CloudButton", "");
		m_CloudButton->SetCommand(COMMAND_CHANGE_CLOUDS_COLOR);

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
		static ConVar* intensity_day = cvar->FindVar("amod_epic_filter_day_intensity");
		static ConVar* intensity_night = cvar->FindVar("amod_epic_filter_night_intensity");

		//set the convar value
		float value = (float)m_FilterIntensitySlider->GetValue() / m_FilterIntensitySlider->GetMax();
		if (m_bNightTimeMode)
			intensity_night->SetValue(value);
		else
			intensity_day->SetValue(value);
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

	BaseClass::Update();
}

//----------------------------------------------------------------------------------------------------
// Purpose: Called on panel paint
//----------------------------------------------------------------------------------------------------
void CMapPropertiesPanelSkyboxFiltersPage::Paint()
{
	BaseClass::Paint();

	//draw the colors
	Color color = m_CloudColor;
	if (color.r() == 255 && color.g() == 255 && color.b() == 255 && color.a() == 255)
		color = Color(51, 103, 153, 255);

	surface()->DrawSetColor(color);
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
		m_ColorPicker->SetTitle("Set Clouds Color (255 255 255 255 = default)", true);
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
		ConVarRef amod_filter_filename(m_bNightTimeMode ? "amod_epic_filter_night_filename" : "amod_epic_filter_day_filename");
		amod_filter_filename.SetValue(m_FilterComboBox->GetActiveItemUserData()->GetName());
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
	AddUndo_SetColor(&m_CloudColor, m_CloudColor._color);

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
		const char* skybox = IsNightPage ? StringFromMapTimeStringTableIndex(info.NightInfo.DefaultNightSky) : StringFromMapTimeStringTableIndex(info.DayInfo.DefaultDaySky);

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

		} while (false);
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
		m_EnableBloomCheckButton->SetSelected(atoi(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomEnabled : info.DayInfo.BloomEnabled)));
		m_BloomScaleSlider->SetValue(atoi(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomScale : info.DayInfo.BloomScale)));
		m_BloomScalarSlider->SetValue(atof(StringFromMapTimeStringTableIndex(IsNightPage ? info.NightInfo.BloomScalarFactor : info.DayInfo.BloomScalarFactor)) * BLOOM_SCALAR_DIVISOR);
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
			info.NightInfo.DefaultNightSky = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());
		else
			info.DayInfo.DefaultDaySky = StringToMapTimeStringTableIndex(m_SkyboxNames->GetActiveItemUserData()->GetName());

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
			info.NightInfo.BloomEnabled = StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableBloomCheckButton->IsSelected()));
			info.NightInfo.BloomScale = StringToMapTimeStringTableIndex(CFmtStr("%d", m_BloomScaleSlider->GetValue()));
			info.NightInfo.BloomScalarFactor = StringToMapTimeStringTableIndex(CFmtStr("%.3f", (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR));
		}
		else
		{
			info.DayInfo.BloomEnabled = StringToMapTimeStringTableIndex(CFmtStr("%d", m_EnableBloomCheckButton->IsSelected()));
			info.DayInfo.BloomScale = StringToMapTimeStringTableIndex(CFmtStr("%d", m_BloomScaleSlider->GetValue()));
			info.DayInfo.BloomScalarFactor = StringToMapTimeStringTableIndex(CFmtStr("%.3f", (float)m_BloomScalarSlider->GetValue() / BLOOM_SCALAR_DIVISOR));
		}

	} while (false);
}
