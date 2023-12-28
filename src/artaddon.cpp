#include "artaddon.h"
#include "helper_game.h"
#define PROFILING_EN

namespace helper
{
    ArtAddon::ArtAddon(const char* modelPath, TESObjectREFR* a_target, NiAVObject* a_attachNode, NiTransform& local)
    {
        AO = ArtAddonManager::GetSingleton()->GetArtForm(modelPath);
        if (AO && a_target && a_target->IsHandleValid())
        {
            /** Ostensibly this returns a ModelReferenceEffect handle for the model, but it seems to return 1 in all circumstances.
             * So instead we'll attach the created NiNode to the target node manually and track it with a NiNode pointer on the next
             * update, which decouples it from the lifespan of the real ModelReferenceEffect in the ProcessList
             */
            a_target->ApplyArtObject(AO);

            desiredTransform = local;
            initialized = false;
            markedForUpdate = true;
            attachNode = a_attachNode;
            target = a_target;
            ArtAddonManager::GetSingleton()->virtualObjects.push_back(this);
        }
        else
        {
            SKSE::log::error("Art addon failed: invalid target or model path");
        }
    }

    ArtAddon::~ArtAddon()
    {
        // delete the NiNodes. if we haven't gotten a pointer to the ninode yet, the manager will catch the orphaned node and delete it
        if (initialized && root3D && attachNode && target && target->IsHandleValid())
        {
            attachNode->AsNode()->DetachChild(root3D);
        }
        else
        {
            ArtAddonManager::GetSingleton()->cancel.push_back(ArtAddonManager::toCancel(AO, target));
        }
        // delete the manager's entry
        std::erase_if(ArtAddonManager::GetSingleton()->virtualObjects, [this](const auto a)
            {
                return a == this;
            });
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
#ifdef PROFILING_EN
        float start = helper::GetQPC();
#endif

        for (auto a : virtualObjects)
        {
            // initialize
            if (!a->initialized)
            {
                if (const auto processLists = RE::ProcessLists::GetSingleton())
                {
                    processLists->ForEachModelEffect([a](RE::ModelReferenceEffect& a_modelEffect)
                        {
                            if (a_modelEffect.artObject)
                            {
                                SKSE::log::debug("initializing");
                                //auto node = a_modelEffect.Get3D();
                                //a->attachNode->AsNode()->AttachChild(node); // TODO - see if this crashes. check if ModelReferenceEffect realy is 0x1

                                //a->markedForUpdate = false;
                                //a_modelEffect.Detach();
                            }
                            return RE::BSContainer::ForEachResult::kContinue;
                        });
                }
            }
            // update
            else if (a->markedForUpdate)
            {
                if (auto n = a->Get3D())
                {
                    a->markedForUpdate = false;
                    NiUpdateData ctx;
                    n->local = a->desiredTransform;
                    n->Update(ctx);
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
            if (const auto processLists = RE::ProcessLists::GetSingleton())
            {
                processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect)
                    {
                        for (auto it = cancel.rbegin(); it != cancel.rend(); ++it)
                        {
                            if (a_modelEffect.target.get()->AsReference() == it->t
                            && a_modelEffect.artObject == it->a)
                            //a_modelEffect.Detach();
                            cancel.pop_back();
                        }
                        return RE::BSContainer::ForEachResult::kContinue;
                    });
            }
        }
#ifdef PROFILING_EN
        float end = helper::GetQPC();
        SKSE::log::debug("update time: {}", end - start);
#endif
    }

    /** Must be called when the game is saved. */
    void ArtAddonManager::PreSaveGame()
    {
        for (auto a : virtualObjects)
        {

        }
        postSaveGameUpdate = true;
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
