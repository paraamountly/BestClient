/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
#include "menus_start.h"

#include <engine/client/updater.h>
#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/components/bestclient/version.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>
#include <game/version.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Geometry and motion tuning is intentionally kept together for visual comparison passes.
	constexpr float LOGO_DIAMETER_RATIO = 0.165f;
	constexpr float STRIP_HEIGHT_RATIO = 0.078f;
	constexpr float MENU_ANCHOR_X_RATIO = 0.34f;
	constexpr float SEGMENT_WIDTH = 88.0f;
	constexpr float SEGMENT_MAX_GROUP_WIDTH = 440.0f;
	constexpr float SEGMENT_SKEW = 7.0f;
	constexpr float HOVER_EXPANSION = 0.04f;
	constexpr float DRAWER_WIDTH_RATIO = 0.32f;
	constexpr float OVERLAY_ALPHA = 0.54f;
	constexpr float HOVER_DURATION = 0.12f;
	constexpr float SUBMENU_DURATION = 0.22f;
	constexpr float DRAWER_DURATION = 0.25f;
	constexpr float PAGE_TRANSITION_DURATION = 0.30f;
	constexpr float ENTRANCE_DURATION = 0.28f;
	constexpr float ANIMATION_SPEED_BASE = 12.0f;

	float EaseOutCubic(float T) { return 1.0f - std::pow(1.0f - T, 3.0f); }
	float EaseInOutCubic(float T) { return T < 0.5f ? 4.0f * T * T * T : 1.0f - std::pow(-2.0f * T + 2.0f, 3.0f) / 2.0f; }

	ColorRGBA Mix(ColorRGBA A, ColorRGBA B, float T)
	{
		return ColorRGBA(A.r + (B.r - A.r) * T, A.g + (B.g - A.g) * T, A.b + (B.b - A.b) * T, A.a + (B.a - A.a) * T);
	}

	constexpr ColorRGBA COLOR_SETTINGS(0.22f, 0.22f, 0.27f, 0.96f);
	constexpr ColorRGBA COLOR_PLAY(0.38f, 0.25f, 0.72f, 0.96f);
	constexpr ColorRGBA COLOR_EDITOR(0.87f, 0.48f, 0.16f, 0.96f);
	constexpr ColorRGBA COLOR_BROWSE(0.43f, 0.69f, 0.18f, 0.96f);
	constexpr ColorRGBA COLOR_EXIT(0.82f, 0.20f, 0.43f, 0.96f);

	bool IsSubmenu(CMenusStart::EState State)
	{
		return State == CMenusStart::EState::PLAY_SUBMENU || State == CMenusStart::EState::EDITOR_SUBMENU || State == CMenusStart::EState::BROWSE_SUBMENU;
	}
} // namespace

void CMenusStart::BeginTransition(EState Target, int PendingPage)
{
	if(m_Interaction.m_Target == Target && m_Interaction.m_Progress < 1.0f)
		return;
	m_Interaction.m_Previous = m_Interaction.m_Current;
	m_Interaction.m_Target = Target;
	m_Interaction.m_Progress = 0.0f;
	m_Interaction.m_PendingPage = PendingPage;
}

