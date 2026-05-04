#include "InputHandler.h"
#include "Hooks.h"
#include "Settings.h"

namespace HaBCR
{
class ReloadInterruptHandler : public RE::PlayerInputHandler
{
  public:
    explicit ReloadInterruptHandler(RE::PlayerControlsData &a_data) : PlayerInputHandler(a_data)
    {
        inputEventHandlingEnabled = true;
    }

    bool ShouldHandleEvent(const RE::InputEvent *a_event) override
    {
        if (!a_event)
        {
            return false;
        }
        return *a_event->eventType == RE::INPUT_EVENT_TYPE::kButton;
    }

    void OnButtonEvent(const RE::ButtonEvent *a_event) override
    {
        if (!a_event)
        {
            return;
        }

        if (!a_event->QJustPressed())
        {
            return;
        }

        const auto &userEvent = a_event->QUserEvent();
        if (userEvent != "ReadyWeapon"sv && userEvent != "PrimaryAttack"sv)
        {
            return;
        }

        auto &state = g_reloadState;
        if (!state.IsReloading())
        {
            return;
        }

        if (!state.stopPressed && state.countedRC > 0 && state.countedRC < state.ShellsToAdd())
        {
            state.stopPressed = true;
            LOG_INFO("Stop requested");
        }
    }
};

void InstallInputHandler()
{
    auto *playerControls = RE::PlayerControls::GetSingleton();
    if (!playerControls)
    {
        logger::error("Failed to get PlayerControls singleton");
        return;
    }

    auto *handler = new ReloadInterruptHandler(playerControls->data);
    playerControls->handlers.emplace(playerControls->handlers.begin(), handler);

    LOG_INFO("Input handler registered");
}

}
