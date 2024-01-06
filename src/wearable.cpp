#include "wearable.h"

namespace wearable
{
	using namespace RE;
	using namespace art_addon;
	using namespace vrinput;

	bool WearableManager::WearableGrabHandler()
	{
		auto manager = WearableManager::GetSingleton();

		if (manager->GetState() == DressingState::kNone)
		{
			
			static auto last_tap_time = std::chrono::steady_clock::now();
SKSE::log::trace("grab button ");
			auto tap_diff = std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::steady_clock::now() - last_tap_time);

			if (tap_diff.count() < manager->config.enter_dressing_tap_delay_max &&
				tap_diff.count() > manager->config.enter_dressing_tap_delay_min)
			{
				manager->dressingmode_selected_index =
					manager->FindWearableWithOverlapID(manager->dressingmode_hovered_sphere_id);
				manager->TransitionState(DressingState::kWait);
			}
		} else
		{
			manager->dressingmode_selected_index =
					manager->FindWearableWithOverlapID(manager->dressingmode_hovered_sphere_id);
		}
		return false;
	}

	void WearableManager::WearableOverlapHandler(const vrinput::OverlapEvent& e)
	{
		auto manager = WearableManager::GetSingleton();
		if (e.entered)
		{
			manager->dressingmode_hovered_sphere_id = e.id;
			SKSE::log::trace("overlap enter");

			vrinput::AddCallback(manager->config.enter_dressing_mode, WearableGrabHandler,
				e.isLeft, Press, ButtonDown);
		} else
		{
			vrinput::RemoveCallback(manager->config.enter_dressing_mode, WearableGrabHandler,
				e.isLeft, Press, ButtonDown);
		}
	}

	void WearableManager::Update()
	{
		if (!managed_objects.empty())
		{
			std::scoped_lock lock(managed_objects_lock);
			if (dressingmode_state != DressingState::kNone &&
				dressingmode_state != DressingState::kWait)
			{
				if (dressingmode_selected_index >= managed_objects.size())
				{
					dressingmode_selected_index = 0;
				}
				if (auto sp = managed_objects[dressingmode_selected_index].lock())
				{
					if (auto node = sp->model->Get3D())
					{
						switch (dressingmode_state)
						{
						case DressingState::kTranslate:
							break;
						case DressingState::kScale:
							break;
						case DressingState::kColor:
							break;
						case DressingState::kCycleAV:
							break;
						}
					}
				} else
				{
					managed_objects.erase(managed_objects.begin() + dressingmode_selected_index);
				}
			} else
			{
				for (auto it = managed_objects.begin(); it != managed_objects.end();)
				{
					if (auto sp = it->lock())
					{
						if (sp->model->Get3D())
						{
							if (sp->interactable)
							{
								sp->Update();
							} else
							{
								sp->interactable =
									OverlapSphere::Make(sp->model->Get3D(), WearableOverlapHandler,
										config.overlap_radius, config.overlap_angle);
							}
						}
						it++;
					} else
					{
						it = managed_objects.erase(it);
					}
				}
			}
		}
	}

	void WearableManager::TransitionState(DressingState a_next_state)
	{
		if (a_next_state != dressingmode_state)
		{
			// exit state actions
			switch (dressingmode_state)
			{
			case DressingState::kNone:
				vrinput::StartBlockingAll();
			}
			// enter state actions
			switch (a_next_state)
			{
			case DressingState::kNone:
				vrinput::StopBlockingAll();
			}
		}
	}

	void WearableManager::Register(std::weak_ptr<Wearable> a_obj)
	{
		std::scoped_lock lock(managed_objects_lock);
		managed_objects.push_back(a_obj);
	}

	int WearableManager::FindWearableWithOverlapID(int a_id)
	{
		std::scoped_lock lock(managed_objects_lock);

		auto it =
			std::find_if(managed_objects.begin(), managed_objects.end(), [a_id](const auto& obj) {
				return obj.expired() ? false : obj.lock()->interactable->GetId() == a_id;
			});
		if (it == managed_objects.end())
		{
			return 0;
		} else
		{
			return managed_objects.begin() - it;
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