bool CMenusStart::UpdateAnimations()
{
	if(m_Interaction.m_Progress >= 1.0f)
		return false;
	float Duration = SUBMENU_DURATION;
	if(m_Interaction.m_Target == EState::SETTINGS_DRAWER || m_Interaction.m_Target == EState::TOP_POPOVER || m_Interaction.m_Target == EState::RIGHT_DRAWER || m_Interaction.m_Previous == EState::SETTINGS_DRAWER || m_Interaction.m_Previous == EState::TOP_POPOVER || m_Interaction.m_Previous == EState::RIGHT_DRAWER)
		Duration = DRAWER_DURATION;
	else if(m_Interaction.m_Target == EState::PAGE_TRANSITION)
		Duration = PAGE_TRANSITION_DURATION;

	const float SpeedScale = std::max(0.01f, (float)g_Config.m_BcMainMenuAnimationSpeed / ANIMATION_SPEED_BASE);
	m_Interaction.m_Progress = g_Config.m_BcMainMenuAnimation ? std::min(1.0f, m_Interaction.m_Progress + Client()->RenderFrameTime() * SpeedScale / Duration) : 1.0f;
	if(m_Interaction.m_Progress == 1.0f)
	{
		m_Interaction.m_Current = m_Interaction.m_Target;
		if(m_Interaction.m_Current == EState::PAGE_TRANSITION && m_Interaction.m_PendingPage != -1)
		{
			GameClient()->m_Menus.SetMenuPage(m_Interaction.m_PendingPage);
			GameClient()->m_Menus.SetShowStart(false);
			m_Interaction = {};
			return true;
		}
	}
	return false;
}

void CMenusStart::OnShowStart()
{
	m_Interaction = {};
	m_EntranceProgress = g_Config.m_BcMainMenuAnimation ? 0.0f : 1.0f;
}

void CMenusStart::RenderDimmer(CUIRect MainView, float Alpha)
{
	MainView.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, Alpha), IGraphics::CORNER_NONE, 0.0f);
}

void CMenusStart::RenderCenterLogo(CUIRect MainView, float Visibility)
{
	const float Diameter = std::clamp(MainView.h * LOGO_DIAMETER_RATIO, 104.0f, 154.0f);
	const vec2 Center(MainView.x + MainView.w * MENU_ANCHOR_X_RATIO, MainView.y + MainView.h * 0.51f);
	// Primitive radial burst, kept subtle so custom/video backgrounds remain visible.
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	for(int i = 0; i < 24; ++i)
	{
		const float A = i * 2.0f * pi / 24.0f;
		Graphics()->SetColor(0.55f, 0.38f, 0.9f, 0.18f * Visibility);
		IGraphics::CLineItem Line(Center.x + std::cos(A) * Diameter * 0.53f, Center.y + std::sin(A) * Diameter * 0.53f, Center.x + std::cos(A) * Diameter * 0.68f, Center.y + std::sin(A) * Diameter * 0.68f);
		Graphics()->LinesDraw(&Line, 1);
	}
	Graphics()->LinesEnd();
	CUIRect Circle{Center.x - Diameter / 2, Center.y - Diameter / 2, Diameter, Diameter};
	Circle.Draw(ColorRGBA(0.05f, 0.04f, 0.08f, 0.7f * Visibility), IGraphics::CORNER_ALL, Diameter / 2);
	Circle.Margin(4.0f, &Circle);
	Circle.Draw(ColorRGBA(0.94f, 0.93f, 1.0f, Visibility), IGraphics::CORNER_ALL, Circle.w / 2);
	Circle.Margin(5.0f, &Circle);
	Circle.Draw(ColorRGBA(0.32f, 0.20f, 0.62f, Visibility), IGraphics::CORNER_ALL, Circle.w / 2);
	TextRender()->TextColor(1, 1, 1, Visibility);
	Ui()->DoLabel(&Circle, "GORES", std::clamp(Diameter * 0.19f, 20.0f, 29.0f), TEXTALIGN_MC);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}

