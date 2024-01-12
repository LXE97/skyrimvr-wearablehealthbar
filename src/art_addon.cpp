#include "art_addon.h"

#include "helper_game.h"

namespace art_addon
{
	using namespace RE;

	std::shared_ptr<ArtAddon> ArtAddon::Make(const char* a_model_path, TESObjectREFR* a_target,
		NiAVObject* a_attach_node, NiTransform& a_local)
	{
		auto manager = ArtAddonManager::GetSingleton();
		auto art_object = manager->GetArtForm(a_model_path);
		int  id = manager->GetNextId();

		std::scoped_lock lock(manager->objects_lock);

		/** Using the duration parameter of the BSTempEffect as an ID because anything < 0 has the
		 * same effect. If another mod happens to use this library there will be ID collisions but
		 * they are still distinguished by the unique BGSArtObject property. */
		if (art_object && a_attach_node && a_target && a_target->IsHandleValid() &&
			a_target->ApplyArtObject(art_object, (float)id))
		{
			auto new_obj = std::shared_ptr<ArtAddon>(new ArtAddon);
			new_obj->art_object = art_object;
			new_obj->local = a_local;
			new_obj->attach_node = a_attach_node;
			new_obj->target = a_target;

			manager->new_objects.emplace(id, new_obj);
			return new_obj;
		}
		else
		{
			SKSE::log::error("Art addon failed: invalid target or model path");
			return nullptr;
		}
	}

	/** Ostensibly ApplyArtObject() returns a ModelReferenceEffect handle for the model, but
	 * it seems to return 1 in all circumstances. So instead we'll clone the created NiNode
	 * and then delete the ModelReferenceEffect and the original NiNode using the ProcessList.
	 */
	void ArtAddonManager::Update()
	{
		if (!new_objects.empty())
		{
			std::scoped_lock lock(objects_lock);
			if (const auto processLists = ProcessLists::GetSingleton())
			{
				processLists->ForEachModelEffect([this](ModelReferenceEffect& a_modelEffect) {
					if (a_modelEffect.Get3D() && a_modelEffect.lifetime < -1.0f)
					{
						int id = a_modelEffect.lifetime;
						if (new_objects.contains(id))
						{
							if (auto addon = new_objects[id].lock())
							{
								// the id is not unique to this mod but the ArtObject is
								if (addon->art_object == a_modelEffect.artObject)
								{
									addon->root3D = a_modelEffect.Get3D()->Clone();
									addon->attach_node->AsNode()->AttachChild(addon->root3D);
									a_modelEffect.lifetime = 0;
									addon->root3D->local = std::move(addon->local);
									addon->OnModelCreation();
								}
							}
							else
							{  // check if it's one of our ArtObjects
								auto it = std::find_if(artobject_cache.begin(),
									artobject_cache.end(), [&a_modelEffect](const auto& pair) {
										return pair.second == a_modelEffect.artObject;
									});
								if (it != artobject_cache.end())
								{  // the artAddon was deleted before initialization finished
									a_modelEffect.lifetime = 0;
									SKSE::log::trace("deleting MRE {} (orphaned)", id);
								}
							}
							// finished with this ArtAddon, no longer need to track it
							new_objects.erase(id);
						}
					}
					return BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	BGSArtObject* ArtAddonManager::GetArtForm(const char* modelPath)
	{
		if (!artobject_cache.contains(modelPath))
		{
			if (base_artobject)
			{
				if (auto dupe = base_artobject->CreateDuplicateForm(false, nullptr))
				{
					auto temp = dupe->As<BGSArtObject>();
					temp->SetModel(modelPath);
					artobject_cache[modelPath] = temp;
					return temp;
				}
			}
			else { return nullptr; }
		}
		return artobject_cache[modelPath];
	}

	int ArtAddonManager::GetNextId() { return next_id--; }

	ArtAddonManager::ArtAddonManager()
	{
		constexpr FormID kBaseArtobjectId = 0x9405f;

		base_artobject = TESForm::LookupByID(kBaseArtobjectId)->As<BGSArtObject>();
	}

	ArtAddonPtr NifChar::Make(char a_ascii, NiAVObject* a_parent, NiTransform& a_local)
	{
		auto           manager = ArtAddonManager::GetSingleton();
		auto           art_object = manager->GetArtForm(kFontModelPath);
		int            id = manager->GetNextId();
		TESObjectREFR* a_target = PlayerCharacter::GetSingleton()->AsReference();

		std::scoped_lock lock(manager->objects_lock);

		/** Using the duration parameter of the BSTempEffect as an ID because anything < 0 has the
		 * same effect. If another mod happens to use this library there will be ID collisions but
		 * they are still distinguished by the unique BGSArtObject property. */
		if (art_object && a_parent && a_target && a_target->IsHandleValid() &&
			a_target->ApplyArtObject(art_object, (float)id))
		{
			auto new_obj = std::shared_ptr<NifChar>(new NifChar);
			new_obj->art_object = art_object;
			new_obj->local = a_local;
			new_obj->attach_node = a_parent;
			new_obj->target = a_target;
			new_obj->ascii = a_ascii;

			manager->new_objects.emplace(id, new_obj);
			return new_obj;
		}
		else
		{
			SKSE::log::error("Art addon failed: invalid target or model path");
			return nullptr;
		}
	}

	void NifChar::OnModelCreation()
	{
		if (auto geo_node = root3D->GetObjectByName("char"))
		{
			if (auto geometry = geo_node->AsGeometry())
			{
				if (auto property = geometry->properties[BSGeometry::States::kEffect].get())
				{
					if (auto shader = netimmerse_cast<RE::BSShaderProperty*>(property))
					{
						if (auto material = shader->material)
						{
							auto temp = AsciiToXY(ascii);
							material->texCoordOffset[0].x = temp.x;
							material->texCoordOffset[0].y = temp.y;
							material->texCoordOffset[1].x =  temp.x;
							material->texCoordOffset[1].y = temp.y;
						}
					}
				}
			}
		}
	}
}
