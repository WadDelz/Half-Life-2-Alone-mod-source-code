#include "gamepadui_interface.h"
#include "gamepadui_frame.h"
#include "gamepadui_scroll.h"
#include "fmtstr.h"

#include "vgui_controls/ComboBox.h"

#include "KeyValues.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar gamepadui_last_amod_options_tab("gamepadui_last_amod_options_tab", "0", FCVAR_ARCHIVE);

//max tabs
const int MAX_OPTIONS_TABS = 8;

#define ALONE_MOD_OPTIONS_FILE GAMEPADUI_RESOURCE_FOLDER "amod_options.res"

//options tab struct
struct GamepadUIAloneModTab
{
    GamepadUIButton* pTabButton;
    CUtlVector< GamepadUIButton* > pButtons;
    GamepadUIScrollState ScrollState;
    bool bAlternating = true;
    bool bHorizontal = true;
};

//alone mod options panel
class GamepadUIAloneModOptionsPanel : public GamepadUIFrame
{
    DECLARE_CLASS_SIMPLE(GamepadUIAloneModOptionsPanel, GamepadUIFrame);
 
public:
    GamepadUIAloneModOptionsPanel(vgui::Panel* pParent, const char* pPanelName);
    ~GamepadUIAloneModOptionsPanel();

    void OnThink() OVERRIDE;
    void Paint() OVERRIDE;
    void LayoutCurrentTab();

    void LoadOptionTabs(const char* pszOptionsFile);
    void ApplySchemeSettings(vgui::IScheme* pScheme);

    //tab stuff
    void SetActiveTab(int nTab);
    int GetActiveTab();
    void OnMouseWheeled(int delta) OVERRIDE;

    void OnGamepadUIButtonNavigatedTo(vgui::VPANEL button);

    void UpdateGradients() OVERRIDE;

    void OnCommand(char const* pCommand);

private:
    //offset stuff
    GAMEPADUI_PANEL_PROPERTY(float, m_flTabsOffsetX, "Tabs.OffsetX", "0", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flTabsOffsetY, "Tabs.OffsetY", "0", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flOptionsFade, "Options.Fade", "80", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flScrollBarOffsetX, "Scrollbar.OffsetX", "10", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flScrollBarWidth, "Scrollbar.Width", "80", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flScrollBarHeight, "Scrollbar.Height", "80", SchemeValueTypes::ProportionalFloat);

    //tab stuff
    GamepadUIAloneModTab m_Tabs[MAX_OPTIONS_TABS];
    int m_nTabCount = 0;

    //glyphs
    GamepadUIGlyph m_leftGlyph;
    GamepadUIGlyph m_rightGlyph;
};

//----------------------------------------------------------------------------------------
// Purpose: Constructor for alone mod options panel
//----------------------------------------------------------------------------------------
GamepadUIAloneModOptionsPanel::GamepadUIAloneModOptionsPanel(vgui::Panel* pParent, const char* pPanelName)
    : BaseClass(pParent, pPanelName)
{
    vgui::HScheme Scheme = vgui::scheme()->LoadSchemeFromFile(GAMEPADUI_DEFAULT_PANEL_SCHEME, "SchemePanel");
    SetScheme(Scheme);

    GetFrameTitle() = GamepadUIString("#GameUI_AModOptions");
    SetFooterButtons(FooterButtons::Apply | FooterButtons::Back);

    Activate();

    LoadOptionTabs(ALONE_MOD_OPTIONS_FILE);

    SetActiveTab(GetActiveTab());

    UpdateGradients();
}

//----------------------------------------------------------------------------------------
// Purpose: Destructor
//----------------------------------------------------------------------------------------
GamepadUIAloneModOptionsPanel::~GamepadUIAloneModOptionsPanel()
{
}

