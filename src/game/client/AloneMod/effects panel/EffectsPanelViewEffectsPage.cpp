#include "cbase.h"
#include "EffectsPanelViewEffectsPage.h"
#include "AloneMod/AmodCvars.h"

extern ConVar v_viewmodel_fov;
extern ConVar r_drawviewmodel;
extern ConVar fov_desired;
extern ConVar cl_pitchdown;
extern ConVar cl_pitchup;

//black box slider values
#define BLACK_BOX_WIDTH_SLIDER_MIN 0
#define BLACK_BOX_WIDTH_SLIDER_MAX 30
#define BLACK_BOX_WIDTH_SLIDER_DEFAULT 0

#define BLACK_BOX_HEIGHT_SLIDER_MIN 0
#define BLACK_BOX_HEIGHT_SLIDER_MAX 30
#define BLACK_BOX_HEIGHT_SLIDER_DEFAULT 6

//claustraphobia command
#define CLAUSTRAPHOBIA_BUTTON_COMMAND "ToggleClaustraphobia"

//claustraphobia slider values
#define CLAUSTRAPHOBIA_SLIDER_MIN 1
#define CLAUSTRAPHOBIA_SLIDER_MAX 40
#define CLAUSTRAPHOBIA_SLIDER_DEFAULT 7

#define CLAUSTRAPHOBIA_FOV_SLIDER_MIN 10
#define CLAUSTRAPHOBIA_FOV_SLIDER_MAX 170
#define CLAUSTRAPHOBIA_FOV_SLIDER_DEFAULT 100

//viewmodel slider value
#define VIEWMODEL_FOV_SLIDER_MIN 5
#define VIEWMODEL_FOV_SLIDER_MAX 360
#define VIEWMODEL_FOV_SLIDER_DEFAULT 54

//smooth angles slider value
#define SMOOTH_ANGLES_SLIDER_MIN 50
#define SMOOTH_ANGLES_SLIDER_MAX 1
#define SMOOTH_ANGLES_SLIDER_DEFAULT 50

//smooth origin slider value
#define SMOOTH_ORIGIN_SLIDER_MIN 20
#define SMOOTH_ORIGIN_SLIDER_MAX 1
#define SMOOTH_ORIGIN_SLIDER_DEFAULT 20
#define SMOOTH_ANGLES_ORIGIN_DIVIDER 1000

//origin/angle offset defaults
#define ORIGIN_OFFSET_DEFAULT "0 0 0"
#define ANGLE_OFFSET_DEFAULT "0 0 0"

//pitch slider values
#define MINIMUM_PITCH_SLIDER_MIN 181
#define MINIMUM_PITCH_SLIDER_MAX 0
#define MINIMUM_PITCH_SLIDER_DEFAULT 181

#define MAXIMUM_PITCH_SLIDER_MIN 181
#define MAXIMUM_PITCH_SLIDER_MAX 0
#define MAXIMUM_PITCH_SLIDER_DEFAULT 181

