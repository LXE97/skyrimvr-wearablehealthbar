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

	void OnOverlap(const OverlapEvent& e) {}

	std::shared_ptr<Wearable> w;

	void fuckoff(RE::NiAVObject* a_target, bool ff)
	{

		/*
		if (auto geom = a_target->AsGeometry())
		{
			std::vector<uint32_t> c;
			NiUpdateData          ctx;
			if (auto tri = static_cast<BSTriShape*>(geom))
			{
				if (auto property = geom->properties[RE::BSGeometry::States::kEffect].get())
				{
					if (auto shader = netimmerse_cast<RE::BSLightingShaderProperty*>(property))
					{
						if (auto material = static_cast<RE::BSLightingShaderMaterialHairTint*>(
								shader->material))
						{
							material->tintColor = NiColor(0xff0000);
							shader->lastRenderPassState = std::numeric_limits<std::int32_t>::max();

							material->texCoordOffset[0].x = 0.5f;
							material->texCoordOffset[0].y = 0.0f;
							material->texCoordOffset[1].x = 0.5f;
							material->texCoordOffset[1].y = 0.0f;

							a_target->Update(ctx);
						}
					}
				}
			}
		}*/
	}

	bool ok = false;
	bool OnDEBUGBtnPressA(const ModInputEvent& e)
	{
		if (e.button_state == ButtonState::kButtonDown)
		{
			auto player = PlayerCharacter::GetSingleton();

			if (!menuchecker::isGameStopped())
			{
				vrinput::adjustable += 0.002;
				ok ^= 1;
				fuckoff(
					PlayerCharacter::GetSingleton()->GetNodeByName("Bone"), ok);
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
		static std::vector<long long> update_times[4];

		auto start = std::chrono::steady_clock::now();
#endif
		//ArtAddonManager::GetSingleton()->Update();

#ifdef PROFILE
		auto AAend = std::chrono::steady_clock::now();
#endif
		//OverlapSphereManager::GetSingleton()->Update();

#ifdef PROFILE
		auto OVend = std::chrono::steady_clock::now();
#endif
		//WearableManager::GetSingleton()->Update();

#ifdef PROFILE
		auto WBend = std::chrono::steady_clock::now();

		update_times[0].push_back(
			std::chrono::duration_cast<std::chrono::nanoseconds>(AAend - start).count());
		update_times[1].push_back(
			std::chrono::duration_cast<std::chrono::nanoseconds>(OVend - AAend).count());
		update_times[2].push_back(
			std::chrono::duration_cast<std::chrono::nanoseconds>(WBend - OVend).count());
		update_times[3].push_back(
			std::chrono::duration_cast<std::chrono::nanoseconds>(WBend - start).count());

		if (update_times[0].size() > 800)
		{
			for (int i = 0; i < 4; i++)
			{
				auto   max = *std::max_element(update_times[i].begin(), update_times[i].end());
				auto   min = *std::min_element(update_times[i].begin(), update_times[i].end());
				auto   sum = std::accumulate(update_times[i].begin(), update_times[i].end(), 0);
				double avg = sum / update_times[i].size();
				SKSE::log::trace("update{} ns avg: {} max: {} min: {}", i, avg, max, min);
				update_times[i].clear();
			}
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
		std::vector<const char*>       parents = { "NPC L ForearmTwist1 [LLt1]",
				  "NPC L ForearmTwist2 [LLt2]", "NPC L Forearm [LLar]", "NPC L Hand [LHnd]" };
		std::vector<Meter::MeterValue> types = { Meter::MeterValue::kHealth,
			Meter::MeterValue::kMagicka, Meter::MeterValue::kStamina, Meter::MeterValue::kAmmo,
			Meter::MeterValue::kEnchantLeft, Meter::MeterValue::kEnchantRight,
			Meter::MeterValue::kShout, Meter::MeterValue::kStealth, Meter::MeterValue::kTime };

		/*w = std::make_shared<WearableMeter>("Wearable/Mara-attach.nif",
			player->Get3D(false)->GetObjectByName("NPC L Hand [LHnd]"), t, NiPoint3(-2.0f, 0, 0),
			&parents);
		WearableManager::GetSingleton()->Register(w);*/
	}

	void PreGameLoad()
	{
		WearableManager::GetSingleton()->StateTransition(ManagerState::kNone);
		w.reset();
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