//----------------------------------------------------------------------------------------
// Purpose: Initalizes the convars
//----------------------------------------------------------------------------------------
void InitAmodConvars()
{
    //load the keyvalues file
    KeyValues* file = new KeyValues("AloneModConfig");
    if (!file->LoadFromFile(g_pFullFileSystem, "cfg/AloneMod_Config.txt", "MOD"))
    {
        //delete the keyvalues
        file->deleteThis();
        return;
    }

    //go through each key (convar name) and set its value
    FOR_EACH_VALUE(file, value)
    {
        const char* varname = value->GetName();
        const char* varvalue = value->GetString();

        //find the var
        ConVar* var = cvar->FindVar(varname);

        //HACK: do these first
        if (!Q_strcmp(varname, "amod_filter_brightness_on"))
        {
            ConVarRef amod_filter_brightness_on("amod_filter_brightness_on");
            amod_filter_brightness_on.SetValue(atoi(varvalue));
        }
        else if (!Q_strcmp(varname, "amod_filter_brightness_on_exp"))
        {
            ConVarRef amod_filter_brightness_on_exp("amod_filter_brightness_on_exp");
            amod_filter_brightness_on_exp.SetValue(atoi(varvalue));
        }
        else if (!Q_strcmp(varname, "amod_filter_brightness_off"))
        {
            ConVarRef amod_filter_brightness_off("amod_filter_brightness_off");
            amod_filter_brightness_off.SetValue(atoi(varvalue));
        }
        else if (!Q_strcmp(varname, "amod_epic_filter"))
        {
            //check for the var
            if (!var)
                continue;

            //HACK: fixes "No Edict's yet" error
            auto t = var->m_fnChangeCallback;
            var->m_fnChangeCallback = nullptr;
            var->SetValue(atoi(varvalue));
            var->m_fnChangeCallback = t;
        }
        else
        {
            //check for the var
            if (!var)
                continue;

            //HACK: do this. idk why but just do it
            GamepadUI::GetInstance().GetEngineClient()->ClientCmd(CFmtStr("%s %s", varname, varvalue));

            //set the convar value
            var->SetValue(varvalue);
        }
    }
}


//----------------------------------------------------------------------------------------
// Purpose: Called on think
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::OnThink()
{
    BaseClass::OnThink();

    LayoutCurrentTab();
}

//----------------------------------------------------------------------------------------
// Purpose: Called on panel paint
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::Paint()
{
    BaseClass::Paint();

    if (!m_nTabCount)
        return;

    const int nLastTabX = m_flTabsOffsetX + m_nTabCount * m_Tabs[0].pTabButton->GetWide();

    const int nTabSize = m_Tabs[0].pTabButton->GetTall();
    const int nGlyphSize = nTabSize * 0.90f;
    const int nGlyphOffsetX = nGlyphSize / 4.0f;
    const int nGlyphOffsetY = nTabSize - nGlyphSize;

    if (m_leftGlyph.SetupGlyph(nGlyphSize, "menu_lb", true))
        m_leftGlyph.PaintGlyph(m_flTabsOffsetX - nGlyphSize - nGlyphOffsetX, m_flTabsOffsetY + nGlyphOffsetY / 2, nGlyphSize, 255);

    if (m_rightGlyph.SetupGlyph(nGlyphSize, "menu_rb", true))
        m_rightGlyph.PaintGlyph(nLastTabX + nGlyphOffsetX, m_flTabsOffsetY + nGlyphOffsetY / 2, nGlyphSize, 255);
}

//----------------------------------------------------------------------------------------
// Purpose: Updates the gradients
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::UpdateGradients()
{
    const float flTime = GamepadUI::GetInstance().GetTime();
    GamepadUI::GetInstance().GetGradientHelper()->ResetTargets(flTime);
    GamepadUI::GetInstance().GetGradientHelper()->SetTargetGradient(GradientSide::Up, { 1.0f, 1.0f }, flTime);
    GamepadUI::GetInstance().GetGradientHelper()->SetTargetGradient(GradientSide::Down, { 1.0f, 0.5f }, flTime);
}