//---------------------------------------------------------------------------------
// Purpose: Constructor for the effects panel view effects page
//---------------------------------------------------------------------------------
CEffectsPanelViewEffects::CEffectsPanelViewEffects(vgui::Panel* parent, const char* name)
	: BaseClass(parent, name)
{
	//set the panel options
	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	//create the left hand side check buttons
	m_DontDrawViewmodel = new vgui::CheckButton(this, "DontDrawViewmodel",					"Dont Draw Weapon Viewmodel");
	m_EnableBlackAndWhiteView = new vgui::CheckButton(this, "EnableBlackAndWhiteView",		"Enable black and white view");
	m_EnableLenseDirtOnScreen = new vgui::CheckButton(this, "EnableLenseDirtOnScreen",		"Enable lense dirt on screen");
	m_EnableTvStyledView = new vgui::CheckButton(this, "EnableTvStyledView",				"Enable tv styled view");
	m_EnableColoredTvStyledView = new vgui::CheckButton(this, "EnableColoredTvStyledView",	"Enable colored tv styled view");
	m_EnableBlurredView = new vgui::CheckButton(this, "EnableBlurredView",					"Enable blurred view");
	m_EnableBlackBoxes = new vgui::CheckButton(this, "EnableBlackBoxes",					"Enable cinematic black boxes");
	m_OverrideViewmodelFov = new vgui::CheckButton(this, "OverrideViewmodelFov",			"Override Viewmodel Fov");
	m_EnableClaustraphobia = new vgui::CheckButton(this, "EnableClaustraphobia",			"Enable claustraphobic view");
	m_EnableClaustraphobia->SetCommand(CLAUSTRAPHOBIA_BUTTON_COMMAND);

	//horizontal divider
	m_HorizontalDivider = new vgui::Divider(this, "HorizontalDivider");

	//create the bottom stuff
	m_BlackBoxWidthText = new vgui::Label(this, "BlackBoxWidthText", "Black Boxes Width");
	m_BlackBoxWidthSlider = new vgui::Slider(this, "BlackBoxWidthSlider");
	m_BlackBoxWidthSlider->AddActionSignalTarget(this);
	m_BlackBoxWidthSlider->SetRange(BLACK_BOX_WIDTH_SLIDER_MIN, BLACK_BOX_WIDTH_SLIDER_MAX);
	m_BlackBoxWidthSlider->SetValue(BLACK_BOX_WIDTH_SLIDER_DEFAULT);

	m_BlackBoxHeightText = new vgui::Label(this, "BlackBoxHeightText", "Black Boxes Height");
	m_BlackBoxHeightSlider = new vgui::Slider(this, "BlackBoxHeightSlider");
	m_BlackBoxHeightSlider->AddActionSignalTarget(this);
	m_BlackBoxHeightSlider->SetRange(BLACK_BOX_HEIGHT_SLIDER_MIN, BLACK_BOX_HEIGHT_SLIDER_MAX);
	m_BlackBoxHeightSlider->SetValue(BLACK_BOX_HEIGHT_SLIDER_DEFAULT);

	m_ClaustrapphobiaAmountText = new vgui::Label(this, "ClaustraphobiaAmountText", "Claustraphobia Amount");
	m_ClaustrapphobiaAmountSlider = new vgui::Slider(this, "ClaustraphobiaAmountSlider");
	m_ClaustrapphobiaAmountSlider->AddActionSignalTarget(this);
	m_ClaustrapphobiaAmountSlider->SetRange(CLAUSTRAPHOBIA_SLIDER_MIN, CLAUSTRAPHOBIA_SLIDER_MAX);
	m_ClaustrapphobiaAmountSlider->SetValue(CLAUSTRAPHOBIA_SLIDER_DEFAULT);

	m_ClaustrapphobiaFovText = new vgui::Label(this, "ClaustraphobiaFovText", "Claustraphobia Fov Amount");
	m_ClaustrapphobiaFovSlider = new vgui::Slider(this, "ClaustraphobiaFovSlider");
	m_ClaustrapphobiaFovSlider->AddActionSignalTarget(this);
	m_ClaustrapphobiaFovSlider->SetRange(CLAUSTRAPHOBIA_FOV_SLIDER_MIN, CLAUSTRAPHOBIA_FOV_SLIDER_MAX);
	m_ClaustrapphobiaFovSlider->SetValue(fov_desired.GetInt());
	m_ClaustrapphobiaFovSlider->SetEnabled(false);

	m_ViewmodelFovOverrideText = new vgui::Label(this, "ViewmodelFovOverrideText", "Viewmodel Fov Amount");
	m_ViewmodelFovOverrideSlider = new vgui::Slider(this, "ViewmodelFovOverrideSlider");
	m_ViewmodelFovOverrideSlider->AddActionSignalTarget(this);
	m_ViewmodelFovOverrideSlider->SetRange(VIEWMODEL_FOV_SLIDER_MIN, VIEWMODEL_FOV_SLIDER_MAX);
	m_ViewmodelFovOverrideSlider->SetValue(v_viewmodel_fov.GetInt());
	
	//verticle divider
	m_VerticalDivider = new vgui::Divider(this, "VerticleDivider");

	//finally create the right hand side stuff
	m_EnableCameraEditor = new vgui::CheckButton(this, "EnableCameraEditor",							"Enable Camera Editor");
	m_EnableCameraEditorViewmodelFix = new vgui::CheckButton(this, "EnableSmoothCameraViewmodelFix",	"Enable Smooth Camera Viewmodel Fix");
	
	m_SmoothAngleAmountText = new vgui::Label(this, "SmoothAngleAmountText", "Angles Smooth Amount");
	m_SmoothAngleAmountSlider = new vgui::Slider(this, "SmoothAngleAmountSlider");
	m_SmoothAngleAmountSlider->AddActionSignalTarget(this);
	m_SmoothAngleAmountSlider->SetRange(SMOOTH_ANGLES_SLIDER_MIN, SMOOTH_ANGLES_SLIDER_MAX);
	m_SmoothAngleAmountSlider->SetValue(SMOOTH_ANGLES_SLIDER_DEFAULT);
	
	m_SmoothOriginAmountText = new vgui::Label(this, "SmoothOriginAmountText", "Origin Smooth Amount");
	m_SmoothOriginAmountSlider = new vgui::Slider(this, "SmoothOriginAmountSlider");
	m_SmoothOriginAmountSlider->AddActionSignalTarget(this);
	m_SmoothOriginAmountSlider->SetRange(SMOOTH_ORIGIN_SLIDER_MIN, SMOOTH_ORIGIN_SLIDER_MAX);
	m_SmoothOriginAmountSlider->SetValue(SMOOTH_ORIGIN_SLIDER_DEFAULT);

	m_OriginOverrideText = new vgui::Label(this, "OriginOffsetText", "Origin Offset");
	m_OriginOverrideTextEntry = new vgui::TextEntry(this, "OriginOffsetTextEntry");
	m_OriginOverrideTextEntry->SetText(ORIGIN_OFFSET_DEFAULT);
	m_OriginOverrideTextEntry->SetMaximumCharCount(30);
	m_OriginOverrideTextEntry->AddActionSignalTarget(this);
	
	m_AngleOverrideText = new vgui::Label(this, "AngleOffsetText", "Angle Offset");
	m_AngleOverrideTextEntry = new vgui::TextEntry(this, "AngleOffsetTextEntry");
	m_AngleOverrideTextEntry->SetText(ANGLE_OFFSET_DEFAULT);
	m_AngleOverrideTextEntry->SetMaximumCharCount(30);
	m_AngleOverrideTextEntry->AddActionSignalTarget(this);

	m_MinimumPitchText = new vgui::Label(this, "MinimumPitchText", "Minimum View Pitch");
	m_MinimumPitchSlider = new vgui::Slider(this, "SmoothAngleAmountSlider");
	m_MinimumPitchSlider->AddActionSignalTarget(this);
	m_MinimumPitchSlider->SetRange(MINIMUM_PITCH_SLIDER_MIN, MINIMUM_PITCH_SLIDER_MAX);
	m_MinimumPitchSlider->SetValue(MINIMUM_PITCH_SLIDER_DEFAULT);

	m_MaximumPitchText = new vgui::Label(this, "MaximumPitchText", "Maximum View Pitch");
	m_MaximumPitchSlider = new vgui::Slider(this, "MaximumPitchSlider");
	m_MaximumPitchSlider->AddActionSignalTarget(this);
	m_MaximumPitchSlider->SetRange(MAXIMUM_PITCH_SLIDER_MIN, MAXIMUM_PITCH_SLIDER_MAX);
	m_MaximumPitchSlider->SetValue(MAXIMUM_PITCH_SLIDER_DEFAULT);

	m_iPrevFovValue = fov_desired.GetInt();

	//add all the tooltips
	ADD_TOOLTIP(m_DontDrawViewmodel, 100, "If selected then the player's weapon model wont show, Else it will.", true);
	ADD_TOOLTIP(m_EnableBlackAndWhiteView, 100, "Enables a noir black and white filter.", false);
	ADD_TOOLTIP(m_EnableLenseDirtOnScreen, 100, "Adds lense dirt onto the players screen.", false);
	ADD_TOOLTIP(m_EnableTvStyledView, 100, "Adds a bodycan styled old tv overlay onto the screen.", true);
	ADD_TOOLTIP(m_EnableColoredTvStyledView, 100, "Adds a blue colored bodycan styled old tv overlay onto the screen.", true);
	ADD_TOOLTIP(m_EnableBlurredView, 100, "Adds a blur effect onto the screen", false);
	ADD_TOOLTIP(m_EnableBlackBoxes, 100, "Adds cinematic black edge boxes onto the screen.\nChange the size with the black box sliders below", true);
	ADD_TOOLTIP(m_EnableClaustraphobia, 100, "Enables a claustraphobic styled view effect that can be modified with the claustraphobia sliders below.", true);
	ADD_TOOLTIP(m_OverrideViewmodelFov, 100, "Overrides the players viewmodel fov using the sliders value below.", true);

	ADD_TOOLTIP(m_EnableCameraEditor, 100, "Enables all the camera features below", false);
	ADD_TOOLTIP(m_EnableCameraEditorViewmodelFix, 100, "When the angle smooth amount or origin smooth amount slider is set to something other then 0, and the player moves/looks around, The viewmodel can move faster then the camera. Select this to fix this", true);
	ADD_TOOLTIP(m_SmoothAngleAmountSlider, 100, "Changes how smooth the players view moves when looking around", true);
	ADD_TOOLTIP(m_SmoothOriginAmountSlider, 100, "Changes how smooth the players view moves moving around", true);
	ADD_TOOLTIP(m_OriginOverrideTextEntry, 100, "Determines the players position offset using the (forward/back left/right up/down) format", true);
	ADD_TOOLTIP(m_AngleOverrideTextEntry, 100, "Determines the players angle offset using the (pitch yaw roll) format", true);
	ADD_TOOLTIP(m_MinimumPitchSlider, 100, "Changes how far the player can look down", false);
	ADD_TOOLTIP(m_MaximumPitchSlider, 100, "Changes how far the player can look up", false);

	ADD_TOOLTIP(m_BlackBoxWidthSlider, 100, "Sets how much room (width) the black boxes should take up on either side of the screen if the 'Enable cinematic black boxes' check button is selected", true);
	ADD_TOOLTIP(m_BlackBoxHeightSlider, 100, "Sets how much room (height) the black boxes should take up on either side of the screen if the 'Enable cinematic black boxes' check button is selected", true);
	ADD_TOOLTIP(m_ClaustrapphobiaAmountSlider, 100, "Sets how claustraphobic the players view should be if the 'Enable claustraphobia' check button is selected", true);
	ADD_TOOLTIP(m_ClaustrapphobiaFovSlider, 100, "Overrides the players fov tho this amount if the 'Enable claustraphobia' check button is selected", true);
	ADD_TOOLTIP(m_ViewmodelFovOverrideSlider, 100, "Overrides the players viewmodel fov to this amount if the 'Override viewmodel fov' check button is selected", true);

	//reset these to the defaults
	ResetEffects();
	
	//set the bounds for each item
	PerformLayout();
}