void CMenusStart::RenderMainStrip(CUIRect MainView, float Visibility, bool InputEnabled)
{
	const float StripH = std::clamp(MainView.h * STRIP_HEIGHT_RATIO, 52.0f, 76.0f);
	const float CenterY = MainView.y + MainView.h * 0.51f;
	const float LogoD = std::clamp(MainView.h * LOGO_DIAMETER_RATIO, 104.0f, 154.0f);
	const float AnchorX = MainView.x + MainView.w * MENU_ANCHOR_X_RATIO;
	CUIRect Strip{MainView.x, CenterY - StripH / 2, MainView.w, StripH};
	Strip.Draw(ColorRGBA(0.025f, 0.022f, 0.035f, 0.80f * Visibility), IGraphics::CORNER_NONE, 0.0f);

	auto DrawSegment = [&](const void *pId, CUIRect Rect, const char *pIcon, const char *pLabel, ColorRGBA Color, float &Hover, bool Enabled, float Opacity) {
		const bool Hovered = Enabled && Ui()->MouseHovered(&Rect);
		const float SpeedScale = std::max(0.01f, (float)g_Config.m_BcMainMenuAnimationSpeed / ANIMATION_SPEED_BASE);
		const float Step = g_Config.m_BcMainMenuAnimation ? Client()->RenderFrameTime() * SpeedScale / HOVER_DURATION : 1.0f;
		Hover += (Hovered ? 1.0f : -1.0f) * Step;
		Hover = std::clamp(Hover, 0.0f, 1.0f);
		const float Expand = Rect.w * HOVER_EXPANSION * EaseOutCubic(Hover);
		Rect.x -= Expand / 2;
		Rect.w += Expand;
		Color = Mix(Color, ColorRGBA(std::min(1.0f, Color.r + .15f), std::min(1.0f, Color.g + .15f), std::min(1.0f, Color.b + .15f), Color.a), Hover);
		const float VisualAlpha = Visibility * Opacity;
		Color.a *= VisualAlpha;
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(Color);
		IGraphics::CFreeformItem Quad(vec2(Rect.x + SEGMENT_SKEW, Rect.y), vec2(Rect.x + Rect.w + SEGMENT_SKEW, Rect.y), vec2(Rect.x + Rect.w - SEGMENT_SKEW, Rect.y + Rect.h), vec2(Rect.x - SEGMENT_SKEW, Rect.y + Rect.h));
		Graphics()->QuadsDrawFreeform(&Quad, 1);
		Graphics()->QuadsEnd();
		CUIRect Icon, Label;
		Rect.HSplitTop(Rect.h * .58f, &Icon, &Label);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, VisualAlpha);
		TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
		Ui()->DoLabel(&Icon, pIcon, 20.0f + Hover * 1.5f, TEXTALIGN_MC);
		TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
		Ui()->DoLabel(&Label, pLabel, 10.0f + Hover, TEXTALIGN_MC);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
		return Enabled && Ui()->DoButtonLogic(pId, 0, &Rect, BUTTONFLAG_LEFT);
	};

	const bool Stable = m_Interaction.m_Progress >= 1.0f;
	EState ShownState = m_Interaction.m_Current;
	float SubmenuAmount = IsSubmenu(ShownState) ? 1.0f : 0.0f;
	if(m_Interaction.m_Target == EState::PAGE_TRANSITION && IsSubmenu(m_Interaction.m_Previous))
	{
		ShownState = m_Interaction.m_Previous;
		SubmenuAmount = 1.0f;
	}
	else if(m_Interaction.m_Progress < 1.0f && (IsSubmenu(m_Interaction.m_Previous) || IsSubmenu(m_Interaction.m_Target)))
	{
		ShownState = IsSubmenu(m_Interaction.m_Target) ? m_Interaction.m_Target : m_Interaction.m_Previous;
		SubmenuAmount = IsSubmenu(m_Interaction.m_Target) ? EaseOutCubic(m_Interaction.m_Progress) : 1.0f - EaseInOutCubic(m_Interaction.m_Progress);
	}
	const float MainAmount = 1.0f - SubmenuAmount;
	const float GroupW = std::min(SEGMENT_MAX_GROUP_WIDTH, MainView.w - AnchorX - LogoD * .52f - 20.0f);
	const float StartX = AnchorX + LogoD * .48f;
	static CButtonContainer s_aPrimary[5];
	// Settings is independent of the right category morph and remains anchored.
	CUIRect Settings{AnchorX - LogoD * .48f - SEGMENT_WIDTH, Strip.y, SEGMENT_WIDTH, Strip.h};
	if(DrawSegment(&s_aPrimary[0], Settings, FontIcon::GEAR, Localize("Settings"), COLOR_SETTINGS, m_Interaction.m_aPrimaryHover[0], InputEnabled && Stable, 1.0f))
		BeginTransition(EState::SETTINGS_DRAWER);
	if(MainAmount > 0.001f)
	{
		const char *apIcons[] = {FontIcon::PLAY, FontIcon::PEN_TO_SQUARE, FontIcon::EARTH_AMERICAS, FontIcon::POWER_OFF};
		const char *apLabels[] = {Localize("Play", "Start menu"), Localize("Editor"), Localize("Browse"), Localize("Exit")};
		const ColorRGBA aColors[] = {COLOR_PLAY, COLOR_EDITOR, COLOR_BROWSE, COLOR_EXIT};
		for(int i = 0; i < 4; ++i)
		{
			const float FinalW = GroupW / 4;
			CUIRect R{StartX + FinalW * i * MainAmount, Strip.y, FinalW * MainAmount, Strip.h};
			if(DrawSegment(&s_aPrimary[i + 1], R, apIcons[i], apLabels[i], aColors[i], m_Interaction.m_aPrimaryHover[i + 1], InputEnabled && Stable && ShownState == EState::MAIN, MainAmount))
			{
				if(i == 0)
					BeginTransition(EState::PLAY_SUBMENU);
				else if(i == 1)
					BeginTransition(EState::EDITOR_SUBMENU);
				else if(i == 2)
					BeginTransition(EState::BROWSE_SUBMENU);
				else if(g_Config.m_BcConfirmQuit || GameClient()->Editor()->HasUnsavedData())
					GameClient()->m_Menus.ShowQuitPopup();
				else
					Client()->Quit();
			}
		}
	}
	if(SubmenuAmount > 0.001f)
	{
		const char *apLabels[4] = {};
		const char *apIcons[4] = {};
		int Count = 0;
		ColorRGBA Color = COLOR_PLAY;
		if(ShownState == EState::PLAY_SUBMENU)
		{
			Count = 4;
			apLabels[0] = Localize("Internet");
			apLabels[1] = Localize("Favorites");
			apLabels[2] = Localize("LAN");
			apLabels[3] = Localize("Back");
			apIcons[0] = FontIcon::EARTH_AMERICAS;
			apIcons[1] = FontIcon::HEART;
			apIcons[2] = FontIcon::NETWORK_WIRED;
			apIcons[3] = FontIcon::CHEVRON_LEFT;
		}
		else if(ShownState == EState::EDITOR_SUBMENU)
		{
			Count = 3;
			Color = COLOR_EDITOR;
			apLabels[0] = Localize("Map editor");
			apLabels[1] = GameClient()->m_LocalServer.IsServerRunning() ? Localize("Stop server") : Localize("Run server");
			apLabels[2] = Localize("Back");
			apIcons[0] = FontIcon::PEN_TO_SQUARE;
			apIcons[1] = FontIcon::NETWORK_WIRED;
			apIcons[2] = FontIcon::CHEVRON_LEFT;
		}
		else
		{
			Count = 3;
			Color = COLOR_BROWSE;
			apLabels[0] = Localize("Demos");
			apLabels[1] = Localize("Clans");
			apLabels[2] = Localize("Back");
			apIcons[0] = FontIcon::FILM;
			apIcons[1] = FontIcon::ICON_USERS;
			apIcons[2] = FontIcon::CHEVRON_LEFT;
		}
		static CButtonContainer s_aSubmenu[4];
		for(int i = 0; i < Count; ++i)
		{
			const float W = GroupW / Count;
			CUIRect R{StartX + (W * i - GroupW * .08f) * SubmenuAmount, Strip.y, W * SubmenuAmount, Strip.h};
			if(DrawSegment(&s_aSubmenu[i], R, apIcons[i], apLabels[i], Color, m_Interaction.m_aSubmenuHover[i], InputEnabled && Stable && IsSubmenu(ShownState), SubmenuAmount))
			{
				if(i == Count - 1)
					BeginTransition(EState::MAIN);
				else if(ShownState == EState::PLAY_SUBMENU)
					BeginTransition(EState::PAGE_TRANSITION, i == 0 ? CMenus::PAGE_INTERNET : i == 1 ? CMenus::PAGE_FAVORITES :
															   CMenus::PAGE_LAN);
				else if(ShownState == EState::BROWSE_SUBMENU)
					BeginTransition(EState::PAGE_TRANSITION, i == 0 ? CMenus::PAGE_DEMOS : CMenus::PAGE_CLANS);
				else if(i == 0)
				{
					g_Config.m_ClEditor = 1;
					Input()->MouseModeRelative();
				}
				else if(GameClient()->m_LocalServer.IsServerRunning())
					GameClient()->m_LocalServer.KillServer();
				else
					GameClient()->m_LocalServer.RunServer({});
			}
		}
	}
}

