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
		kHolding,
		kIdle,
		kTranslate,
		kScale,
		kColor,
		kCycleFunctions
	};

	class Wearable;
	using WearablePtr = std::shared_ptr<Wearable>;

	class WearableManager
	{
		friend Wearable;
		// TODO: read from file
		struct Settings
		{
			float           overlap_angle = 45.f;
			float           overlap_radius = 8.f;
			float           movement_scale = 1.0f;
			float           smoothing = 1.0f;
			float           reset_distance = 20.f;
			int             enter_config_tap_delay_min_ms = 100;
			int             enter_config_tap_delay_max_ms = 900;
			int 			update_frames = 10;
			bool            block_mode_control_on_overlap = true;
			bool            allow_reattach = true;
			vr::EVRButtonId enter_config_mode = vr::k_EButton_Grip;
			vr::EVRButtonId translate = vr::k_EButton_Grip;
			vr::EVRButtonId scale = vr::k_EButton_ApplicationMenu;
			vr::EVRButtonId color = vr::k_EButton_A;
			vr::EVRButtonId cycle_AV = vr::k_EButton_SteamVR_Trigger;
			vr::EVRButtonId exiting_buttons[4] = { vr::k_EButton_SteamVR_Trigger, vr::k_EButton_A,
				vr::k_EButton_ApplicationMenu, vr::k_EButton_Grip };
		};

	public:
		static constexpr const char* kColorWheelModelPath = "Wearable/colorwheel.nif";
		static constexpr const char* kcolor_name = "cursor_color";
		static constexpr const char* kvalue_name = "cursor_value";
		static constexpr const char* kglow_name = "cursor_glow";
		static constexpr float       kMaxGlow = 10.0f;
		static constexpr int         kBoneOffHex = 0x3dff4a;
		static constexpr int         kBoneOnHex = 0xff260e;

		static WearableManager* GetSingleton()
		{
			static WearableManager singleton;
			return &singleton;
		}

		/** Must be called by main plugin */
		void Update();

		Settings& GetSettings() { return settings; }

		ManagerState const GetState() { return configmode_state; }
		void const         SetState(ManagerState a_next) { configmode_next_state = a_next; }
		std::weak_ptr<Wearable> const GetSelectedItem() { return configmode_selected_item; }

		/** this is only public so that the plugin can shut down config mode on game save event */
		void StateTransition(ManagerState a_next_state);
		void Register(std::weak_ptr<Wearable> a_obj);

	private:
		static constexpr float kColorLeashDistance = 400.f;

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
		static bool CycleFunctionControl(const vrinput::ModInputEvent& e);
		static bool LeftStick(const vrinput::ModInputEvent& e);
		static bool RightStick(const vrinput::ModInputEvent& e);
		void        CycleSelection(bool increment);
		static void OverlapHandler(const vrinput::OverlapEvent& e);

		std::weak_ptr<Wearable> FindWearableWithOverlapID(int a_id);

		/* if arg is nullptr, removes the selection shader */
		void ApplySelectionShader(RE::NiAVObject* a_target = nullptr);

		void RegisterConfigModeButtons(bool a_unregister_register);

		void CaptureInitialTransforms();

		int ShowEligibleBones(WearablePtr a_selected);

		std::vector<std::weak_ptr<Wearable>> managed_objects;
		std::mutex                           managed_objects_lock;
		Settings                             settings;
		RE::TESEffectShader*                 shader = nullptr;
		std::atomic<ManagerState>            configmode_state = ManagerState::kNone;
		ManagerState                         configmode_next_state = ManagerState::kNone;
		RE::NiTransform                      configmode_wearable_initial_world;
		RE::NiTransform                      configmode_wearable_initial_local;
		RE::NiTransform                      configmode_hand_initial;
		std::weak_ptr<Wearable>              configmode_selected_item;
		int                                  configmode_selected_subitem_index;
		int                                  configmode_hovered_sphere_id = 0;
		vrinput::Hand                        configmode_active_hand;
		std::vector<art_addon::ArtAddonPtr>  configmode_visible_bones;
		int                                  configmode_parentbone_index;
		art_addon::ArtAddonPtr               configmode_colorwheel;
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
			eligible_attach_nodes(*a_eligible_nodes)
		{
			model = art_addon::ArtAddon::Make(a_model_path,
				RE::PlayerCharacter::GetSingleton()->AsReference(), a_attach_node, a_local);
		}

	protected:
		virtual void            Update(int arg) = 0;
		virtual RE::NiAVObject* Get3D() { return model ? model->Get3D() : nullptr; }


		virtual void        CycleSubitems(bool a_inc) {}
		virtual void        CycleFunction(bool a_inc) {}
		virtual void        SetColor(RE::NiColorA a_color) {}
		virtual void        GetColor(RE::NiColorA* out) {}
		virtual const char* GetFunctionName() = 0;

		art_addon::ArtAddonPtr    model = nullptr;
		vrinput::OverlapSpherePtr interactable = nullptr;

		RE::NiAVObject*                          attach_node;
		RE::NiTransform                          default_local;
		const char*                              model_path;
		std::vector<const char*>                 eligible_attach_nodes;
		vrinput::Hand                            active_hand;
		RE::NiPoint3                             overlap_offset;
		RE::NiPoint3                             overlap_normal = { 1, 0, 0 };
		std::unique_ptr<art_addon::AddonTextBox> configmode_function_text;
	};
}