//---------------------------------------------------------------------------------
// Purpose: Resets all the effects on this page
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::ResetEffects()
{
	//set/select left hand side items
	m_EnableBlackAndWhiteView->SetSelected(false);
	m_EnableLenseDirtOnScreen->SetSelected(false);
	m_EnableTvStyledView->SetSelected(false);
	m_EnableColoredTvStyledView->SetSelected(false);
	m_EnableBlurredView->SetSelected(false);
	m_EnableBlackBoxes->SetSelected(false);
	m_EnableClaustraphobia->SetSelected(false);
	m_OverrideViewmodelFov->SetSelected(false);
	m_OverrideViewmodelFov->SetEnabled(false);
	m_DontDrawViewmodel->SetSelected(false);

	//bottom sliders
	m_BlackBoxWidthSlider->SetEnabled(false);
	m_BlackBoxWidthSlider->SetValue(BLACK_BOX_WIDTH_SLIDER_DEFAULT);

	m_BlackBoxHeightSlider->SetEnabled(false);
	m_BlackBoxHeightSlider->SetValue(BLACK_BOX_HEIGHT_SLIDER_DEFAULT);

	m_ClaustrapphobiaAmountSlider->SetEnabled(false);
	m_ClaustrapphobiaAmountSlider->SetValue(CLAUSTRAPHOBIA_SLIDER_DEFAULT);

	m_ClaustrapphobiaFovSlider->SetEnabled(false);
	m_ClaustrapphobiaFovSlider->SetValue(CLAUSTRAPHOBIA_FOV_SLIDER_DEFAULT);

	m_ViewmodelFovOverrideSlider->SetEnabled(false);
	m_ViewmodelFovOverrideSlider->SetValue(VIEWMODEL_FOV_SLIDER_DEFAULT);

	//right items
	m_EnableCameraEditor->SetSelected(false);

	m_EnableCameraEditorViewmodelFix->SetEnabled(true);
	m_EnableCameraEditorViewmodelFix->SetSelected(true);

	m_SmoothAngleAmountSlider->SetEnabled(false);
	m_SmoothAngleAmountSlider->SetValue(SMOOTH_ANGLES_SLIDER_DEFAULT);
	
	m_SmoothOriginAmountSlider->SetEnabled(false);
	m_SmoothOriginAmountSlider->SetValue(SMOOTH_ORIGIN_SLIDER_DEFAULT);

	m_OriginOverrideTextEntry->SetEnabled(false);
	m_OriginOverrideTextEntry->SetText(ORIGIN_OFFSET_DEFAULT);
	
	m_AngleOverrideTextEntry->SetEnabled(false);
	m_AngleOverrideTextEntry->SetText(ANGLE_OFFSET_DEFAULT);
	
	m_MinimumPitchSlider->SetEnabled(false);
	m_MinimumPitchSlider->SetValue(MINIMUM_PITCH_SLIDER_DEFAULT);
	
	m_MaximumPitchSlider->SetEnabled(false);
	m_MaximumPitchSlider->SetValue(MAXIMUM_PITCH_SLIDER_DEFAULT);

	//convars
	v_viewmodel_fov.SetValue(54);
	r_drawviewmodel.SetValue(true);
	fov_desired.SetValue(m_iPrevFovValue);
	amod_camera_cinematic.SetValue(0);
	cl_pitchdown.SetValue(89);
	cl_pitchup.SetValue(89);
}