//----------------------------------------------------------------------------------------
// Purpose: Lays out the current tab
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::LayoutCurrentTab()
{
    int nParentW, nParentH;
    GetParent()->GetSize(nParentW, nParentH);

    int x = m_flTabsOffsetX;
    int y = 0;
    for (int i = 0; i < m_nTabCount; i++)
    {
        GamepadUIAloneModTab& tab = m_Tabs[i];
        tab.pTabButton->SetPos(x, m_flTabsOffsetY);
        tab.pTabButton->SetVisible(true);
        x += tab.pTabButton->GetWide();
        y = m_flTabsOffsetY + tab.pTabButton->GetTall();

        for (GamepadUIButton* pButton : tab.pButtons)
            pButton->SetVisible(false);
    }

    int yMax = 0;
    int nActiveTab = GetActiveTab();
    {
        int nScrollCount = m_Tabs[nActiveTab].pButtons.Count() - 8;
        for (int i = 0; i < m_Tabs[nActiveTab].pButtons.Count(); i++)
        {
            GamepadUIButton* pButton = m_Tabs[nActiveTab].pButtons[i];
            if (i < nScrollCount)
                yMax += pButton->GetTall();
        }
        m_Tabs[nActiveTab].ScrollState.UpdateScrollBounds(0.0f, yMax);
    }

    int i = 0;
    int previousSizes = 0;
    int buttonWide = 0;
    for (GamepadUIButton* pButton : m_Tabs[nActiveTab].pButtons)
    {
        int fade = 255;

        int buttonY = y;
        int buttonX = m_flTabsOffsetX;
        if (m_Tabs[nActiveTab].bHorizontal)
        {
            buttonX += previousSizes;
        }
        else
        {
            buttonY = y + previousSizes - m_Tabs[nActiveTab].ScrollState.GetScrollProgress();
            if (buttonY < y)
                fade = RemapValClamped(y - buttonY, m_flOptionsFade - pButton->GetTall(), 0, 0, 255);
            else if (buttonY > (nParentH - m_flFooterButtonsOffsetY - m_nFooterButtonHeight - m_flOptionsFade))
                fade = RemapValClamped((nParentH - m_flFooterButtonsOffsetY - m_nFooterButtonHeight) - (buttonY + pButton->GetTall()), 0, m_flOptionsFade, 0, 255);
            if ((pButton->HasFocus() && pButton->IsEnabled()) && fade != 0)
                fade = 255;
        }

        GamepadUIConvarButton* pCvarButton = dynamic_cast<GamepadUIConvarButton*>(pButton);
        bool bHasDependencies = true;
        if (pCvarButton)
        {
            const char* pszDependentCVar = pCvarButton->GetDependentCVar();
            if (pszDependentCVar && *pszDependentCVar)
            {
                bHasDependencies = false;

                bool bFound = false;

                for (GamepadUIButton* pOtherButton : m_Tabs[nActiveTab].pButtons)
                {
                    GamepadUIConvarButton* pOtherCvarButton = dynamic_cast<GamepadUIConvarButton*>(pOtherButton);
                    if (pOtherCvarButton)
                    {
                        const char* pszConVarName = pOtherCvarButton->GetConVarName();
                        if (pszConVarName && *pszConVarName && !V_strcmp(pszDependentCVar, pszConVarName))
                        {
                            bHasDependencies = pOtherCvarButton->IsConVarEnabled();
                            bFound = true;
                            break;
                        }
                    }
                }

                if (!bFound)
                {
                    ConVarRef cvar(pszDependentCVar);
                    bHasDependencies = cvar.GetBool();
                }
            }
        }

        pButton->SetAlpha(fade);
        pButton->SetVisible(bHasDependencies);
        pButton->SetPos(buttonX, buttonY);

        if (pButton->IsEnabled() && pButton->IsVisible() && fade && m_Tabs[nActiveTab].bAlternating)
        {
            buttonWide = pButton->GetWide();
            if (i % 2)
                vgui::surface()->DrawSetColor(Color(0, 0, 0, (20 * Min(255, fade + 127)) / 255));
            else
                vgui::surface()->DrawSetColor(Color(fade, fade, fade, fade > 64 ? 1 : 0));

            vgui::surface()->DrawFilledRect(buttonX, buttonY, buttonX + buttonWide, buttonY + pButton->GetTall());
        }

        if (m_Tabs[nActiveTab].bHorizontal)
            previousSizes += pButton->GetWide();
        else
            previousSizes += pButton->GetTall();
        i++;
    }

    if (yMax != 0)
    {
        vgui::surface()->DrawSetColor(Color(255, 255, 255, 200));
        int scrollbarY = RemapValClamped(m_Tabs[nActiveTab].ScrollState.GetScrollProgress(), 0, yMax, y, nParentH - m_flFooterButtonsOffsetY - m_nFooterButtonHeight - m_flScrollBarHeight);
        vgui::surface()->DrawFilledRect(m_flTabsOffsetX + m_flScrollBarOffsetX, scrollbarY, m_flTabsOffsetX + m_flScrollBarOffsetX + m_flScrollBarWidth, scrollbarY + m_flScrollBarHeight);
    }

    m_Tabs[nActiveTab].ScrollState.UpdateScrolling(2.0f, GamepadUI::GetInstance().GetTime());
}

