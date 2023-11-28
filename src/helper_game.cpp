#include "helper_game.h"

namespace helper
{
    using namespace RE;

    TESForm* LookupByName(FormType typeEnum, const char* name)
    {
        auto* data = TESDataHandler::GetSingleton();
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

    float GetHealthPercent(Actor* a)
    {
        SKSE::log::info("GetActorValue: {} \nGetBaseActorValue: {}\nGetClampedActorValue: {}\nGetPermanentActorValue: {}  ",
        a->AsActorValueOwner()->GetActorValue(ActorValue::kHealth),
        a->AsActorValueOwner()->GetBaseActorValue(ActorValue::kHealth),
        a->AsActorValueOwner()->GetClampedActorValue(ActorValue::kHealth),
        a->AsActorValueOwner()->GetPermanentActorValue(ActorValue::kHealth)
        );
        return 0;
    }
    float GetMagickaPercent(Actor* a) { return 0; }
    float GetStaminaPercent(Actor* a) { return 0; }
    float GetEnchantPercent(Actor* a, bool isLeft) { return 0; }
    float GetIngameTime(Actor* a) { return 0; }
    float GetAmmoPercent(Actor* a) { return 0; }
    float GetStealthPercent(Actor* a) { return 0; }
    float GetShoutCooldownPercent(Actor* a) { return 0; }

    void SetGlowMult() {}
    void SetGlowColor() {}
    void SetSpecularMult() {}
    void SetSpecularColor() {}
    void SetTintColor() {}

    void CastSpellInstant(Actor* src, Actor* target, SpellItem* spell) {
        if (src && target && spell) {
            auto caster = src->GetMagicCaster(MagicSystem::CastingSource::kInstant);
            if (caster)
            {
                caster->CastSpellImmediate(spell, false, target, 1.0, false, 1.0, src);
            }
        }
    }

    void Dispel(Actor* src, Actor* target, SpellItem* spell) {
        if (src && target && spell) {
            auto handle = src->GetHandle();
            if (handle)
            {
                target->GetMagicTarget()->DispelEffect(spell, handle);
            }
        }
    }
}