void CMenusStart::RenderTopUtilityBar(CUIRect MainView, bool InputEnabled)
{
	CUIRect Bar{MainView.x, MainView.y, MainView.w, 25.0f};
	Bar.Draw(ColorRGBA(0.02f, 0.02f, 0.03f, 0.72f), IGraphics::CORNER_NONE, 0.0f);
	CUIRect Brand{Bar.x + 12.0f, Bar.y, 170.0f, Bar.h};
	Ui()->DoLabel(&Brand, "GORES CLIENT  /  HOME", 11.0f, TEXTALIGN_ML);
	static CButtonContainer s_Profile, s_Info;
	CUIRect Info{Bar.x + Bar.w - 36.0f, Bar.y, 32.0f, Bar.h};
	CUIRect Profile{Info.x - 38.0f, Bar.y, 32.0f, Bar.h};
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	if(InputEnabled && GameClient()->m_Menus.DoButton_Menu(&s_Info, FontIcon::INFO, 0, &Info, BUTTONFLAG_LEFT, nullptr, 0, 0.0f, 0.0f, ColorRGBA(0, 0, 0, 0)))
		BeginTransition(EState::RIGHT_DRAWER);
	if(InputEnabled && GameClient()->m_Menus.DoButton_Menu(&s_Profile, FontIcon::USER, 0, &Profile, BUTTONFLAG_LEFT, nullptr, 0, 0.0f, 0.0f, ColorRGBA(0, 0, 0, 0)))
		BeginTransition(EState::TOP_POPOVER);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
}

