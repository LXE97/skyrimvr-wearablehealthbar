#include "overlap_sphere.h"

namespace vrinput
{
	using namespace RE;
	using namespace art_addon;

	const bool OverlapSphere::is_overlapping(bool isLeft) { return overlap_state[isLeft]; }

	std::weak_ptr<art_addon::ArtAddon> OverlapSphere::Get3D()
	{
		std::weak_ptr<art_addon::ArtAddon> weak = visible_debug_sphere;
		return weak;
	};

	std::shared_ptr<OverlapSphere> OverlapSphere::Make(RE::NiAVObject* a_attach_to,
		OverlapCallback a_callback, float a_radius, float a_max_angle_deg, Hand a_active_hand,
		const RE::NiPoint3& a_offset, const RE::NiPoint3& a_normal)
	{
		std::scoped_lock lock(OverlapSphereManager::GetSingleton()->process_lock);

		auto new_obj = std::shared_ptr<OverlapSphere>(new OverlapSphere);
		new_obj->attach_node = a_attach_to;
		new_obj->callback = a_callback;
		new_obj->which_hand_activates = a_active_hand;
		new_obj->squared_radius = a_radius * a_radius;
		new_obj->max_angle = helper::deg2rad(a_max_angle_deg);
		new_obj->local_position = a_offset;
		new_obj->normal = a_normal;
		new_obj->id = OverlapSphereManager::GetSingleton()->GetNextId();

		if (OverlapSphereManager::GetSingleton()->draw_spheres)
		{
			new_obj->visible_debug_sphere =
				OverlapSphereManager::GetSingleton()->CreateVisibleSphere(*new_obj.get());
		}

		OverlapSphereManager::GetSingleton()->process_list.push_back(new_obj);
		return new_obj;
	}

	bool OverlapSphereManager::Update()
	{
		constexpr float ktracking_error_threshold = 1.0f;

		auto player = PlayerCharacter::GetSingleton();
		if (player && player->Get3D(true))
		{
			// check difference between 1st person and 3rd person nodes
			NiAVObject* controllers[2]{ player->Get3D(false)->GetObjectByName(
											kControllerNodeName[0]),
				player->Get3D(false)->GetObjectByName(kControllerNodeName[1]) };

			if (auto firstpersoncheck =
					player->Get3D(true)->GetObjectByName(kControllerNodeName[0]))
			{
				if (firstpersoncheck->world.translate.GetSquaredDistance(
						controllers[0]->world.translate) < ktracking_error_threshold)
				{
					hand_transform_cache[0] = controllers[0]->world;
					hand_transform_cache[1] = controllers[1]->world;

					NiPoint3 sphere_world;
					for (auto& wp : process_list)
					{
						if (auto sphere = wp.lock())
						{
							if (auto node = sphere->attach_node)
							{
								if (sphere->local_position != NiPoint3::Zero())
								{
									sphere_world = node->world.translate +
										node->world.rotate * sphere->local_position;
								}
								else { sphere_world = node->world.translate; }

								// Test collision.
								bool changed = false;
								for (Hand hand : { Hand::kRight, Hand::kLeft })
								{
									if (sphere->which_hand_activates == hand ||
										sphere->which_hand_activates == Hand::kBoth)
									{
										float dist = sphere_world.GetSquaredDistance(
											controllers[(bool)hand]->world.translate +
											controllers[(bool)hand]->world.rotate * palm_offset);

										float angle = 0.0f;
										if (sphere->normal != NiPoint3::Zero() && sphere->max_angle)
										{
											auto normal_worldspace = helper::VectorNormalized(
												node->world.rotate * sphere->normal);
											auto palm_normal_worldspace = helper::VectorNormalized(
												controllers[(bool)hand]->world.rotate *
												kHandPalmNormal);
											angle = std::acos(helper::DotProductSafe(
												normal_worldspace, palm_normal_worldspace));
										}

										// entered sphere
										if (dist <= sphere->squared_radius &&
											angle <= sphere->max_angle &&
											!sphere->overlap_state[(bool)hand])
										{
											sphere->overlap_state[(bool)hand] = true;
											changed = SendEvent(*sphere, true, hand);
										}  // use hysteresis on exit threshold
										else if ((dist > sphere->squared_radius + kHysteresis ||
													 angle >
														 sphere->max_angle + kHysteresisAngular) &&
											sphere->overlap_state[(bool)hand])
										{
											sphere->overlap_state[(bool)hand] = false;
											changed = SendEvent(*sphere, false, hand);
										}

										// Reflect both collision states in sphere color
										if (changed && draw_spheres && sphere->visible_debug_sphere)
										{
											changed = false;

											if (auto visible_node =
													sphere->visible_debug_sphere->Get3D())
											{
												if (sphere->overlap_state[0] ||
													sphere->overlap_state[1])
												{
													helper::SetGlowColor(visible_node, on);
												}
												else if (!sphere->overlap_state[0] &&
													!sphere->overlap_state[1])
												{
													helper::SetGlowColor(visible_node, off_);
												}
											}
										}
									}
								}
							}
						}
					}

					return true;
				}
			}
		}
		return false;
	}

