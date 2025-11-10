#ifndef GAMEPADUI_BUTTON_H
#define GAMEPADUI_BUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "gamepadui_panel.h"
#include "gamepadui_string.h"
#include "gamepadui_glyph.h"
#include "vgui_controls/Button.h"
#include "vgui/IInput.h"
#include "gamepadui_image.h"

namespace ButtonStates
{
    enum ButtonState
    {
        Out,
        Over,
        Pressed,

        Count,
    };
}
using ButtonState = ButtonStates::ButtonState;

namespace FooterButtons
{
    enum FooterButton
    {
        None   = 0,
        Back   = ( 1 << 0 ),
        Cancel = ( 1 << 1 ),
        LeftSelect = ( 1 << 2 ),

        // This button and any before are to the left.
        LeftMask = ( LeftSelect | Back | Cancel ),
        // Any buttons after here are to the right.
        DeclineMask = ( Back | Cancel ),

        Select = ( 1 << 3 ),
        Apply  = ( 1 << 4 ),
        Okay   = ( 1 << 5 ),
        Commentary  = ( 1 << 6 ),
        BonusMaps  = ( 1 << 7 ),
        Challenge  = ( 1 << 8 ),
        UseDefaults  = ( 1 << 9 ),

        // Buttons that are 'confirmatory'
        ConfirmMask = ( LeftSelect | Select | Okay ),
    };
    static const int MaxFooterButtons = 10;

    inline const char* GetButtonName( FooterButton button )
    {
        switch ( button )
        {
            case Back:   return "#GameUI_Back";
            case Cancel: return "#GameUI_Cancel";
            case LeftSelect:
            case Select: return "#GameUI_Select";
            case Apply:  return "#GameUI_Apply";
            case Okay:   return "#GameUI_Ok";
            case Commentary: return "#GameUI_Commentary";
            case BonusMaps: return "#Deck_BonusMaps";
            case Challenge: return "#Deck_Challenges";
            case UseDefaults: return "#GameUI_UseDefaults";
        }
        return "Unknown";
    }

    inline const char* GetButtonAction( FooterButton button )
    {
        switch ( button )
        {
            case Back:   return "action_back";
            case Cancel: return "action_cancel";
            case LeftSelect:
            case Select: return "action_select";
            case Apply:  return "action_apply";
            case Okay:   return "action_okay";
            case Commentary: return "action_commentary";
            case BonusMaps: return "action_bonus_maps";
            case Challenge: return "action_challenges";
            case UseDefaults: return "action_usedefaults";
        }
        return "";
    }

    inline const char* GetButtonActionHandleString( FooterButton button )
    {
        switch ( button )
        {
            case Back:   return "menu_cancel";
            case Cancel: return "menu_cancel";
            case LeftSelect:
            case Select: return "menu_select";
            case Apply:  return "menu_y";
            case Okay:   return "menu_select";
            case Commentary: return "menu_y";
            case BonusMaps: return "menu_x";
            case Challenge: return "menu_y";
            case UseDefaults: return "menu_x";
        }
        return "";
    }

	inline FooterButton GetButtonByIdx( int i )
	{
		return static_cast< FooterButton >( 1 << i );
	}

	using FooterButtonMask = unsigned int;
}
using FooterButton = FooterButtons::FooterButton;
using FooterButtonMask = FooterButtons::FooterButtonMask;

class GamepadUIButton : public vgui::Button, public SchemeValueMap
{
    DECLARE_CLASS_SIMPLE( GamepadUIButton, vgui::Button );
public:
    GamepadUIButton( vgui::Panel *pParent, vgui::Panel* pActionSignalTarget, const char *pSchemeFile, const char *pCommand, const char *pText, const char *pDescription );
    GamepadUIButton( vgui::Panel *pParent, vgui::Panel* pActionSignalTarget, const char *pSchemeFile, const char *pCommand, const wchar_t *pText, const wchar_t *pDescription );

    void ApplySchemeSettings( vgui::IScheme* pScheme ) OVERRIDE;
    void OnThink() OVERRIDE;
    void Paint() OVERRIDE;
    void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
    void OnKeyCodeReleased( vgui::KeyCode code ) OVERRIDE;
    void NavigateTo() OVERRIDE;
    void NavigateFrom() OVERRIDE;
    void OnCursorEntered() OVERRIDE;
    void FireActionSignal() OVERRIDE;