//---------------------------------------------------------------------------------
// Purpose: Reads from the file
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::ReadFromFile(KeyValues* keyvalues, bool reset)
{
	//reset everything
	if (reset)
		ResetEffects();

	//find the view KeyValues
	KeyValues* view = keyvalues->FindKey("View");
	if (!view)
		return;

	//set all the panel/checkbutton/slider values from KeyValues
	m_DontDrawViewmodel->SetSelected(!view->GetBool("ViewModel:Draw", true));
	m_OverrideViewmodelFov->SetSelected(view->GetBool("ViewModel:OverrideFov", false));
	m_ViewmodelFovOverrideSlider->SetValue(view->GetInt("ViewModel:FovAmount", VIEWMODEL_FOV_SLIDER_DEFAULT));

	//other filter stuff
	m_EnableBlackAndWhiteView->SetSelected(view->GetBool("Filter:BlackAndWhite", false));
	m_EnableLenseDirtOnScreen->SetSelected(view->GetBool("Filter:LenseDirt", false));
	m_EnableTvStyledView->SetSelected(view->GetBool("Filter:TvView", false));
	m_EnableColoredTvStyledView->SetSelected(view->GetBool("Filter:ColoredTvView", false));
	m_EnableBlurredView->SetSelected(view->GetBool("Filter:BluredView", false));

	//black boxes
	m_EnableBlackBoxes->SetSelected(view->GetBool("Filter:BlackBoxes:Enable", false));
	m_BlackBoxWidthSlider->SetValue(view->GetInt("Filter:BlackBoxes:Width", BLACK_BOX_WIDTH_SLIDER_DEFAULT));
	m_BlackBoxHeightSlider->SetValue(view->GetInt("Filter:BlackBoxes:Height", BLACK_BOX_HEIGHT_SLIDER_DEFAULT));

	//claustraphobia
	m_EnableClaustraphobia->SetSelected(view->GetBool("Filter:Claustraphobia:Enable", false));
	m_ClaustrapphobiaAmountSlider->SetValue(view->GetInt("Filter:Claustraphobia:Amount", CLAUSTRAPHOBIA_SLIDER_DEFAULT));
	m_ClaustrapphobiaFovSlider->SetValue(view->GetInt("Filter:Claustraphobia:Fov", CLAUSTRAPHOBIA_FOV_SLIDER_DEFAULT));

	//camera stuff
	m_EnableCameraEditor->SetSelected(view->GetBool("Camera:EnableEditor", false));
	m_EnableCameraEditorViewmodelFix->SetSelected(view->GetBool("Camera:ViewmodelFix", true));
	m_SmoothAngleAmountSlider->SetValue(view->GetInt("Camera:SmoothAnglesAmount", SMOOTH_ANGLES_SLIDER_DEFAULT));
	m_SmoothOriginAmountSlider->SetValue(view->GetInt("Camera:SmoothOriginAmount", SMOOTH_ORIGIN_SLIDER_DEFAULT));
	m_MinimumPitchSlider->SetValue(view->GetInt("Camera:MinPitch", MINIMUM_PITCH_SLIDER_DEFAULT));
	m_MaximumPitchSlider->SetValue(view->GetInt("Camera:MaxPitch", MAXIMUM_PITCH_SLIDER_DEFAULT));
	m_OriginOverrideTextEntry->SetText(view->GetString("Camera:OriginOffset", ORIGIN_OFFSET_DEFAULT));
	m_AngleOverrideTextEntry->SetText(view->GetString("Camera:AngleOffset", ANGLE_OFFSET_DEFAULT));
}

