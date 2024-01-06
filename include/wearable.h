#pragma once
#include "art_addon.h"
#include "mod_input.h"
#include "overlap_sphere.h"

#include <memory>

namespace wearable
{
	enum class DressingState
	{
		kNone = 0,
		kWait,
		kTranslate,
		kScale,
		kColor,
		kCycleAV
	};

	class Wearable;
	using WearablePtr = std::shared_ptr<Wearable>;
	class WearableManager
	{
		struct Config
		{
			float           overlap_angle = 0.f;
			float           overlap_radius = 3.f;
			float           movement_scale = 1.0f;
			float           smoothing = 1.0f;
			float           enter_dressing_tap_delay_min = 0.1f;
			float           enter_dressing_tap_delay_max = 0.7f;
			vr::EVRButtonId enter_dressing_mode = vr::k_EButton_Grip;
			vr::EVRButtonId translate = vr::k_EButton_Grip;
			vr::EVRButtonId scale = vr::k_EButton_SteamVR_Trigger;
			vr::EVRButtonId color = vr::k_EButton_A;
			vr::EVRButtonId cycle_AV = vr::k_EButton_ApplicationMenu;
		};

	public:
		static WearableManager* GetSingleton()
		{
			static WearableManager singleton;
			return &singleton;
		}

		/** Must be called by main plugin */
		void Update();

		void TransitionState(DressingState a_next_state);

		void Register(std::weak_ptr<Wearable> a_obj);

		Config&             GetConfig() { return config; }
		DressingState const GetState() { return dressingmode_state; }
		int                 FindWearableWithOverlapID(int a_id);

	private:
		static constexpr const RE::FormID shader_id = 0x6F00;
		static constexpr const char*      higgs_esp = "higgs_vr.esp";

		static bool WearableGrabHandler();
		static void WearableOverlapHandler(const vrinput::OverlapEvent& e);

		std::vector<std::weak_ptr<Wearable>> managed_objects;
		std::mutex                           managed_objects_lock;
		Config                               config;
		RE::TESEffectShader*                 shader;
		DressingState                        dressingmode_state = DressingState::kNone;
		RE::NiTransform                      dressingmode_initial;
		bool                                 dressingmode_active_isLeft = false;
		int                                  dressingmode_selected_index = 0;
		int                                  dressingmode_hovered_sphere_id = 0;
	};

	/** Represents a model attached to the player body with an accompanying interaction sphere
	 * allowing it to be interactively repositioned. */
	class Wearable : public std::enable_shared_from_this<Wearable>
	{
		friend WearableManager;

	public:
		virtual ~Wearable() = default;
		Wearable(
			const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local) :
			model_path(a_model_path),
			attach_node(a_attach_node),
			local(a_local)
		{}

	protected:
		virtual void Update() = 0;

		art_addon::ArtAddonPtr    model = nullptr;
		vrinput::OverlapSpherePtr interactable = nullptr;

		RE::NiAVObject* attach_node;
		RE::NiTransform local;
		const char*     model_path;
	};

	/** One or more meters in a single model */
	class Meter : public Wearable
	{
		friend WearableManager;

	public:
		enum class MeterType
		{
			kHealth = 0,
			kMagicka,
			kStamina,
			kAmmo,
			kEnchantLeft,
			kEnchantRight,
			kShout,
			kStealth,
			kTime
		};

		Meter(const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local,
			std::vector<MeterType> a_meter_types, std::vector<std::string> a_node_names);

	protected:
		Meter(const Meter&) = delete;
		Meter(Meter&&) = delete;
		Meter& operator=(const Meter&) = delete;
		Meter& operator=(Meter&&) = delete;

		std::vector<MeterType>   types;
		std::vector<std::string> node_names;

	private:
		void Update();
	};

	/** A single meter composed of multiple identical models*/
	class CompoundMeter : public Meter
	{
		friend WearableManager;

	public:
	private:
		void Update();

		std::vector<WearablePtr> meters;
	};
}