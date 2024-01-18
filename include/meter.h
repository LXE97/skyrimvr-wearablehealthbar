#pragma once
#include "wearable.h"

namespace wearable
{
	class Meter
	{
	public:
		static constexpr const char* node_name = "meter";
		// TODO: support survival mods
		enum class MeterValue
		{
			kNone = 0,
			kHealth,
			kMagicka,
			kStamina,
			kAmmo,
			kEnchantLeft,
			kEnchantRight,
			kShout,
			kStealth,
			kTime,
		};
		// o_O
		static constexpr const char* const meter_names[] = {
			"Disabled",
			"Health",
			"Magicka",
			"Stamina",
			"Ammo",
			"Charge(Left)",
			"Charge(Right)",
			"Shout",
			"Stealth",
			"Time",
		};

		Meter(int a_selected, std::vector<MeterValue>& a_eligible) :
			value_selector(a_selected),
			eligible(a_eligible)
		{}
		virtual ~Meter() = default;

		virtual void Update(float a_ratio) = 0;

		std::vector<MeterValue>                     eligible;
		int                                         value_selector;
		RE::NiAVObject*                             node = nullptr;
		std::unordered_map<MeterValue, RE::NiColor> color_changes;
	};

	class LinearMeter : public Meter
	{
	public:
		void Update(float a_ratio) override;
	};

	class RadialMeter : public Meter
	{
	public:
		RadialMeter(int a_selected, std::vector<MeterValue>& a_eligible, float a_max, float a_min) :
			Meter(a_selected, a_eligible),
			max_angle_deg(a_max),
			min_angle_deg(a_min)
		{}

		void Update(float a_ratio) override;

		float max_angle_deg;
		float min_angle_deg;
	};

	/** One or more meters in a single model */
	class WearableMeter : public Wearable
	{
		friend WearableManager;

	public:
		WearableMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
			RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
			std::vector<const char*>* a_eligible_nodes);

	protected:
		WearableMeter(const WearableMeter&) = delete;
		WearableMeter(WearableMeter&&) = delete;
		WearableMeter& operator=(const WearableMeter&) = delete;
		WearableMeter& operator=(WearableMeter&&) = delete;

		std::vector<std::unique_ptr<Meter>> meters;
		int                                 item_selector;
		const char*                         GetFunctionName() override;
		void                                GetColor(RE::NiColor* a_out, float* a_glow) override;
		void                                SetColor(RE::NiColor a_color, float a_alpha) override;

	private:
		void Update() override;
		void CycleSubitems(bool a_inc) override;
		void CycleFunction(bool a_inc) override;
	};

	/** A single meter composed of separate identical models*/
	class CompoundMeter : public WearableMeter
	{
		friend WearableManager;

	public:
		CompoundMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
			RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
			std::vector<const char*>* a_eligible_nodes, std::vector<int> a_additional_model_attach);

	private:
		RE::NiAVObject* Get3D()
		{
			return additional_models.at(item_selector % additional_models.size())->Get3D();
		}
		void Update() override;
		void CycleFunction(bool a_inc) override;
		void SetColor(RE::NiColor a_color, float a_alpha) override;

		std::vector<art_addon::ArtAddonPtr> additional_models;
	};
}