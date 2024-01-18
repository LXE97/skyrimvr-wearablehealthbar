#include "meter.h"

namespace wearable
{
	using namespace art_addon;
	using namespace RE;

	WearableMeter::WearableMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
		RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
		std::vector<const char*>* a_eligible_nodes) :
		Wearable(a_model_path, a_attach_node, a_local, a_overlap_offset, vrinput::Hand::kBoth,
			a_eligible_nodes)

	{
		
	}

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

	void WearableMeter::GetColor(RE::NiColor* a_out, float* a_glow)
	{
		if (model && !meters.empty())
		{
			if (auto node = meters[item_selector % meters.size()]->node)
			{
				if (auto shader = helper::GetShaderProperty(node, nullptr))
				{
					if (auto property = static_cast<RE::BSLightingShaderProperty*>(shader))
					{
						*a_glow = property->emissiveMult;
					}
					if (auto material =
							static_cast<RE::BSLightingShaderMaterialFacegenTint*>(shader->material))
					{
						*a_out = material->tintColor;
					}
				}
			}
		}
	}

	void WearableMeter::SetColor(RE::NiColor a_color, float a_alpha)
	{
		auto node = RE::PlayerCharacter::GetSingleton()->GetNodeByName("meter1");
		helper::GetVertexData(node);
		if (model && !meters.empty())
		{
			if (auto node = meters[item_selector % meters.size()]->node)
			{
				

				if (auto shader = helper::GetShaderProperty(node, nullptr))
				{
					if (auto property = static_cast<RE::BSLightingShaderProperty*>(shader))
					{
						property->emissiveMult = a_alpha;
					}
					if (auto material =
							static_cast<RE::BSLightingShaderMaterialFacegenTint*>(shader->material))
					{
						material->tintColor = a_color;
					}
				}
			}
		}
	}

	void WearableMeter::Update() {}

	void WearableMeter::CycleSubitems(bool a_inc) {}

	void WearableMeter::CycleFunction(bool a_inc) {}

	CompoundMeter::CompoundMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
		RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
		std::vector<const char*>* a_eligible_nodes, std::vector<int> a_additional_model_attach) :
		WearableMeter(a_model_path, a_attach_node, a_local, a_overlap_offset, a_eligible_nodes)
	{}

	void CompoundMeter::Update() {}

	void CompoundMeter::CycleFunction(bool a_inc) {}

	void CompoundMeter::SetColor(RE::NiColor a_color, float a_alpha) {}
}
