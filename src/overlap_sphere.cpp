#include "overlap_sphere.h"

namespace vrinput
{
	using namespace RE;
	using namespace art_addon;

	const bool OverlapSphere::is_overlapping(bool isLeft) { return overlap_state[isLeft]; };

	std::shared_ptr<OverlapSphere> OverlapSphere::Make(NiAVObject* a_attach_to,
		OverlapCallback a_callback, float a_radius, float a_max_angle_deg, const NiPoint3& a_offset,
		const NiPoint3& a_normal)
	{
		std::scoped_lock lock(OverlapSphereManager::GetSingleton()->process_lock);

		auto new_obj = std::shared_ptr<OverlapSphere>(new OverlapSphere);
		new_obj->attach_node = a_attach_to;
		new_obj->callback = a_callback;
		new_obj->squared_radius = a_radius * a_radius;
		new_obj->max_angle = a_max_angle_deg;
		new_obj->local_position = a_offset;
		new_obj->normal = a_normal;
		new_obj->id = OverlapSphereManager::GetSingleton()->GetNextId();

		OverlapSphereManager::GetSingleton()->process_list.push_back(new_obj);
		return new_obj;
	}

	float OverlapSphereManager::Update()
	{
		constexpr float ktracking_error_threshold = 4.0f;
		auto            player = PlayerCharacter::GetSingleton();
		if (player->Get3D(true))
		{
			auto start = std::chrono::steady_clock::now();
			// check difference between 1st person and 3rd person nodes
			NiAVObject* controllers[2]{ player->Get3D(true)->GetObjectByName(
											kControllerNodeName[0]),
				player->Get3D(true)->GetObjectByName(kControllerNodeName[1]) };

			std::scoped_lock lock(process_lock);

			NiPoint3 sphere_world;
			for (auto& wp : process_list)
			{
				if (auto sphere = wp.lock())
				{
					if (auto node = sphere->attach_node)
					{
						if (sphere->local_position != NiPoint3::Zero())
						{
							sphere_world =
								node->world.translate + node->world.rotate * sphere->local_position;
						} else
						{
							sphere_world = node->world.translate;
						}

						// Test collision.
						bool changed = false;
						for (bool isLeft : { true })
						{
							float dist = sphere_world.GetSquaredDistance(
								controllers[isLeft]->world.translate +
								controllers[isLeft]->world.rotate * palm_offset);

							float angle = 0.0f;
							if (sphere->normal != NiPoint3::Zero() && sphere->max_angle)
							{
								auto normal_worldspace =
									helper::VectorNormalized(node->world.rotate * sphere->normal);
								auto palm_normal_worldspace = helper::VectorNormalized(
									controllers[isLeft]->world.rotate * kHandPalmNormal);
								angle = std::acos(helper::DotProductSafe(
									normal_worldspace, palm_normal_worldspace));
							}

							// entered sphere
							if (dist <= sphere->squared_radius && angle <= sphere->max_angle &&
								!sphere->overlap_state[isLeft])
							{
								sphere->overlap_state[isLeft] = true;
								changed = SendEvent(*sphere, true, isLeft);
							}  // use hysteresis on exit threshold
							else if ((dist > sphere->squared_radius + kHysteresis ||
										 angle > sphere->max_angle + kHysteresisAngular) &&
									 sphere->overlap_state[isLeft])
							{
								sphere->overlap_state[isLeft] = false;
								changed = SendEvent(*sphere, false, isLeft);
							}

							// Reflect both collision states in sphere color
							if (changed && draw_spheres && sphere->visible_debug_sphere)
							{
								changed = false;

								if (auto visible_node = sphere->visible_debug_sphere->Get3D())
								{
									if (sphere->overlap_state[0] || sphere->overlap_state[1])
									{
										helper::SetGlowColor(visible_node, on);
									} else if (!sphere->overlap_state[0] &&
											   !sphere->overlap_state[1])
									{
										helper::SetGlowColor(visible_node, off_);
									}
								}
							}
						}
					}
				} else
				{  // expired ptr
				}
			}
			auto end = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		}
		return 0;
	}

	void OverlapSphereManager::ShowDebugSpheres()
	{
		if (!draw_spheres)
		{
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

			std::scoped_lock lock(process_lock);
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
			t.rotate = helper::RotateBetweenVectors(NiPoint3(1.0, 0, 0), a_s.normal);
		}
		if (auto attach = player->Get3D(false)->GetObjectByName(a_s.attach_node->name))
		{
			return ArtAddon::Make(kModelPath, player->AsReference(), attach, t);
		} else
		{
			return nullptr;
		}
	}

	int OverlapSphereManager::GetNextId() { return nextId++; }

	bool OverlapSphereManager::SendEvent(const OverlapSphere& a_s, bool a_entered, bool isLeft)
	{
		if (a_s.callback)
		{
			a_s.callback(OverlapEvent{ .entered = a_entered, .isLeft = isLeft, .id = a_s.id });
		}
		return true;
	}

	OverlapSphereManager::OverlapSphereManager()
	{
		on = new NiColor(kOnHexColor);
		off_ = new NiColor(kOffHexColor);
	}

}