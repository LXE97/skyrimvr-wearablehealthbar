#pragma once

#include "wearable.h"

namespace wearable
{

	struct HolsterSettings
	{
		const char*              model_path = "wearable/HelperSphereAxis.nif";
		RE::NiAVObject*          attach_node = nullptr;
		std::vector<const char*> eligible_parents;
		RE::NiTransform          local;
		RE::NiPoint3             overlap_offset;
	};

	/* abstract for laying out basic holster functions
    * TODO: is this useful?
    */
	class Holster : public Wearable
	{
		friend WearableManager;

	public:
		Holster(HolsterSettings& a_settings) :
			Wearable(a_settings.model_path, a_settings.attach_node, a_settings.local,
				a_settings.overlap_offset, vrinput::Hand::kBoth, &(a_settings.eligible_parents)){};

		Holster(const Holster&) = delete;
		Holster(Holster&&) = delete;
		Holster& operator=(const Holster&) = delete;
		Holster& operator=(Holster&&) = delete;

		const char* GetFunctionName() override { return ""; };

	private:
		void Update(int arg) override{};
	};

	/* Manages its own list of selected ammo types.
    * Interaction: equip the selected ammo
    */
	class Quiver : public Holster
	{
		friend WearableManager;

	public:
		Quiver(HolsterSettings& a_settings) : Holster(a_settings){};

	private:
		void OverlapHandler(const vrinput::OverlapEvent& e) override;

		RE::TESAmmo* selected_ammo = nullptr;
	};

	/* Interaction: drop the selected ammo, then grab it with HIGGS
    */
	class BoltQuiver : public Holster
	{
		friend WearableManager;

	public:
		BoltQuiver(HolsterSettings& a_settings) : Holster(a_settings){};

	private:
		void OverlapHandler(const vrinput::OverlapEvent& e) override;

		// TODO this is annoying, figure out if I can use non static methods
		static bool EquipButtonHandler(const vrinput::ModInputEvent& e);

		// might not be so bad actually
		void EquipButtonHandler_impl(const vrinput::ModInputEvent& e);

		vr::EVRButtonId interact_button = vr::k_EButton_SteamVR_Trigger;
		RE::TESAmmo*    selected_ammo = nullptr;
	};
}