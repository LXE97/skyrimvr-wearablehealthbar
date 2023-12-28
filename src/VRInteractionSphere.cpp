#include "VRInteractionSphere.h"

namespace vrinput
{
    OverlapSphereManager::OverlapSphereManager()
    {
        //  generate a temporary Form to apply models to the player skeleton
        //  this doesn't get preserved in the savefile
        if (auto realForm = RE::TESForm::LookupByID(dummyArt)) {
            if (auto temp = realForm->CreateDuplicateForm(false, nullptr)) {
                DrawNodeArt = temp->As<RE::BGSArtObject>();
                if (DrawNodeArt)
                {
                    DrawNodeArt->SetModel(DrawNodeModelPath);
                }
                SKSE::log::debug("BGSArtObject created with formID: {:X}", temp->GetFormID());
            }
        }

        TURNON = new RE::NiColor(0x16ff75);
        TURNOFF = new RE::NiColor(0x00b6ff);
    }

    void OverlapSphereManager::SetOverlapEventHandler(OverlapCallback cb)
    {
        _cb = cb;
    }

    void OverlapSphereManager::Update()
    {
        auto player = RE::PlayerCharacter::GetSingleton();
        if (player->Get3D(false))
        {
            controllers[1] = player->GetVRNodeData()->NPCLHnd.get();
            controllers[0] = player->GetVRNodeData()->NPCRHnd.get();
            static int i = 1;
            static RE::NiPoint3 sphereWorld;
            static int debugcount = 0;
            for (auto& s : spheres)
            {
                if (auto sphereNode = player->Get3D(true)->GetObjectByName(s.attachNode))
                {
                    // Update virtual sphere
                    if (s.localPosition)
                    {
                        if (s.onlyHeading)
                        {
                            RE::NiPoint3 rotated = *(s.localPosition);
                            helper::RotateZ(rotated, sphereNode->world.rotate);
                            sphereWorld = sphereNode->world.translate + rotated;
                        }
                        else
                        {
                            sphereWorld = sphereNode->world.translate + sphereNode->world.rotate * *(s.localPosition);
                        }
                    }
                    else
                    {
                        sphereWorld = sphereNode->world.translate;
                    }

                    // Update visible sphere
                    RE::NiAVObject* sphereVisNode = nullptr;
                    if (DrawHolsters)
                    {
                        // look for initialized node
                        if (sphereVisNode = player->Get3D(false)->GetObjectByName(DrawNodeName + std::to_string(s.ID)))
                        {
                            if (s.onlyHeading)
                            {
                                RE::NiUpdateData ctx;
                                sphereVisNode->local.translate = sphereNode->world.Invert().rotate * (sphereWorld - sphereNode->world.translate);
                                sphereVisNode->Update(ctx);
                            }
                        }
                        // look for uninitialized node and assign it to this sphere
                        else if (sphereVisNode = sphereNode->GetObjectByName(DrawNodeInitialName))
                        {
                            InitializeVisible(s, sphereVisNode);
                        }
                    }

                    if (!s.debugOnly)
                    {
                        // Test sphere collision
                        bool changed = false;
                        for (bool isLeft : {false, true}) {

                            auto dist = sphereWorld.GetSquaredDistance(controllers[isLeft]->world.translate + controllers[isLeft]->world.rotate * palmoffset);
                            float angle = 0.0f;

                            if (s.normal)
                            {
                                auto sphereNorm = helper::VectorNormalized(sphereNode->world.rotate * *(s.normal));
                                auto palmNorm = helper::VectorNormalized(controllers[isLeft]->world.rotate * NPCHandPalmNormal);
                                angle = std::acos(helper::DotProductSafe(sphereNorm, palmNorm)); // both unit vectors
                            }

                            if (dist <= s.squaredRadius && angle <= s.maxAngle && !s.overlapState[isLeft])
                            {
                                SKSE::log::debug("VR Overlap event: {} hand entered sphere {}", isLeft ? "left" : "right", s.ID);
                                s.overlapState[isLeft] = true;
                                OverlapEvent e = OverlapEvent(s.ID, true, isLeft);
                                changed = true;
                                if (_cb)
                                {
                                    _cb(e);
                                }
                            }
                            else if ((dist > s.squaredRadius + hysteresis || angle > s.maxAngle + hysteresis_angular) && s.overlapState[isLeft])
                            {
                                SKSE::log::debug("VR Overlap event: {} hand exited sphere {}", isLeft ? "left" : "right", s.ID);
                                s.overlapState[isLeft] = false;
                                OverlapEvent e = OverlapEvent(s.ID, false, isLeft);
                                changed = true;
                                if (_cb)
                                {
                                    _cb(e);
                                }
                            }
                        }

                        // reflect both collision states in sphere color
                        if (changed && DrawHolsters && sphereVisNode)
                        {
                            if (s.overlapState[0] || s.overlapState[1])
                            {
                                SetGlowColor(sphereVisNode, TURNON);
                            }
                            else if (!s.overlapState[0] && !s.overlapState[1])
                            {
                                SetGlowColor(sphereVisNode, TURNOFF);
                            }
                        }
                    }
                }
            }
        }
    }

    void OverlapSphereManager::SetPalmOffset(RE::NiPoint3& offset) {
        palmoffset = offset;
    }

