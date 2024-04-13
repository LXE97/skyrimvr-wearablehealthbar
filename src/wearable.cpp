#include "wearable.h"

namespace wearable
{
	using namespace RE;
	using namespace art_addon;

	bool WearableManager::EnterConfigModeControl(const vrinput::ModInputEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.button_state == vrinput::ButtonState::kButtonDown)
		{
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
						mgr->ApplySelectionShader(sp->Get3D());
						mgr->SetState(ManagerState::kIdle);
					}
				}
				else { last_tap_time = std::chrono::steady_clock::now(); }
			}
			else
			{  // TODO secondary function: change selected object
				/*if (auto sp = mgr->configmode_selected_item.lock())
				{
					if (sp->interactable->GetId() != mgr->configmode_hovered_sphere_id)
					{
						mgr->configmode_selected_item =
							mgr->FindWearableWithOverlapID(mgr->configmode_hovered_sphere_id);

						if (auto sp2 = mgr->configmode_selected_item.lock())
						{
							mgr->ApplySelectionShader(sp2->Get3D());
						}
					}
				}*/
			}
		}

		return true;
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
				if (auto sp = configmode_selected_item.lock()) { sp->CycleSubitems(increment); }
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
							kBoneOffHex);

						configmode_parentbone_index += increment ? 1 : -1;

						// set apperance of new parent
						helper::SetGlowColor(
							configmode_visible_bones.at(configmode_parentbone_index % len)->Get3D(),
							kBoneOnHex);
					}
					else
					{
						if (settings.allow_reattach)
						{
							configmode_parentbone_index = ShowEligibleBones(sp);
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}

	/* Primary function: prepare to enter config mode by selecting the hovered-over Wearable and 
	* registering the config mode button callback.
	* Secondary function: TODO delegate overlap events to Holster objects. They will handle registering
	* input callbacks in their own methods
	*/
	void WearableManager::OverlapHandler(const vrinput::OverlapEvent& e)
	{
		auto mgr = WearableManager::GetSingleton();

		if (e.entered)
		{
			// in configuration mode, all non-configurative interaction is disabled
			if (mgr->configmode_state == ManagerState::kNone)
			{
				// look up which wearable the overlapped sphere belongs to
				mgr->configmode_hovered_sphere_id = e.id;
				auto wp = mgr->FindWearableWithOverlapID(e.id);

				if (auto sp = wp.lock())
				{
					// update our weak pointer
					mgr->hovered_wearable = wp;
					// delegate the overlap event to the wearable
					sp->OverlapHandler(e);
				}
			}

			vrinput::AddCallback(EnterConfigModeControl, mgr->settings.enter_config_mode,
				e.triggered_by, vrinput::ActionType::kPress);
		}
		else
		{
			// on exiting the overlap sphere, we need to delegate the exit event to the hovered wearable.
			// Since we're exiting, it must already be selected and it can use our weak pointer to get a reference to itself
			// from within the static button handler

			// TODO: but what about the case where you have overlapping sphere from multiple wearables...
			// you could have 2 Enter events in a row with no exit. Then the next Exit event might not refer to the
			// currently selected item ._.

			if (auto sp = mgr->configmode_selected_item.lock())
			{
				// delegate the overlap event to the wearable
				sp->OverlapHandler(e);
			}

			// clear the selection variables
			mgr->hovered_wearable = std::weak_ptr<Wearable>();
			//mgr->configmode_hovered_sphere_id = -1;

			vrinput::RemoveCallback(EnterConfigModeControl, mgr->settings.enter_config_mode,
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
			if (configmode_state != ManagerState::kNone)
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
								// TODO: crash if sphere is drawn at the same time as this
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
										// 1. Update UI position
										auto item_hand_pos =
											PlayerCharacter::GetSingleton()
												->GetNodeByName(vrinput::kControllerNodeName[(int)
														vrinput::GetOtherHand(
															configmode_active_hand)])
												->world.translate;

										auto item_pos = node->world.translate;

										// move UI from player root node to above wearable
										colorUI->local.translate =
											colorUI->parent->world.rotate.Transpose() *
											(item_pos - colorUI->parent->world.translate);
										colorUI->local.translate += { 0, 0, 15 };

										helper::FaceCamera(colorUI);

										colorUI->Update(ctx);

										// 2. Calculate finger location in UI plane
										auto finger =
											configmode_active_hand == vrinput::Hand::kLeft ?
											PlayerCharacter::GetSingleton()
												->GetNodeByName("NPC L Finger12 [LF12]")
												->world :
											PlayerCharacter::GetSingleton()
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
												if (c++ % 10 == 0)
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

													NiColor temp_color =
														helper::HSV_to_RGB(hue, sat, value);
													NiColorA temp_colorA;
													temp_colorA = temp_color;
													temp_colorA.alpha = glow;
													sp->SetColor(temp_colorA);
												}
											}
										}
									}
								}
							}
							break;
						case ManagerState::kCycleFunctions:
							break;
						default:
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
						if (sp->interactable)
						{
							static int frame_counter = 0;
							if (frame_counter++ % settings.update_frames == 0) { sp->Update(1); }
						}
						else if (sp->Get3D())
						// 4. Attach overlap spheres if needed
						{
							sp->interactable = vrinput::OverlapSphere::Make(sp->Get3D(),
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
			case ManagerState::kNone:  // i.e. we are entering config mode
				vrinput::StartBlockingAll();
				vrinput::StartSmoothing();
				RegisterConfigModeButtons(true);
				if (g_vrikInterface) { g_vrikInterface->beginGestureProfile(); }
				if (g_higgsInterface)
				{
					// disable right and left hands
					g_higgsInterface->DisableHand(true);
					g_higgsInterface->DisableHand(false);
				}

				// tell meters to go into configmode specific state. only relevant for eye meter currently
				if (auto sp = WearableManager::GetSingleton()->GetSelectedItem().lock())
				{
					sp->Update(-1);
				}
				break;
			case ManagerState::kIdle:
				break;
			case ManagerState::kTranslate:
				{
					if (auto sp = configmode_selected_item.lock())
					{
						if (auto node = sp->Get3D())
						{
							if (1)
							{
								auto rt = node->local.rotate;
								auto tr = node->local.translate;
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
									if (new_parent != sp->attach_node)
									{
										node->local.translate =
											new_parent->world.rotate.Transpose() *
											(node->world.translate - new_parent->world.translate);
										node->local.rotate = new_parent->world.rotate.Transpose() *
											node->world.rotate;

										new_parent->AsNode()->AttachChild(node);
										sp->attach_node = new_parent;
									}
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
					// enable right and left hands
					g_higgsInterface->EnableHand(true);
					g_higgsInterface->EnableHand(false);
				}
				break;
			case ManagerState::kIdle:
				break;
			case ManagerState::kTranslate:
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
							auto mgr = WearableManager::GetSingleton();
							if (auto sp = mgr->GetSelectedItem().lock())
							{
								if (auto model = a->Get3D())
								{
									SKSE::log::trace("color wheel creation callback");
									RE::NiColorA temp(0, 0, 0, 0);
									sp->GetColor(&temp);

									auto glowcursor = model->GetObjectByName(mgr->kglow_name);
									auto valuecursor = model->GetObjectByName(mgr->kvalue_name);
									auto huecursor = model->GetObjectByName(mgr->kcolor_name);

									auto hsv = helper::RGBtoHSV(temp);

									glowcursor->local.translate.z = -glowcursor->local.translate.z +
										2 * glowcursor->local.translate.z * temp.alpha / kMaxGlow;
									valuecursor->local.translate.z =
										-valuecursor->local.translate.z +
										2 * valuecursor->local.translate.z * hsv.z;

									float theta = hsv.x * std::numbers::pi * 2;
									float radius = hsv.y * huecursor->local.translate.z;

									huecursor->local.translate.z = radius * std::sin(theta);
									huecursor->local.translate.y = radius * std::cos(theta);
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
					t.scale = 2.f;
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
			vrinput::AddCallback(PositionControl, settings.translate, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(
				ScaleControl, settings.scale, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(
				ColorControl, settings.color, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::AddCallback(CycleFunctionControl, settings.cycle_AV, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(LeftStick, vr::k_EButton_DPad_Left, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::AddCallback(RightStick, vr::k_EButton_DPad_Right, configmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : settings.exiting_buttons)
			{
				vrinput::AddCallback(ExitConfigModeControl, button,
					GetOtherHand(configmode_active_hand), vrinput::ActionType::kPress);
			}
		}
		else
		{
			vrinput::RemoveCallback(PositionControl, settings.translate, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				ScaleControl, settings.scale, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(
				ColorControl, settings.color, configmode_active_hand, vrinput::ActionType::kPress);
			vrinput::RemoveCallback(CycleFunctionControl, settings.cycle_AV, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(LeftStick, vr::k_EButton_DPad_Left, configmode_active_hand,
				vrinput::ActionType::kPress);
			vrinput::RemoveCallback(RightStick, vr::k_EButton_DPad_Right, configmode_active_hand,
				vrinput::ActionType::kPress);

			for (auto button : settings.exiting_buttons)
			{
				vrinput::RemoveCallback(ExitConfigModeControl, button,
					GetOtherHand(configmode_active_hand), vrinput::ActionType::kPress);
			}
		}
	}

	void WearableManager::CaptureInitialTransforms()
	{
		if (auto sp = configmode_selected_item.lock())
		{
			if (auto target = sp->Get3D())
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
					parent_index = std::distance(a_selected->eligible_attach_nodes.begin(), it);

					configmode_visible_bones.push_back(ArtAddon::Make(
						vrinput::kDebugModelPath, player->AsReference(), bone, t, [](ArtAddon* a) {
							if (auto model = a->Get3D())
							{
								helper::SetGlowColor(
									model, WearableManager::GetSingleton()->kBoneOnHex);
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
