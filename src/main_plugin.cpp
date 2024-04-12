
#include "main_plugin.h"

#include "helper_game.h"
#include "helper_math.h"
#include "hooks.h"
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
	using namespace wearable;

	// constants
	constexpr FormID g_playerID = 0x14;

	SKSE::detail::SKSETaskInterface* g_task;
	OpenVRHookManagerAPI*            g_OVRHookManager;
	PapyrusVR::VRManagerAPI*         g_VRManager;

	// DEBUG
	NiPoint3                       g_higgs_palmPosHandspace;
	std::vector<float>             times;
	std::shared_ptr<WearableMeter> w;
	OverlapSpherePtr               bowsphere;

	bool OnDEBUGBtnPressB(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			SKSE::log::trace("B press");
			SetFakeButtonState({ .device = Hand::kRight,
				.touch_or_press = ActionType::kPress,
				.button_state = ButtonState::kButtonDown,
				.button_ID = vr::k_EButton_SteamVR_Trigger });
			SetFakeButtonState({ .device = Hand::kRight,
				.touch_or_press = ActionType::kTouch,
				.button_state = ButtonState::kButtonDown,
				.button_ID = vr::k_EButton_SteamVR_Trigger });
		}
		else
		{
			ClearFakeButtonState({ .device = Hand::kRight,
				.touch_or_press = ActionType::kPress,
				.button_state = ButtonState::kButtonDown,
				.button_ID = vr::k_EButton_SteamVR_Trigger });
			ClearFakeButtonState({ .device = Hand::kRight,
				.touch_or_press = ActionType::kTouch,
				.button_state = ButtonState::kButtonDown,
				.button_ID = vr::k_EButton_SteamVR_Trigger });
		}
		return false;
	}

	bool OnDEBUGBtnPressStick(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			SKSE::log::trace("stick press");

			static bool toggle = false;
			toggle ^= 1;
			if (toggle) { vrinput::OverlapSphereManager::GetSingleton()->ShowDebugSpheres(); }
			else { vrinput::OverlapSphereManager::GetSingleton()->HideDebugSpheres(); }
		}
		return false;
	}

	bool AutoArrowNockEnd(const ModInputEvent& e)
	{
		SKSE::log::trace(
			"clear fake trigger, event from {} button {}", (int)e.device, (int)e.button_ID);
		ClearFakeButtonState({ .device = Hand::kRight,
			.touch_or_press = ActionType::kPress,
			.button_state = ButtonState::kButtonDown,
			.button_ID = vr::k_EButton_SteamVR_Trigger });
		ClearFakeButtonState({ .device = Hand::kRight,
			.touch_or_press = ActionType::kTouch,
			.button_state = ButtonState::kButtonDown,
			.button_ID = vr::k_EButton_SteamVR_Trigger });

		// TODO: does e actually contain all the parameters? cb not being removed
		vrinput::RemoveCallback(AutoArrowNockEnd, e.button_ID, e.device, vrinput::ActionType::kPress);

		return false;
	}

	void AutoArrowNockStart(const OverlapEvent& e)
	{
		if (e.entered)
		{
			SKSE::log::trace("bow overlap");
			// Check if the player is holding a bow
			if (auto equip = PlayerCharacter::GetSingleton()->GetEquippedObject(true))
			{
				if (auto weap = equip->As<TESObjectWEAP>(); weap && weap->IsBow())
				{
					// Check if the player is holding an arrow
					if (PlayerCharacter::GetSingleton()->GetCurrentAmmo())
					{
						// TODO: left handed mode
						auto arrow_hand = vrinput::Hand::kRight;

						if (GetButtonState(vr::k_EButton_SteamVR_Trigger, arrow_hand,
								vrinput::ActionType::kPress) == vrinput::ButtonState::kButtonDown)
						{
							// no callback needed for the trigger, just send momentary release
							SendFakeInputEvent({ .device = arrow_hand,
								.touch_or_press = ActionType::kPress,
								.button_state = ButtonState::kButtonUp,
								.button_ID = vr::k_EButton_SteamVR_Trigger });

							return;
						}

						// Find out which buttons are held down on the arrow's hand. We will
						// add a release listener to only one of them based on priority

						if (GetButtonState(vr::k_EButton_A, arrow_hand,
								vrinput::ActionType::kPress) == vrinput::ButtonState::kButtonDown)
						{
							vrinput::AddCallback(AutoArrowNockEnd, vr::k_EButton_A, arrow_hand,
								vrinput::ActionType::kPress);
						}
						else if (GetButtonState(vr::k_EButton_Knuckles_B, arrow_hand,
									 vrinput::ActionType::kPress) ==
							vrinput::ButtonState::kButtonDown)
						{
							vrinput::AddCallback(AutoArrowNockEnd, vr::k_EButton_Knuckles_B, arrow_hand,
								vrinput::ActionType::kPress);
						}
						else if (GetButtonState(vr::k_EButton_SteamVR_Touchpad, arrow_hand,
									 vrinput::ActionType::kPress) ==
							vrinput::ButtonState::kButtonDown)
						{
							vrinput::AddCallback(AutoArrowNockEnd, vr::k_EButton_SteamVR_Touchpad,
								arrow_hand, vrinput::ActionType::kPress);
						}
						else if (GetButtonState(
									 vr::k_EButton_Grip, arrow_hand, vrinput::ActionType::kPress) ==
							vrinput::ButtonState::kButtonDown)
						{
							vrinput::AddCallback(AutoArrowNockEnd, vr::k_EButton_Grip, arrow_hand,
								vrinput::ActionType::kPress);
						}
						else
						{
							SKSE::log::trace("AutoArrowNockStart: no button press");
							return;  // the player must have let go of the arrow button, do nothing
						}

						SKSE::log::trace("AutoArrowNockStart: starting fake trigger press");
						// Start holding down the trigger
						SetFakeButtonState({ .device = arrow_hand,
							.touch_or_press = ActionType::kPress,
							.button_state = ButtonState::kButtonDown,
							.button_ID = vr::k_EButton_SteamVR_Trigger });
						SetFakeButtonState({ .device = arrow_hand,
							.touch_or_press = ActionType::kTouch,
							.button_state = ButtonState::kButtonDown,
							.button_ID = vr::k_EButton_SteamVR_Trigger });
					}
				}
			}
		}
	}

	// test meter
	void meterinit()
	{
		w.reset();
		NiTransform t;
		t.rotate = { { -0.22850621, -0.9327596, -0.2670444 },
			{ -0.9581844, 0.21687761, 0.08220619 }, { -0.020597734, 0.2866351, -0.9453561 } };
		t.translate = { 0.028520077, 0.91731554, 4.03214 };

		std::vector<const char*> parents = { "NPC L ForearmTwist1 [LLt1]",
			"NPC L ForearmTwist2 [LLt2]", "NPC L Forearm [LLar]", "NPC L Hand [LHnd]" };

		std::vector<Meter::MeterValue> testValues = { Meter::MeterValue::kHealth,
			Meter::MeterValue::kMagicka, Meter::MeterValue::kStamina, Meter::MeterValue::kStealth };

		w = std::make_shared<WearableMeter>("wearable\\meters\\eye-triple.nif",
			PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), t,
			NiPoint3(-2.0f, 0, 0), &parents);
		WearableManager::GetSingleton()->Register(w);
		w->meters.push_back(std::make_unique<LinearMeter>(1, testValues));
		w->meters.push_back(std::make_unique<LinearMeter>(0, testValues));
		w->meters.push_back(std::make_unique<LinearMeter>(2, testValues));
		w->meters.push_back(std::make_unique<EyeMeter>(3, testValues));
	}

	bool runonce = false;
	void Update()
	{
		if (!runonce)
		{
			//meterinit();
			// TODO: put this in equip event for bow
			bowsphere = OverlapSphere::Make(
				PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName("SHIELD"),
				AutoArrowNockStart, 15, 0, Hand::kRight, { -14, 6, 0 });
			runonce = true;
		}

#ifdef PROFILE
		static std::vector<long long> update_times[4];

		auto start = std::chrono::steady_clock::now();
#endif
		ArtAddonManager::GetSingleton()->Update();

#ifdef PROFILE
		auto AAend = std::chrono::steady_clock::now();
#endif
		OverlapSphereManager::GetSingleton()->Update();

#ifdef PROFILE
		auto OVend = std::chrono::steady_clock::now();
#endif
		WearableManager::GetSingleton()->Update();

#ifdef PROFILE
		auto WBend = std::chrono::steady_clock::now();

		update_times[0].push_back(
			std::chrono::duration_cast<std::chrono::microseconds>(AAend - start).count());
		update_times[1].push_back(
			std::chrono::duration_cast<std::chrono::microseconds>(OVend - AAend).count());
		update_times[2].push_back(
			std::chrono::duration_cast<std::chrono::microseconds>(WBend - OVend).count());
		update_times[3].push_back(
			std::chrono::duration_cast<std::chrono::microseconds>(WBend - start).count());

		if (update_times[0].size() > 800)
		{
			for (int i = 0; i < 4; i++)
			{
				auto   max = *std::max_element(update_times[i].begin(), update_times[i].end());
				auto   min = *std::min_element(update_times[i].begin(), update_times[i].end());
				auto   sum = std::accumulate(update_times[i].begin(), update_times[i].end(), 0);
				double avg = sum / update_times[i].size();
				SKSE::log::trace("update {} avg: {} max: {} min: {}", i, avg, max, min);
				update_times[i].clear();
			}
		}
#endif
	}

	void GameLoad()
	{
		runonce = false;

		OverlapSphereManager::GetSingleton()->ShowDebugSpheres();
		WearableManager::GetSingleton()->StateTransition(ManagerState::kNone);
	}

	void PreGameLoad() {}

	void StartMod()
	{
		menuchecker::begin();
		hooks::Install();

		RegisterVRInputCallback();

		// HIGGS setup
		if (g_higgsInterface)
		{
			// TODO: read this from config
			g_higgs_palmPosHandspace = { 0, -2.4, 6 };

			vrinput::OverlapSphereManager::GetSingleton()->SetPalmOffset(g_higgs_palmPosHandspace);
		}

		// Other Manager setup

		// register event sinks and handlers
		vrinput::AddCallback(
			OnDEBUGBtnPressB, vr::k_EButton_Knuckles_B, Hand::kRight, ActionType::kPress);

		vrinput::AddCallback(
			OnDEBUGBtnPressStick, vr::k_EButton_SteamVR_Touchpad, Hand::kRight, ActionType::kPress);
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

				vrinput::g_leftcontroller =
					g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(
						vr::TrackedControllerRole_LeftHand);
				vrinput::g_rightcontroller =
					g_OVRHookManager->GetVRSystem()->GetTrackedDeviceIndexForControllerRole(
						vr::TrackedControllerRole_RightHand);
				g_OVRHookManager->RegisterControllerStateCB(vrinput::ControllerInputCallback);
				g_OVRHookManager->RegisterGetPosesCB(vrinput::ControllerPoseCallback);
			}
		}
	}
}
