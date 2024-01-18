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

	bool WearableManager::CycleFunctionControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
			if (mgr->GetState() == ManagerState::kIdle)
			{
				mgr->SetState(ManagerState::kCycleFunctions);
			}
		}
		else
		{
			if (mgr->GetState() == ManagerState::kCycleFunctions)
			{
				mgr->SetState(ManagerState::kIdle);
			}
		}
		return false;
	}

	bool WearableManager::LeftStick(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown) { mgr->CycleSelection(true); }
		return false;
	}

	bool WearableManager::RightStick(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown) { mgr->CycleSelection(false); }
		return false;
	}

	void WearableManager::CycleSelection(bool increment)
	{
		switch (configmode_state)
		{
		case ManagerState::kIdle:
			{
				if (auto sp = configmode_selected_item.lock())
				{
					sp->CycleSubitems(increment);
					//TODO: apply / clear shaders
				}
			}
			break;
		case ManagerState::kCycleFunctions:
			{
				if (auto sp = configmode_selected_item.lock())
				{
					sp->CycleFunction(increment);

					NiTransform t;
					t.translate = { 0, 0, 5 };
					sp->configmode_function_text =
						std::make_unique<AddonTextBox>(sp->GetFunctionName(), 0.f, sp->Get3D(), t);
				}
			}
			break;
		case ManagerState::kTranslate:
			{
				if (auto sp = configmode_selected_item.lock())
				{
					if (auto len = configmode_visible_bones.size(); len > 0)
					{
						// reset appearance of current parent
						helper::SetGlowColor(
							configmode_visible_bones.at(configmode_parentbone_index % len)->Get3D(),
							&bone_off);

						configmode_parentbone_index += increment ? 1 : -1;

						// set apperance of new parent
						helper::SetGlowColor(
							configmode_visible_bones.at(configmode_parentbone_index % len)->Get3D(),
							&bone_on);
					}
				}
			}
			break;
		default:
			break;
		}
	}

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
		if (configmode_next_state != configmode_state) { StateTransition(configmode_next_state); }

		if (!managed_objects.empty())
		{
			std::scoped_lock lock(managed_objects_lock);
			// 2. Process active configuration mode
			if (configmode_state != ManagerState::kNone && configmode_state != ManagerState::kIdle)
			{
				if (auto sp = configmode_selected_item.lock())
				{
					if (auto node = sp->Get3D())
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
									node->local = configmode_wearable_initial_local;
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
								}
								node->UpdateDownwardPass(ctx, 0);
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

										helper::FaceCamera(colorUI);

										// move UI from player root node to hand
										colorUI->local.translate =
											colorUI->parent->world.rotate.Transpose() *
											(item_hand_pos - colorUI->parent->world.translate);
										colorUI->local.translate += { 0, 0, 12 };
										colorUI->Update(ctx);

										// Calculate finger location in UI plane
										auto finger = PlayerCharacter::GetSingleton()
														  ->GetNodeByName("NPC R Finger12 [RF12]")
														  ->world;
										auto vector_to_finger = finger.translate +
											(finger.rotate * NiPoint3(0, 0, 2)) -
											colorUI->world.translate;

										auto finger_UI_space =
											colorUI->world.rotate.Transpose() * vector_to_finger;

										constexpr float extent_top = 7;
										constexpr float extent_bottom = -7;
										constexpr float extent_colorwheel_right = 7;
										constexpr float extent_colorwheel_left = -7;
										constexpr float extent_valueslider_left = 10;
										constexpr float extent_valueslider_right = 13;
										constexpr float extent_glowslider_left = -13;
										constexpr float extent_glowslider_right = -10;

										float depth = finger_UI_space.x;
										float y = finger_UI_space.z;
										float x = finger_UI_space.y;

										if (depth > -3 && depth < 1)
										{
											// check which element is touching
											if (y < extent_top && y > extent_bottom)
											{
												auto glowcursor =
													colorUI->GetObjectByName(kglow_name);
												auto valuecursor =
													colorUI->GetObjectByName(kvalue_name);
												auto huecursor =
													colorUI->GetObjectByName(kcolor_name);

												float radius;

												if (x < extent_glowslider_right &&
													x > extent_glowslider_left)
												{
													// glow slider
													glowcursor->local.translate.z = y;
												}
												else if (x < extent_valueslider_right &&
													x > extent_valueslider_left)
												{
													// value slider
													valuecursor->local.translate.z = y;
												}
												else if (x < extent_colorwheel_right &&
													x > extent_colorwheel_left)
												{
													// color wheel
													radius = std::sqrt(y * y + x * x);
													if (radius < extent_top)
													{
														huecursor->local.translate.z = y;
														huecursor->local.translate.y = x;
													}
												}

												static int c = 0;
												// don't need to be changing color every frame
												if (c++ % 50 == 0)
												{
													auto glow = (glowcursor->local.translate.z -
																	extent_bottom) /
														(extent_top - extent_bottom) * kMaxGlow;

													auto value = (valuecursor->local.translate.z -
																	 extent_bottom) /
														(extent_top - extent_bottom);

													auto hue = (atan2f(huecursor->local.translate.z,
																	huecursor->local.translate.y) +
																   std::numbers::pi) /
														std::numbers::pi;

													auto sat =
														std::sqrt(huecursor->local.translate.z *
																huecursor->local.translate.z +
															huecursor->local.translate.y *
																huecursor->local.translate.y) /
														extent_top;

													sp->SetColor(
														helper::HSV_to_RGB(hue, sat, value), glow);
												}
											}
										}
									}
								}
							}
							break;
						case ManagerState::kCycleFunctions:
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
						// 3. Call individual update functions
						if (sp->interactable) { sp->Update(); }
						else
						// 4. Attach overlap spheres if needed
						{
							sp->interactable = vrinput::OverlapSphere::Make(sp->model->Get3D(),
								OverlapHandler, settings.overlap_radius, settings.overlap_angle,
								sp->active_hand, sp->overlap_offset, sp->overlap_normal);
						}

						it++;
					}
					else { it = managed_objects.erase(it); }
				}
			}
		}
	}

	void WearableManager::StateTransition(ManagerState a_next_state)
	{
		if (a_next_state != configmode_state)
		{
			// exit state event
			switch (configmode_state)
			{
			case ManagerState::kNone:
				vrinput::StartBlockingAll();
				vrinput::StartSmoothing();
				RegisterConfigModeButtons(true);
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

							// switch to new parent
							if (!configmode_visible_bones.empty())
							{
								if (auto selected_bone =
										configmode_visible_bones.at(configmode_parentbone_index %
											configmode_visible_bones.size()))
								{
									auto new_parent = selected_bone->GetParent();

									node->local.translate = new_parent->world.rotate.Transpose() *
										(node->world.translate - new_parent->world.translate);
									node->local.rotate =
										new_parent->world.rotate.Transpose() * node->world.rotate;

									new_parent->AsNode()->AttachChild(node);
									sp->attach_node = new_parent;
								}
								// hide/delete bone visualization
								configmode_visible_bones.clear();
							}
						}
					}
				}
				break;
			case ManagerState::kColor:
				configmode_colorwheel.reset();

				g_vrikInterface->restoreFingers((bool)configmode_active_hand);
				break;
			case ManagerState::kCycleFunctions:
				if (auto sp = configmode_selected_item.lock())
				{
					sp->configmode_function_text.reset();
				}
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
				RegisterConfigModeButtons(false);
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
						if (settings.allow_reattach)
						{
							configmode_parentbone_index = ShowEligibleBones(sp);
						}
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
						PlayerCharacter::GetSingleton()->Get3D(false), t, [](ArtAddon* a) {
							if (1)
							{
								auto mgr = WearableManager::GetSingleton();
								if (auto sp = mgr->GetSelectedItem().lock())
								{
									if (auto model = a->Get3D())
									{
										RE::NiColor temp(0,0,0);
										float       glow = 1.f;
										sp->GetColor(&temp, &glow);

										auto glowcursor = model->GetObjectByName(mgr->kglow_name);
										auto valuecursor = model->GetObjectByName(mgr->kvalue_name);
										auto huecursor = model->GetObjectByName(mgr->kcolor_name);

										auto hsv = helper::RGBtoHSV(temp);

										glowcursor->local.translate.z =
											-glowcursor->local.translate.z +
											2 * glowcursor->local.translate.z * glow / kMaxGlow;
										valuecursor->local.translate.z =
											-valuecursor->local.translate.z +
											2 * valuecursor->local.translate.z * hsv.z;

										float theta = hsv.x * std::numbers::pi * 2;
										float radius = hsv.y * huecursor->local.translate.z;

										huecursor->local.translate.z = radius * std::sin(theta);
										huecursor->local.translate.y = radius * std::cos(theta);
									}
								}
							}
						});
					g_vrikInterface->setFingerRange(
						(bool)configmode_active_hand, 0, 0, 1.0, 1.0, 0, 0, 0, 0, 0, 0);
				}
				break;
			case ManagerState::kCycleFunctions:
				if (auto sp = configmode_selected_item.lock())
				{
					NiTransform t;
					t.translate = { 0, 0, 5 };
					sp->configmode_function_text =
						std::make_unique<AddonTextBox>(sp->GetFunctionName(), 0.f, sp->Get3D(), t);
				}
			default:
				break;
			}

			configmode_state = a_next_state;
		}
	}

	void WearableManager::Register(std::weak_ptr<Wearable> a_obj)
	{
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

	void WearableManager::RegisterConfigModeButtons(bool a_unregister_register)
	{
		if (a_unregister_register)
		{
			vrinput::AddCallback(settings.translate, PositionControl, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(
				settings.scale, ScaleControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(
				settings.color, ColorControl, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(settings.cycle_AV, CycleFunctionControl, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(vr::k_EButton_DPad_Left, LeftStick, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(vr::k_EButton_DPad_Right, RightStick, configmode_active_hand,
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
			vrinput::RemoveCallback(settings.cycle_AV, CycleFunctionControl, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(vr::k_EButton_DPad_Left, LeftStick, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(vr::k_EButton_DPad_Right, RightStick, configmode_active_hand,
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

	int WearableManager::ShowEligibleBones(WearablePtr a_selected)
	{
		auto        player = PlayerCharacter::GetSingleton();
		NiTransform t;
		int         parent_index = 0;

		for (auto it = a_selected->eligible_attach_nodes.begin();
			 it != a_selected->eligible_attach_nodes.end(); it++)
		{
			if (auto bone = player->Get3D(false)->GetObjectByName(*it))
			{
				if (bone == a_selected->attach_node)
				{
					SKSE::log::trace("found parent");
					parent_index = std::distance(a_selected->eligible_attach_nodes.begin(), it);

					configmode_visible_bones.push_back(ArtAddon::Make(
						vrinput::kDebugModelPath, player->AsReference(), bone, t, [](ArtAddon* a) {
							if (auto model = a->Get3D())
							{
								helper::SetGlowColor(
									model, &(WearableManager::GetSingleton()->bone_on));
							}
						}));
				}
				else
				{
					configmode_visible_bones.push_back(
						ArtAddon::Make(vrinput::kDebugModelPath, player->AsReference(), bone, t));
				}
			}
		}
		return parent_index;
	}

}  // namespace wearable