void CMenusStart::RenderSettingsDrawer(CUIRect MainView, float Progress)
{
	const float W = std::clamp(MainView.w * DRAWER_WIDTH_RATIO, 360.0f, 550.0f);
	CUIRect Drawer{MainView.x - W * (1.0f - Progress), MainView.y, W, MainView.h};
	Drawer.Draw(ColorRGBA(0.075f, 0.065f, 0.105f, 0.99f), IGraphics::CORNER_NONE, 0.0f);
	CUIRect Rail, Content;
	Drawer.VSplitLeft(std::clamp(W * .25f, 86.0f, 118.0f), &Rail, &Content);
	Rail.Draw(ColorRGBA(0.035f, 0.03f, 0.055f, 1.0f), IGraphics::CORNER_NONE, 0.0f);
	CUIRect Back{Rail.x + 8, Rail.y + 12, Rail.w - 16, 38};
	static CButtonContainer s_Back;
	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	if(Progress >= 1.0f && GameClient()->m_Menus.DoButton_Menu(&s_Back, FontIcon::CHEVRON_LEFT, 0, &Back, BUTTONFLAG_LEFT))
		BeginTransition(EState::MAIN);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);
	const char *apLabels[] = {"General", "Tee", "Appearance", "Controls", "Graphics", "Sound", "DDNet", "Assets", "TClient", "Gores", "Profiles", "Configs"};
	const int aPages[] = {CMenus::SETTINGS_GENERAL, CMenus::SETTINGS_TEE, CMenus::SETTINGS_APPEARANCE, CMenus::SETTINGS_CONTROLS, CMenus::SETTINGS_GRAPHICS, CMenus::SETTINGS_SOUND, CMenus::SETTINGS_DDNET, CMenus::SETTINGS_ASSETS, CMenus::SETTINGS_TCLIENT, CMenus::SETTINGS_BESTCLIENT, CMenus::SETTINGS_PROFILES, CMenus::SETTINGS_CONFIGS};
	static CButtonContainer s_aNavigation[std::size(aPages)];
	for(size_t i = 0; i < std::size(aPages); ++i)
	{
		CUIRect Item{Rail.x + 5.0f, Rail.y + 60.0f + i * 31.0f, Rail.w - 10.0f, 27.0f};
		if(Progress < 1.0f)
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Progress);
			Ui()->DoLabel(&Item, Localize(apLabels[i]), 9.0f, TEXTALIGN_MC);
			TextRender()->TextColor(TextRender()->DefaultTextColor());
		}
		else if(GameClient()->m_Menus.DoButton_Menu(&s_aNavigation[i], Localize(apLabels[i]), g_Config.m_UiSettingsPage == aPages[i], &Item, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f))
			g_Config.m_UiSettingsPage = aPages[i];
	}
	Content.Margin(10.0f, &Content);
	if(Progress >= 1.0f)
	{
		const int Page = std::clamp(g_Config.m_UiSettingsPage, 0, CMenus::SETTINGS_LENGTH - 1);
		const bool OwnsScrolling = Page == CMenus::SETTINGS_TEE || Page == CMenus::SETTINGS_CONTROLS || Page == CMenus::SETTINGS_ASSETS || Page == CMenus::SETTINGS_TCLIENT || Page == CMenus::SETTINGS_PROFILES || Page == CMenus::SETTINGS_CONFIGS;
		if(OwnsScrolling)
		{
			Ui()->ClipEnable(&Content);
			GameClient()->m_Menus.RenderSettingsInStartDrawer(Content);
			Ui()->ClipDisable();
		}
		else
		{
			static CScrollRegion s_aPageScrollRegions[CMenus::SETTINGS_LENGTH];
			CScrollRegionParams Params;
			Params.m_ScrollUnit = 60.0f;
			Params.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
			Params.m_ScrollbarWidth = 12.0f;
			Params.m_ScrollbarMargin = 3.0f;
			vec2 ScrollOffset(0.0f, 0.0f);
			s_aPageScrollRegions[Page].Begin(&Content, &ScrollOffset, &Params);
			CUIRect PageView = Content;
			PageView.y += ScrollOffset.y;
			PageView.h = Page == CMenus::SETTINGS_GENERAL ? 980.0f : Page == CMenus::SETTINGS_GRAPHICS ? 820.0f :
														     900.0f;
			GameClient()->m_Menus.RenderSettingsInStartDrawer(PageView);
			s_aPageScrollRegions[Page].AddRect(PageView);
			s_aPageScrollRegions[Page].End();
		}
	}
	else
	{
		Content.x -= 12.0f * (1.0f - Progress);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Progress);
		Ui()->DoLabel(&Content, Localize("Settings"), 24.0f, TEXTALIGN_MC);
		TextRender()->TextColor(TextRender()->DefaultTextColor());
	}
}

