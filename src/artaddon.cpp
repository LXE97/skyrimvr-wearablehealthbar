#include "artaddon.h"
#include "helper_game.h"
#undef PROFILING_EN

namespace helper
{
	ArtAddon::~ArtAddon()
	{
		SKSE::log::trace("destroying art addon {}", (void*)this);
		// delete the NiNodes. if we haven't gotten a pointer to the ninode yet (it usually takes 1-2 frames), the manager will catch the orphaned node and delete it
		if (root3D && attachNode && target && target->IsHandleValid())
		{
			attachNode->AsNode()->DetachChild(root3D);
		} else
		{
			// add parameters to list of in-progress ModelReferenceEffects to delete
			ArtAddonManager::GetSingleton()->cancel.push_back(
				ArtAddonManager::toCancel{ .a = AO, .t = target });
		}
		ArtAddonManager::GetSingleton()->Deregister(weak_from_this());
		SKSE::log::trace("		destroyed {}", (void*)this);
	}

	std::shared_ptr<ArtAddon> ArtAddon::Create(const char* modelPath, TESObjectREFR* a_target,
		NiAVObject* a_attachNode, NiTransform& local)
	{
		auto new_obj = std::shared_ptr<ArtAddon>(new ArtAddon());
		SKSE::log::trace("constructing art addon {}", (void*)new_obj.get());
		auto AO = ArtAddonManager::GetSingleton()->GetArtForm(modelPath);
		if (AO && a_target && a_target->IsHandleValid() && a_attachNode)
		{
			/** Ostensibly this returns a ModelReferenceEffect handle for the model, but it seems to return 1 in all circumstances.
             * So instead we'll clone the created NiNode and then immediately delete the ModelReferenceEffect and the original NiNode
             * using the ProcessList. This is for the best as ModelReferenceEffects that reference temporary BGSArtObject forms become
             * baked into the save as null references that can't be deleted in any way. The Manager also deletes all added geometry and
             * effects on game save just to be safe. It's left up to the user to delete ArtAddon objects on game load.
             */
			a_target->ApplyArtObject(AO);

			new_obj->AO = AO;
			new_obj->desiredTransform = local;
			new_obj->initialized = false;
			new_obj->markedForUpdate = true;
			new_obj->attachNode = a_attachNode;
			new_obj->target = a_target;

			ArtAddonManager::GetSingleton()->Register(new_obj->weak_from_this());
		} else
		{
			SKSE::log::error("Art addon failed: invalid target or model path");
		}
		return new_obj;
	}

	void ArtAddon::SetTransform(NiTransform& local)
	{
		desiredTransform = local;
		markedForUpdate = true;
	}

	NiAVObject* ArtAddon::Get3D()
	{
		if (initialized && target->IsHandleValid())
		{
			return root3D;
		}
		return nullptr;
	}