//---------------------------------------------------------------------------------
// Purpose: Writes to the file
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::WriteToFile(KeyValues* keyvalues)
{
	//create a new KeyValues*
	KeyValues* view = new KeyValues("View");

	//set all the keyvalue stuff
	view->SetBool("ViewModel:Draw", !m_DontDrawViewmodel->IsSelected());
	view->SetBool("ViewModel:OverrideFov", m_OverrideViewmodelFov->IsSelected());
	view->SetBool("ViewModel:FovAmount", m_ViewmodelFovOverrideSlider->GetValue());

	//other filter stuff
	view->SetBool("Filter:BlackAndWhite", m_EnableBlackAndWhiteView->IsSelected());
	view->SetBool("Filter:LenseDirt", m_EnableLenseDirtOnScreen->IsSelected());
	view->SetBool("Filter:TvView", m_EnableTvStyledView->IsSelected());
	view->SetBool("Filter:ColoredTvView", m_EnableColoredTvStyledView->IsSelected());
	view->SetBool("Filter:BluredView", m_EnableBlurredView->IsSelected());
	
	//black boxes
	view->SetBool("Filter:BlackBoxes:Enable", m_EnableBlackBoxes->IsSelected());
	view->SetInt("Filter:BlackBoxes:Width", m_BlackBoxWidthSlider->GetValue());
	view->SetInt("Filter:BlackBoxes:Height", m_BlackBoxHeightSlider->GetValue());

	//claustraphobia
	view->SetBool("Filter:Claustraphobia:Enable", m_EnableClaustraphobia->IsSelected());
	view->SetInt("Filter:Claustraphobia:Amount", m_ClaustrapphobiaAmountSlider->GetValue());
	view->SetInt("Filter:Claustraphobia:Fov", m_ClaustrapphobiaFovSlider->GetValue());

	//camera stuff
	view->SetBool("Camera:EnableEditor", m_EnableCameraEditor->IsSelected());
	view->SetBool("Camera:ViewmodelFix", m_EnableCameraEditorViewmodelFix->IsSelected());
	view->SetInt("Camera:SmoothAnglesAmount", m_SmoothAngleAmountSlider->GetValue());
	view->SetInt("Camera:SmoothOriginAmount", m_SmoothOriginAmountSlider->GetValue());
	view->SetInt("Camera:MinPitch", m_MinimumPitchSlider->GetValue());
	view->SetInt("Camera:MaxPitch", m_MaximumPitchSlider->GetValue());

	//get origin and angle offset
	char origin[32], angle[32];
	m_OriginOverrideTextEntry->GetText(origin, sizeof(origin));
	m_AngleOverrideTextEntry->GetText(angle, sizeof(angle));
	view->SetString("Camera:OriginOffset", origin);
	view->SetString("Camera:AngleOffset", angle);

	//add the view settings
	keyvalues->AddSubKey(view);
}

