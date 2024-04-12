#include "meter.h"

#include "hooks.h"

namespace wearable
{
	using namespace art_addon;
	using namespace RE;

	WearableMeter::WearableMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
		RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
		std::vector<const char*>* a_eligible_nodes) :
		Wearable(a_model_path, a_attach_node, a_local, a_overlap_offset, vrinput::Hand::kBoth,
			a_eligible_nodes)

	{}

	const char* WearableMeter::GetFunctionName()
	{
		int name_index = 0;
		if (!meters.empty())
		{
			auto& meter = meters.at(item_selector % meters.size());
			name_index = (int)meter->eligible.at(meter->value_selector);
		}

		return Meter::meter_names[name_index];
	}

	void WearableMeter::GetColor(RE::NiColorA* a_out)
	{
		if (model && !meters.empty())
		{
			if (auto node = meters[item_selector % meters.size()]->node)
			{
				if (auto shader = helper::GetShaderProperty(node, nullptr))
				{
					if (auto property = static_cast<RE::BSLightingShaderProperty*>(shader))
					{
						*a_out = *(property->emissiveColor);
						(*a_out).alpha = property->emissiveMult;
					} /* skin tint did not work out - only using glow color for now
					if (auto material =
							static_cast<RE::BSLightingShaderMaterialFacegenTint*>(shader->material))
					{
						*a_out = material->tintColor;
					}*/
				}
			}
		}
	}

	void WearableMeter::SetColor(RE::NiColorA a_color)
	{
		if (model && !meters.empty())
		{
			auto index = item_selector % meters.size();
			if (auto node = meters[index]->node)
			{
				if (auto shader = helper::GetShaderProperty(node, nullptr))
				{
					if (auto property = static_cast<RE::BSLightingShaderProperty*>(shader))
					{
						property->emissiveMult = a_color.alpha;
						*(property->emissiveColor) = a_color;

						// save the chosen color for this model / actor value combination
						Meter::MeterValue selected_value =
							meters[index]->eligible[meters[index]->value_selector];
						model_color_changes[model_path][selected_value] = a_color;
					}
				}
			}
		}
	}

	void WearableMeter::Update(int arg)
	{
		// Normally the argument to Update() just tells the meter which index it has
		// so that it can select the appropriate node. But I needed a way to tell the meter
		// that configuration mode is active e.g. for the eye meter to open fully
		if (arg < 0)
		{
			for (int i = 0; i < meters.size(); i++) { meters[i]->Update(-1); }
		}
		else
		{
			for (int i = 0; i < meters.size(); i++) { meters[i]->Update(i); }
		}
	}

	void WearableMeter::CycleSubitems(bool a_inc) { item_selector += a_inc ? 1 : -1; }

	void WearableMeter::CycleFunction(bool a_inc)
	{
		if (!meters.empty())
		{
			auto selected = item_selector % meters.size();
			meters[selected]->value_selector += a_inc ? 1 : -1;
			meters[selected]->value_selector %= meters[selected]->eligible.size();

			SetColor(
				model_color_changes[model_path]
								   [meters[selected]->eligible[meters[selected]->value_selector]]);
		}
	}

	CompoundMeter::CompoundMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
		RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
		std::vector<const char*>* a_eligible_nodes, std::vector<int> a_additional_model_attach) :
		WearableMeter(a_model_path, a_attach_node, a_local, a_overlap_offset, a_eligible_nodes)
	{}

	void CompoundMeter::Update(int arg) {}

	void CompoundMeter::CycleFunction(bool a_inc) {}

	void CompoundMeter::SetColor(RE::NiColorA a_color) {}

	void LinearMeter::Update(int this_index)
	{
		if (!node)
		{
			node = PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName(
				node_name + std::to_string(this_index + 1));
		}
		else
		{
			auto cur_val = std::clamp(GetMeterValuePercent(), 0.f, 1.f);
			if (meter_value != cur_val)
			{
				meter_value = cur_val;
				cur_val = 1.f - cur_val;
				// set the texture x UVs. 0.0 = 100%, 1.0 = 0%
				if (auto shader = helper::GetShaderProperty(node, nullptr))
				{
					shader->material->texCoordOffset[0].x = cur_val;
					shader->material->texCoordOffset[1].x = cur_val;
				}
			}
		}
	}

	void RadialMeter::Update(int this_index) {}

	void setPitch(float a_y, RE::NiMatrix3& t)
	{
		t.entry[0][0] = cosf(a_y);
		t.entry[0][2] = sinf(a_y);
		t.entry[2][0] = t.entry[0][2] * -1;
		t.entry[2][2] = t.entry[0][0];
	}

	void EyeMeter::Update(int this_index)
	{
		if (!node)
		{
			node = PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName(node_name_main);
			upper_node =
				PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName(node_name_upper);
			lower_node =
				PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName(node_name_lower);
		}
		else
		{
			if (this_index > 0)
			{
				constexpr float max_rad_per_frame = 0.06f;
				auto            cur_val = std::clamp(GetMeterValuePercent(), 0.f, 1.f);

				if (abs(meter_value - cur_val) > max_rad_per_frame)
				{
					// limit difference per frame
					if (meter_value > cur_val) { cur_val = meter_value - max_rad_per_frame; }
					else { cur_val = meter_value + max_rad_per_frame; }

					// update buffer value
					meter_value = cur_val;

					if (inverted) { cur_val = 1.f - cur_val; }

					// scale the rotation
					cur_val *= max_angle_deg;

					setPitch(cur_val, upper_node->local.rotate);
					setPitch(-1 * cur_val, lower_node->local.rotate);
				}
			}
			else  // config mode, full open
			{
				setPitch(max_angle_deg, upper_node->local.rotate);
				setPitch(-1 * max_angle_deg, lower_node->local.rotate);
			}
		}
	}

	float Meter::GetMeterValuePercent()
	{
		switch (eligible[value_selector])
		{
		case MeterValue::kHealth:
			return helper::GetAVPercent(PlayerCharacter::GetSingleton(), ActorValue::kHealth);
			break;
		case MeterValue::kMagicka:
			return helper::GetAVPercent(PlayerCharacter::GetSingleton(), ActorValue::kMagicka);
			break;
		case MeterValue::kStamina:
			return helper::GetAVPercent(PlayerCharacter::GetSingleton(), ActorValue::kStamina);
			break;
		case MeterValue::kAmmo:
			return helper::GetAmmoPercent(PlayerCharacter::GetSingleton(), ammo_mult);
			break;
		case MeterValue::kEnchantLeft:
			return helper::GetChargePercent(PlayerCharacter::GetSingleton(), true);
			break;
		case MeterValue::kEnchantRight:
			return helper::GetChargePercent(PlayerCharacter::GetSingleton(), true);
			break;
		case MeterValue::kShout:
			return helper::GetShoutCooldownPercent(PlayerCharacter::GetSingleton(), 30);
			break;
		case MeterValue::kStealth:
			return hooks::g_detection_level;
			break;
		case MeterValue::kTime:
			return helper::GetGameHour() / 24.f;
			break;
		default:
			break;
		}
		return 0.f;
	}
}
