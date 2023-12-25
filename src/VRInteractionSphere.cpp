#include "VRInteractionSphere.h"

namespace vrinput
{
    OverlapSphereManager::OverlapSphereManager()
    {
        if (auto temp = RE::TESForm::LookupByID(dummyArt)) {
            auto temp2 = temp->CreateDuplicateForm(false, nullptr);
            if (temp2) {
                DrawNodeArt = temp2->As<RE::BGSArtObject>();
                if (DrawNodeArt)
                {
                    DrawNodeArt->SetModel(DrawNodeModelPath);
                }
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
            controllers[1] = player->GetVRNodeData()->NPCLHnd;
            controllers[0] = player->GetVRNodeData()->NPCRHnd;
            static int i = 1;
            static RE::NiPoint3 sphereWorld;

            for (auto& s : spheres)
            {
                if (s.attachNode)
                {
                    // Update virtual sphere position
                    if (s.localPosition)
                    {
                        if (s.onlyHeading)
                        {
                            RE::NiPoint3 rotated = *(s.localPosition);
                            helper::RotateZ(rotated, s.attachNode->world.rotate);
                            sphereWorld = s.attachNode->world.translate + rotated;
                        }
                        else
                        {
                            sphereWorld = s.attachNode->world.translate + s.attachNode->world.rotate * *(s.localPosition);
                        }
                    }
                    else
                    {
                        sphereWorld = s.attachNode->world.translate;
                    }

                    // Update visible sphere position
                    RE::NiAVObject* sphereVisNode = nullptr;
                    if (DrawHolsters)
                    {
                        // look for initialized node
                        if (sphereVisNode = player->Get3D(false)->GetObjectByName(DrawNodeName + std::to_string(s.ID)))
                        {
                            if (auto parentNode = player->Get3D(false)->GetObjectByName(DrawNewParentNode))
                            {
                                RE::NiUpdateData ctx;
                                sphereVisNode->local.translate = parentNode->world.rotate.Transpose() * (sphereWorld - parentNode->world.translate);

                                if (!s.onlyHeading)
                                { // by default, the sphere model has the same rotation as the NPC root node which has no pitch or roll
                                    sphereVisNode->local.rotate = parentNode->world.rotate.Transpose() * s.attachNode->world.rotate;
                                }
                                sphereVisNode->Update(ctx);
                            }
                        } // look for uninitialized node and assign it to this sphere
                        else if (sphereVisNode = player->Get3D(false)->GetObjectByName(DrawNodeInitialName))
                        {
                            InitializeVisible(s, sphereVisNode);
                        }
                    }

                    if (!s.debugOnly)
                    {
                        // Test sphere collision
                        bool changed = false;
                        for (bool isLeft : {false, true})
                        {
                            auto dist = sphereWorld.GetSquaredDistance(controllers[isLeft]->world.translate + controllers[isLeft]->world.rotate * palmoffset);
                            float angle = 0.0f;
                            if (s.normal)
                            {
                                auto sphereNorm = helper::VectorNormalized(s.attachNode->world.rotate * *(s.normal));
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
                                // set shader color to red
                                SetGlowColor(sphereVisNode, TURNON);
                            }
                            else if (!s.overlapState[0] && !s.overlapState[1])
                            { // set shader color back to green
                                SetGlowColor(sphereVisNode, TURNOFF);
                            }
                        }
                    }
                }
            }
        }
    }

    int32_t OverlapSphereManager::Create(RE::NiNode* attachNode, RE::NiPoint3* localPosition, float radius, RE::NiPoint3* normal, float maxAngle, bool onlyHeading, bool debugOnly)
    {

        if (attachNode)
        {
            spheres.push_back(OverlapSphere(attachNode, localPosition, radius, next_ID, normal, maxAngle, onlyHeading, debugOnly));
            if (DrawHolsters)
            {
                AddVisibleHolster(spheres.back());
            }
            SKSE::log::debug("holster created with id {}", next_ID);
            return next_ID++;
        }
        else
        {
            SKSE::log::debug("holster creation failed: {}", next_ID);
            return -1;
        }
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
                player->ApplyArtObject(DrawNodeArt, -1.0F, nullptr, false, false, player->Get3D(false)->GetObjectByName(DrawNewParentNode));
            }
        }
    }

    void OverlapSphereManager::DestroyVisibleHolster(int32_t id)
    {
        if (auto sphereVisNode = RE::PlayerCharacter::GetSingleton()->Get3D(false)->GetObjectByName(DrawNodeName + std::to_string(id)))
        {
            sphereVisNode->parent->DetachChild2(sphereVisNode);
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
            if (s.debugOnly)
            { // set to always draw
                if (auto geometry = holsterNode->GetFirstGeometryOfShaderType(RE::BSShaderMaterial::Feature::kGlowMap))
                {
                    auto shaderProp = geometry->GetGeometryRuntimeData().properties[RE::BSGeometry::States::kEffect].get();
                    if (shaderProp)
                    {
                        auto shader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProp);
                        if (shader)
                        {
                            shader->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kZBufferTest, false);
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
}