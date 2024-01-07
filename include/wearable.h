#pragma once
#include "art_addon.h"
#include "higgsinterface001.h"
#include "mod_input.h"
#include "overlap_sphere.h"
#include "vrikinterface001.h"

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
		// TODO: read from file
		struct Config
		{
			float           overlap_angle = 45.f;
			float           overlap_radius = 4.f;
			float           movement_scale = 1.0f;
			float           smoothing = 1.0f;
			int             enter_dressing_tap_delay_min_ms = 100;
			int             enter_dressing_tap_delay_max_ms = 900;
			bool            block_mode_control_on_overlap = true;
			vr::EVRButtonId enter_dressing_mode = vr::k_EButton_Grip;
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

		void TransitionState(DressingState a_next_state);

		void Register(std::weak_ptr<Wearable> a_obj);

		Config&             GetConfig() { return config; }
		DressingState const GetState() { return dressingmode_state; }
		void const          SetState(DressingState a_next) { dressingmode_next_state = a_next; }

	private:
		WearableManager();
		~WearableManager() = default;
		WearableManager(const WearableManager&) = delete;
		WearableManager(WearableManager&&) = delete;
		WearableManager& operator=(const WearableManager&) = delete;
		WearableManager& operator=(WearableManager&&) = delete;

		static bool EnterDressingModeControl(const vrinput::ModInputEvent& e);
		static bool ExitDressingModeControl(const vrinput::ModInputEvent& e);
		static bool TranslateControl(const vrinput::ModInputEvent& e);
		static bool ScaleControl(const vrinput::ModInputEvent& e);
		static bool ColorControl(const vrinput::ModInputEvent& e);
		static bool CycleAVControl(const vrinput::ModInputEvent& e);

		static void OverlapHandler(const vrinput::OverlapEvent& e);

		std::weak_ptr<Wearable> FindWearableWithOverlapID(int a_id);

		/* if arg is nullptr, removes the selection shader */
		void ApplySelectionShader(RE::NiAVObject* a_target = nullptr);

		void RegisterDressingModeButtons(bool a_unregister_register);

		void CaptureInitialTransforms();

		std::vector<std::weak_ptr<Wearable>> managed_objects;
		std::mutex                           managed_objects_lock;
		Config                               config;
		RE::TESEffectShader*                 shader = nullptr;
		DressingState                        dressingmode_state = DressingState::kNone;
		DressingState                        dressingmode_next_state = DressingState::kNone;
		RE::NiTransform                      dressingmode_wearable_initial;
		RE::NiTransform                      dressingmode_hand_initial;
		std::weak_ptr<Wearable>              dressingmode_selected_item;
		int                                  dressingmode_hovered_sphere_id = 0;
		vrinput::Hand                        dressingmode_active_hand;
	};

	/** Represents a model attached to the player body with an accompanying interaction sphere
	 * allowing it to be interactively repositioned. */
	class Wearable : public std::enable_shared_from_this<Wearable>
	{
		friend WearableManager;

	public:
		virtual ~Wearable() = default;
		Wearable(const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local,
			vrinput::Hand a_active_hand = vrinput::Hand::kBoth) :
			model_path(a_model_path),
			attach_node(a_attach_node),
			local(a_local),
			active_hand(a_active_hand)
		{}

	public:
		virtual void Update() = 0;

		art_addon::ArtAddonPtr    model = nullptr;
		vrinput::OverlapSpherePtr interactable = nullptr;

		RE::NiAVObject* attach_node;
		RE::NiTransform local;
		const char*     model_path;
		vrinput::Hand   active_hand;
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