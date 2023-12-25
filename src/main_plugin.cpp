#include "main_plugin.h"
#include <cmath>
#include <algorithm>

#include "menuChecker.h"
#include "helper_math.h"
#include "helper_game.h"
#include "Hooks.hpp"

namespace wearable
{
    using namespace vrinput;
    using namespace RE;

    // constants
    constexpr FormID g_playerID = 0x14;

    PlayerCharacter* g_player;
    SKSE::detail::SKSETaskInterface* g_task;
    bool g_isVrikPresent;
    OpenVRHookManagerAPI* g_OVRHookManager;
    PapyrusVR::VRManagerAPI* g_VRManager;
    vr::TrackedDeviceIndex_t g_l_controller;
    vr::TrackedDeviceIndex_t g_r_controller;

    TESForm* ThumbRForm;
    TESForm* ThumbLForm;
    TESForm* MultiRForm;
    TESForm* MultLForm;

    // DEBUG
    int32_t g_debugLHandDrawSphere;
    int32_t g_debugRHandDrawSphere;
    NiPoint3 g_higgs_palmPosHandspace;
    NiPoint3 g_NPCHandPalmNormal = { 0, -1, 0 };

    // TODO config file
    vr::EVRButtonId g_config_SecondaryBtn = vr::k_EButton_A;
    vr::EVRButtonId g_config_PrimaryBtn = vr::k_EButton_SteamVR_Trigger;
    auto h = "heyey";
    TESForm* crap;
    BGSArtObject* shit;

    bool onDEBUGBtnPressA()
    {
        SKSE::log::trace("A press ");
        if (!MenuChecker::isGameStopped())
        {



            if (shit) {
                //SKSE::log::trace("cool 4");
                shit->SetModel("debug/InteractionSphere.nif");
                //SKSE::log::trace("cool 5");
                g_player->ApplyArtObject(shit, -1.0f, nullptr, false, false, g_player->GetNodeByName("NPC Root [Root]"));
                //SKSE::log::trace("cool 6");
            }




            /*
            auto* data = RE::TESDataHandler::GetSingleton();

            auto& forms = data->GetFormArray(FormType::ArtObject);
            for (auto*& form : forms)
            {
                SKSE::log::trace("{}", form->GetFormID());
            }

            const auto& [map, lock] = TESForm::GetAllForms();
            [[maybe_unused]] const BSReadWriteLock l{ lock };
 SKSE::log::trace("shitffffff");
            auto it = map->begin();
            for (; it != map->end(); std::advance(it, 1))
            {
                if (it->second->GetFormType() == FormType::ArtObject)
                SKSE::log::trace("{}", it->first);

            }
*/

        }
        return false;
    }

    bool onDEBUGBtnPressB()
    {
        SKSE::log::trace("B press ");
        if (!MenuChecker::isGameStopped())
        {
            crap = TESForm::LookupByID(0x9405f);// trailer fx02
            //SKSE::log::trace("cool ");
            if (crap) {
                auto temp = crap->CreateDuplicateForm(false, nullptr);
                //SKSE::log::trace("cool 2");
                if (temp)
                {
                    //SKSE::log::trace("cool 3");
                    shit = temp->As<BGSArtObject>();

                }

            }
        }return false;
    }

    void onEquipEvent(const TESEquipEvent* event)
    {
        SKSE::log::info("equip event: getting actor");
        if (g_player && g_player == event->actor.get())
        {
            SKSE::log::info("equip event: looking up formid");
            auto item = TESForm::LookupByID(event->baseObject);
        }
    }

    void OnOverlap(const OverlapEvent& e)
    {
    }

    void Update(){
        vrinput::OverlapSphereManager::GetSingleton()->Update();
    }

    void StartMod()
    {
        Hooks::Install();

        // VR init
        g_l_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
        g_r_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
        RegisterVRInputCallback();

        MenuChecker::begin();

        // HIGGS setup
        if (g_higgsInterface)
        {
            // TODO: read this from config
            g_higgs_palmPosHandspace = { 0, -2.4, 6 };
            vrinput::OverlapSphereManager::GetSingleton()->palmoffset = g_higgs_palmPosHandspace;
            g_higgsInterface->AddPostVrikPostHiggsCallback(&Update);
        }

        // register event sinks and handlers
        auto equipSink = EventSink<TESEquipEvent>::GetSingleton();
        ScriptEventSourceHolder::GetSingleton()->AddEventSink(equipSink);
        equipSink->AddCallback(onEquipEvent);

        vrinput::AddCallback(vr::k_EButton_A, onDEBUGBtnPressA, Right, Press, ButtonDown);
        vrinput::AddCallback(vr::k_EButton_ApplicationMenu, onDEBUGBtnPressB, Right, Press, ButtonDown);
    }

    void GameLoad()
    {
        g_player = RE::PlayerCharacter::GetSingleton();

        // DEBUG: draw hand nodes with higgs offset

        vrinput::OverlapSphereManager::GetSingleton()->ShowHolsterSpheres();
        g_debugLHandDrawSphere = vrinput::OverlapSphereManager::GetSingleton()->Create(
            g_player->GetVRNodeData()->NPCLHnd.get(), &g_higgs_palmPosHandspace, 10, &g_NPCHandPalmNormal, 0, false, true);
        g_debugRHandDrawSphere = vrinput::OverlapSphereManager::GetSingleton()->Create(
            g_player->GetVRNodeData()->NPCRHnd.get(), &g_higgs_palmPosHandspace, 10, &g_NPCHandPalmNormal, 0, false, true);

    }

    void PreGameLoad()
    {
        vrinput::OverlapSphereManager::GetSingleton()->Destroy(g_debugLHandDrawSphere);
        vrinput::OverlapSphereManager::GetSingleton()->Destroy(g_debugRHandDrawSphere);
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

        if (pControllerState && !MenuChecker::isGameStopped())
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
                // InitSystem(g_OVRHookManager->GetVRSystem()); required for haptic triggers, set up later
                g_OVRHookManager->RegisterControllerStateCB(ControllerInput_CB);
            }
        }
    }
}
