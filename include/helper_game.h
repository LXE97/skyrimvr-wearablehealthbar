#pragma once
#include "Windows.h"

namespace helper
{
	RE::TESForm* LookupByName(RE::FormType a_typeEnum, const char* a_name);
	RE::FormID   GetFullFormID(uint8_t a_modindex, RE::FormID a_localID);

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
	void SetUVCoords(RE::NiAVObject* a_target, float a_x, float a_y);

	void PrintPlayerModelEffects();
	void PrintPlayerShaderEffects();

}
