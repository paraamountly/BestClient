/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_START_H
#define GAME_CLIENT_COMPONENTS_MENUS_START_H

#include <game/client/component.h>
#include <game/client/ui_rect.h>

#include <array>

class CMenusStart : public CComponentInterfaces
{
public:
	void RenderStartMenu(CUIRect MainView);
	enum class EState
	{
		MAIN,
		PLAY_SUBMENU,
		EDITOR_SUBMENU,
		BROWSE_SUBMENU,
		SETTINGS_DRAWER,
		TOP_POPOVER,
		RIGHT_DRAWER,
		PAGE_TRANSITION,
	};

private:
	struct SInteractionState
	{
		EState m_Current = EState::MAIN;
		EState m_Previous = EState::MAIN;
		EState m_Target = EState::MAIN;
		float m_Progress = 1.0f;
		int m_PendingPage = -1;
		std::array<float, 5> m_aPrimaryHover{};
		std::array<float, 4> m_aSubmenuHover{};
	} m_Interaction;

	void BeginTransition(EState Target, int PendingPage = -1);
	void UpdateAnimations();
	void RenderMainStrip(CUIRect MainView, float Visibility, bool InputEnabled);
	void RenderCenterLogo(CUIRect MainView, float Visibility);
	void RenderTopUtilityBar(CUIRect MainView, bool InputEnabled);
	void RenderSettingsDrawer(CUIRect MainView, float Progress);
	void RenderTopPopover(CUIRect MainView, float Progress);
	void RenderRightDrawer(CUIRect MainView, float Progress);
	void RenderDimmer(CUIRect MainView, float Alpha);
	void HandleEscape();
	bool CheckHotKey(int Key) const;
};

#endif
