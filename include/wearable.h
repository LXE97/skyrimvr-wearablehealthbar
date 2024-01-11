#pragma once
#include "art_addon.h"
#include "higgsinterface001.h"
#include "mod_input.h"
#include "overlap_sphere.h"
#include "vrikinterface001.h"

#include <memory>

namespace wearable
{
	enum class ManagerState
	{
		kNone = 0,
		kIdle,
		kTranslate,
		kScale,
		kColor,
		kCycleAV
	};

	class Wearable;
	using WearablePtr = std::shared_ptr<Wearable>;
	class WearableManager
	{
		// TODO: read from file
		struct Settings
		{
			float           overlap_angle = 45.f;
			float           overlap_radius = 4.f;
			float           movement_scale = 1.0f;
			float           smoothing = 1.0f;
			float           reset_distance = 16.f;
			int             enter_config_tap_delay_min_ms = 100;
			int             enter_config_tap_delay_max_ms = 900;
			bool            block_mode_control_on_overlap = true;
			bool            allow_reattach = true;
			vr::EVRButtonId enter_config_mode = vr::k_EButton_Grip;
			vr::EVRButtonId translate = vr::k_EButton_Grip;
			vr::EVRButtonId scale = vr::k_EButton_SteamVR_Trigger;
			vr::EVRButtonId color = vr::k_EButton_A;
			vr::EVRButtonId cycle_AV = vr::k_EButton_ApplicationMenu;
			vr::EVRButtonId exit[4] = { vr::k_EButton_SteamVR_Trigger, vr::k_EButton_A,
				vr::k_EButton_ApplicationMenu, vr::k_EButton_Grip };
		};

	public:
		static WearableManager* GetSingleton()
		{
			static WearableManager singleton;
			return &singleton;
		}

		/** Must be called by main plugin */
		void Update();

		void TransitionState(ManagerState a_next_state);

		void Register(std::weak_ptr<Wearable> a_obj);

		Settings&          GetSettings() { return settings; }
		ManagerState const GetState() { return configmode_state; }
		void const         SetState(ManagerState a_next)
		{
			configmode_next_state = a_next;
		}

	private:
		static constexpr const char* kColorWheelModelPath = "colorwheel.nif";
		static constexpr float       kColorLeashDistance = 400.f;

		WearableManager();
		~WearableManager() = default;
		WearableManager(const WearableManager&) = delete;
		WearableManager(WearableManager&&) = delete;
		WearableManager& operator=(const WearableManager&) = delete;
		WearableManager& operator=(WearableManager&&) = delete;

		static bool EnterConfigModeControl(const vrinput::ModInputEvent& e);
		static bool ExitConfigModeControl(const vrinput::ModInputEvent& e);
		static bool PositionControl(const vrinput::ModInputEvent& e);
		static bool ScaleControl(const vrinput::ModInputEvent& e);
		static bool ColorControl(const vrinput::ModInputEvent& e);
		static bool CycleAVControl(const vrinput::ModInputEvent& e);
		static void OverlapHandler(const vrinput::OverlapEvent& e);

		std::weak_ptr<Wearable> FindWearableWithOverlapID(int a_id);

		/* if arg is nullptr, removes the selection shader */
		void ApplySelectionShader(RE::NiAVObject* a_target = nullptr);

		void RegisterconfigModeButtons(bool a_unregister_register);

		void CaptureInitialTransforms();

		void                               ShowEligibleSkeleton(WearablePtr a_selected);
		std::weak_ptr<art_addon::ArtAddon> FindClosestSkeleton(RE::NiAVObject* a_selected);

		std::vector<std::weak_ptr<Wearable>> managed_objects;
		std::mutex                           managed_objects_lock;
		Settings                             settings;
		RE::TESEffectShader*                 shader = nullptr;
		RE::NiColor                          bone_on;
		RE::NiColor                          bone_off;
		std::atomic<ManagerState>            configmode_state = ManagerState::kNone;
		ManagerState                         configmode_next_state = ManagerState::kNone;
		RE::NiTransform                      configmode_wearable_initial_world;
		RE::NiTransform                      configmode_wearable_initial_local;
		RE::NiTransform                      configmode_hand_initial;
		std::weak_ptr<Wearable>              configmode_selected_item;
		int                                  configmode_hovered_sphere_id = 0;
		vrinput::Hand                        configmode_active_hand;
		std::vector<art_addon::ArtAddonPtr>  configmode_skeletons;
		art_addon::ArtAddonPtr               configmode_colorwheel;
		art_addon::ArtAddonPtr               testt;
	};

	/** Represents a model attached to the player body with an accompanying interaction sphere
	 * allowing it to be interactively repositioned. */
	class Wearable : public std::enable_shared_from_this<Wearable>
	{
		friend WearableManager;

	public:
		virtual ~Wearable() = default;
		Wearable(const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local,
			RE::NiPoint3 a_overlap_offset, vrinput::Hand a_active_hand = vrinput::Hand::kBoth,
			std::vector<const char*>* a_eligible_nodes = nullptr) :
			model_path(a_model_path),
			attach_node(a_attach_node),
			default_local(a_local),
			active_hand(a_active_hand),
			overlap_offset(a_overlap_offset),
			eligible_nodes(*a_eligible_nodes)

		{}

	protected:
		virtual void Update() = 0;

		art_addon::ArtAddonPtr    model = nullptr;
		vrinput::OverlapSpherePtr interactable = nullptr;

		RE::NiAVObject*          attach_node;
		RE::NiTransform          default_local;
		const char*              model_path;
		std::vector<const char*> eligible_nodes;
		vrinput::Hand            active_hand;
		RE::NiPoint3             overlap_offset;
		RE::NiPoint3             overlap_normal = { 1, 0, 0 };
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
			RE::NiPoint3 a_overlap_offset, std::vector<MeterType> a_meter_types,
			std::vector<std::string>  a_meternode_names,
			std::vector<const char*>* a_eligible_nodes = nullptr);

		void SetMeterValue(float a_ratio);

	protected:
		Meter(const Meter&) = delete;
		Meter(Meter&&) = delete;
		Meter& operator=(const Meter&) = delete;
		Meter& operator=(Meter&&) = delete;

		std::vector<MeterType>                     eligible_types;
		std::vector<std::string>                   meternode_names;
		std::unordered_map<MeterType, RE::NiColor> color_changes;

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