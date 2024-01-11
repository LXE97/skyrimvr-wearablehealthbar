#include "wearable.h"

namespace wearable
{
	using namespace RE;
	using namespace art_addon;

	bool WearableManager::EnterConfigModeControl(const vrinput::ModInputEvent& e)
	{
		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			auto mgr = WearableManager::GetSingleton();

			// primary function: enter config mode
			if (mgr->GetState() == ManagerState::kNone)
			{
				static auto last_tap_time = std::chrono::steady_clock::now();

				auto tap_diff = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - last_tap_time);

				if (tap_diff.count() < mgr->settings.enter_config_tap_delay_max_ms &&
					tap_diff.count() > mgr->settings.enter_config_tap_delay_min_ms)
				{
					mgr->configmode_selected_item =
						mgr->FindWearableWithOverlapID(mgr->configmode_hovered_sphere_id);

					if (auto sp = mgr->configmode_selected_item.lock())
					{
						mgr->configmode_active_hand = e.device;
						mgr->ApplySelectionShader(sp->model->Get3D());
						mgr->SetState(ManagerState::kIdle);
					}
				}
				else { last_tap_time = std::chrono::steady_clock::now(); }
			}
			else
			{  // secondary function: change selected object
				if (auto sp = mgr->configmode_selected_item.lock())
				{
					if (sp->interactable->GetId() != mgr->configmode_hovered_sphere_id)
					{
						mgr->configmode_selected_item =
							mgr->FindWearableWithOverlapID(mgr->configmode_hovered_sphere_id);

						if (auto sp2 = mgr->configmode_selected_item.lock())
						{
							mgr->ApplySelectionShader(sp2->model->Get3D());
						}
					}
				}
			}
		}
		return false;
	}

	bool WearableManager::ExitConfigModeControl(const vrinput::ModInputEvent& e)
	{
		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			auto mgr = WearableManager::GetSingleton();
			mgr->SetState(ManagerState::kNone);
		}
		return false;
	}

	bool WearableManager::PositionControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			if (mgr->GetState() == ManagerState::kIdle) { mgr->SetState(ManagerState::kTranslate); }
		}
		else
		{
			if (mgr->GetState() == ManagerState::kTranslate) { mgr->SetState(ManagerState::kIdle); }
		}
		return false;
	}

	bool WearableManager::ScaleControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			if (mgr->GetState() == ManagerState::kIdle) { mgr->SetState(ManagerState::kScale); }
			else if (mgr->GetState() == ManagerState::kColor)
			{
				if (mgr->configmode_colorwheel)
				{
					if (auto colorUI = mgr->configmode_colorwheel->Get3D())
					{
						auto color_normal = colorUI->world.rotate * NiPoint3(1, 0, 0);
						auto vector_to_head =
							colorUI->parent->world.translate - colorUI->world.translate;
						vector_to_head.z = 0;

						colorUI->local.rotate = colorUI->parent->world.rotate.Transpose();
					}
				}
			}
		}
		else
		{
			if (mgr->GetState() == ManagerState::kScale) { mgr->SetState(ManagerState::kIdle); }
		}
		return false;
	}

	bool WearableManager::ColorControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			if (mgr->GetState() == ManagerState::kIdle) { mgr->SetState(ManagerState::kColor); }
		}
		else
		{
			if (mgr->GetState() == ManagerState::kColor) { mgr->SetState(ManagerState::kIdle); }
		}
		return false;
	}

	bool WearableManager::CycleAVControl(const vrinput::ModInputEvent& e) { return false; }

	void WearableManager::OverlapHandler(const vrinput::OverlapEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();
		if (e.entered)
		{
			// Only change selected object when not in configuration mode or event was triggered
			// by the hand that activated configuration mode
			if (mgr->configmode_state == ManagerState::kNone ||
				e.triggered_by == mgr->configmode_active_hand)
			{
				mgr->configmode_hovered_sphere_id = e.id;
			}

			vrinput::AddCallback(mgr->settings.enter_config_mode, EnterConfigModeControl,
				e.triggered_by, vrinput::ActionType::kPress);
		}
		else
		{
			mgr->configmode_hovered_sphere_id = -1;
			vrinput::RemoveCallback(mgr->settings.enter_config_mode, EnterConfigModeControl,
				e.triggered_by, vrinput::ActionType::kPress);
		}
	}

	void WearableManager::Update()
	{
		// 1. State transitions
		if (configmode_next_state != configmode_state) { TransitionState(configmode_next_state); }

		if (!managed_objects.empty())
		{
			std::scoped_lock lock(managed_objects_lock);
			// 2. Process active configuration mode
			if (configmode_state != ManagerState::kNone && configmode_state != ManagerState::kIdle)
			{
				if (auto sp = configmode_selected_item.lock())
				{
					if (auto node = sp->model->Get3D())
					{
						NiUpdateData ctx;

						auto hand_world =
							PlayerCharacter::GetSingleton()
								->GetNodeByName(
									vrinput::kControllerNodeName[(int)configmode_active_hand])
								->world;

						auto hand_delta =
							hand_world.translate.GetDistance(configmode_hand_initial.translate);

						switch (configmode_state)
						{
						case ManagerState::kTranslate:
							{
								if (hand_delta > settings.reset_distance)
								{
									node->local.rotate = sp->default_local.rotate;
									node->local.translate = sp->default_local.translate;
								}
								else
								{
									auto parent_world = node->parent->world;
									auto hand_rot_diff = hand_world.rotate *
										configmode_hand_initial.rotate.Transpose();

									node->local.rotate = parent_world.rotate.Transpose() *
										hand_rot_diff * configmode_wearable_initial_world.rotate;

									// world position = new grabber world position + ( initial distance from grabber, rotated by change in grabber rotation)
									NiTransform new_world;
									new_world.translate = hand_world.translate +
										hand_rot_diff *
											(configmode_wearable_initial_world.translate -
												configmode_hand_initial.translate);

									// convert to local transform:
									// position: vector from parent to new child world position, rotated by the inverse of the parent's rotation
									node->local.translate = parent_world.rotate.Transpose() *
										(new_world.translate - parent_world.translate);

									// skeleton visuals
									if (!configmode_skeletons.empty())
									{
										static std::weak_ptr<ArtAddon> closest;

										if (auto cur_closest = FindClosestSkeleton(node).lock())
										{
											auto prev_closest = closest.lock();

											if (prev_closest != cur_closest)
											{
												if (prev_closest)
												{
													SKSE::log::trace("tur noff");
													helper::SetGlowColor(
														prev_closest->Get3D(), &bone_off);
												}
												SKSE::log::trace("turn on ");
												helper::SetGlowColor(
													cur_closest->Get3D(), &bone_on);
												// turns back into a weak ptr so don't have to worry about
												// when the art addons get destroyed
												closest = cur_closest;
											}
										}
									}
								}
								node->Update(ctx);
							}
							break;
						case ManagerState::kScale:
							{
								if (hand_delta > settings.reset_distance)
								{
									node->local.scale = sp->default_local.scale;
								}
								else
								{
									auto initial_distance =
										configmode_hand_initial.translate.GetDistance(
											configmode_wearable_initial_world.translate);

									auto new_distance =
										hand_world.translate.GetDistance(node->world.translate);

									float new_scale = configmode_wearable_initial_local.scale *
										std::expf(vrinput::adjustable *
											(initial_distance - new_distance));

									node->local.scale = new_scale;
								}
								if (auto overlap_visible = sp->interactable->Get3D().lock())
								{
									overlap_visible->Get3D()->local.scale =
										sp->interactable->GetRadius() / node->local.scale;
								}
							}
							break;
						case ManagerState::kColor:
							{
								if (configmode_colorwheel)
								{
									if (auto colorUI = configmode_colorwheel->Get3D())
									{
										// Update UI position
										auto item_hand_pos =
											PlayerCharacter::GetSingleton()
												->GetNodeByName(vrinput::kControllerNodeName[(int)
														vrinput::GetOtherHand(
															configmode_active_hand)])
												->world.translate;

										auto vector_to_camera =
											PlayerCharacter::GetSingleton()
												->GetNodeByName("NPC Head [Head]")
												->world.translate -
											item_hand_pos;

										float heading =
											std::atan2f(vector_to_camera.y, vector_to_camera.x);

										colorUI->local.rotate.SetEulerAnglesXYZ({ 0, 0, -heading });
										colorUI->local.rotate =
											colorUI->parent->world.rotate.Transpose() *
											colorUI->local.rotate;

										colorUI->local.translate =
											colorUI->parent->world.rotate.Transpose() *
											(item_hand_pos - colorUI->parent->world.translate);
										colorUI->local.translate += { 0, 0, 12 };
										colorUI->Update(ctx);
										NiTransform t;
										t.translate = { -4, 0, 0 };
										if (!testt)
										{
											testt = ArtAddon::Make(vrinput::kDebugModelPath,
												PlayerCharacter::GetSingleton()->AsReference(),
												colorUI, t);
										}
										else
										{
											// Calculate finger location in UI plane
											auto finger =
												PlayerCharacter::GetSingleton()
													->GetNodeByName("NPC R Finger12 [RF12]")
													->world;
											auto vector_to_finger = finger.translate +
												(finger.rotate * NiPoint3(0, 0, 2)) -
												colorUI->world.translate;

											if (auto testnode = testt->Get3D())
											{
												testnode->local.translate =
													testnode->parent->world.rotate.Transpose() *
													vector_to_finger;
											}

											auto local = colorUI->world.rotate.Transpose() *
												vector_to_finger;

											constexpr float       extent_top = 7;
											constexpr float       extent_bottom = -7;
											constexpr float       extent_colorwheel_right = 7;
											constexpr float       extent_colorwheel_left = -7;
											constexpr float       extent_valueslider_left = 10;
											constexpr float       extent_valueslider_right = 13;
											constexpr float       extent_glowslider_left = -13;
											constexpr float       extent_glowslider_right = -10;
											constexpr const char* color_name = "cursor_color";
											constexpr const char* value_name = "cursor_value";
											constexpr const char* glow_name = "cursor_glow";

											float depth = local.x;
											float y = local.z;
											float x = local.y;

											if (depth > -3 && depth < 1)
											{
												// check which element is touching
												if (y < extent_top && y > extent_bottom)
												{
													if (x < extent_glowslider_right &&
														x > extent_glowslider_left)
													{
														// glow slider
														auto sliderval = (y - extent_bottom) /
															(extent_top - extent_bottom);
														if (auto cursor = colorUI->GetObjectByName(
																glow_name))
														{
															cursor->local.translate.z = y;
														}
													}
													else if (x < extent_valueslider_right &&
														x > extent_valueslider_left)
													{
														// value slider
														auto sliderval = (y - extent_bottom) /
															(extent_top - extent_bottom);
														if (auto cursor = colorUI->GetObjectByName(
																value_name))
														{
															cursor->local.translate.z = y;
														}
													}
													else if (x < extent_colorwheel_right &&
														x > extent_colorwheel_left)
													{
														// color wheel
														if (auto cursor = colorUI->GetObjectByName(
																color_name))
														{
															cursor->local.translate.z = y;
                                                            cursor->local.translate.y = x;
														}
													}
												}
											}
										}
									}
								}
							}
							break;
						case ManagerState::kCycleAV:
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
							// 3. Call individual update functions
							if (sp->interactable) { sp->Update(); }
							else
							// 4. Attach overlap spheres to newly-created geometry
							{
								sp->interactable = vrinput::OverlapSphere::Make(sp->model->Get3D(),
									OverlapHandler, settings.overlap_radius, settings.overlap_angle,
									sp->active_hand, sp->overlap_offset, sp->overlap_normal);
							}
						}
						it++;
					}
					else { it = managed_objects.erase(it); }
				}
			}
		}
	}

	void WearableManager::TransitionState(ManagerState a_next_state)
	{
		if (a_next_state != configmode_state)
		{
			// exit state event
			switch (configmode_state)
			{
			case ManagerState::kNone:
				vrinput::StartBlockingAll();
				vrinput::StartSmoothing();
				RegisterconfigModeButtons(true);
				if (g_vrikInterface) { g_vrikInterface->beginGestureProfile(); }
				if (g_higgsInterface)
				{
					g_higgsInterface->DisableHand(true);
					g_higgsInterface->DisableHand(false);
				}
				break;
			case ManagerState::kIdle:
				break;
			case ManagerState::kTranslate:
				{
					if (auto sp = configmode_selected_item.lock())
					{
						if (auto node = sp->model->Get3D())
						{
							auto rt = node->local.rotate;
							auto tr = node->local.translate;
							if (0)
							{
								SKSE::log::trace(
									"Rotation Matrix:\n {} {} {}\n {} {} {}\n {} {} {}\n",
									rt.entry[0][0], rt.entry[0][1], rt.entry[0][2], rt.entry[1][0],
									rt.entry[1][1], rt.entry[1][2], rt.entry[2][0], rt.entry[2][1],
									rt.entry[2][2]);
								SKSE::log::trace(
									"translate Array:\n {} \n {} \n {}\n", tr[0], tr[1], tr[2]);
							}

							// switch to new closest parent
							if (!configmode_skeletons.empty())
							{
								if (auto closest = FindClosestSkeleton(node).lock())
								{
									auto closest_parent = closest->GetParent();

									node->local.translate =
										closest_parent->world.rotate.Transpose() *
										(node->world.translate - closest_parent->world.translate);
									node->local.rotate = closest_parent->world.rotate.Transpose() *
										node->world.rotate;

									closest->GetParent()->AsNode()->AttachChild(node);
									sp->attach_node = closest->GetParent();
								}
								configmode_skeletons.clear();
							}
						}
					}
				}
				break;
			case ManagerState::kColor:
				testt.reset();
				configmode_colorwheel.reset();

				g_vrikInterface->restoreFingers((bool)configmode_active_hand);
				break;
			default:
				break;
			}

			// enter state event
			switch (a_next_state)
			{
			case ManagerState::kNone:
				vrinput::StopBlockingAll();
				vrinput::StopSmoothing();
				ApplySelectionShader();
				RegisterconfigModeButtons(false);
				if (g_vrikInterface) { g_vrikInterface->endGestureProfile(); }
				if (g_higgsInterface)
				{
					g_higgsInterface->EnableHand(true);
					g_higgsInterface->EnableHand(false);
				}
				break;
			case ManagerState::kIdle:
				break;
			case ManagerState::kTranslate:
				if (auto sp = configmode_selected_item.lock())
				{
					if (auto node = sp->model->Get3D())
					{
						if (settings.allow_reattach) { ShowEligibleSkeleton(sp); }
					}
				}
				CaptureInitialTransforms();
				break;
			case ManagerState::kScale:
				CaptureInitialTransforms();
				break;
			case ManagerState::kColor:
				{
					NiTransform t;
					configmode_colorwheel = ArtAddon::Make(kColorWheelModelPath,
						PlayerCharacter::GetSingleton()->AsReference(),
						PlayerCharacter::GetSingleton()->Get3D(false), t);
					g_vrikInterface->setFingerRange(
						(bool)configmode_active_hand, 0, 0, 1.0, 1.0, 0, 0, 0, 0, 0, 0);
				}
				break;
			default:
				break;
			}

			configmode_state = a_next_state;
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
		static constexpr int    bone_off_hex = 0x3dff4a;
		static constexpr int    bone_on_hex = 0xff260e;

		bone_on = NiColor(bone_on_hex);
		bone_off = NiColor(bone_off_hex);

		if (auto temp = TESForm::LookupByID(shader_formID))
		{
			shader = temp->As<TESEffectShader>();
		}
	}

	std::weak_ptr<Wearable> WearableManager::FindWearableWithOverlapID(int a_id)
	{
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
					if (a_shader_effect.lifetime == shader_ID)
					{
						SKSE::log::trace("shader removed on {} {} {}",
							a_shader_effect.target.get()->GetName(), a_shader_effect.lifetime,
							shader_ID);
						a_shader_effect.lifetime = 0;
					}
					return BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	void WearableManager::RegisterconfigModeButtons(bool a_unregister_register)
	{
		if (a_unregister_register)
		{
			vrinput::AddCallback(settings.translate, PositionControl, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(
				settings.scale, ScaleControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(
				settings.color, ColorControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(settings.cycle_AV, CycleAVControl, configmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : settings.exit)
			{
				vrinput::AddCallback(button, ExitConfigModeControl,
					GetOtherHand(configmode_active_hand), vrinput::ActionType::kPress);
			}
		}
		else
		{
			vrinput::RemoveCallback(settings.translate, PositionControl, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				settings.scale, ScaleControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				settings.color, ColorControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(settings.cycle_AV, CycleAVControl, configmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : settings.exit)
			{
				vrinput::RemoveCallback(button, ExitConfigModeControl,
					GetOtherHand(configmode_active_hand), vrinput::ActionType::kPress);
			}
		}
	}

	void WearableManager::CaptureInitialTransforms()
	{
		if (auto sp = configmode_selected_item.lock())
		{
			if (auto target = sp->model->Get3D())
			{
				configmode_wearable_initial_world = target->world;
				configmode_wearable_initial_local = target->local;
				configmode_hand_initial =
					PlayerCharacter::GetSingleton()
						->GetNodeByName(vrinput::kControllerNodeName[(int)configmode_active_hand])
						->world;
			}
		}
	}

	void WearableManager::ShowEligibleSkeleton(WearablePtr a_selected)
	{
		auto        player = PlayerCharacter::GetSingleton();
		NiTransform t;

		for (auto& nodename : a_selected->eligible_nodes)
		{
			if (auto skeleton = player->Get3D(false)->GetObjectByName(nodename))
			{
				configmode_skeletons.push_back(
					ArtAddon::Make(vrinput::kDebugModelPath, player->AsReference(), skeleton, t));
			}
		}
	}

	std::weak_ptr<ArtAddon> WearableManager::FindClosestSkeleton(RE::NiAVObject* a_selected)
	{
		auto        player = PlayerCharacter::GetSingleton();
		NiPoint3    target = a_selected->world.translate;
		float       min = 999999.f;
		ArtAddonPtr closest;

		for (auto ptr : configmode_skeletons)
		{
			if (auto node = ptr->GetParent())
			{
				float dist = node->world.translate.GetSquaredDistance(target);

				if (dist < min)
				{
					min = dist;
					closest = ptr;
				}
			}
		}

		return closest;
	}

	Meter::Meter(const char* a_model_path, RE::NiAVObject* a_attach_node, RE::NiTransform& a_local,
		RE::NiPoint3 a_overlap_offset, std::vector<MeterType> a_meter_types,
		std::vector<std::string> a_meternode_names, std::vector<const char*>* a_eligible_nodes) :
		Wearable(a_model_path, a_attach_node, a_local, a_overlap_offset, vrinput::Hand::kBoth,
			a_eligible_nodes),
		eligible_types(a_meter_types),
		meternode_names(a_meternode_names)

	{
		model = ArtAddon::Make(
			a_model_path, PlayerCharacter::GetSingleton()->AsReference(), a_attach_node, a_local);
		WearableManager::GetSingleton()->Register(weak_from_this());
	}

	void Meter::Update() {}

	void CompoundMeter::Update() {}

}  // namespace wearable
