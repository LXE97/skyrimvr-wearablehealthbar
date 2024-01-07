#undef PROFILE

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

	void OnOverlap(const OverlapEvent& e) {}

	std::shared_ptr<Wearable> w;

	bool OnDEBUGBtnPressA(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			auto player = PlayerCharacter::GetSingleton();
			SKSE::log::trace("A right");

			if (!menuchecker::isGameStopped()) {}
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
				static bool toggle = false;
				if (toggle ^= 1)
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
		static std::vector<long> update_times;

		auto start = std::chrono::steady_clock::now();
#endif

		ArtAddonManager::GetSingleton()->Update();

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
		NiTransform                   t;
		std::vector<std::string>      names = { "meter1" };
		std::vector<Meter::MeterType> av = { Meter::MeterType::kHealth };

		w = std::make_shared<Meter>("armor/SoulGauge/Mara-attach.nif",
			player->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), t, av, names);
		WearableManager::GetSingleton()->Register(w);
	}

	void PreGameLoad() { WearableManager::GetSingleton()->TransitionState(DressingState::kNone); }

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

			g_higgsInterface->AddPostVrikPostHiggsCallback(&Update);

			vrinput::OverlapSphereManager::GetSingleton()->SetPalmOffset(g_higgs_palmPosHandspace);
		}

		// Other Manager setup

		// TODO: workaround for problem with accessing post-VRIK-update third person skeleton
		std::thread p1([]() {
			while (1)
			{
				if (vrinput::OverlapSphereManager::GetSingleton()->Update())
				{
					WearableManager::GetSingleton()->Update();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(30));
			}
		});
		p1.detach();

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
				g_OVRHookManager->RegisterControllerStateCB(vrinput::ControllerInput_CB);
			}
		}
	}
}
