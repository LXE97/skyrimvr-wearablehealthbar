#include "main_plugin.h"
#include <cmath>
#include <algorithm>

#include "menuchecker.h"
#include "helper_math.h"
#include "helper_game.h"
#include "Hooks.hpp"

namespace wearable
{
    using namespace vrinput;
    using namespace RE;

    // constants
    constexpr FormID g_playerID = 0x14;

    SKSE::detail::SKSETaskInterface* g_task;
    OpenVRHookManagerAPI* g_OVRHookManager;
    PapyrusVR::VRManagerAPI* g_VRManager;
    vr::TrackedDeviceIndex_t g_l_controller;
    vr::TrackedDeviceIndex_t g_r_controller;

    // DEBUG
    int32_t g_debugLHandDrawSphere;
    int32_t g_debugRHandDrawSphere;
    NiPoint3 g_higgs_palmPosHandspace;
    NiPoint3 g_NPCHandPalmNormal = { 0, -1, 0 };
    helper::ArtAddon* gg;

    // TODO config file
    vr::EVRButtonId g_config_SecondaryBtn = vr::k_EButton_SteamVR_Trigger;
    vr::EVRButtonId g_config_PrimaryBtn = vr::k_EButton_Grip;

    bool onDEBUGBtnReleaseA()
    {
        SKSE::log::trace("A release ");

        return false;
    }
    bool onDEBUGBtnPressA()
    {
        SKSE::log::trace("A press ");
        if (!menuchecker::isGameStopped())
        {
            if (const auto processLists = RE::ProcessLists::GetSingleton()) {
                processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect) {
                    SKSE::log::trace("model effect addr: {} AO: {}", (void*)&a_modelEffect, (void*)(a_modelEffect.artObject));
                    return RE::BSContainer::ForEachResult::kContinue;
                    });
            }
        }
        return false;
    }

    bool onDEBUGBtnPressB()
    {
        SKSE::log::trace("B press ");
        if (!menuchecker::isGameStopped())
        {
            auto player = PlayerCharacter::GetSingleton();

            NiTransform g;
            g.translate = g_higgs_palmPosHandspace;
            gg = new helper::ArtAddon("debug/debugsphere.nif", player->AsReference(), player->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), g);
        }
        return false;
    }

    void onEquipEvent(const TESEquipEvent* event)
    {
        SKSE::log::info("equip event: getting actor");
        auto player = PlayerCharacter::GetSingleton();
        if (player && player == event->actor.get())
        {
            SKSE::log::info("equip event: looking up formid");
            auto item = TESForm::LookupByID(event->baseObject);
        }
    }

    void OnOverlap(const OverlapEvent& e)
    {
    }

    void Update()
    {
        helper::ArtAddonManager::GetSingleton()->Update();
    }

    void StartMod()
    {
        menuchecker::begin();

        hooks::Install();

        // VR init
        g_l_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
        g_r_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
        RegisterVRInputCallback();

        // HIGGS setup
        if (g_higgsInterface)
        {
            // TODO: read this from config
            g_higgs_palmPosHandspace = { 0, -2.4, 6 };
            vrinput::OverlapSphereManager::GetSingleton()->SetPalmOffset(g_higgs_palmPosHandspace);
            g_higgsInterface->AddPostVrikPostHiggsCallback(&Update);
        }

        // register event sinks and handlers
        auto equipSink = EventSink<TESEquipEvent>::GetSingleton();
        ScriptEventSourceHolder::GetSingleton()->AddEventSink(equipSink);
        equipSink->AddCallback(onEquipEvent);

        vrinput::AddCallback(vr::k_EButton_A, onDEBUGBtnPressA, Right, Press, ButtonDown);
        vrinput::AddCallback(vr::k_EButton_A, onDEBUGBtnReleaseA, Right, Press, ButtonUp);
        vrinput::AddCallback(vr::k_EButton_ApplicationMenu, onDEBUGBtnPressB, Right, Press, ButtonDown);
    }


    void GameLoad()
    {
        auto player = RE::PlayerCharacter::GetSingleton();
    }

    void PreGameLoad()
    {

    }

    void GameSave()
    {
        helper::ArtAddonManager::GetSingleton()->PreSaveGame();
    }

    // handles low level button/trigger events
    bool ControllerInput_CB(vr::TrackedDeviceIndex_t unControllerDeviceIndex, const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize, vr::VRControllerState_t* pOutputControllerState)
    {
        // save last controller input to only do processing on button changes
        static uint64_t prev_Pressed[2] = {};
        static uint64_t prev_Touched[2] = {};

        // need to remember the last output sent to the game in order to maintain input blocking without calling our game logic every packet
        static uint64_t prev_Pressed_out[2] = {};
        static uint64_t prev_Touched_out[2] = {};

        if (pControllerState && !menuchecker::isGameStopped())
        {
            bool isLeft = unControllerDeviceIndex == g_l_controller;
            if (isLeft || unControllerDeviceIndex == g_r_controller)
            {
                uint64_t pressedChange = prev_Pressed[isLeft] ^ pControllerState->ulButtonPressed;
                uint64_t touchedChange = prev_Touched[isLeft] ^ pControllerState->ulButtonTouched;
                if (pressedChange)
                {
                    vrinput::processButtonChanges(pressedChange, pControllerState->ulButtonPressed, isLeft, false, pOutputControllerState);
                    prev_Pressed[isLeft] = pControllerState->ulButtonPressed;
                    prev_Pressed_out[isLeft] = pOutputControllerState->ulButtonPressed;
                }
                else
                {
                    pOutputControllerState->ulButtonPressed = prev_Pressed_out[isLeft];
                }
                if (touchedChange)
                {
                    vrinput::processButtonChanges(touchedChange, pControllerState->ulButtonTouched, isLeft, true, pOutputControllerState);
                    prev_Touched[isLeft] = pControllerState->ulButtonTouched;
                    prev_Touched_out[isLeft] = pOutputControllerState->ulButtonTouched;
                }
                else
                {
                    pOutputControllerState->ulButtonTouched = prev_Touched_out[isLeft];
                }
            }
        }
        return true;
    }

    // Register SkyrimVRTools callback
    void RegisterVRInputCallback()
    {
        if (g_OVRHookManager->IsInitialized())
        {
            g_OVRHookManager = RequestOpenVRHookManagerObject();
            if (g_OVRHookManager)
            {
                SKSE::log::info("Successfully requested OpenVRHookManagerAPI.");
                // TODO: set up haptics: InitSystem(g_OVRHookManager->GetVRSystem()); 
                g_OVRHookManager->RegisterControllerStateCB(ControllerInput_CB);
            }
        }
    }
}
