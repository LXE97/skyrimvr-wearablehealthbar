#pragma once

#include "higgsinterface001.h"
#include "vrikinterface001.h"
#include "RE/N/NiRTTI.h"
#include "SKSE/Impl/Stubs.h"
#include "VR/PapyrusVRAPI.h"
#include "VR/VRManagerAPI.h"
#include "VR/OpenVRUtils.h"
#include "VRInteractionSphere.h"

#include "mod_input.h"
#include "mod_eventSink.hpp"

class AnimationDataManager;

namespace wearable
{
    extern SKSE::detail::SKSETaskInterface* g_task;
    extern OpenVRHookManagerAPI* g_OVRHookManager;
    extern PapyrusVR::VRManagerAPI* g_VRManager;
    extern vr::TrackedDeviceIndex_t g_l_controller;
    extern vr::TrackedDeviceIndex_t g_r_controller;

    // Main plugin entry point
    void StartMod();
    void GameLoad();
    void PreGameLoad();

    // Event handlers
    void onMenuOpenClose(const RE::MenuOpenCloseEvent* event);
    void OnOverlap(const vrinput::OverlapEvent& e);

    void RegisterVRInputCallback();

    // Utilities
    inline double GetQPC() noexcept
    {
        LARGE_INTEGER f, i;
        if (QueryPerformanceCounter(&i) && QueryPerformanceFrequency(&f))
        {
            auto frequency = 1.0 / static_cast<double>(f.QuadPart);
            return static_cast<double>(i.QuadPart) * frequency;
        }
        return 0.0;
    }

}
