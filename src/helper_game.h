#pragma once

namespace helper
{
    RE::TESForm* LookupByName(RE::FormType typeEnum, const char* name);

    float GetHealthPercent();
    float GetMagickaPercent();
    float GetStaminaPercent();
    float GetEnchantPercent(bool isLeft);
    float GetIngameTime();
    float GetAmmoPercent();
    float GetStealthPercent();
    float GetShoutCooldownPercent();

    void CastSpellInstant(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);
    void Dispel(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);

    void SetGlowMult();
    void SetGlowColor();
    void SetSpecularMult();
    void SetSpecularColor();
    void SetTintColor();
}