//---------------------------------------------------------------------------------
// Purpose: Called every 30ms
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::OnTick()
{
	//get/set all the convars
	static ConVar* mat_yuv = cvar->FindVar("mat_yuv");

	//set the viewmodel stuff
	r_drawviewmodel.SetValue(!m_DontDrawViewmodel->IsSelected());
	m_OverrideViewmodelFov->SetEnabled(!m_DontDrawViewmodel->IsSelected());
	m_ViewmodelFovOverrideSlider->SetEnabled(m_OverrideViewmodelFov->IsSelected() && m_OverrideViewmodelFov->IsEnabled());
	if (m_ViewmodelFovOverrideSlider->IsEnabled())
		v_viewmodel_fov.SetValue(m_ViewmodelFovOverrideSlider->GetValue());
	else
		v_viewmodel_fov.SetValue(VIEWMODEL_FOV_SLIDER_DEFAULT);





	//black boxes
	amod_view_square.SetValue(m_EnableBlackBoxes->IsSelected());
	m_BlackBoxWidthSlider->SetEnabled(amod_view_square.GetBool());
	m_BlackBoxHeightSlider->SetEnabled(amod_view_square.GetBool());
	if (amod_view_square.GetBool())
	{
		amod_view_square_width.SetValue((float)(m_BlackBoxWidthSlider->GetValue()) / BLACK_BOX_WIDTH_SLIDER_MAX);
		amod_view_square_height.SetValue((float)(m_BlackBoxHeightSlider->GetValue()) / BLACK_BOX_HEIGHT_SLIDER_MAX);
	}





	//claustraphobia
	amod_view_claustrophobia.SetValue(m_EnableClaustraphobia->IsSelected());
	m_ClaustrapphobiaAmountSlider->SetEnabled(amod_view_claustrophobia.GetBool());
	m_ClaustrapphobiaFovSlider->SetEnabled(amod_view_claustrophobia.GetBool());
	if (amod_view_claustrophobia.GetBool())
	{
		amod_view_claustrophobia_amt.SetValue((float)(m_ClaustrapphobiaAmountSlider->GetValue()) / ((float)CLAUSTRAPHOBIA_SLIDER_MAX/10));
		fov_desired.SetValue(m_ClaustrapphobiaFovSlider->GetValue());
	}
	else
	{
		m_iPrevFovValue = fov_desired.GetInt();
	}






	//camera editor
	bool CameraEditor = m_EnableCameraEditor->IsSelected();
	m_EnableCameraEditorViewmodelFix->SetEnabled(CameraEditor);
	amod_camera_cinematic.SetValue(CameraEditor);
	amod_camera_cinematic_fix.SetValue(m_EnableCameraEditorViewmodelFix->IsSelected());



	//smooth angle slider
	m_SmoothAngleAmountSlider->SetEnabled(CameraEditor);
	if (m_SmoothAngleAmountSlider->IsEnabled())
	{
		amod_camera_cinematic_lag_angles.SetValue(m_SmoothAngleAmountSlider->GetValue() != SMOOTH_ANGLES_SLIDER_MIN);
		amod_camera_cinematic_lag_angles_amt.SetValue(((float)m_SmoothAngleAmountSlider->GetValue() / SMOOTH_ANGLES_ORIGIN_DIVIDER));
	}
	else
	{
		amod_camera_cinematic_lag_angles.SetValue(0);
	}



	//smooth origin slider
	m_SmoothOriginAmountSlider->SetEnabled(CameraEditor);
	if (m_SmoothOriginAmountSlider->IsEnabled())
	{
		amod_camera_cinematic_lag_origin.SetValue(m_SmoothOriginAmountSlider->GetValue() != SMOOTH_ORIGIN_SLIDER_MIN);
		amod_camera_cinematic_lag_origin_amt.SetValue(((float)m_SmoothOriginAmountSlider->GetValue() / SMOOTH_ANGLES_ORIGIN_DIVIDER));
	}
	else
	{
		amod_camera_cinematic_lag_origin.SetValue(0);
	}
	
	
	
	
	//origin offset text entry
	m_OriginOverrideTextEntry->SetEnabled(CameraEditor);
	m_AngleOverrideTextEntry->SetEnabled(CameraEditor);
	if (m_EnableCameraEditor->IsEnabled())
	{
		//set the origin
		char origin[32];
		m_OriginOverrideTextEntry->GetText(origin, sizeof(origin));
		amod_view_override_xyz_amt.SetValue(origin);
		
		//set the angle
		char angle[32];
		m_AngleOverrideTextEntry->GetText(angle, sizeof(angle));
		amod_view_override_pyr_amt.SetValue(angle);
	}



	//pitch sliders
	m_MinimumPitchSlider->SetEnabled(CameraEditor);
	m_MaximumPitchSlider->SetEnabled(CameraEditor);
	if (CameraEditor)
	{
		int MinPitch = m_MinimumPitchSlider->GetValue() == MAXIMUM_PITCH_SLIDER_MIN ? 89 : m_MinimumPitchSlider->GetValue();
		int MaxPitch = m_MaximumPitchSlider->GetValue() == MAXIMUM_PITCH_SLIDER_MIN ? 89 : m_MaximumPitchSlider->GetValue();
		cl_pitchdown.SetValue(MinPitch);
		cl_pitchup.SetValue(MaxPitch);
	}
	else
	{
		cl_pitchdown.SetValue(89);
		cl_pitchup.SetValue(89);
	}


	//now set the other filter/effect stuff
	amod_view_lense_dirt.SetValue(m_EnableLenseDirtOnScreen->IsSelected());
	amod_view_bodycam.SetValue(m_EnableTvStyledView->IsSelected());
	amod_view_binoculars.SetValue(m_EnableColoredTvStyledView->IsSelected());
	amod_view_blur.SetValue(m_EnableBlurredView->IsSelected());
	amod_view_claustrophobia.SetValue(m_EnableClaustraphobia->IsSelected());

	//check black and white
	if (mat_yuv)
		mat_yuv->SetValue(m_EnableBlackAndWhiteView->IsSelected());
}

