#include "overlap_sphere.h"

namespace vrinput
{
	using namespace RE;

	const bool OverlapSphere::is_overlapping(bool isLeft) { return overlap_state_[isLeft]; };

	OverlapSphere::OverlapSphere(const char* node_name, OverlapCallback callback, float radius,
		float max_angle_deg, const RE::NiPoint3& offset, const RE::NiPoint3& normal)
		: attach_node_name_(node_name), callback_(callback), local_position_(offset),
		  squared_radius_(radius * radius), max_angle_(max_angle_deg), normal_(normal)
	{
		SKSE::log::trace("constructing overlap: {}", (void*)this);
		OverlapSphereManager::GetSingleton()->Register(this);
	}

	OverlapSphere::~OverlapSphere()
	{
		SKSE::log::trace("destroying overlap: {}", (void*)this);
		OverlapSphereManager::GetSingleton()->UnRegister(this);
	}

	void OverlapSphereManager::Update()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		if (player->Get3D(true))
		{
			// with VRIK, use 1st person nodes for collision detection
			NiAVObject* controllers[2]{ player->Get3D(true)->GetObjectByName(
											kControllerNodeName[0]),
				player->Get3D(true)->GetObjectByName(kControllerNodeName[1]) };

			NiPoint3         sphere_world;
			std::scoped_lock lock(vector_lock_);
			for (auto& sphere : process_list_)
			{
				if (auto node = player->Get3D(true)->GetObjectByName(sphere->attach_node_name_))
				{
					if (sphere->local_position_ != NiPoint3::Zero())
					{
						sphere_world =
							node->world.translate + node->world.rotate * sphere->local_position_;
					} else
					{
						sphere_world = node->world.translate;
					}

					// Test collision.
					bool changed = false;
					for (bool isLeft : { false, true })
					{
						float dist = sphere_world.GetSquaredDistance(
							controllers[isLeft]->world.translate +
							controllers[isLeft]->world.rotate * palm_offset_);

						float angle = 0.0f;
						if (sphere->normal_ != NiPoint3::Zero() && sphere->max_angle_)
						{
							auto normal_worldspace =
								helper::VectorNormalized(node->world.rotate * sphere->normal_);
							auto palm_normal_worldspace = helper::VectorNormalized(
								controllers[isLeft]->world.rotate * kHandPalmNormal);
							angle = std::acos(
								helper::DotProductSafe(normal_worldspace, palm_normal_worldspace));
						}

						// entered sphere
						if (dist <= sphere->squared_radius_ && angle <= sphere->max_angle_ &&
							!sphere->overlap_state_[isLeft])
						{
							changed = SendEvent(sphere, true, isLeft);
						}  // use hysteresis on exit threshold
						else if ((dist > sphere->squared_radius_ + kHysteresis ||
									 angle > sphere->max_angle_ + kHysteresisAngular) &&
								 sphere->overlap_state_[isLeft])
						{
							changed = SendEvent(sphere, false, isLeft);
						}

						// Reflect both collision states in sphere color
						if (changed && draw_spheres_ && sphere->visible_debug_sphere_)
						{
							changed = false;

							if (auto visible_node = sphere->visible_debug_sphere_->Get3D())
							{
								if (sphere->overlap_state_[0] || sphere->overlap_state_[1])
								{
									helper::SetGlowColor(visible_node, on_);
								} else if (!sphere->overlap_state_[0] && !sphere->overlap_state_[1])
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

	void OverlapSphereManager::Reset()
	{
		std::scoped_lock lock(vector_lock_);
		process_list_.clear();
	}

	void OverlapSphereManager::ShowDebugSpheres()
	{
		if (!draw_spheres_)
		{
			auto        player = PlayerCharacter::GetSingleton();
			NiTransform t;
			t.translate = palm_offset_;
			t.scale = kControllerDebugModelScale;
			t.rotate = helper::RotateBetweenVectors(NiPoint3(1.0, 0, 0), kHandPalmNormal);
			for (auto isLeft : { false, true })
			{
				controller_debug_models_[isLeft] =
					helper::ArtAddon::Create("debug/debugsphere.nif", player->AsReference(),
						player->Get3D(false)->GetObjectByName(kControllerNodeName[isLeft]), t);
			}

			std::scoped_lock lock(vector_lock_);
			for (auto s : process_list_)
			{
				s->visible_debug_sphere_ = CreateDebugSphere(s);
			}

			draw_spheres_ = true;
		}
	}

	void OverlapSphereManager::HideDebugSpheres()
	{
		std::scoped_lock lock(vector_lock_);
		if (draw_spheres_)
		{
			for (auto isLeft : { false, true })
			{
				controller_debug_models_[isLeft].reset();
			}
			for (auto& s : process_list_)
			{
				if (s->visible_debug_sphere_)
				{
					s->visible_debug_sphere_.reset();
				}
			}
			draw_spheres_ = false;
		}
	}

	void OverlapSphereManager::set_palm_offset(RE::NiPoint3& offset) { palm_offset_ = offset; }

	const void OverlapSphereManager::Register(OverlapSphere* s)
	{
		std::scoped_lock lock(vector_lock_);
		process_list_.push_back(s);
		if (draw_spheres_)
		{
			CreateDebugSphere(s);
		}
	}

	const void OverlapSphereManager::UnRegister(OverlapSphere* s)
	{
		std::scoped_lock lock(vector_lock_);
		std::erase_if(process_list_, [s](const auto entry) { return entry == s; });
		SKSE::log::trace("		destroyed overlap: {}", (void*)s);
	}

	std::shared_ptr<helper::ArtAddon> OverlapSphereManager::CreateDebugSphere(OverlapSphere* s)
	{
		auto        player = PlayerCharacter::GetSingleton();
		NiTransform t;
		t.translate = s->local_position_;
		t.scale = std::sqrt(s->squared_radius_);
		if (s->normal_ != NiPoint3::Zero())
		{
			t.rotate = helper::RotateBetweenVectors(NiPoint3(1.0, 0, 0), s->normal_);
		}
		return helper::ArtAddon::Create(kDebugModelPath, player->AsReference(),
			player->Get3D(false)->GetObjectByName(s->attach_node_name_), t);
	}

	bool OverlapSphereManager::SendEvent(OverlapSphere* s, bool entered, bool isLeft)
	{
		s->overlap_state_[isLeft] = entered;
		if (s->callback_)
		{
			s->callback_(OverlapEvent{ .entered = entered, .isLeft = isLeft });
		}
		return true;
	}

	OverlapSphereManager::OverlapSphereManager()
	{
		on_ = new NiColor(kOnHexColor);
		off_ = new NiColor(kOffHexColor);
	}

}