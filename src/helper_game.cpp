#include "helper_game.h"

namespace helper
{
    RE::TESForm* LookupByName(RE::FormType typeEnum, const char* name)
    {
        auto* data = RE::TESDataHandler::GetSingleton();
        auto& forms = data->GetFormArray(typeEnum);
        for (auto*& form : forms)
        {
            if (!strcmp(form->GetName(), name))
            {
                return form;
            }
        }
        return nullptr;
    }

    float GetHealthPercent() { return 0; }
    float GetMagickaPercent() { return 0; }
    float GetStaminaPercent() { return 0; }
    float GetEnchantPercent(bool isLeft) { return 0; }
    float GetIngameTime() { return 0; }
    float GetAmmoPercent() { return 0; }
    float GetStealthPercent() { return 0; }
    float GetShoutCooldownPercent() { return 0; }

    void SetGlowMult() {}
    void SetGlowColor() {}
    void SetSpecularMult() {}
    void SetSpecularColor() {}
    void SetTintColor() {}

    void CastSpellInstant(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell) {
        if (src && target && spell) {
            auto caster = src->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
            if (caster)
            {
                caster->CastSpellImmediate(spell, false, target, 1.0, false, 1.0, src);
            }
        }
    }

    void Dispel(RE::Actor* src, RE::Actor* target, RE::SpellItem* spell) {
        if (src && target && spell) {
            auto handle = src->GetHandle();
            if (handle)
            {
                target->GetMagicTarget()->DispelEffect(spell, handle);
            }
        }
    }
}