    MESSAGE_FUNC( OnSiblingGamepadUIButtonOpened, "OnSiblingGamepadUIButtonOpened" );

    virtual void RunAnimations( ButtonState state );
    void DoAnimations( bool bForce = false );
    void PaintButton();
    void PaintBorders();
    int  PaintText();

    void SetPriority( int nPriority ) { m_nPriority = nPriority; }
    int  GetPriority() const { return m_nPriority; }

    void SetFooterButton( FooterButton button );
    FooterButton GetFooterButton() const;

    void SetForwardToParent( bool bForwardToParent );
    bool GetForwardToParent() const;

          GamepadUIString& GetButtonText()              { return m_strButtonText; }
    const GamepadUIString& GetButtonText()        const { return m_strButtonText; }
          GamepadUIString& GetButtonDescription()       { return m_strButtonDescription; }
    const GamepadUIString& GetButtonDescription() const { return m_strButtonDescription; }

    virtual ButtonState GetCurrentButtonState();

    bool IsFooterButton() const;

    float GetDrawHeight() const { return m_flHeight + m_flExtraHeight; }
    float GetMaxHeight() const { return m_flHeightAnimationValue[ ButtonStates::Over ] + m_flCachedExtraHeight; }

    void SetMouseNavigate( bool bMouseNavigate ) { m_bMouseNavigate = bMouseNavigate; }

protected:

    ButtonState m_ePreviousState = ButtonStates::Out;

    bool m_bCursorOver = false;
    bool m_bControllerPressed = false;
    bool m_bNavigateTo = false;
    bool m_bForwardToParent = false;
    bool m_bMouseNavigate = true;

    float m_flExtraHeight = 0;
    float m_flCachedExtraHeight = 0;
    float m_flTargetExtraHeight = 0;
    float m_flLastExtraHeight = 0;
    float m_flExtraHeightTime = 0;

    FooterButton m_eFooterButton = FooterButtons::None;

    GamepadUIString m_strButtonText;
    GamepadUIString m_strButtonDescription;
    GamepadUIGlyph m_glyph;

public:

    int m_nPriority = 0;

    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flWidth,                "Button.Width",               "392",             SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flHeight,               "Button.Height",              "40",              SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flTextOffsetX,          "Button.Text.OffsetX",        "10",              SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flTextOffsetY,          "Button.Text.OffsetY",        "0",               SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flDescriptionOffsetX,   "Button.Description.OffsetX", "0",               SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flDescriptionOffsetY,   "Button.Description.OffsetY", "0",               SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( Color, m_colBackgroundColor,     "Button.Background",          "0 0 0 0",         SchemeValueTypes::Color );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( Color, m_colTextColor,           "Button.Text",                "255 255 255 255", SchemeValueTypes::Color );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( Color, m_colDescriptionColor,    "Button.Description",         "255 255 255 255", SchemeValueTypes::Color );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flGlyphFade,            "Button.Glyphs.Fade",         "0",               SchemeValueTypes::Float );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( bool,  m_bDescriptionHide,       "Button.Description.Hide",    "0",               SchemeValueTypes::Bool );

    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flTextLeftBorder,       "Button.Text.LeftBorder",       "0",               SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( Color, m_colLeftBorder,          "Button.Background.LeftBorder", "0 0 0 0",         SchemeValueTypes::Color );

    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( float, m_flTextBottomBorder,       "Button.Text.BottomBorder",       "0",               SchemeValueTypes::ProportionalFloat );
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY( Color, m_colBottomBorder,          "Button.Background.BottomBorder", "0 0 0 0",         SchemeValueTypes::Color );

    GAMEPADUI_PANEL_PROPERTY( bool, m_CenterX,          "Button.Text.CenterX",     "0", SchemeValueTypes::Bool );
    GAMEPADUI_PANEL_PROPERTY( bool, m_bDescriptionWrap, "Button.Description.Wrap", "1", SchemeValueTypes::Bool );

    vgui::HFont m_hTextFont        = vgui::INVALID_FONT;
    vgui::HFont m_hTextFontOver    = vgui::INVALID_FONT;
    vgui::HFont m_hDescriptionFont = vgui::INVALID_FONT;
};

//other buttons
class GamepadUICheckButton : public GamepadUIButton
{
public:
    DECLARE_CLASS_SIMPLE(GamepadUICheckButton, GamepadUIButton);

