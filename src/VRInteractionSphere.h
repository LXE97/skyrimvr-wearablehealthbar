#pragma once
#include "helper_math.h"
#include "helper_game.h"

namespace vrinput
{
    struct OverlapEvent
    {
        OverlapEvent(int32_t id, bool entered, bool isLeft) : ID(id), entered(entered), isLeft(isLeft) {}
        int32_t ID;
        bool entered;

        // indicates which controller entered the sphere
        bool isLeft;
    };

    using OverlapCallback = void (*)(const OverlapEvent& e);

    class OverlapSphereManager
    {
    public:
        // This has to be called by some periodic function in the plugin file, e.g. PlayerCharacter::Update or OpenVR input callbacks
        void Update();

        static OverlapSphereManager* GetSingleton()
        {
            static OverlapSphereManager singleton;
            return &singleton;
        }

        RE::NiPoint3 palmoffset;

        int32_t Create(RE::NiNode* attachNode, RE::NiPoint3* localPosition, float radius, RE::NiPoint3* normal = nullptr, float maxAngle = 0, bool onlyHeading = false, bool debugOnly = false);
        void Destroy(int32_t targetID);
        void SetOverlapEventHandler(OverlapCallback cb);
        void ShowHolsterSpheres();
        void HideHolsterSpheres();

    private:
        OverlapSphereManager();
        OverlapSphereManager(const OverlapSphereManager&) = delete;
        OverlapSphereManager& operator=(const OverlapSphereManager&) = delete;

        struct OverlapSphere
        {
        public:
            OverlapSphere(RE::NiNode* attachNode, RE::NiPoint3* localPosition, float radius, int32_t ID, RE::NiPoint3* normal = nullptr, float maxAngle = 0, bool onlyHeading = false, bool debugOnly = false)
                : attachNode(attachNode), localPosition(localPosition), squaredRadius(radius* radius), ID(ID), onlyHeading(onlyHeading), debugOnly(debugOnly), normal(normal), maxAngle(maxAngle) {}

            RE::NiNode* attachNode;
            RE::NiPoint3* localPosition;
            RE::NiPoint3* normal;   // vector in the local space of the attachNode. X = right, Y = forward, Z = up
            float maxAngle; // angle between normal and the colliding palm vector to activate
            float squaredRadius;
            bool onlyHeading; // determines whether the sphere inherits pitch and roll (only matters if localPosition is set)
            bool debugOnly;      // if true, collision is not checked and model is always visible (ignoring depth)
            int32_t ID;
            bool overlapState[2];
        };
        void AddVisibleHolster(OverlapSphere& s);
        void DestroyVisibleHolster(int32_t ID);
        void InitializeVisible(OverlapSphere& s, RE::NiAVObject* holsterNode);
        void SetGlowColor(RE::NiAVObject* target, RE::NiColor* c);

        RE::NiPointer<RE::NiNode> controllers[2];
        std::vector<OverlapSphere> spheres;
        OverlapCallback _cb;
        bool DrawHolsters = false;
        int32_t next_ID = 1;

        // constants
        static constexpr float hysteresis = 20; // squared distance threshold before changing to off state
        static constexpr float hysteresis_angular = helper::deg2rad(3);
        static constexpr RE::NiPoint3 NPCHandPalmNormal = { 0, -1, 0 };
        static constexpr RE::FormID dummyArt = 0x9405f;        
        static constexpr const char* DrawNodeModelPath = "debug/InteractionSphere.nif";
        static constexpr const char* DrawNodeInitialName = "DEBUGDRAWSPHERE";
        static constexpr const char* DrawNodeName = "Z4K_DRAWSPHERE";
        static constexpr const char* DrawNodePointerName = "Z4K_OVERLAPNORMAL";
        static constexpr const char* DrawParentNode = "skeleton.nif"; // needs to be attached to a visible third person node

        RE::BGSArtObject* DrawNodeArt;
        RE::NiColor* TURNON;
        RE::NiColor* TURNOFF;
    };
}