//----------------------------------------------------------------------------------------
// Purpose: Called on mouse wheel
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::OnMouseWheeled(int delta)
{
    m_Tabs[GetActiveTab()].ScrollState.OnMouseWheeled(delta * 100.0f, GamepadUI::GetInstance().GetTime());
}

//----------------------------------------------------------------------------------------
// Purpose: Gets the active tab
//----------------------------------------------------------------------------------------
int GamepadUIAloneModOptionsPanel::GetActiveTab()
{
    int nUserTab = gamepadui_last_amod_options_tab.GetInt();
    int nActiveTab = Clamp(nUserTab, 0, Max(0, m_nTabCount - 1));

    if (nUserTab != nActiveTab)
        gamepadui_last_amod_options_tab.SetValue(nActiveTab);

    return nActiveTab;
}

//----------------------------------------------------------------------------------------
// Purpose: on button navigated to
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::OnGamepadUIButtonNavigatedTo(vgui::VPANEL button)
{
    GamepadUIButton* pButton = dynamic_cast<GamepadUIButton*>(vgui::ipanel()->GetPanel(button, GetModuleName()));
    if (pButton)
    {
        int nParentW, nParentH;
        GetParent()->GetSize(nParentW, nParentH);

        const float flTabButtonHeight = m_Tabs[GetActiveTab()].pTabButton->m_flHeight;

        float flScrollRegion = nParentH - (m_flTabsOffsetY + m_flFooterButtonsOffsetY + m_nFooterButtonHeight + flTabButtonHeight);
        int nX, nY;
        pButton->GetPos(nX, nY);
        if (nY + m_flFooterButtonsOffsetY + m_nFooterButtonHeight + pButton->GetTall() > nParentH || nY < m_flTabsOffsetY + flTabButtonHeight)
        {
            int nTargetY = 0;
            int nThisButton = -1;
            int nHeader = -1;
            for (int i = 0; i < m_Tabs[GetActiveTab()].pButtons.Count(); i++)
            {
                if (m_Tabs[GetActiveTab()].pButtons[i] == pButton)
                {
                    nThisButton = i;
                    break;
                }

                // For now, headers can be identified as disabled buttons
                if (!m_Tabs[GetActiveTab()].pButtons[i]->IsEnabled())
                    nHeader = i;
                else
                    nHeader = -1;

                nTargetY += m_Tabs[GetActiveTab()].pButtons[i]->m_flHeight;
            }

            // This button isn't part of the current tab, so don't scroll
            if (nThisButton == -1)
                return;

            // If this button has a section header above it and we're going up, scroll to it
            if (nHeader != -1 && nY < nParentH / 2)
                nTargetY -= m_Tabs[GetActiveTab()].pButtons[nHeader]->m_flHeight;

            if (nY < nParentH / 2)
            {
                nTargetY -= (pButton->m_flHeightAnimationValue[ButtonStates::Over] / 2);
            }
            else
            {
                nTargetY -= flScrollRegion;
                nTargetY += pButton->m_flHeight;
                nTargetY += (pButton->m_flHeightAnimationValue[ButtonStates::Over] / 2);
            }

            m_Tabs[GetActiveTab()].ScrollState.SetScrollTarget(nTargetY, GamepadUI::GetInstance().GetTime());
        }
    }
}

//----------------------------------------------------------------------------------------
// Purpose: Sets the active tab
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::SetActiveTab(int nTab)
{
    nTab = Clamp(nTab, 0, Max(m_nTabCount - 1, 0));
    gamepadui_last_amod_options_tab.SetValue(nTab);

    int nActiveTab = GetActiveTab();
    for (int i = 0; i < m_nTabCount; i++)
        m_Tabs[i].pTabButton->ForceDepressed(i == nActiveTab);

    for (GamepadUIButton* pButton : m_Tabs[nActiveTab].pButtons)
    {
        if (pButton->GetCurrentButtonState() == ButtonState::Pressed)
        {
            pButton->NavigateTo();
            return;
        }
    }

    for (GamepadUIButton* pButton : m_Tabs[nActiveTab].pButtons)
    {
        if (pButton->IsEnabled())
        {
            pButton->NavigateTo();
            return;
        }
    }
}