    GamepadUICheckButton(vgui::Panel* pParent, vgui::Panel* pActionSignalTarget, const char* pSchemeFile, const char* pCommand, const char* pText, const char* pDescription)
        : BaseClass(pParent, pActionSignalTarget, pSchemeFile, pCommand, pText, pDescription)
    {
    }

    virtual void ApplySchemeSettings(vgui::IScheme* pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);

        m_hIconFont = pScheme->GetFont("Button.Check.Font", true);
    }

    virtual void Paint()
    {
        BaseClass::Paint();

        vgui::surface()->DrawSetTextColor(m_colTextColor);
        vgui::surface()->DrawSetTextFont(m_hIconFont);
        vgui::surface()->DrawSetTextPos(m_flCheckOffsetX, m_flHeight / 2 - m_flCheckHeight / 2);
        vgui::surface()->DrawPrintText(L"j", 1);
        vgui::surface()->DrawSetTextPos(m_flCheckOffsetX, m_flHeight / 2 - m_flCheckHeight / 2);
        vgui::surface()->DrawPrintText(L"k", 1);
    }

private:

    GAMEPADUI_PANEL_PROPERTY(float, m_flCheckOffsetX, "Button.Check.OffsetX", "10", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flCheckHeight, "Button.Check.Height", "18", SchemeValueTypes::ProportionalFloat);

    vgui::HFont m_hIconFont = vgui::INVALID_FONT;
};

struct GamepadUIOption
{
    GamepadUIString strOptionText;
    int nValue = 0;

    union
    {
        struct
        {
            int nWidth;
            int nHeight;
        };
        void* pData;
    } userdata;
};

class GamepadUIConvarButton : public GamepadUIButton
{
public:
    DECLARE_CLASS_SIMPLE(GamepadUIConvarButton, GamepadUIButton);

    GamepadUIConvarButton(const char* pszCvar, const char* pszCvarDepends, bool bInstantApply, vgui::Panel* pParent, vgui::Panel* pActionSignalTarget, const char* pSchemeFile, const char* pCommand, const char* pText, const char* pDescription)
        : BaseClass(pParent, pActionSignalTarget, pSchemeFile, pCommand, pText, pDescription)
        , m_cvar(pszCvar)
        , m_szDependentCVar(pszCvarDepends)
        , m_bInstantApply(bInstantApply)
    {
    }

    virtual void UpdateConVar() = 0;
    virtual bool IsDirty() = 0;
    virtual void SetToDefault() = 0;
    virtual bool IsConVarEnabled() const { return true; }

    const char* GetDependentCVar() const
    {
        return m_szDependentCVar.String();
    }

    const char* GetConVarName() const
    {
        return m_cvar.GetName();
    }

public:
    ConVarRef m_cvar;
    CUtlString m_szDependentCVar;
    bool m_bInstantApply = false;
};

class GamepadUIWheelyWheel : public GamepadUIConvarButton
{
public:
    DECLARE_CLASS_SIMPLE(GamepadUIWheelyWheel, GamepadUIConvarButton);

    GamepadUIWheelyWheel(const char* pszCvar, const char* pszCvarDepends, bool bInstantApply, bool bSignOnly, vgui::Panel* pParent, vgui::Panel* pActionSignalTarget, const char* pSchemeFile, const char* pCommand, const char* pText, const char* pDescription)
        : BaseClass(pszCvar, pszCvarDepends, bInstantApply, pParent, pActionSignalTarget, pSchemeFile, pCommand, pText, pDescription)
        , m_bSignOnly(bSignOnly)
    {
    }

    void OnKeyCodePressed(vgui::KeyCode code)
    {
        ButtonCode_t buttonCode = GetBaseButtonCode(code);
        switch (buttonCode)
        {
        case KEY_LEFT:
        case KEY_XBUTTON_LEFT:

#ifdef HL2_RETAIL // Steam input and Steam Controller are not supported in SDK2013 (Madi)
        case STEAMCONTROLLER_DPAD_LEFT:
#endif
            if (--m_nSelectedItem < 0)
                m_nSelectedItem = Max(0, m_Options.Count() - 1);
            if (m_bInstantApply)
                UpdateConVar();
            break;

        case KEY_RIGHT:
        case KEY_XBUTTON_RIGHT:

#ifdef HL2_RETAIL
        case STEAMCONTROLLER_DPAD_RIGHT:
#endif
            if (m_Options.Count())
                m_nSelectedItem = (m_nSelectedItem + 1) % m_Options.Count();
            if (m_bInstantApply)
                UpdateConVar();
            break;

        default:
            BaseClass::OnKeyCodePressed(code);
            break;
        }
    }

