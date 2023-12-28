#include "artaddon.h"
#include "helper_game.h"
#undef PROFILING_EN

namespace helper
{
    ArtAddon::ArtAddon(const char* modelPath, TESObjectREFR* a_target, NiAVObject* a_attachNode, NiTransform& local)
    {
        SKSE::log::trace("Creating art addon with target: {}", a_target->GetName());
        AO = ArtAddonManager::GetSingleton()->GetArtForm(modelPath);
        if (AO && a_target && a_target->IsHandleValid())
        {
            /** Ostensibly this returns a ModelReferenceEffect handle for the model, but it seems to return 1 in all circumstances.
             * So instead we'll clone the created NiNode and then immediately delete the ModelReferenceEffect and the original NiNode
             * using the ProcessList. This is for the best as ModelReferenceEffects that reference temporary BGSArtObject forms become
             * baked into the save as null references that can't be deleted in any way. The Manager also deletes all added geometry and
             * effects on game save just to be safe. It's left up to the user to delete ArtAddon objects on game load.
             */
            a_target->ApplyArtObject(AO);

            desiredTransform = local;
            initialized = false;
            markedForUpdate = true;
            attachNode = a_attachNode;
            target = a_target;

            ArtAddonManager::GetSingleton()->addChild(this);
        }
        else
        {
            SKSE::log::error("Art addon failed: invalid target or model path");
            throw std::invalid_argument("invalid target or model path");
        }
    }

    ArtAddon::~ArtAddon()
    {
        SKSE::log::trace("destroying art addon with target: {}", this->target->GetName());
        // delete the NiNodes. if we haven't gotten a pointer to the ninode yet (it usually takes 1-2 frames), the manager will catch the orphaned node and delete it
        if (initialized && root3D && attachNode && target && target->IsHandleValid())
        {
            SKSE::log::trace("detach child");
            attachNode->AsNode()->DetachChild(root3D);
        }
        else
        {
            // add parameters to list of in-progress ModelReferenceEffects to delete
            ArtAddonManager::GetSingleton()->cancel.push_back(ArtAddonManager::toCancel(AO, target));
        }
        ArtAddonManager::GetSingleton()->removeChild(this);
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
        for (auto a : virtualObjects)
        {
            // initialize
            if (!a->initialized)
            {
                SKSE::log::debug("update artaddon {}: checking for model", (void*)a);
                if (const auto processLists = RE::ProcessLists::GetSingleton())
                {
                    // find a modelEffect that matches this object's target and art
                    processLists->ForEachModelEffect([a](RE::ModelReferenceEffect& a_modelEffect)
                        {
                            if (a_modelEffect.artObject == a->AO && a_modelEffect.target.get()->AsReference() == a->target && a_modelEffect.Get3D())
                            {
                                SKSE::log::debug("update artaddon {}: cloning", (void*)a);
                                a->root3D = a_modelEffect.Get3D()->Clone();
                                SKSE::log::debug("attaching");
                                a->attachNode->AsNode()->AttachChild(a->root3D);
                                SKSE::log::debug("deleting modeleffect");
                                a_modelEffect.Detach();

                                NiUpdateData ctx;
                                SKSE::log::debug("update transform");
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
        }
        postSaveGameUpdate = false;

        // cleanup orphaned models
        if (!cancel.empty())
        {
            SKSE::log::debug("deleting orphan ModelReferenceEffects");
            if (const auto processLists = RE::ProcessLists::GetSingleton())
            {
                processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect)
                    {
                        for (auto it = cancel.rbegin(); it != cancel.rend(); ++it)
                        {
                            if (a_modelEffect.target.get()->AsReference() == it->t
                                && a_modelEffect.artObject == it->a)
                            {
                                a_modelEffect.Detach();
                                cancel.pop_back();
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
        for (auto a : virtualObjects)
        {
            SKSE::log::debug("presave: addon found {}", (void*)a->root3D);
            if (a->initialized && a->root3D)
            {
                SKSE::log::debug("presave: deleting");
                a->attachNode->AsNode()->DetachChild(a->root3D);
                a->initialized = false;
                a->root3D = nullptr;
            }
        }

        // delete any hanging ModelReferenceEffects that use temporary forms we created
        if (const auto processLists = RE::ProcessLists::GetSingleton())
        {
            processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect)
                {
                    if (std::any_of(AOcache.begin(), AOcache.end(), [&](const auto& pair)
                        {
                            return pair.second == a_modelEffect.artObject;
                        }))
                    {
                        a_modelEffect.Detach();
                    }
                    return RE::BSContainer::ForEachResult::kContinue;
                });
        }

        postSaveGameUpdate = true;
    }

    /** only called by ArtAddon ctor */
    void ArtAddonManager::addChild(ArtAddon* a)
    {
        virtualObjects.push_back(a);
    }

    /** only called by ArtAddon dtor */
    void ArtAddonManager::removeChild(ArtAddon* a)
    {
        std::erase_if(virtualObjects, [a](const auto entry)
            {
                SKSE::log::debug("destroying virtual object {}", (void*)a);
                return entry == a;
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
                }
            }
            else
            {
                return nullptr;
            }
        }
        return AOcache[modelPath];
    }
}
