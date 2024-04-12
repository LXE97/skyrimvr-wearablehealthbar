#pragma once
#include "Relocation.h"
#include "SKSE/Impl/Stubs.h"
#include "main_plugin.h"
#include "xbyak/xbyak.h"

namespace hooks
{
	extern float g_detection_level;

	// hook from Doodlum/Contextual Crosshair
	struct StealthMeter_Update
	{
		static char thunk(RE::StealthMeter* a1, int64_t a2, int64_t a3, int64_t a4)
		{
			auto result = func(a1, a2, a3, a4);

			g_detection_level = static_cast<float>(a1->unk88);

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void PostWandUpdateHook();

	void Install();
}