    void FireActionSignal()
    {
        BaseClass::FireActionSignal();

        if (m_Options.Count())
            m_nSelectedItem = (m_nSelectedItem + 1) % m_Options.Count();
        if (m_bInstantApply)
            UpdateConVar();
    }

    void UpdateConVar() OVERRIDE
    {
        if (IsDirty())
        {
            if (m_bSignOnly)
                m_cvar.SetValue(abs(m_cvar.GetFloat()) * m_Options[m_nSelectedItem].nValue);
            else
                m_cvar.SetValue(m_Options[m_nSelectedItem].nValue);
        }
    }


    bool IsDirty() OVERRIDE
    {
        return m_cvar.IsValid() && GetCvarValue() != m_Options[m_nSelectedItem].nValue;
    }

    virtual void Paint()
    {
        BaseClass::Paint();

        if (m_nSelectedItem >= m_Options.Count())
            return;

        GamepadUIString& strOption = m_Options[m_nSelectedItem].strOptionText;
        ButtonState state = GetCurrentButtonState();

        int nTextW, nTextH;
        vgui::surface()->GetTextSize(m_hTextFont, strOption.String(), nTextW, nTextH);

        int nScrollerSize = vgui::surface()->GetCharacterWidth(m_hTextFont, L'<') + vgui::surface()->GetCharacterWidth(m_hTextFont, L' ');

        int nTextY = m_flHeight / 2 - nTextH / 2 + m_flTextOffsetY;

        vgui::surface()->DrawSetTextFont(m_hTextFont);
        if (state != ButtonStates::Out)
        {
            vgui::surface()->DrawSetTextPos(m_flWidth - m_flTextOffsetX - nTextW - 2 * nScrollerSize, nTextY);
            vgui::surface()->DrawPrintText(L"< ", 2);
        }
        vgui::surface()->DrawSetTextPos(m_flWidth - m_flTextOffsetX - nTextW - nScrollerSize, nTextY);
        vgui::surface()->DrawPrintText(strOption.String(), strOption.Length());
        if (state != ButtonStates::Out)
        {
            vgui::surface()->DrawSetTextPos(m_flWidth - m_flTextOffsetX - nScrollerSize, nTextY);
            vgui::surface()->DrawPrintText(L" >", 2);
        }
    }

    void ClearOptions()
    {
        m_Options.RemoveAll();
    }

    void AddOptionItem(GamepadUIOption option)
    {
        m_Options.AddToTail(option);
    }

    int GetOptionCount()
    {
        return m_Options.Count();
    }

    int GetCvarValue()
    {
        if (m_bSignOnly)
            return (int)Sign(m_cvar.GetFloat());
        else
            return m_cvar.GetInt();
    }

    void SetToDefault() OVERRIDE
    {
        if (m_cvar.IsValid())
        {
            const int nCurrentValue = GetCvarValue();
            for (int i = 0; i < m_Options.Count(); i++)
            {
                if (m_Options[i].nValue == nCurrentValue)
                    m_nSelectedItem = i;
            }
        }
    }

    GamepadUIOption* GetOption(int nIndex)
    {
        if (nIndex < 0 || nIndex >= m_Options.Count())
            return NULL;
        return &m_Options[nIndex];
    }

    bool IsConVarEnabled() const OVERRIDE
    {
        return !!m_Options[m_nSelectedItem].nValue;
    }

private:

    bool m_bSignOnly = false;
    int m_nSelectedItem = 0;
    CUtlVector< GamepadUIOption > m_Options;

};

class GamepadUISlideySlide : public GamepadUIConvarButton
{
public:
    DECLARE_CLASS_SIMPLE(GamepadUISlideySlide, GamepadUIConvarButton);

    GamepadUISlideySlide(const char* pszCvar, const char* pszCvarDepends, bool bInstantApply, float flMin, float flMax, float flStep, int nTextPrecision, vgui::Panel* pParent, vgui::Panel* pActionSignalTarget, const char* pSchemeFile, const char* pCommand, const char* pText, const char* pDescription)
        : BaseClass(pszCvar, pszCvarDepends, bInstantApply, pParent, pActionSignalTarget, pSchemeFile, pCommand, pText, pDescription)
        , m_flMin(flMin)
        , m_flMax(flMax)
        , m_flStep(flStep)
        , nTextPrecision(nTextPrecision)
    {
    }