void CMenusStart::RenderTopPopover(CUIRect MainView, float Progress)
{
	CUIRect Panel{MainView.x + MainView.w - 260.0f, MainView.y + 30.0f - 10.0f * (1.0f - Progress), 215.0f, 142.0f * Progress};
	Panel.Draw(ColorRGBA(0.065f, 0.055f, 0.09f, .98f * Progress), IGraphics::CORNER_ALL, 5.0f);
	if(Progress < 1.0f)
		return;
	const char *apLabels[] = {Localize("Player settings"), Localize("Clans"), Localize("Settings")};
	static CButtonContainer s_aButtons[3];
	for(int i = 0; i < 3; ++i)
	{
		CUIRect R{Panel.x + 10, Panel.y + 10 + i * 40.0f, Panel.w - 20, 34};
		if(GameClient()->m_Menus.DoButton_Menu(&s_aButtons[i], apLabels[i], 0, &R, BUTTONFLAG_LEFT))
		{
			if(i == 2)
				BeginTransition(EState::SETTINGS_DRAWER);
			else
				BeginTransition(EState::PAGE_TRANSITION, i == 0 ? CMenus::PAGE_SETTINGS : CMenus::PAGE_CLANS);
		}
	}
}

void CMenusStart::RenderRightDrawer(CUIRect MainView, float Progress)
{
	const float W = std::clamp(MainView.w * .25f, 280.0f, 420.0f);
	CUIRect Drawer{MainView.x + MainView.w - W * Progress, MainView.y, W, MainView.h};
	Drawer.Draw(ColorRGBA(0.055f, 0.05f, 0.075f, .99f), IGraphics::CORNER_NONE, 0.0f);
	CUIRect Title{Drawer.x + 20, Drawer.y + 28, Drawer.w - 40, 35};
	Ui()->DoLabel(&Title, Localize("Gores Client"), 24.0f, TEXTALIGN_ML);
	char aVersion[128];
	str_format(aVersion, sizeof(aVersion), "Gores %s  ·  DDNet %s", BESTCLIENT_VERSION, GAME_RELEASE_VERSION);
	CUIRect Version{Title.x, Title.y + 42, Title.w, 25};
	Ui()->DoLabel(&Version, aVersion, 12.0f, TEXTALIGN_ML);
	if(Progress < 1.0f)
		return;
	const char *apLabels[] = {Localize("Check update"), Localize("Discord"), Localize("Telegram"), Localize("Close")};
	static CButtonContainer s_aButtons[4];
	for(int i = 0; i < 4; ++i)
	{
		CUIRect R{Drawer.x + 20, Drawer.y + 110 + i * 45.0f, Drawer.w - 40, 36};
		if(GameClient()->m_Menus.DoButton_Menu(&s_aButtons[i], apLabels[i], 0, &R, BUTTONFLAG_LEFT))
		{
			if(i == 0)
			{
#if defined(CONF_AUTOUPDATE)
				Updater()->CheckForUpdate();
#endif
			}
			else if(i == 1)
				Client()->ViewLink("https://discord.gg/bestclient");
			else if(i == 2)
				Client()->ViewLink("https://t.me/bestddnet");
			else
				BeginTransition(EState::MAIN);
		}
	}
}

