#pragma once
#include "Windows.h"

namespace helper
{
	RE::TESForm* LookupByName(RE::FormType typeEnum, const char* name);

	void CastSpellInstant(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);
	void Dispel(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);

	float GetAVPercent(RE::Actor* a, RE::ActorValue v);
	float GetChargePercent(RE::Actor* a, bool isLeft);
	float GetGameHour();  //  24hr time
	float GetAmmoPercent(RE::Actor* a, float ammoCountMult);
	float GetShoutCooldownPercent(RE::Actor* a, float MaxCDTime);

	void SetGlowMult();
	void SetGlowColor(RE::NiAVObject* target, RE::NiColor* c);
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
