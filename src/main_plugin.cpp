#include "main_plugin.h"

#include "helper_game.h"
#include "helper_math.h"
#include "hooks.hpp"
#include "menu_checker.h"

#include <algorithm>
#include <cmath>
#include <thread>

namespace wearable_plugin
{
	using namespace vrinput;
	using namespace RE;
	using namespace helper;
	using namespace art_addon;

	// constants
	constexpr FormID g_playerID = 0x14;

	SKSE::detail::SKSETaskInterface* g_task;
	OpenVRHookManagerAPI*            g_OVRHookManager;
	PapyrusVR::VRManagerAPI*         g_VRManager;
	vr::TrackedDeviceIndex_t         g_l_controller;
	vr::TrackedDeviceIndex_t         g_r_controller;

	// DEBUG
	int32_t  g_debugLHandDrawSphere;
	int32_t  g_debugRHandDrawSphere;
	NiPoint3 g_higgs_palmPosHandspace;
	NiPoint3 g_NPCHandPalmNormal = { 0, -1, 0 };

	// TODO config file
	vr::EVRButtonId    g_config_SecondaryBtn = vr::k_EButton_SteamVR_Trigger;
	vr::EVRButtonId    g_config_PrimaryBtn = vr::k_EButton_Grip;
	std::vector<float> times;

	void OnOverlap(const OverlapEvent& e) { SKSE::log::trace("overlap event {}", e.id);
	 }
OverlapSpherePtr s;
	bool onDEBUGBtnReleaseA()
	{
		SKSE::log::trace("A release ");
		if (!menuchecker::isGameStopped()) {
			s.reset();
		}
		return false;
	}

	bool onDEBUGBtnPressA()
	{
		auto player = PlayerCharacter::GetSingleton();
		SKSE::log::trace("A press");
		if (!menuchecker::isGameStopped())
		{
			bool once = true;
			{
				if (once) once = false;
				s =  OverlapSphere::Make(
					player->Get3D(true)->GetObjectByName("NPC R Hand [RHnd]"), &OnOverlap, 6);
					OverlapSphereManager::GetSingleton()->ShowDebugSpheres();
			}
		}
		return false;
	}

	bool onDEBUGBtnPressB()
	{
		auto player = PlayerCharacter::GetSingleton();
		SKSE::log::trace("B press");
		if (!menuchecker::isGameStopped())
		{
			int    skips = 0;
			float  max = 0;
			float  min = 0;
			double sum = 0;
			for (auto t : times)
			{
				if (t > max)
				{
					max = t;
				} else if (t > 0 && t < min)
				{
					min = t;
				}
				if (t > 0)
				{
					sum += t;
				} else
				{
					skips++;
				}
			}
			SKSE::log::trace(
				"skips: {} max: {} min: {} avg: {}", skips, max, min, sum / times.size());
			times.clear();
		}
		return false;
	}

	void Update()
	{
		ArtAddonManager::GetSingleton()->Update();
		times.push_back(OverlapSphereManager::GetSingleton()->Update());
	}

	void GameLoad() { auto player = RE::PlayerCharacter::GetSingleton(); }

	void PreGameLoad() {}

	void GameSave() { SKSE::log::trace("game save event"); }

	void StartMod()
	{
		menuchecker::begin();
		hooks::Install();
		// VR init
		g_l_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(
			vr::TrackedControllerRole_LeftHand);
		g_r_controller = g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(
			vr::TrackedControllerRole_RightHand);
		RegisterVRInputCallback();

		// HIGGS setup
		if (g_higgsInterface)
		{
			// TODO: read this from config
			g_higgs_palmPosHandspace = { 0, -2.4, 6 };
			g_higgsInterface->AddPostVrikPostHiggsCallback(&Update);
		}

		// Other Manager setup
		vrinput::OverlapSphereManager::GetSingleton()->SetPalmOffset(g_higgs_palmPosHandspace);

		// register event sinks and handlers
		vrinput::AddCallback(vr::k_EButton_A, onDEBUGBtnPressA, Right, Press, ButtonDown);
		vrinput::AddCallback(vr::k_EButton_A, onDEBUGBtnReleaseA, Left, Press, ButtonUp);
		vrinput::AddCallback(
			vr::k_EButton_ApplicationMenu, onDEBUGBtnPressB, Right, Press, ButtonDown);
	}

	// handles low level button/trigger events
	bool ControllerInput_CB(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
		const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize,
		vr::VRControllerState_t* pOutputControllerState)
	{
		// save last controller input to only do processing on button changes
		static uint64_t prev_Pressed[2] = {};
		static uint64_t prev_Touched[2] = {};

		// need to remember the last output sent to the game in order to maintain input blocking
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
					vrinput::processButtonChanges(pressedChange, pControllerState->ulButtonPressed,
						isLeft, false, pOutputControllerState);
					prev_Pressed[isLeft] = pControllerState->ulButtonPressed;
					prev_Pressed_out[isLeft] = pOutputControllerState->ulButtonPressed;
				} else
				{
					pOutputControllerState->ulButtonPressed = prev_Pressed_out[isLeft];
				}
				if (touchedChange)
				{
					vrinput::processButtonChanges(touchedChange, pControllerState->ulButtonTouched,
						isLeft, true, pOutputControllerState);
					prev_Touched[isLeft] = pControllerState->ulButtonTouched;
					prev_Touched_out[isLeft] = pOutputControllerState->ulButtonTouched;
				} else
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
