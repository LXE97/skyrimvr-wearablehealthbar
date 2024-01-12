#include "helper_game.h"

#include "helper_math.h"

namespace helper
{
	using namespace RE;

	TESForm* LookupByName(FormType a_typeEnum, const char* a_name)
	{
		auto* data = TESDataHandler::GetSingleton();
		auto& forms = data->GetFormArray(a_typeEnum);
		for (auto*& form : forms)
		{
			if (!strcmp(form->GetName(), a_name)) { return form; }
		}
		return nullptr;
	}

	RE::FormID GetFullFormID(uint8_t a_modindex, RE::FormID a_localID)
	{
		return (a_modindex << 24) | a_localID;
	}

	float GetAVPercent(Actor* a_a, ActorValue a_v)
	{
		float current = a_a->AsActorValueOwner()->GetActorValue(a_v);
		float base = a_a->AsActorValueOwner()->GetBaseActorValue(a_v);
		float mod = a_a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kPermanent, a_v) +
			a_a->GetActorValueModifier(ACTOR_VALUE_MODIFIER::kTemporary, a_v);
		return current / (base + mod);
	}

	float GetChargePercent(Actor* a_a, bool isLeft)
	{
		if (auto equipped = a_a->GetEquippedObject(isLeft))
		{
			if (equipped->IsWeapon())
			{
				float current =
					a_a->AsActorValueOwner()->GetActorValue(ActorValue::kRightItemCharge);

				// player made items
				if (auto entryData = a_a->GetEquippedEntryData(isLeft))
				{
					for (auto& x : *(entryData->extraLists))
					{
						if (auto e = x->GetByType<ExtraEnchantment>())
						{
							return std::clamp(current / e->charge, 0.f, 100.f);
						}
					}
				}

				// prefab items
				if (auto ench = equipped->As<TESEnchantableForm>())
				{
					return std::clamp(current / (float)(ench->amountofEnchantment), 0.f, 100.f);
				}
			}
		}
		return 0;
	}

	float GetGameHour()
	{
		if (auto c = Calendar::GetSingleton()) { return c->GetHour(); }
		return 0;
	}

	float GetAmmoPercent(Actor* a_a, float a_ammoCountMult)
	{
		if (auto ammo = a_a->GetCurrentAmmo())
		{
			auto countmap = a_a->GetInventoryCounts();
			if (countmap[ammo]) { return std::clamp(countmap[ammo] * a_ammoCountMult, 0.f, 100.f); }
		}
		return 0;
	}

	float GetShoutCooldownPercent(Actor* a_a, float a_MaxCDTime)
	{
		return std::clamp(a_a->GetVoiceRecoveryTime() / a_MaxCDTime, 0.f, 100.f);
	}

	void SetGlowMult() {}

	void SetGlowColor(NiAVObject* a_target, NiColor* c)
	{
		if (a_target)
		{
			if (auto geometry =
					a_target->GetFirstGeometryOfShaderType(RE::BSShaderMaterial::Feature::kGlowMap))
			{
				if (auto shaderProp = geometry->GetGeometryRuntimeData()
										  .properties[RE::BSGeometry::States::kEffect]
										  .get())
				{
					if (auto shader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp))
					{
						shader->emissiveColor = c;
					}
				}
			}
		}
	}

	void SetUVCoords(NiAVObject* a_target, float a_x, float a_y)
	{
		if (a_target)
		{
			if (auto geometry =
					a_target->GetFirstGeometryOfShaderType(BSShaderMaterial::Feature::kNone))
			{
				if (auto shaderProp = geometry->GetGeometryRuntimeData()
										  .properties[RE::BSGeometry::States::kEffect]
										  .get())
				{
					if (auto shader = netimmerse_cast<RE::BSShaderProperty*>(shaderProp))
					{
						shader->material->texCoordOffset[0].x = a_x;
						shader->material->texCoordOffset[0].y = a_y;
						shader->material->texCoordOffset[1].x = a_x;
						shader->material->texCoordOffset[1].y = a_y;
					}
				}
			}
		}
	}

	void SetSpecularMult() {}
	void SetSpecularColor() {}
	void SetTintColor() {}

	void CastSpellInstant(Actor* src, Actor* a_target, SpellItem* a_spell)
	{
		if (src && a_target && a_spell)
		{
			auto caster = src->GetMagicCaster(MagicSystem::CastingSource::kInstant);
			if (caster)
			{
				caster->CastSpellImmediate(a_spell, false, a_target, 1.0, false, 1.0, src);
			}
		}
	}

	void Dispel(Actor* src, Actor* a_target, SpellItem* a_spell)
	{
		if (src && a_target && a_spell)
		{
			auto handle = src->GetHandle();
			if (handle) { a_target->GetMagicTarget()->DispelEffect(a_spell, handle); }
		}
	}

	void PrintPlayerModelEffects()
	{
		if (const auto processLists = RE::ProcessLists::GetSingleton())
		{
			int player = 0;
			int dangling = 0;
			processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect) {
				if (a_modelEffect.target.get()->AsReference() ==
					RE::PlayerCharacter::GetSingleton()->AsReference())
				{
					if (a_modelEffect.artObject)
					{
						SKSE::log::debug(
							"MRE:{}  AO:{}", (void*)&a_modelEffect, (void*)a_modelEffect.artObject);
						player++;
					}
					else { dangling++; }
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			SKSE::log::debug("{} player effects and {} dangling MRE", player, dangling);
		}
	}

	void PrintPlayerShaderEffects()
	{
		if (const auto processLists = RE::ProcessLists::GetSingleton())
		{
			int player = 0;
			int dangling = 0;
			processLists->ForEachShaderEffect([&](RE::ShaderReferenceEffect& a_shaderEffect) {
				if (a_shaderEffect.target.get()->AsReference() ==
					RE::PlayerCharacter::GetSingleton()->AsReference())
				{
					if (a_shaderEffect.effectData)
					{
						SKSE::log::debug("SRE:{}  AO:{}", (void*)&a_shaderEffect,
							(void*)a_shaderEffect.effectData);
						player++;
					}
					else { dangling++; }
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
			SKSE::log::debug("{} player effects and {} dangling SRE", player, dangling);
		}
	}
}