#pragma once

namespace helper
{
    RE::TESForm* LookupByName(RE::FormType typeEnum, const char* name);

    float GetHealthPercent(RE::ActorValueOwner* a);
    float GetMagickaPercent(RE::Actor* a);
    float GetStaminaPercent(RE::Actor* a);
    float GetEnchantPercent(RE::Actor* a, bool isLeft);
    float GetIngameTime();
    float GetAmmoPercent(RE::Actor* a);
    float GetStealthPercent(RE::Actor* a);
    float GetShoutCooldownPercent(RE::Actor* a);

    void CastSpellInstant(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);
    void Dispel(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell);

    void SetGlowMult();
    void SetGlowColor();
    void SetSpecularMult();
    void SetSpecularColor();
    void SetTintColor();
}