	/** Must be called by some periodic function in the plugin file, e.g. PlayerCharacter::Update */
	void ArtAddonManager::Update()
	{
		std::scoped_lock lock(vector_lock_);
		auto             it = process_list_.begin();
		while (it != process_list_.end())
		{
			if (auto a = it->lock())
			{
				// initialize
				if (!a->initialized)
				{
					SKSE::log::debug("update artaddon {}: checking for model", (void*)a.get());
					if (const auto processLists = RE::ProcessLists::GetSingleton())
					{
						// find a modelEffect that matches this object's target and art
						processLists->ForEachModelEffect(
							[a](RE::ModelReferenceEffect& a_modelEffect) {
								if (a_modelEffect.artObject == a->AO &&
									a_modelEffect.target.get()->AsReference() == a->target &&
									a_modelEffect.Get3D())
								{
									SKSE::log::debug("update artaddon {}: cloning", (void*)a.get());
									a->root3D = a_modelEffect.Get3D()->Clone();
									a->attachNode->AsNode()->AttachChild(a->root3D);
									a_modelEffect.Detach();

									NiUpdateData ctx;
									a->root3D->local = a->desiredTransform;
									a->root3D->Update(ctx);
									a->markedForUpdate = false;
									a->initialized = true;

									return RE::BSContainer::ForEachResult::kStop;
								}
								return RE::BSContainer::ForEachResult::kContinue;
							});
					}
				}
				// update
				else if (a->markedForUpdate)
				{
					SKSE::log::debug("update artaddon {}:", (void*)a.get());
					if (a->root3D)
					{
						a->markedForUpdate = false;
						NiUpdateData ctx;
						a->root3D->local = a->desiredTransform;
						a->root3D->Update(ctx);
					}
				}
				// refresh after game save
				if (postSaveGameUpdate)
				{
					a->target->ApplyArtObject(a->AO);
					a->initialized = false;
				}

				++it;
			} else
			{
				SKSE::log::debug("expired ptr, deregistering artaddon {}", (void*)&(*it));
				it = process_list_.erase(it);
			}
		}
		postSaveGameUpdate = false;

		// cleanup orphaned models
		if (!cancel.empty())
		{
			SKSE::log::debug("deleting orphan ModelReferenceEffects {}", cancel.size());

			helper::PrintPlayerModelEffects();
			if (const auto processLists = RE::ProcessLists::GetSingleton())
			{
				processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect) {
					auto it = cancel.begin();
					while (it != cancel.end())
					{
						if (a_modelEffect.target.get()->AsReference() == it->t &&
							a_modelEffect.artObject == it->a)
						{
							SKSE::log::debug("deleting MRE {} with AO {}", (void*)&a_modelEffect, (void*)(a_modelEffect.artObject));
							a_modelEffect.Detach();
							it = cancel.erase(it);
						} else
						{
							++it;
						}
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
		}
	}

	/** Must be called when the game is saved.
     * Deletes all ModelReferenceEffects and geometry added by this mod and sets the flag to re-create them.
     */
	void ArtAddonManager::PreSaveGame()
	{
		std::scoped_lock lock(vector_lock_);
		for (const auto& wp : process_list_)
		{
			if (auto a = wp.lock())
			{
				SKSE::log::debug("presave: addon found {}", (void*)a->root3D);
				if (a->initialized && a->root3D)
				{
					SKSE::log::debug("		presave: deleting");
					a->attachNode->AsNode()->DetachChild(a->root3D);
					a->initialized = false;
					a->root3D = nullptr;
				}
			}
		}

		// delete any hanging ModelReferenceEffects that use temporary forms we created
		if (const auto processLists = RE::ProcessLists::GetSingleton())
		{
			processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect) {
				if (std::any_of(AOcache.begin(), AOcache.end(),
						[&](const auto& pair) { return pair.second == a_modelEffect.artObject; }))
				{
					a_modelEffect.Detach();
				}
				return RE::BSContainer::ForEachResult::kContinue;
			});
		}

		postSaveGameUpdate = true;
	}

	void ArtAddonManager::Register(const std::weak_ptr<ArtAddon>& a)
	{
		std::scoped_lock lock(vector_lock_);
		process_list_.push_back(a);
	}

	void ArtAddonManager::Deregister(const std::weak_ptr<ArtAddon>& a)
	{
		std::scoped_lock lock(vector_lock_);
		std::erase_if(process_list_, [a](const std::weak_ptr<ArtAddon>& wp) {
			return wp.expired() || wp.lock() == a.lock();
		});
	}

	ArtAddonManager::ArtAddonManager()
	{
		if (auto temp = TESForm::LookupByID(baseAOID))
		{
			baseAO = temp->As<BGSArtObject>();
		}
	}

	BGSArtObject* ArtAddonManager::GetArtForm(const char* modelPath)
	{
		if (!AOcache.contains(modelPath))
		{
			if (baseAO)
			{
				if (auto dupe = baseAO->CreateDuplicateForm(false, nullptr))
				{
					auto temp = dupe->As<BGSArtObject>();
					temp->SetModel(modelPath);
					AOcache[modelPath] = temp;
					SKSE::log::trace("Created new BGSArtObject: {}", (void*)temp);
				}
			} else
			{
				return nullptr;
			}
		}
		return AOcache[modelPath];
	}
}