    void OnKeyCodePressed(vgui::KeyCode code)
    {
        ButtonCode_t buttonCode = GetBaseButtonCode(code);
        switch (buttonCode)
        {
        case KEY_LEFT:
        case KEY_XBUTTON_LEFT:

#ifdef HL2_RETAIL // Steam input and Steam Controller are not supported in SDK2013 (Madi)
        case STEAMCONTROLLER_DPAD_LEFT:
#endif
            m_flValue = Clamp(m_flValue - m_flStep, m_flMin, m_flMax);
            if (m_bInstantApply)
                UpdateConVar();
            break;

        case KEY_RIGHT:
        case KEY_XBUTTON_RIGHT:

#ifdef HL2_RETAIL
        case STEAMCONTROLLER_DPAD_RIGHT:
#endif
            m_flValue = Clamp(m_flValue + m_flStep, m_flMin, m_flMax);
            if (m_bInstantApply)
                UpdateConVar();
            break;

        default:
            BaseClass::OnKeyCodePressed(code);
            break;
        }
    }

    void UpdateConVar() OVERRIDE
    {
        if (IsDirty())
            m_cvar.SetValue(m_flValue);
    }

    bool IsDirty() OVERRIDE
    {
        return m_cvar.IsValid() && m_cvar.GetFloat() != m_flValue;
    }

    float GetMultiplier() const
    {
        return (m_flValue - m_flMin) / (m_flMax - m_flMin);
    }

    virtual void Paint()
    {
        BaseClass::Paint();

        if (nTextPrecision >= 0)
        {
            wchar_t szValue[256];
            V_snwprintf(szValue, sizeof(szValue), L"%.*f", nTextPrecision, m_flValue);

            int w, h;
            vgui::surface()->GetTextSize(m_hTextFont, szValue, w, h);
            vgui::surface()->DrawSetTextPos(m_flWidth - 2 * m_flTextOffsetX - m_flSliderWidth - w, m_flHeight / 2 - h / 2);
            vgui::surface()->DrawPrintText(szValue, V_wcslen(szValue));
        }

        vgui::surface()->DrawSetColor(m_colSliderBacking);
        vgui::surface()->DrawFilledRect(m_flWidth - m_flTextOffsetX - m_flSliderWidth, m_flHeight / 2 - m_flSliderHeight / 2, m_flWidth - m_flTextOffsetX, m_flHeight / 2 + m_flSliderHeight / 2);

        float flFill = m_flSliderWidth * (1.0f - GetMultiplier());

        vgui::surface()->DrawSetColor(m_colSliderFill);
        vgui::surface()->DrawFilledRect(m_flWidth - m_flTextOffsetX - m_flSliderWidth, m_flHeight / 2 - m_flSliderHeight / 2, m_flWidth - m_flTextOffsetX - flFill, m_flHeight / 2 + m_flSliderHeight / 2);
    }

    void SetToDefault() OVERRIDE
    {
        if (m_cvar.IsValid())
            m_flValue = m_cvar.GetFloat();
    }

    void RunAnimations(ButtonState state) OVERRIDE
    {
        BaseClass::RunAnimations(state);

        GAMEPADUI_RUN_ANIMATION_COMMAND(m_colSliderBacking, vgui::AnimationController::INTERPOLATOR_LINEAR);
        GAMEPADUI_RUN_ANIMATION_COMMAND(m_colSliderFill, vgui::AnimationController::INTERPOLATOR_LINEAR);
    }

private:

    float m_flValue = 0.0f;

    float m_flMin = 0.0f;
    float m_flMax = 1.0f;
    float m_flStep = 0.1f;

    int nTextPrecision = -1;

    GAMEPADUI_BUTTON_ANIMATED_PROPERTY(Color, m_colSliderBacking, "Slider.Backing", "255 255 255 22", SchemeValueTypes::Color);
    GAMEPADUI_BUTTON_ANIMATED_PROPERTY(Color, m_colSliderFill, "Slider.Fill", "255 255 255 255", SchemeValueTypes::Color);

    GAMEPADUI_PANEL_PROPERTY(float, m_flSliderWidth, "Slider.Width", "160", SchemeValueTypes::ProportionalFloat);
    GAMEPADUI_PANEL_PROPERTY(float, m_flSliderHeight, "Slider.Height", "11", SchemeValueTypes::ProportionalFloat);

};

#endif // GAMEPADUI_BUTTON_H