    int32_t OverlapSphereManager::Create(const char* attachNodeName, RE::NiPoint3* localPosition, float radius, RE::NiPoint3* normal, float maxAngle, bool onlyHeading, bool debugOnly)
    {
        spheres.push_back(OverlapSphere(attachNodeName, localPosition, radius, next_ID, normal, helper::deg2rad(maxAngle), onlyHeading, debugOnly));
        if (DrawHolsters)
        {
            AddVisibleHolster(spheres.back());
        }
        SKSE::log::debug("holster created with id {}", next_ID);
        return next_ID++;
    }

    void OverlapSphereManager::Destroy(int32_t targetID)
    {
        if (DrawHolsters)
        {
            DestroyVisibleHolster(targetID);
        }

        // delete virtual holster
        std::erase_if(spheres, [targetID](const OverlapSphere& x)
            { return (x.ID == targetID); });
    }

    void OverlapSphereManager::ShowHolsterSpheres()
    {
        if (!DrawHolsters)
        {
            for (auto& s : spheres)
            {
                AddVisibleHolster(s);
            }

            DrawHolsters = true;
        }
    }

    void OverlapSphereManager::HideHolsterSpheres()
    {
        if (DrawHolsters)
        {
            for (auto& s : spheres)
            {
                DestroyVisibleHolster(s.ID);
            }
            DrawHolsters = false;
        }
    }

    void OverlapSphereManager::AddVisibleHolster(OverlapSphere& s)
    {
        if (DrawNodeArt)
        {
            SKSE::log::trace("Adding visible node {}", s.ID);
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player)
            {
                // model takes some time to appear, so it's edited further in the main update loop
                if (s.onlyHeading)
                {// TODO: re-implement independent spheres
                    player->ApplyArtObject(DrawNodeArt, -1.0F, nullptr, false, false, player->Get3D(false));
                }
                else {
                    player->ApplyArtObject(DrawNodeArt, -1.0F, nullptr, false, false, player->Get3D(false)->GetObjectByName(s.attachNode));
                }
            }
        }
    }

    void OverlapSphereManager::DestroyVisibleHolster(int32_t id)
    {
        if (const auto processLists = RE::ProcessLists::GetSingleton())
        {
            processLists->ForEachModelEffect([&](RE::ModelReferenceEffect& a_modelEffect)
                {
                    if (a_modelEffect.target.get()->AsReference() == RE::PlayerCharacter::GetSingleton()->AsReference())
                    {
                        if (auto model = a_modelEffect.Get3D())
                        {
                            if (auto sdata = model->GetExtraData("source"))
                            {
                                auto s = static_cast<RE::NiStringExtraData*>(sdata);
                                if (std::strcmp(s->value, pluginName)) {
                                    if (auto idata = model->GetExtraData("id"))
                                    {
                                        auto i = static_cast<RE::NiIntegerExtraData*>(idata);
                                        if (i->value == id) {
                                            a_modelEffect.Detach();
                                        }
                                    }
                                }
                            }

                        }
                    }
                    return RE::BSContainer::ForEachResult::kContinue;
                });
        }
    }

    void OverlapSphereManager::InitializeVisible(OverlapSphere& s, RE::NiAVObject* holsterNode)
    {
        auto player = RE::PlayerCharacter::GetSingleton();

        if (holsterNode)
        {
            SKSE::log::trace("Found uninitialized node: {}", s.ID);
            holsterNode->local.scale = std::sqrt(s.squaredRadius);
            holsterNode->name = DrawNodeName + std::to_string(s.ID);
            auto exString = static_cast<RE::NiStringExtraData*>(holsterNode->GetExtraData("type"));
            exString->value = new char[]("InteractionSphere");

            if (s.localPosition)
            {
                holsterNode->local.translate = *s.localPosition;
            }

            if (!s.debugOnly)
            { // disable always-draw
                if (auto geometry = holsterNode->GetFirstGeometryOfShaderType(RE::BSShaderMaterial::Feature::kGlowMap))
                {
                    auto shaderProp = geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::kEffect].get();
                    if (shaderProp)
                    {
                        auto shader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp);
                        if (shader)
                        {
                            shader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kZBufferTest, true);
                        }
                    }
                }
            }
            if (s.normal)
            {
                if (auto normalPointer = holsterNode->GetObjectByName(DrawNodePointerName))
                {   // Show the pointer and rotate to align with the desired normal vector
                    normalPointer->local.scale = 1.2f;
                    RE::NiPoint3 defaultNorm = { 1, 0, 0 };  // arbitrary but comes from the .nif
                    RE::NiPoint3 axis = defaultNorm.UnitCross(*(s.normal));

                    if (axis.Length() < std::numeric_limits<float>::epsilon())
                    {   // handle the case where the vectors are collinear...
                        axis = { 0, 1, 0 };
                    }
                    auto angle = std::acos(helper::DotProductSafe(defaultNorm, *(s.normal)));

                    SKSE::log::trace("rotating node normal {} by {} on axis {} {} {}", s.ID, angle, axis.x, axis.y, axis.z);
                    normalPointer->local.rotate = helper::getRotationAxisAngle(axis, angle);
                }
            }
        }
    }

    void OverlapSphereManager::SetGlowColor(RE::NiAVObject* target, RE::NiColor* c)
    {
        auto geometry = target->GetFirstGeometryOfShaderType(RE::BSShaderMaterial::Feature::kGlowMap);
        if (geometry)
        {
            auto shaderProp = geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::kEffect].get();
            if (shaderProp)
            {
                auto shader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp);
                if (shader)
                {
                    shader->emissiveColor = c;
                }
            }
        }
    }

    void OverlapSphereManager::GameLoaded()
    {
        for (auto& s : spheres) {
            Destroy(s.ID);
        }

       
    }
}