//----------------------------------------------------------------------------------------
// Purpose: Loads the options
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::LoadOptionTabs(const char* pszOptionsFile)
{
    //load the file
    KeyValues* pDataFile = new KeyValues("Options");
    if (pDataFile->LoadFromFile(g_pFullFileSystem, pszOptionsFile))
    {
        //go through each settings tab
        for (KeyValues* pTabData = pDataFile->GetFirstSubKey(); pTabData != NULL; pTabData = pTabData->GetNextKey())
        {
            //load the tab stuff
            {
                //get the command
                char buttonCmd[64];
                V_snprintf(buttonCmd, sizeof(buttonCmd), "tab %d", m_nTabCount);

                //create new tab button
                auto button = new GamepadUIButton(
                    this, this,
                    GAMEPADUI_RESOURCE_FOLDER "schemetab.res",
                    buttonCmd,
                    pTabData->GetString("title"), "");
                button->SetZPos(50);
                
                if (m_nTabCount == gamepadui_last_amod_options_tab.GetInt())
                    button->ForceDepressed(true);

                //set tab button
                m_Tabs[m_nTabCount].pTabButton = button;
            }

            m_Tabs[m_nTabCount].pTabButton->SetName(pTabData->GetName());
            m_Tabs[m_nTabCount].bAlternating = pTabData->GetBool("alternating");
            m_Tabs[m_nTabCount].bHorizontal = pTabData->GetBool("horizontal");

            //get the items
            KeyValues* pTabItems = pTabData->FindKey("items");
            if (pTabItems)
            {
                //get subkey
                for (KeyValues* pItemData = pTabItems->GetFirstSubKey(); pItemData != NULL; pItemData = pItemData->GetNextKey())
                {
                    const char* pItemType = pItemData->GetString("type", "droppydown");

                    //check for check box
                    if (!V_strcmp(pItemType, "checkybox"))
                    {
                        auto button = new GamepadUICheckButton(
                            this, this,
                            GAMEPADUI_RESOURCE_FOLDER "schemeoptions_checkybox.res",
                            "button_pressed",
                            pItemData->GetString("text", ""), pItemData->GetString("description", ""));

                        m_Tabs[m_nTabCount].pButtons.AddToTail(button);
                    }

                    //check for slider
                    else if (!V_strcmp(pItemType, "slideyslide"))
                    {
                        const char* pszCvar = pItemData->GetString("convar");
                        const char* pszCvarDepends = pItemData->GetString("depends_on");
                        bool bInstantApply = pItemData->GetBool("instantapply");
                        float flMin = pItemData->GetFloat("min", 0.0f);
                        float flMax = pItemData->GetFloat("max", 1.0f);
                        float flStep = pItemData->GetFloat("step", 0.1f);
                        int nTextPrecision = pItemData->GetInt("textprecision", -1);
                        auto button = new GamepadUISlideySlide(
                            pszCvar, pszCvarDepends, bInstantApply, flMin, flMax, flStep, nTextPrecision,
                            this, this,
                            GAMEPADUI_RESOURCE_FOLDER "schemeoptions_slideyslide.res",
                            "button_pressed",
                            pItemData->GetString("text", ""), pItemData->GetString("description", ""));
                        button->SetToDefault();
                        m_Tabs[m_nTabCount].pButtons.AddToTail(button);
                    }

                    //check for wheel
                    else if (!V_strcmp(pItemType, "wheelywheel"))
                    {
                        const char* pszCvar = pItemData->GetString("convar");
                        const char* pszCvarDepends = pItemData->GetString("depends_on");
                        bool bInstantApply = pItemData->GetBool("instantapply");
                        bool bSignOnly = pItemData->GetBool("signonly");
                        auto button = new GamepadUIWheelyWheel(
                            pszCvar, pszCvarDepends, bInstantApply, bSignOnly,
                            this, this,
                            GAMEPADUI_RESOURCE_FOLDER "schemeoptions_wheelywheel.res",
                            "button_pressed",
                            pItemData->GetString("text", ""), pItemData->GetString("description", ""));

                        KeyValues* pOptions = pItemData->FindKey("options");
                        if (pOptions)
                        {
                            for (KeyValues* pOptionData = pOptions->GetFirstSubKey(); pOptionData != NULL; pOptionData = pOptionData->GetNextKey())
                            {
                                GamepadUIOption option;
                                option.nValue = V_atoi(pOptionData->GetName());
                                option.strOptionText = GamepadUIString(pOptionData->GetString());
                                button->AddOptionItem(option);
                            }
                        }

                        button->SetToDefault();
                        m_Tabs[m_nTabCount].pButtons.AddToTail(button);
                    }

                    //check for header
                    else if (!V_strcmp(pItemType, "headeryheader"))
                    {
                        // add header item
                        auto button = new GamepadUIButton(
                            this, this,
                            GAMEPADUI_RESOURCE_FOLDER "schemeoptions_sectiontitle.res",
                            "button_pressed",
                            pItemData->GetString("text", ""), pItemData->GetString("description", ""));
                        
                        button->SetEnabled(false);
                        m_Tabs[m_nTabCount].pButtons.AddToTail(button);
                    }
                }
            }

            m_nTabCount++;
        }
    }

    pDataFile->deleteThis();
}

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when on) using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOnValue(int fv)
{
    return 2.5f - (0.05f * fv);
}

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when on)'s exponent using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOnExponentValue(int fv)
{
    return 1.15f + (0.05f * fv);
}

