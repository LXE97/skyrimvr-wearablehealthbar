#pragma once
#include "Windows.h"

namespace helper
{
	RE::TESForm* LookupByName(RE::FormType a_typeEnum, const char* a_name);

	void CastSpellInstant(RE::Actor* a_src, RE::Actor* a_target, RE::SpellItem* sa_pell);
	void Dispel(RE::Actor* a_src, RE::Actor* a_target, RE::SpellItem* a_spell);

	float GetAVPercent(RE::Actor* a_a, RE::ActorValue a_v);
	float GetChargePercent(RE::Actor* a_a, bool isLeft);
	float GetGameHour();  //  24hr time
	float GetAmmoPercent(RE::Actor* a_a, float a_ammoCountMult);
	float GetShoutCooldownPercent(RE::Actor* a_a, float a_MaxCDTime);

	void SetGlowMult();
	void SetGlowColor(RE::NiAVObject* a_target, RE::NiColor* a_c);
	void SetSpecularMult();
	void SetSpecularColor();
	void SetTintColor();

	void PrintPlayerModelEffects();

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
