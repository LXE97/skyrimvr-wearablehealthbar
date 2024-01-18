#pragma once

#include "RE/N/NiRTTI.h"
#include "SKSE/Impl/Stubs.h"
#include "VR/OpenVRUtils.h"
#include "VR/PapyrusVRAPI.h"
#include "VR/VRManagerAPI.h"
#include "art_addon.h"
#include "higgsinterface001.h"
#include "mod_event_sink.hpp"
#include "mod_input.h"
#include "overlap_sphere.h"
#include "vrikinterface001.h"
#include "wearable.h"
#include "meter.h"

namespace wearable_plugin
{
	extern SKSE::detail::SKSETaskInterface* g_task;
	extern OpenVRHookManagerAPI*            g_OVRHookManager;
	extern PapyrusVR::VRManagerAPI*         g_VRManager;


	void StartMod();
	void DataLoaded();
	void GameLoad();
	void PreGameLoad();
	void Update();

	bool OnDEBUGBtnPressA();
	bool OnDEBUGBtnPressB();

	void RegisterVRInputCallback();
}
