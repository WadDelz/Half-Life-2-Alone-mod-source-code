//========= Created by Waddelz. https://www.youtube.com/@WadDeIz_Real. ============//
//
// Purpose: Main menu page for the geo-guesser panel
//
// $NoKeywords: $
//
//=================================================================================//
#include "cbase.h"
#include "GG_GamePage.h"
#include "GG_MainMenuPage.h"
#include "GG_FinishedPage.h"
#include "fmtstr.h"

//vgui headers
#include "vgui_controls/Button.h"
#include "vgui_controls/QueryBox.h"

#define GAME_RES_FILENAME "resource/geo_guesser/game.res"

#define SUBMIT_COMMAND "Submit"
#define SKIP_COMMAND "Skip"
#define NEXT_COMMAND "Next"
#define FINISH_COMMAND "Finish"
#define FINISH_COMMAND_CONFIRM "FinishConfirm"

//---------------------------------------------------------------------------------
// Purpose: Constructor for main manu page
//---------------------------------------------------------------------------------
CGG_Game_Page::CGG_Game_Page(CGG_MainPanel* parent)
	: I_GG_Page(parent, GAME_PAGE_NAME)
{
	//add the buttons
	AddChild((m_CurrentFindFromImageBackground = new vgui::ImagePanel(parent, "FindFromImageBg")));
	AddChild((m_CurrentFindFromImage = new CStretchingImage(parent, "FindFromImage")));
	AddChild((m_MiniMap = new CGG_MiniMap(parent, "MiniMap")));
	AddChild((m_CurrentScoreLabel = new vgui::Label(parent, "CurrentScoreLabel", "Current Score = 0")));
	AddChild((m_CurrentRoundLabel = new vgui::Label(parent, "CurrentRoundLabel", CFmtStr("Current Round = 0/%d", parent->GetGGInfo().rounds))));
	AddChild((m_SubmitAnswerButton = new vgui::Button(parent, "SubmitAnswer", "Submit Answer", parent, SUBMIT_COMMAND)));
	AddChild((m_SkipButtonm = new vgui::Button(parent, "SkipLevel", "Skip Level", parent, SKIP_COMMAND)));
	AddChild((m_FinishButton = new vgui::Button(parent, "FinishButton", "Finish Now", parent, FINISH_COMMAND)));
	AddChild((m_MapList = new vgui::ComboBox(parent, "MapListComboBox", 10, false)));

	//load GAME_RES_FILENAME on navigate to
	SetSettingsFile(GAME_RES_FILENAME);
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_Game_Page::SetFinishedState(bool state)
{
	//set the minimap mark state
	m_MiniMap->SetCanMark(!state);

	//check state
	if (state)
	{
		//set the texts and enabled states
		if (GetPanel()->GetGGInfo().rounds_played + 1 >= GetPanel()->GetGGInfo().rounds)
			m_SubmitAnswerButton->SetText("Finish");
		else
			m_SubmitAnswerButton->SetText("Next Location");

		m_SubmitAnswerButton->SetCommand(NEXT_COMMAND);
		m_SkipButtonm->SetEnabled(false);
		m_FinishButton->SetEnabled(false);

		//get difficulty and map locations
		CGG_MainPanel::MapData_t::MapType_e difficulty = (CGG_MainPanel::MapData_t::MapType_e)GetPanel()->GetGGInfo().difficulty;
		CGG_MainPanel::MapData_t::MapLocation_t* location = &m_MapData[CurrentSelectedMap]->MapLocations[CurrentSelectedPosition];

		//get the position
		Vector2D position = location->positions[difficulty];

		//get the map list text
		char text[512];
		m_MapList->GetText(text, sizeof(text));

		//get the actuall map name
		const char* ActuallMap = GetPanel()->GetStringForSymbol(location->actuall_map[difficulty]);

		//see if we are not on the correct map AND we are using the combo box
		if (m_MapList->IsEnabled() && Q_strcmp(ActuallMap, text))
		{
			//set the points label
			m_CurrentScoreLabel->SetText("You Scored 0");

			//get the correct map
			for (int i = 0; i < m_MapList->GetItemCount(); i++)
			{
				//get the item text
				char itemtext[512];
				m_MapList->GetItemText(i, itemtext, sizeof(itemtext));

				//compare with the actuall map name
				if (!Q_strcmp(itemtext, ActuallMap))
				{
					m_MapList->ActivateItem(i);
					break;
				}
			}

			//set the minimap image
			KeyValues* data = m_MapList->GetActiveItemUserData();
			if (data)
			{
				//set the minimap image
				const char* image = data->GetString("Image");
				char minimapimage[512];
				Q_snprintf(minimapimage, sizeof(minimapimage), "geo_guesser/full_maps/%s", image);
				m_MiniMap->SetImage(minimapimage);
			}

			//disable the combo box for now
			m_MapList->SetEnabled(false);
		}
		else
		{
			//get the points value
			int points = m_MiniMap->GetPointsValue(position);

			//set the points label
			m_CurrentScoreLabel->SetText(CFmtStr("You Scored %d", points));

			//add points to the current score
			GetPanel()->GetGGInfo().current_score += points;
		}

		//show the actuall pos pin marker
		m_MiniMap->SetActuallPosMarker(position);

		//reset the minimap image bounds
		m_MiniMap->ResetImageBounds();
	}
	else
	{
		//set texts and states
		m_SubmitAnswerButton->SetText("Submit Answer");
		m_SubmitAnswerButton->SetCommand(SUBMIT_COMMAND);
		m_SkipButtonm->SetEnabled(true);
		m_FinishButton->SetEnabled(true);

		//reset the minimap
		m_MiniMap->Reset();
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called after a map has been finished. Goes to the next map
//---------------------------------------------------------------------------------
void CGG_Game_Page::OnMapFinished()
{
	CGG_MainPanel::GeoGuesserInfo_t& info = GetPanel()->GetGGInfo();

	//get the position
	CGG_MainPanel::MapData_t::MapType_e difficulty = (CGG_MainPanel::MapData_t::MapType_e)GetPanel()->GetGGInfo().difficulty;
	Vector2D position = m_MapData[CurrentSelectedMap]->MapLocations[CurrentSelectedPosition].positions[difficulty];

	//get the score text
	m_CurrentScoreLabel->SetText(CFmtStr("Current Score: %d", info.current_score));

	//set the finished states
	SetFinishedState(false);

	//get the number of rounds
	info.rounds_played++;
	if (info.rounds_played >= info.rounds)
	{
		//compare the score to the high score
		if (info.current_score >= info.high_score)
			info.high_score = info.current_score;

		//if we finished then go to the finished page. TODO: ADD FINISHING PAGE
		GetPanel()->NavigateToPage(FINISHED_PAGE_NAME);
		return;
	}

	//now get the next map
	PostNavigateTo();
}

//---------------------------------------------------------------------------------
// Purpose: Called on command (e.g when a button is pressed)
//---------------------------------------------------------------------------------
void CGG_Game_Page::OnCommand(const char* command)
{
	//check for finish command
	if (!Q_strcmp(command, FINISH_COMMAND))
	{
		//prompt the user
		vgui::QueryBox* prompt = new vgui::QueryBox("Are you sure", "Are you sure you would like to leave now?", nullptr);
		prompt->SetOKButtonText("Yes");
		prompt->SetCancelButtonText("No");
		prompt->SetOKCommand(new KeyValues("Command", "command", FINISH_COMMAND_CONFIRM));
		prompt->SetCancelButtonVisible(false);
		prompt->DoModal(GetPanel());
		prompt->AddActionSignalTarget(GetPanel());
		prompt->MoveToCenterOfScreen();
		return;
	}
	
	//check for confirm finish command
	else if (!Q_strcmp(command, FINISH_COMMAND_CONFIRM))
		GetPanel()->NavigateToPage(FINISHED_PAGE_NAME);

	//check for skip command
	else if (!Q_strcmp(command, SKIP_COMMAND))
	{
		//remove the pin marker
		m_MiniMap->RemovePinMarker();
		SetFinishedState(true);
	}

	//check for next command
	else if (!Q_strcmp(command, NEXT_COMMAND))
		OnMapFinished();

	//check for sumbit command
	else if (!Q_strcmp(command, SUBMIT_COMMAND))
	{
		//see if we have a pin or not
		if (!m_MiniMap->IsMarked())
		{
			GetPanel()->ShowError("Error", "No position marked on the mini map!");
			return;
		}

		//set the map finished state
		SetFinishedState(true);
	}
}

//---------------------------------------------------------------------------------
// Purpose: Called when text in the combo box is changed
//---------------------------------------------------------------------------------
void CGG_Game_Page::OnTextChanged()
{
	//see if the combo box is enabled or not
	if (!m_MapList->IsEnabled())
		return;

	//get the data
	KeyValues* data = m_MapList->GetActiveItemUserData();
	if (!data)
		return;

	//set the minimap image
	const char* image = data->GetString("Image");
	char minimapimage[512];
	Q_snprintf(minimapimage, sizeof(minimapimage), "geo_guesser/full_maps/%s", image);

	m_MiniMap->SetImage(minimapimage);
}

//---------------------------------------------------------------------------------
// Purpose: Called when this page gets navigated to
//---------------------------------------------------------------------------------
void CGG_Game_Page::NavigateTo()
{
	//reset the rounds
	GetPanel()->GetGGInfo().rounds_played = 0;
	GetPanel()->GetGGInfo().current_score = 0;

	//clear the map data and queue
	m_MapData.RemoveAll();
	m_MapQueue.RemoveAll();

	//call base function
	I_GG_Page::NavigateTo();

	//set current score label
	m_CurrentScoreLabel->SetText("Current Score: 0");
}

//---------------------------------------------------------------------------------
// Purpose: Called after the page has been navigated to
//---------------------------------------------------------------------------------
void CGG_Game_Page::PostNavigateTo()
{
	//clear the current map data for this page
	CurrentSelectedMap = 0;
	CurrentSelectedPosition = 0;

	//disable the map list combo box and remove everything
	m_MapList->RemoveAll();
	m_MapList->SetEnabled(false);

	//see if we have any maps?
	if (m_MapData.Count() == 0)
	{
		//Store the map data
		CUtlVector<CGG_MainPanel::MapData_t*>& data = GetPanel()->GetGGInfo().mapdata;

		//get the map list
		for (int i = 0; i < data.Count(); i++)
		{
			if (data[i]->enabled)
				m_MapData.AddToTail(data[i]);
		}
	}

	//remove queue[0]
	if (m_MapQueue.Count() > 0)
		m_MapQueue.Remove(0);

	//check the queue size
	if (m_MapQueue.Count() <= 0)
	{
		//rebuild the queue
		for (int i = 0; i < m_MapData.Count(); i++)
		{
			for (int j = 0; j < m_MapData[i]->MapLocations.Count(); j++)
			{
				//add the index into the queue
				if (m_MapData[i]->MapLocations[j].enabled)
					m_MapQueue.AddToTail(MapQueueItem_t{ i, j });
			}
		}

		//shuffle the queue twice
		for (int _ = 0; _ < 2; _++)
		{
			for (int i = 0; i < m_MapQueue.Count(); i++)
			{
				int index = random->RandomInt(0, m_MapQueue.Count() - 1);

				MapQueueItem_t temp = m_MapQueue[i];
				m_MapQueue[i] = m_MapQueue[index];
				m_MapQueue[index] = temp;
			}
		}
	}

	//see if we failed to add any items
	if (m_MapQueue.Count() < 0)
	{
		//error
		GetPanel()->ShowError("Error", "Error: Got 0 selected maps. Failed to create map queue!");
		GetPanel()->NavigateToPage(MAINMENU_PAGE_NAME);
		return;
	}

	//get the next position in the queue
	CurrentSelectedMap = m_MapQueue[0].map;
	CurrentSelectedPosition = m_MapQueue[0].position;
	
	//get locations
	CUtlVector<CGG_MainPanel::MapData_t::MapLocation_t>& locations = m_MapData[CurrentSelectedMap]->MapLocations;

	//get the reference image
	char reference[512];
	Q_snprintf(reference, sizeof(reference), "geo_guesser/positions/%s/%s", GetPanel()->GetStringForSymbol(m_MapData[CurrentSelectedMap]->MapName), GetPanel()->GetStringForSymbol(locations[CurrentSelectedPosition].image));

	//set the find image
	m_CurrentFindFromImage->SetImage(reference);

	//get the minimap image(s)
	CUtlVector<CGG_MainPanel::MapData_t::MapImages_t>& MapImages = m_MapData[CurrentSelectedMap]->MapImages[(CGG_MainPanel::MapData_t::MapType_e)GetPanel()->GetGGInfo().difficulty];
	
	//if only 1 image then set the mini map image and do nothing else
	if (MapImages.Count() <= 1)
	{
		char minimapimage[512];
		Q_snprintf(minimapimage, sizeof(minimapimage), "geo_guesser/full_maps/%s", GetPanel()->GetStringForSymbol(MapImages[0].MapImage));

		//set the minimap image to the image
		m_MiniMap->SetImage(minimapimage);
	}
	else
	{
		//activate the combo box and add each item
		m_MapList->SetEnabled(true);

		for (int i = 0; i < MapImages.Count(); i++)
			m_MapList->AddItem(GetPanel()->GetStringForSymbol(MapImages[i].MapName), new KeyValues("Data", "Image", GetPanel()->GetStringForSymbol(MapImages[i].MapImage)));

		//activate item 0
		m_MapList->ActivateItem(0);
	}

	//set the m_CurrentRoundLabel text
	m_CurrentRoundLabel->SetText(CFmtStr("Current Round = %d/%d", GetPanel()->GetGGInfo().rounds_played + 1, GetPanel()->GetGGInfo().rounds));
}