	void OverlapSphereManager::ShowDebugSpheres()
	{
		if (!draw_spheres)
		{
			std::scoped_lock lock(process_lock);

			auto player = PlayerCharacter::GetSingleton();

			NiTransform t;

			t.translate = palm_offset;
			t.scale = kModelScale;
			t.rotate = helper::RotateBetweenVectors(NiPoint3(1.0, 0, 0), kHandPalmNormal);
			for (auto isLeft : { false, true })
			{
				controller_models[isLeft] =
					art_addon::ArtAddon::Make("debug/debugsphere.nif", player->AsReference(),
						player->Get3D(false)->GetObjectByName(kControllerNodeName[isLeft]), t);
			}

			for (auto& wp : process_list)
			{
				if (auto s = wp.lock()) { s->visible_debug_sphere = CreateVisibleSphere(*s); }
			}

			draw_spheres = true;
		}
	}

	void OverlapSphereManager::HideDebugSpheres()
	{
		if (draw_spheres)
		{
			std::scoped_lock lock(process_lock);
			for (auto isLeft : { false, true }) { controller_models[isLeft].reset(); }
			for (auto& wp : process_list)
			{
				if (auto s = wp.lock())
				{
					if (s->visible_debug_sphere) { s->visible_debug_sphere.reset(); }
				}
			}
			draw_spheres = false;
		}
	}

	void OverlapSphereManager::SetPalmOffset(const RE::NiPoint3& a_offset)
	{
		palm_offset = a_offset;
	}

	ArtAddonPtr OverlapSphereManager::CreateVisibleSphere(const OverlapSphere& a_s)
	{
		auto        player = PlayerCharacter::GetSingleton();
		NiTransform t;
		t.translate = a_s.local_position;
		t.scale = std::sqrt(a_s.squared_radius);
		if (a_s.normal != NiPoint3::Zero())
		{
			t.rotate = helper::RotateBetweenVectors(NiPoint3(1.0, 0.0, 0.0), a_s.normal);
		}
		if (auto attach = player->Get3D(false)->GetObjectByName(a_s.attach_node->name))
		{
			t.scale /= attach->local.scale;
			return ArtAddon::Make(kModelPath, player->AsReference(), attach, t);
		}
		else { return nullptr; }
	}

	int OverlapSphereManager::GetNextId() { return nextId++; }

	bool OverlapSphereManager::SendEvent(
		const OverlapSphere& a_s, bool a_entered, Hand triggered_by)
	{
		if (a_s.callback) { a_s.callback(OverlapEvent{ a_entered, triggered_by, a_s.id }); }
		return true;
	}

	OverlapSphereManager::OverlapSphereManager()
	{
		on = new NiColor(kOnHexColor);
		off_ = new NiColor(kOffHexColor);
	}

}