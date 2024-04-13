#include "holster.h"

void wearable::Quiver::OverlapHandler(const vrinput::OverlapEvent& e) {}

void wearable::BoltQuiver::OverlapHandler(const vrinput::OverlapEvent& e)
{
	if (e.entered)
	{
		vrinput::AddCallback(
			EquipButtonHandler, interact_button, e.triggered_by, vrinput::ActionType::kPress);
	}
	else
	{
		vrinput::RemoveCallback(
			EquipButtonHandler, interact_button, e.triggered_by, vrinput::ActionType::kPress);
	}
}

bool wearable::BoltQuiver::EquipButtonHandler(const vrinput::ModInputEvent& e)
{
	// need to look up which Wearable is actually being touched
	auto mgr = WearableManager::GetSingleton();
	if (auto sp = mgr->GetSelectedItem().lock())
	{
		auto derived = std::dynamic_pointer_cast<BoltQuiver>(sp);
		derived->EquipButtonHandler_impl(e);
	}

	// always block the input
	return true;
}

void wearable::BoltQuiver::EquipButtonHandler_impl(const vrinput::ModInputEvent& e)
{
	// TODO check if a crossbow is equipped (user setting?)

	if (g_higgsInterface->CanGrabObject((bool)e.device))
	{
		SKSE::log::info("grab ok");
		// TODO ammo selection. for now just use what the player has equipped
		auto ammoToGrab = RE::PlayerCharacter::GetSingleton()->GetCurrentAmmo();
		if (ammoToGrab)
		{
			SKSE::log::info("ammo ok");
			// Drop one of the item
			RE::NiPoint3 droploc = vrinput::GetHandNode(e.device, false)->world.translate;

			if (auto droppedHandle = RE::PlayerCharacter::GetSingleton()->RemoveItem(
					ammoToGrab, 1, RE::ITEM_REMOVE_REASON::kDropping, nullptr, nullptr, &droploc))
			{
				// Grab it with HIGGS
				SKSE::log::info("dropped ammo");
				g_higgsInterface->GrabObject(droppedHandle.get().get(), (bool)e.device);
			}
		}
		else { SKSE::log::info("ammo bad"); }
	}
	else { SKSE::log::info("grab bad"); }
}
