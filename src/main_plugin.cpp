#include "main_plugin.h"
#include <algorithm>
#include <cmath>
#include <thread>

#include "Hooks.hpp"
#include "helper_game.h"
#include "helper_math.h"
#include "menuchecker.h"

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
	void OnOverlap(const OverlapEvent& e) { SKSE::log::trace("overlap event {}", e.id); }

	std::vector<std::shared_ptr<ArtAddon>> test1;
	std::vector<std::shared_ptr<ArtAddon>> test3;
	std::vector<std::shared_ptr<ArtAddon>> testo;
	std::vector<const char*> str = { "NPC Pelvis [Pelv]", "NPC L Thigh [LThg]", "NPC L Calf [LClf]",
		"NPC L Foot [Lft ]", "NPC R Thigh [RThg]", "NPC R Calf [RClf]", "NPC R Foot [Rft ]",
		"NPC Spine [Spn0]", "NPC Spine1 [Spn1]", "NPC Spine2 [Spn2]", "NPC Neck [Neck]",
		"NPC Head [Head]", "NPC R Clavicle [RClv]", "NPC R UpperArm [RUar]", "NPC R Forearm [RLar]",
		"NPC R Hand [RHnd]", "NPC R Finger00 [RF00]", "NPC R Finger01 [RF01]",
		"NPC R Finger02 [RF02]", "NPC R Finger10 [RF10]", "NPC R Finger11 [RF11]",
		"NPC R Finger12 [RF12]", "NPC R Finger20 [RF20]", "NPC R Finger21 [RF21]",
		"NPC R Finger22 [RF22]", "NPC R Finger30 [RF30]", "NPC R Finger31 [RF31]",
		"NPC R Finger32 [RF32]", "NPC R Finger40 [RF40]", "NPC R Finger41 [RF41]",
		"NPC R Finger42 [RF42]", "NPC R ForearmTwist1 [RLt1]", "NPC R ForearmTwist2 [RLt2]",
		"NPC R UpperarmTwist1 [RUt1]", "NPC R UpperarmTwist2 [RUt2]", "NPC L Clavicle [LClv]",
		"NPC L UpperArm [LUar]", "NPC L Forearm [LLar]", "NPC L Hand [LHnd]",
		"NPC L Finger00 [LF00]", "NPC L Finger01 [LF01]", "NPC L Finger02 [LF02]",
		"NPC L Finger10 [LF10]", "NPC L Finger11 [LF11]", "NPC L Finger12 [LF12]",
		"NPC L Finger20 [LF20]", "NPC L Finger21 [LF21]", "NPC L Finger22 [LF22]",
		"NPC L Finger30 [LF30]", "NPC L Finger31 [LF31]", "NPC L Finger32 [LF32]",
		"NPC L Finger40 [LF40]", "NPC L Finger41 [LF41]", "NPC L Finger42 [LF42]",
		"NPC L ForearmTwist1 [LLt1]", "NPC L ForearmTwist2 [LLt2]", "NPC L UpperarmTwist1 [LUt1]",
		"NPC L UpperarmTwist2 [LUt2]" };
	//,"CockingMechanismCtrl"
	bool    piss = true;
	NiColor green(0x00ff00);
	NiColor orange(0xff9900);
	bool    onDEBUGBtnReleaseA()
	{
		SKSE::log::trace("A release ");
		if (!menuchecker::isGameStopped())
		{
			auto player = PlayerCharacter::GetSingleton();
			if (piss)
			{
				piss = false;
				NiTransform shitl;

				for (int i = 0; i < str.size(); i++)
				{
					test1.push_back(ArtAddon::Make("debug/debugsphere.nif", player,
						player->Get3D(false)->GetObjectByName("skeleton.nif"), shitl));
					test3.push_back(ArtAddon::Make("debug/debugsphere.nif", player,
						player->Get3D(false)->GetObjectByName("skeleton.nif"), shitl));
					testo.push_back(ArtAddon::Make("debug/debugsphere.nif", player,
						player->Get3D(false)->GetObjectByName(str[i]), shitl));
					if (!player->Get3D(false)->GetObjectByName(str[i]))
					{
						SKSE::log::trace("{}", str[i]);
					}
				}
			} else
			{
				if (auto n = test1[15]->Get3D())
				{
					auto n2 = testo[15]->Get3D();

					SKSE::log::trace("testo parent name {} addr {}", n2->parent->name,
						(void*)n2->parent);
					SKSE::log::trace("testo parent->parent name {} addr {}",
						n2->parent->parent->name, (void*)n2->parent->parent);

					auto n3 = player->Get3D(false)->GetObjectByName(str[15]);
					SKSE::log::trace("getnode parent name {} addr {}", n3->parent->name,
						(void*)n3->parent);
					SKSE::log::trace("getnode parent->parent name {} addr {}",
						n3->parent->parent->name, (void*)n3->parent->parent);

					NiAVObject* aa = player->Get3D(true)->GetObjectByName("NPC R Hand [RHnd]");
					auto        third_person_right =
						player->Get3D(false)->GetObjectByName("NPC R Hand [RHnd]");
					SKSE::log::trace("dist 1st to 3rd:{}",
						(third_person_right->world.translate.GetDistance(aa->world.translate)));
					SKSE::log::trace("dist 1st to art addon: {}",
						n->world.translate.GetDistance(n2->world.translate));
				}
			}
		}
		return false;
	}

	std::vector<OverlapSpherePtr> a;

	void poop()
	{
		NiTransform n;
		a.clear();
		for (int i = 0; i < 1; i++)
		{
			a.push_back(OverlapSphere::Make(
				PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName("AnimObjectR"),
				&OnOverlap, 5.0f));
		}
		PrintPlayerModelEffects();
	}
	static float hehe = 0.123456791043281555f;

	bool onDEBUGBtnPressA()
	{
		SKSE::log::trace("A press");
		if (!menuchecker::isGameStopped())
		{
			static int i = 0;
			if (i++ % 2 == 0)
			{
				g_vrikInterface->setFingerRange(false, hehe, hehe, 0, 0, 0, 0, 0, 0, 0, 0);
			} else
			{
				g_vrikInterface->restoreFingers(false);
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
			PrintPlayerModelEffects();
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
			SKSE::log::trace("skips: {} max: {} min: {} avg: {}", skips, max, min,
				sum / times.size());
			times.clear();
		}
		return false;
	}

	bool puke = true;
	void Update()
	{
		static int c = 0;
		if (c++ % 10 == 0)
		{
			times.push_back(OverlapSphereManager::GetSingleton()->Update());
		} else
		{
			OverlapSphereManager::GetSingleton()->Update();
		}
		auto         player = RE::PlayerCharacter::GetSingleton();
		auto         root = player->Get3D(false)->GetObjectByName("skeleton.nif")->world;
		auto         rot = root.Invert().rotate;
		NiUpdateData xd;
		for (int i = 0; i < test3.size(); i++)
		{
			if (auto node1 = test1[i]->Get3D())
			{
				auto dest = player->Get3D(true)->GetObjectByName(str[i])->world.translate +
				            player->Get3D(true)->GetObjectByName(str[i])->world.rotate *
				                player->Get3D(true)->GetObjectByName(str[i])->local.translate;
				node1->local.translate = rot * (dest - root.translate);
				//node1->Update(xd);
				if (puke) { helper::SetGlowColor(node1, &green); }
			}
			if (auto node3 = test3[i]->Get3D())
			{
				auto dest = testo[i]->Get3D()->world * testo[i]->Get3D()->local;
				node3->local.translate = rot * (dest.translate - root.translate);
				node3->Update(xd);
				if (puke) { helper::SetGlowColor(node3, &orange); }
			}
		}
		if (!test3.empty() && test3[0]->Get3D()) puke = false;

		ArtAddonManager::GetSingleton()->Update();
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
		vrinput::AddCallback(vr::k_EButton_ApplicationMenu, onDEBUGBtnPressB, Right, Press,
			ButtonDown);
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
