#define PROFILE 1
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

	// DEBUG
	int32_t  g_debugLHandDrawSphere;
	int32_t  g_debugRHandDrawSphere;
	NiPoint3 g_higgs_palmPosHandspace;
	NiPoint3 g_NPCHandPalmNormal = { 0, -1, 0 };

	// TODO config file
	vr::EVRButtonId    g_config_SecondaryBtn = vr::k_EButton_SteamVR_Trigger;
	vr::EVRButtonId    g_config_PrimaryBtn = vr::k_EButton_Grip;
	std::vector<float> times;

	void OnOverlap(const vrinput::OverlapEvent& e) {}

	std::shared_ptr<Wearable> w;
	OverlapSpherePtr          huh;

	bool OnDEBUGBtnReleaseA()
	{
		SKSE::log::trace("A left ");
		if (!menuchecker::isGameStopped()) { auto player = RE::PlayerCharacter::GetSingleton(); }
		return false;
	}

	bool OnDEBUGBtnPressA()
	{
		auto player = PlayerCharacter::GetSingleton();
		SKSE::log::trace("A right");
		if (!menuchecker::isGameStopped())
		{
			bool once = true;
			{
				NiTransform                   t;
				std::vector<std::string>      names = { "meter1" };
				std::vector<Meter::MeterType> av = { Meter::MeterType::kHealth };

				w = std::make_shared<Meter>("armor/SoulGauge/Mara-attach.nif",
					player->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), t, av, names);
				WearableManager::GetSingleton()->Register(w);
			}
		}
		return false;
	}

	bool OnDEBUGBtnPressB()
	{
		auto player = PlayerCharacter::GetSingleton();
		SKSE::log::trace("B press");
		if (!menuchecker::isGameStopped())
		{
			OverlapSphereManager::GetSingleton()->ShowDebugSpheres();
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

		static int c = 0;
		if (c++ % 1 == 0) { WearableManager::GetSingleton()->Update(); }

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

	void GameLoad() {}

	void PreGameLoad() {}

	void GameSave() { SKSE::log::trace("game save event"); }

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
		}

		// Other Manager setup
		vrinput::OverlapSphereManager::GetSingleton()->SetPalmOffset(g_higgs_palmPosHandspace);

		// TODO: workaround for problem with accessing post-VRIK update third person skeleton
		std::thread p1([]() {
			auto man = vrinput::OverlapSphereManager::GetSingleton();
			while (1)
			{
				man->Update();
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		});
		p1.detach();

		// register event sinks and handlers
		vrinput::AddCallback(vr::k_EButton_A, OnDEBUGBtnPressA, Right, Press, ButtonDown);
		vrinput::AddCallback(vr::k_EButton_A, OnDEBUGBtnReleaseA, Left, Press, ButtonUp);
		vrinput::AddCallback(
			vr::k_EButton_ApplicationMenu, OnDEBUGBtnPressB, Right, Press, ButtonDown);
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