void CMenusStart::HandleEscape()
{
	if(!Ui()->ConsumeHotkey(CUi::HOTKEY_ESCAPE))
		return;
	// Opening/closing geometry is deliberately not reversed mid-flight. Consume
	// Escape until the deterministic transition reaches a stable state.
	if(m_Interaction.m_Progress < 1.0f)
		return;
	if(m_Interaction.m_Current == EState::TOP_POPOVER || m_Interaction.m_Current == EState::RIGHT_DRAWER || m_Interaction.m_Current == EState::SETTINGS_DRAWER || IsSubmenu(m_Interaction.m_Current))
		BeginTransition(EState::MAIN);
	else
		GameClient()->m_Menus.ShowQuitPopup();
}

void CMenusStart::RenderStartMenu(CUIRect MainView)
{
	GameClient()->m_MenuBackground.ChangePosition(CMenuBackground::POS_START);
	if(UpdateAnimations())
		return;
	const float SpeedScale = std::max(0.01f, (float)g_Config.m_BcMainMenuAnimationSpeed / ANIMATION_SPEED_BASE);
	if(m_EntranceProgress < 1.0f)
		m_EntranceProgress = g_Config.m_BcMainMenuAnimation ? std::min(1.0f, m_EntranceProgress + Client()->RenderFrameTime() * SpeedScale / ENTRANCE_DURATION) : 1.0f;
	HandleEscape();
	const bool Transitioning = m_Interaction.m_Progress < 1.0f;
	const float P = EaseInOutCubic(m_Interaction.m_Progress);
	const bool PageLeaving = m_Interaction.m_Target == EState::PAGE_TRANSITION;
	const float Entrance = EaseOutCubic(m_EntranceProgress);
	const float Visibility = (PageLeaving ? 1.0f - P : 1.0f) * Entrance;
	const bool Overlay = m_Interaction.m_Current == EState::SETTINGS_DRAWER || m_Interaction.m_Current == EState::TOP_POPOVER || m_Interaction.m_Current == EState::RIGHT_DRAWER || m_Interaction.m_Target == EState::SETTINGS_DRAWER || m_Interaction.m_Target == EState::TOP_POPOVER || m_Interaction.m_Target == EState::RIGHT_DRAWER;
	RenderMainStrip(MainView, Visibility, !Transitioning && !Overlay);
	RenderCenterLogo(MainView, Visibility);
	RenderTopUtilityBar(MainView, !Transitioning && !Overlay);

	float OverlayProgress = Overlay ? 1.0f : 0.0f;
	EState OverlayState = m_Interaction.m_Current;
	if(Transitioning && (m_Interaction.m_Target == EState::MAIN || Overlay))
	{
		OverlayState = m_Interaction.m_Target == EState::MAIN ? m_Interaction.m_Previous : m_Interaction.m_Target;
		OverlayProgress = m_Interaction.m_Target == EState::MAIN ? 1.0f - P : P;
	}
	if(OverlayProgress > 0.0f)
	{
		RenderDimmer(MainView, OVERLAY_ALPHA * OverlayProgress * (OverlayState == EState::TOP_POPOVER ? .65f : 1.0f));
		if(OverlayState == EState::SETTINGS_DRAWER)
			RenderSettingsDrawer(MainView, OverlayProgress);
		else if(OverlayState == EState::TOP_POPOVER)
			RenderTopPopover(MainView, OverlayProgress);
		else if(OverlayState == EState::RIGHT_DRAWER)
			RenderRightDrawer(MainView, OverlayProgress);
		if(OverlayProgress >= 1.0f && Ui()->MouseButtonClicked(0))
		{
			const float DrawerW = OverlayState == EState::SETTINGS_DRAWER ? std::clamp(MainView.w * DRAWER_WIDTH_RATIO, 360.0f, 550.0f) : OverlayState == EState::RIGHT_DRAWER ? std::clamp(MainView.w * .25f, 280.0f, 420.0f) :
																							     0.0f;
			const CUIRect Popover{MainView.x + MainView.w - 260.0f, MainView.y + 30.0f, 215.0f, 142.0f};
			if((OverlayState == EState::SETTINGS_DRAWER && Ui()->MouseX() > MainView.x + DrawerW) || (OverlayState == EState::RIGHT_DRAWER && Ui()->MouseX() < MainView.x + MainView.w - DrawerW) || (OverlayState == EState::TOP_POPOVER && !Ui()->MouseHovered(&Popover)))
				BeginTransition(EState::MAIN);
		}
	}
	CUIRect Bottom{MainView.x + MainView.w - 260, MainView.y + MainView.h - 35, 245, 24};
	Bottom.Draw(ColorRGBA(0.02f, 0.02f, 0.03f, .58f), IGraphics::CORNER_ALL, 4.0f);
	char aVersion[96];
	str_format(aVersion, sizeof(aVersion), "Gores Client %s  ·  DDNet %s", BESTCLIENT_VERSION, GAME_RELEASE_VERSION);
	Ui()->DoLabel(&Bottom, aVersion, 10.0f, TEXTALIGN_MC);
}

bool CMenusStart::CheckHotKey(int Key) const
{
	return !Input()->ShiftIsPressed() && !Input()->ModifierIsPressed() && !Input()->AltIsPressed() && Input()->KeyPress(Key) && !GameClient()->m_GameConsole.IsActive();
}