//---------------------------------------------------------------------------------
// Macros for layout
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::OnCommand(const char* pszCommand)
{
	if (!Q_strcmp(pszCommand, CLAUSTRAPHOBIA_BUTTON_COMMAND))
	{
		//reset the fov to a normal value if needed
		if (!m_EnableClaustraphobia->IsSelected())
			fov_desired.SetValue(m_iPrevFovValue);

		return;
	}

	BaseClass::OnCommand(pszCommand);
}

//---------------------------------------------------------------------------------
// Macros for layout
//---------------------------------------------------------------------------------

//macro for left hand side buttons
#define SET_CHECKBUTTON_BOUNDS(ctrl, x, y, w, h) \
	ctrl->SetBounds(x, y, w, h); \
	ctrl->SetVisible(true); \
	y = y + h + 2;

//macro for right-hand side label + slider pairs
#define SET_SLIDER_BOUNDS(label, slider, x, y, w, h) \
	label->SetBounds(x, y, w, h - 2); \
	label->SetVisible(true); \
	y = y + h; \
	slider->SetBounds(x, y, w, h); \
	slider->SetVisible(true); \
	y = y + h + 10;

//macro for generic single control that increments Y
#define SET_CONTROL_BOUNDS(ctrl, x, y, w, h, add) \
	ctrl->SetBounds(x, y, w, h); \
	ctrl->SetVisible(true); \
	y = y + add;

