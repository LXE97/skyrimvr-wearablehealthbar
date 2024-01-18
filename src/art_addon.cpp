#include "art_addon.h"

#include "helper_game.h"

namespace art_addon
{
	using namespace RE;

	std::shared_ptr<ArtAddon> ArtAddon::Make(const char* a_model_path, TESObjectREFR* a_target,
		NiAVObject* a_attach_node, NiTransform& a_local, std::function<void(ArtAddon*)> a_callback)
	{
		auto manager = ArtAddonManager::GetSingleton();
		auto art_object = manager->GetArtForm(a_model_path);
		int  id = manager->GetNextId();
		//std::scoped_lock lock(manager->objects_lock);

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
			if (a_callback) { new_obj->callback = a_callback; }

			manager->new_objects.emplace(id, new_obj);
			return new_obj;
		}
		else
		{
			SKSE::log::error("Art addon failed: invalid target or model path");
			return nullptr;
		}
	}

	/** clone the created NiNode and then delete the ModelReferenceEffect and the original NiNode
	 *  using the ProcessList. */
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
									if (addon->callback) { addon->callback(addon.get()); }
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

	BGSArtObject* ArtAddonManager::GetArtForm(const char* a_modelPath)
	{
		if (!artobject_cache.contains(a_modelPath))
		{
			if (base_artobject)
			{
				if (auto dupe = base_artobject->CreateDuplicateForm(false, nullptr))
				{
					auto temp = dupe->As<BGSArtObject>();
					temp->SetModel(a_modelPath);
					artobject_cache[a_modelPath] = temp;
					return temp;
				}
			}
			else { return nullptr; }
		}
		return artobject_cache[a_modelPath];
	}

	int ArtAddonManager::GetNextId() { return next_id--; }

	ArtAddonManager::ArtAddonManager()
	{
		constexpr FormID kBaseArtobjectId = 0x9405f;

		base_artobject = TESForm::LookupByID(kBaseArtobjectId)->As<BGSArtObject>();
	}

	NifChar::NifChar(char a_ascii, RE::NiAVObject* a_parent, RE::NiTransform& a_local) :
		ascii(a_ascii)
	{
		artaddon = ArtAddon::Make(kFontModelPath, PlayerCharacter::GetSingleton()->AsReference(),
			a_parent, a_local, std::bind(&NifChar::OnModelCreation, this));
	}

	void NifChar::OnModelCreation()
	{
		if (auto shader = helper::GetShaderProperty(artaddon->Get3D(), "char"))
		{
			auto oldmat = shader->material;
			auto newmat = oldmat->Create();
			newmat->CopyMembers(oldmat);
			shader->material = newmat;
			newmat->IncRef();
			oldmat->DecRef();
			// TODO: see if this new material decref in dtor

			auto temp = AsciiToXY(ascii);

			newmat->texCoordOffset[0].x = temp.x;
			newmat->texCoordOffset[0].y = temp.y;
			newmat->texCoordOffset[1].x = temp.x;
			newmat->texCoordOffset[1].y = temp.y;
		}
	}

	AddonTextBox::AddonTextBox(const char* a_string, const float a_spacing,
		RE::NiAVObject* a_attach_to, RE::NiTransform& a_local) :
		string(a_string),
		spacing(a_spacing)
	{
		root =
			ArtAddon::Make(NifChar::kFontModelPath, PlayerCharacter::GetSingleton()->AsReference(),
				a_attach_to, a_local, std::bind(&AddonTextBox::MakeString, this));
	}

	void AddonTextBox::MakeString()
	{
		if (auto root_node = root->Get3D())
		{
			NiTransform t;
			for (int i = 0; string[i] != '\0'; i++)
			{
				characters.push_back(std::make_unique<NifChar>(string[i], root_node, t));
				t.translate.y += NifChar::kCharacterWidth + spacing;
			}
			root_node->local.rotate;
		}
	}
}
