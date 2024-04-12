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

		virtual void Update(int this_index) = 0;

		float GetMeterValuePercent();

		bool                    inverted = false;
		std::vector<MeterValue> eligible;
		unsigned int            value_selector = 0;
		RE::NiAVObject*         node = nullptr;
		RE::NiAVObject*         parent = nullptr;
		static constexpr float  ammo_mult = 2.f;

	protected:
		float meter_value = 0.f;
	};

	class LinearMeter : public Meter
	{
	public:
		LinearMeter(int a_selected, std::vector<MeterValue>& a_eligible) :
			Meter(a_selected, a_eligible)
		{}

		void Update(int this_index) override;
	};

	class RadialMeter : public Meter
	{
	public:
		RadialMeter(int a_selected, std::vector<MeterValue>& a_eligible, float a_max, float a_min) :
			Meter(a_selected, a_eligible),
			max_angle_deg(a_max),
			min_angle_deg(a_min)
		{}

		void Update(int this_index) override;

		float max_angle_deg;
		float min_angle_deg;
	};

	class EyeMeter : public RadialMeter
	{
	public:
		EyeMeter(int a_selected, std::vector<MeterValue>& a_eligible) :
			RadialMeter(a_selected, a_eligible, helper::deg2rad(50.f), 0.f)

		{
			inverted = false;
		}

		static constexpr const char* node_name_lower = "eye_lower";
		static constexpr const char* node_name_upper = "eye_upper";
		static constexpr const char* node_name_main = "Mara";

		RE::NiAVObject* upper_node = nullptr;
		RE::NiAVObject* lower_node = nullptr;

		void Update(int this_index) override;
	};

	/** One or more meters in a single model */
	class WearableMeter : public Wearable
	{
		friend WearableManager;

	public:
		WearableMeter(const char* a_model_path, RE::NiAVObject* a_attach_node,
			RE::NiTransform& a_local, RE::NiPoint3 a_overlap_offset,
			std::vector<const char*>* a_eligible_nodes);

		WearableMeter(const WearableMeter&) = delete;
		WearableMeter(WearableMeter&&) = delete;
		WearableMeter& operator=(const WearableMeter&) = delete;
		WearableMeter& operator=(WearableMeter&&) = delete;

		std::vector<std::unique_ptr<Meter>> meters;
		unsigned int                        item_selector = 0;
		const char*                         GetFunctionName() override;
		void                                GetColor(RE::NiColorA* a_out) override;
		void                                SetColor(RE::NiColorA a_color) override;

	private:
		void Update(int arg) override;
		void CycleSubitems(bool a_inc) override;
		void CycleFunction(bool a_inc) override;
	};

	/** A single wearable meter composed of separate identical models*/
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
		void Update(int arg) override;
		void CycleFunction(bool a_inc) override;
		void SetColor(RE::NiColorA a_color) override;

		std::vector<art_addon::ArtAddonPtr> additional_models;
	};

	static std::map<const char*, std::map<Meter::MeterValue, RE::NiColorA>> model_color_changes;

}