//macro for controls that do NOT modify Y (static placement)
#define SET_STATIC_CONTROL_BOUNDS(ctrl, x, y, w, h) \
	ctrl->SetBounds(x, y, w, h); \
	ctrl->SetVisible(true);


//---------------------------------------------------------------------------------
// Purpose: Sets the bounds for each item
//---------------------------------------------------------------------------------
void CEffectsPanelViewEffects::PerformLayout()
{
	int CheckButtonTall = 24;
	int CheckButtonY = 10;

	SET_CHECKBUTTON_BOUNDS(m_DontDrawViewmodel,			5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableBlackAndWhiteView,	5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableLenseDirtOnScreen,	5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableTvStyledView,		5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableColoredTvStyledView,	5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableBlurredView,			5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableBlackBoxes,			5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableClaustraphobia,		5, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_OverrideViewmodelFov,		5, CheckButtonY, 225, CheckButtonTall);

	CheckButtonY += 5;
	// Horizontal divider (static position — doesn’t modify Y)
	SET_STATIC_CONTROL_BOUNDS(m_HorizontalDivider, 0, CheckButtonY, 600, 2);

	const int SliderWidth = 172;
	int SlidersY = CheckButtonY + 15;

	SET_SLIDER_BOUNDS(m_BlackBoxWidthText, m_BlackBoxWidthSlider, 10, SlidersY, SliderWidth, CheckButtonTall);
	SET_SLIDER_BOUNDS(m_BlackBoxHeightText, m_BlackBoxHeightSlider, 10, SlidersY, SliderWidth, CheckButtonTall);

	SlidersY = CheckButtonY + 15;
	SET_SLIDER_BOUNDS(m_ClaustrapphobiaAmountText, m_ClaustrapphobiaAmountSlider, 10 + SliderWidth, SlidersY, SliderWidth, CheckButtonTall);
	SET_SLIDER_BOUNDS(m_ClaustrapphobiaFovText, m_ClaustrapphobiaFovSlider, 10 + SliderWidth, SlidersY, SliderWidth, CheckButtonTall);

	SlidersY = CheckButtonY + 15;
	SET_SLIDER_BOUNDS(m_ViewmodelFovOverrideText, m_ViewmodelFovOverrideSlider, 10 + (SliderWidth * 2), SlidersY, SliderWidth, CheckButtonTall);
	// Vertical divider (also static)
	SET_STATIC_CONTROL_BOUNDS(m_VerticalDivider, 250, 0, 2, CheckButtonY);

	CheckButtonY = 10;
	CheckButtonTall = 20;

	SET_CHECKBUTTON_BOUNDS(m_EnableCameraEditor, 255, CheckButtonY, 225, CheckButtonTall);
	SET_CHECKBUTTON_BOUNDS(m_EnableCameraEditorViewmodelFix, 255, CheckButtonY, 255, CheckButtonTall);

	SET_CONTROL_BOUNDS(m_SmoothAngleAmountText, 260, CheckButtonY, 265, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_SmoothAngleAmountSlider, 260, CheckButtonY, 265, CheckButtonTall, CheckButtonTall + 4);

	SET_CONTROL_BOUNDS(m_SmoothOriginAmountText, 260, CheckButtonY, 265, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_SmoothOriginAmountSlider, 260, CheckButtonY, 265, CheckButtonTall, CheckButtonTall + 4);

	SET_CONTROL_BOUNDS(m_OriginOverrideText, 260, CheckButtonY, 125, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_OriginOverrideTextEntry, 260, CheckButtonY, 125, CheckButtonTall, -CheckButtonTall);

	SET_CONTROL_BOUNDS(m_AngleOverrideText, 395, CheckButtonY, 125, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_AngleOverrideTextEntry, 395, CheckButtonY, 125, CheckButtonTall, CheckButtonTall + 10);

	SET_CONTROL_BOUNDS(m_MinimumPitchText, 260, CheckButtonY, 125, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_MinimumPitchSlider, 260, CheckButtonY, 125, CheckButtonTall, -CheckButtonTall);

	SET_CONTROL_BOUNDS(m_MaximumPitchText, 395, CheckButtonY, 125, CheckButtonTall, CheckButtonTall);
	SET_CONTROL_BOUNDS(m_MaximumPitchSlider, 395, CheckButtonY, 125, CheckButtonTall, CheckButtonTall);

	BaseClass::PerformLayout();
}
