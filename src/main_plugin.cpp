#define PROFILE

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
	using namespace wearable;

	// constants
	constexpr FormID g_playerID = 0x14;

	SKSE::detail::SKSETaskInterface* g_task;
	OpenVRHookManagerAPI*            g_OVRHookManager;
	PapyrusVR::VRManagerAPI*         g_VRManager;

	TESEffectShader* g_selection_effect;

	// DEBUG
	NiPoint3 g_higgs_palmPosHandspace;

	// TODO config file
	vr::EVRButtonId    g_config_SecondaryBtn = vr::k_EButton_SteamVR_Trigger;
	vr::EVRButtonId    g_config_PrimaryBtn = vr::k_EButton_Grip;
	std::vector<float> times;

	std::vector<std::unique_ptr<NifChar>> characters;

	bool ass = true;

	void makeit()

	{
		NiTransform t;
		auto        shit =
			PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]");
		int c = 0;
		while (ass)
		{
			c++;
			if (c > 200)
			{
				c = 0;
				characters.clear();
				SKSE::log::error("200 reacehd");
			}
			else { characters.push_back(std::make_unique<NifChar>('l', shit, t)); }
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}

	void OnOverlap(const OverlapEvent& e) {}

	std::shared_ptr<Wearable>             w;
	NifTextBox*                           x;
	NifChar*                              p;
	std::vector<std::unique_ptr<NifChar>> v;
	bool                                  OnDEBUGBtnPressA(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			auto player = PlayerCharacter::GetSingleton();

			if (!menuchecker::isGameStopped())
			{
				//x = new NifTextBox("MAGICKA", 0.1f, vrinput::GetHandNode(Hand::kRight, false), l);

				std::thread p1(makeit);
				p1.detach();

				vrinput::adjustable += 0.002;
			}
		}
		return false;
	}

	bool OnDEBUGBtnPressB(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			auto player = PlayerCharacter::GetSingleton();
			SKSE::log::trace("B press");
			if (!menuchecker::isGameStopped())
			{
				vrinput::adjustable -= 0.002;
				ass = false;
				static bool toggle = false;
				toggle ^= 1;
				if (toggle)
					vrinput::OverlapSphereManager::GetSingleton()->ShowDebugSpheres();
				else
					vrinput::OverlapSphereManager::GetSingleton()->HideDebugSpheres();
			}
		}
		return false;
	}

	void Update()
	{
#ifdef PROFILE
		static std::vector<long long> update_times;

		auto start = std::chrono::steady_clock::now();
#endif

		ArtAddonManager::GetSingleton()->Update();
		OverlapSphereManager::GetSingleton()->Update();
		WearableManager::GetSingleton()->Update();

#ifdef PROFILE
		auto end = std::chrono::steady_clock::now();
		update_times.push_back(
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());

		if (update_times.size() > 800)
		{
			auto   max = *std::max_element(update_times.begin(), update_times.end());
			auto   sum = std::accumulate(update_times.begin(), update_times.end(), 0);
			double avg = sum / update_times.size();
			SKSE::log::trace("update ns avg: {} worst: {}", avg, max);
			update_times.clear();
		}
#endif
	}

	void GameLoad()
	{
		auto player = RE::PlayerCharacter::GetSingleton();

		//OverlapSphereManager::GetSingleton()->ShowDebugSpheres();
		w.reset();
		NiTransform t;
		t.rotate = { { -0.22850621, -0.9327596, -0.2670444 },
			{ -0.9581844, 0.21687761, 0.08220619 }, { -0.020597734, 0.2866351, -0.9453561 } };
		t.translate = { 0.028520077, 0.91731554, 4.03214 };
		std::vector<std::string>      names = { "meter1" };
		std::vector<const char*>      parents = { "NPC L ForearmTwist1 [LLt1]",
				 "NPC L ForearmTwist2 [LLt2]", "NPC L Forearm [LLar]", "NPC L Hand [LHnd]" };
		std::vector<Meter::MeterType> types = { Meter::MeterType::kHealth,
			Meter::MeterType::kMagicka, Meter::MeterType::kStamina, Meter::MeterType::kAmmo,
			Meter::MeterType::kEnchantLeft, Meter::MeterType::kEnchantRight,
			Meter::MeterType::kShout, Meter::MeterType::kStealth, Meter::MeterType::kTime };

		w = std::make_shared<Meter>("armor/SoulGauge/Mara-attach.nif",
			player->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), t, NiPoint3(-2.0f, 0, 0),
			types, names, &parents);

		WearableManager::GetSingleton()->Register(w);
	}

	void PreGameLoad()
	{
		WearableManager::GetSingleton()->TransitionState(ManagerState::kNone);
		delete x;
	}

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
		vrinput::AddCallback(vr::k_EButton_A, OnDEBUGBtnPressA, Hand::kRight, ActionType::kPress);

		vrinput::AddCallback(
			vr::k_EButton_ApplicationMenu, OnDEBUGBtnPressB, Hand::kRight, ActionType::kPress);
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
