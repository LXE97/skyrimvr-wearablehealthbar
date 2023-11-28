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
        float current = a->AsActorValueOwner()->GetActorValue(ActorValue::kHealth);
        float base = a->AsActorValueOwner()->GetBaseActorValue(ActorValue::kHealth);
        float mod = a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kPermanent, ActorValue::kHealth) +
            a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kTemporary, ActorValue::kHealth);
        return current / (base + mod);
    }

    float GetMagickaPercent(Actor* a)
    {
        float current = a->AsActorValueOwner()->GetActorValue(ActorValue::kMagicka);
        float base = a->AsActorValueOwner()->GetBaseActorValue(ActorValue::kMagicka);
        float mod = a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kPermanent, ActorValue::kMagicka) +
            a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kTemporary, ActorValue::kMagicka);
        return current / (base + mod);
    }

    float GetStaminaPercent(Actor* a)
    {
        float current = a->AsActorValueOwner()->GetActorValue(ActorValue::kStamina);
        float base = a->AsActorValueOwner()->GetBaseActorValue(ActorValue::kStamina);
        float mod = a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kPermanent, ActorValue::kStamina) +
            a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kTemporary, ActorValue::kHealth);
        return current / (base + mod);
    }
    
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