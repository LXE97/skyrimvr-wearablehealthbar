#include "wearable.h"

namespace wearable
{
	using namespace RE;
	using namespace art_addon;

	bool WearableManager::EnterDressingModeControl(const vrinput::ModInputEvent& e)
	{
		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			auto mgr = WearableManager::GetSingleton();

			// primary function: enter dressing mode
			if (mgr->GetState() == DressingState::kNone)
			{
				static auto last_tap_time = std::chrono::steady_clock::now();

				auto tap_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - last_tap_time);

				if (tap_diff.count() < mgr->config.enter_dressing_tap_delay_max_ms &&
					tap_diff.count() > mgr->config.enter_dressing_tap_delay_min_ms)
				{
					mgr->dressingmode_selected_item =
						mgr->FindWearableWithOverlapID(mgr->dressingmode_hovered_sphere_id);

					if (auto sp = mgr->dressingmode_selected_item.lock())
					{
						mgr->dressingmode_active_hand = e.device;
						mgr->ApplySelectionShader(sp->model->Get3D());
						mgr->SetState(DressingState::kWait);
					}
				}
				else { last_tap_time = std::chrono::steady_clock::now(); }
			}
			else
			{  // secondary function: change selected object
				mgr->dressingmode_selected_item =
					mgr->FindWearableWithOverlapID(mgr->dressingmode_hovered_sphere_id);

				if (auto sp = mgr->dressingmode_selected_item.lock())
				{
					mgr->ApplySelectionShader(sp->model->Get3D());
				}
			}
			return mgr->config.block_mode_control_on_overlap;
		}
		return false;
	}

	bool WearableManager::ExitDressingModeControl(const vrinput::ModInputEvent& e)
	{
		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			auto mgr = WearableManager::GetSingleton();
			mgr->SetState(DressingState::kNone);
		}
		return false;
	}

	bool WearableManager::TranslateControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			if (mgr->GetState() == DressingState::kWait)
			{
				mgr->SetState(DressingState::kTranslate);
			}
		}
		else
		{
			if (mgr->GetState() == DressingState::kTranslate)
			{
				mgr->SetState(DressingState::kWait);
			}
		}
		return false;
	}

	bool WearableManager::ScaleControl(const vrinput::ModInputEvent& e) { return false; }

	bool WearableManager::ColorControl(const vrinput::ModInputEvent& e) { return false; }

	bool WearableManager::CycleAVControl(const vrinput::ModInputEvent& e) { return false; }

	void WearableManager::OverlapHandler(const vrinput::OverlapEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();
		if (e.entered)
		{
			// Only change selected object when not in configuration mode or event was triggered
			// by the hand that activated configuration mode
			if (mgr->dressingmode_state == DressingState::kNone ||
				e.triggered_by == mgr->dressingmode_active_hand)
			{
				mgr->dressingmode_hovered_sphere_id = e.id;
			}

			vrinput::AddCallback(mgr->config.enter_dressing_mode, EnterDressingModeControl,
				e.triggered_by, vrinput::ActionType::kPress);
		}
		else
		{
			mgr->dressingmode_hovered_sphere_id = -1;
			vrinput::RemoveCallback(mgr->config.enter_dressing_mode, EnterDressingModeControl,
				e.triggered_by, vrinput::ActionType::kPress);
		}
	}

	void WearableManager::Update()
	{
		if (dressingmode_next_state != dressingmode_state)
		{
			TransitionState(dressingmode_next_state);
		}

		if (!managed_objects.empty())
		{
			std::scoped_lock lock(managed_objects_lock);
			if (dressingmode_state != DressingState::kNone &&
				dressingmode_state != DressingState::kWait)
			{
				if (auto sp = dressingmode_selected_item.lock())
				{
					if (auto node = sp->model->Get3D())
					{
						switch (dressingmode_state)
						{
						case DressingState::kTranslate:
							{
								NiUpdateData ctx;

								auto parent = node->parent->world.Invert();

								node->local.translate = parent.rotate *
									(dressingmode_wearable_initial.translate -
										node->parent->world.translate);

								node->local.rotate =
									parent.rotate * dressingmode_wearable_initial.rotate;

								node->Update(ctx);
							}
							break;
						case DressingState::kScale:
							break;
						case DressingState::kColor:
							break;
						case DressingState::kCycleAV:
							break;
						}
					}
				}
			}
			else
			{
				for (auto it = managed_objects.begin(); it != managed_objects.end();)
				{
					if (auto sp = it->lock())
					{
						if (sp->model->Get3D())
						{
							if (sp->interactable) { sp->Update(); }
							else
							{
								sp->interactable = vrinput::OverlapSphere::Make(sp->model->Get3D(),
									OverlapHandler, config.overlap_radius, config.overlap_angle,
									sp->active_hand);
							}
						}
						it++;
					}
					else { it = managed_objects.erase(it); }
				}
			}
		}
	}

	void WearableManager::TransitionState(DressingState a_next_state)
	{
		if (a_next_state != dressingmode_state)
		{
			// exit state event
			switch (dressingmode_state)
			{
			case DressingState::kNone:
				vrinput::StartBlockingAll();
				RegisterDressingModeButtons(true);
				if (g_vrikInterface) { g_vrikInterface->beginGestureProfile(); }
				if (g_higgsInterface)
				{
					g_higgsInterface->DisableHand(true);
					g_higgsInterface->DisableHand(false);
				}
				break;
			case DressingState::kWait:

				break;
			}

			// enter state event
			switch (a_next_state)
			{
			case DressingState::kNone:
				vrinput::StopBlockingAll();
				ApplySelectionShader();
				RegisterDressingModeButtons(false);
				if (g_vrikInterface) { g_vrikInterface->endGestureProfile(); }
				if (g_higgsInterface)
				{
					g_higgsInterface->EnableHand(true);
					g_higgsInterface->EnableHand(false);
				}
				break;
			case DressingState::kWait:
				break;
			case DressingState::kTranslate:
			case DressingState::kScale:
			case DressingState::kColor:
				CaptureInitialTransforms();
			}

			dressingmode_state = a_next_state;
		}
	}

	void WearableManager::Register(std::weak_ptr<Wearable> a_obj)
	{
		std::scoped_lock lock(managed_objects_lock);
		managed_objects.push_back(a_obj);
	}

	WearableManager::WearableManager()
	{
		static constexpr FormID shader_formID = 0xfe68c;
		if (auto temp = TESForm::LookupByID(shader_formID))
		{
			shader = temp->As<TESEffectShader>();
		}
	}

	std::weak_ptr<Wearable> WearableManager::FindWearableWithOverlapID(int a_id)
	{
		std::scoped_lock lock(managed_objects_lock);

		auto it =
			std::find_if(managed_objects.begin(), managed_objects.end(), [a_id](const auto& obj) {
				return obj.expired() ? false : obj.lock()->interactable->GetId() == a_id;
			});
		if (it != managed_objects.end()) { return *it; }
		return std::weak_ptr<Wearable>();
	}

	void WearableManager::ApplySelectionShader(RE::NiAVObject* a_target)
	{
		static constexpr float shader_ID = -1.142578125f;
		if (shader && a_target)
		{
			ApplySelectionShader(nullptr);
			PlayerCharacter::GetSingleton()->ApplyEffectShader(
				shader, shader_ID, nullptr, false, false, a_target);
		}
		else
		{
			if (const auto processLists = ProcessLists::GetSingleton())
			{
				processLists->ForEachShaderEffect([](ShaderReferenceEffect& a_shader_effect) {
					if (a_shader_effect.lifetime == shader_ID) { a_shader_effect.Detach(); }
					return BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	void WearableManager::RegisterDressingModeButtons(bool a_unregister_register)
	{
		if (a_unregister_register)
		{
			vrinput::AddCallback(config.translate, TranslateControl, dressingmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(
				config.scale, ScaleControl, dressingmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(
				config.color, ColorControl, dressingmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(config.cycle_AV, CycleAVControl, dressingmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : config.exit)
			{
				vrinput::AddCallback(button, ExitDressingModeControl,
					GetOtherHand(dressingmode_active_hand), vrinput::ActionType::kPress);
			}
		}
		else
		{
			vrinput::RemoveCallback(config.translate, TranslateControl, dressingmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				config.scale, ScaleControl, dressingmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				config.color, ColorControl, dressingmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(config.cycle_AV, CycleAVControl, dressingmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : config.exit)
			{
				vrinput::RemoveCallback(button, ExitDressingModeControl,
					GetOtherHand(dressingmode_active_hand), vrinput::ActionType::kPress);
			}
		}
	}

	void WearableManager::CaptureInitialTransforms()
	{
		if (auto sp = dressingmode_selected_item.lock())
		{
			if (auto target = sp->model->Get3D())
			{
				dressingmode_wearable_initial = target->world;
				dressingmode_hand_initial =
					*vrinput::OverlapSphereManager::GetSingleton()->GetCachedHand(
						(bool)dressingmode_active_hand);
			}
		}
	}

	Meter::Meter(const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local,
		std::vector<MeterType> a_meter_types, std::vector<std::string> a_node_names) :
		Wearable(a_model_path, a_attach_node, a_local),
		types(a_meter_types),
		node_names(a_node_names)

	{
		model = ArtAddon::Make(
			a_model_path, PlayerCharacter::GetSingleton()->AsReference(), a_attach_node, a_local);
		WearableManager::GetSingleton()->Register(weak_from_this());
	}

	void Meter::Update() {}

	void CompoundMeter::Update() {}

}  // namespace wearable