//------------------------------------------------------------------------------------
// Purpose: Gets the value for the filter (when off) by using an int (the slider value) 
//			as the input
//------------------------------------------------------------------------------------
float GetFilterOffValue(int fv)
{
    return 2.3f - (0.05f * fv);
}

static bool gs_FilterIsOn = true;

CON_COMMAND(gamepadui_filterset, "")
{
    gs_FilterIsOn = atoi(args.Arg(1));
}

//----------------------------------------------------------------------------------------
// Purpose: called on command
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::OnCommand(char const* pCommand)
{
    if (!V_strcmp(pCommand, "action_back"))
    {
        Close();
    }
    else if (!V_strcmp(pCommand, "action_apply"))
    {
        for (int i = 0; i < m_Tabs[GetActiveTab()].pButtons.Count(); i++)
        {
            GamepadUIConvarButton* pConVarButton = dynamic_cast<GamepadUIConvarButton*>(m_Tabs[GetActiveTab()].pButtons[i]);
            if (pConVarButton)
                pConVarButton->UpdateConVar();

            //check for filter stuff
            if (!Q_strcmp(pConVarButton->m_cvar.GetName(), "amod_filter_brightness_on"))
            {
                //we need to set the filter values. So do that now
                float FilterOnValue = GetFilterOnValue(pConVarButton->m_cvar.GetInt());

                //set the brightness (filter values)
                GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted(CFmtStr("alias tf1 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 1; Amod_FilterSet 1; alias Amod_ToggleFilter tf2\"", FilterOnValue));
            }
            else if (!Q_strcmp(pConVarButton->m_cvar.GetName(), "amod_filter_brightness_off"))
            {
                float FilterOffValue = GetFilterOffValue(pConVarButton->m_cvar.GetInt());

                //set off value
                GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted(CFmtStr("alias tf2 \"mat_monitorgamma %f; mat_monitorgamma_tv_enabled 0; Amod_FilterSet 0; alias Amod_ToggleFilter tf1\"", FilterOffValue));
            }
            else if (!Q_strcmp(pConVarButton->m_cvar.GetName(), "amod_filter_brightness_on_exp"))
            {
                float FilterExponentValue = GetFilterOnExponentValue(pConVarButton->m_cvar.GetInt());

                //set exponent
                GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted(CFmtStr("mat_monitorgamma_tv_exp %f", FilterExponentValue));
            }
        }

        ConVarRef amod_day("amod_day");

        if (gs_FilterIsOn)
            GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted("tf1");
        else
            GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted("tf2");

        GamepadUI::GetInstance().GetEngineClient()->ClientCmd_Unrestricted("amod_writeconfig");
    }
    else if (StringHasPrefixCaseSensitive(pCommand, "tab "))
    {
        const char* pszTab = &pCommand[4];
        if (*pszTab)
            SetActiveTab(atoi(pszTab));
    }
    else
    {
        BaseClass::OnCommand(pCommand);
    }
}

//----------------------------------------------------------------------------------------
// Purpose: apply the scheme settings
//----------------------------------------------------------------------------------------
void GamepadUIAloneModOptionsPanel::ApplySchemeSettings(vgui::IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    if (GamepadUI::GetInstance().GetScreenRatio() != 1.0f)
    {
        float flScreenRatio = GamepadUI::GetInstance().GetScreenRatio();
        m_flTabsOffsetX *= (flScreenRatio * flScreenRatio);
    }
}

CON_COMMAND(gamepadui_openamodoptionsdialog, "")
{
    new GamepadUIAloneModOptionsPanel(GamepadUI::GetInstance().GetBasePanel(), "");
}
