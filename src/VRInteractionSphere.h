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
        /** Should be called by some periodic function in the plugin file, e.g. PlayerCharacter::Update */
        void Update();

        /** Should be called every time a saved game is loaded.
         * Deletes all orphaned ModelReferenceEffects on the player and all models added by this mod, and resets mod state */
        void GameLoaded();

        static OverlapSphereManager* GetSingleton()
        {
            static OverlapSphereManager singleton;
            return &singleton;
        }
        
        int32_t Create(const char* attachNodeName, RE::NiPoint3* localPosition, float radius, RE::NiPoint3* normal = nullptr, float maxAngle = 0, bool onlyHeading = false, bool debugOnly = false);
        void Destroy(int32_t targetID);
        void SetPalmOffset(RE::NiPoint3& offset);
        void SetOverlapEventHandler(OverlapCallback cb);
        void ShowHolsterSpheres();
        void HideHolsterSpheres();

    private:
        struct OverlapSphere
        {
            OverlapSphere(const char* attachNode, RE::NiPoint3* localPosition, const float& radius, const int32_t& ID, RE::NiPoint3* normal = nullptr, const float& maxAngle = 360, bool onlyHeading = false, bool debugOnly = false)
                : attachNode(attachNode), localPosition(localPosition), squaredRadius(radius* radius), ID(ID), onlyHeading(onlyHeading), debugOnly(debugOnly), normal(normal), maxAngle(maxAngle) {}

            const char* attachNode; // use node name because third person will be used for drawing and first person for collision
            RE::NiPoint3* localPosition;
            RE::NiPoint3* normal;   // vector in the local space of the attachNode. X = right, Y = forward, Z = up
            float maxAngle; // angle (DEGREES) between normal and the colliding palm vector to activate
            float squaredRadius;
            bool onlyHeading; // determines whether the sphere inherits pitch and roll (only matters if localPosition is set)
            bool debugOnly;      // if true, collision is not checked and model is always visible (ignoring depth)
            int32_t ID;
            bool overlapState[2] = { false, false };
        };

        void AddVisibleHolster(OverlapSphere& s);
        void DestroyVisibleHolster(int32_t ID);
        void InitializeVisible(OverlapSphere& s, RE::NiAVObject* holsterNode);
        void SetGlowColor(RE::NiAVObject* target, RE::NiColor* c);

        RE::NiNode* controllers[2];
        std::vector<OverlapSphere> spheres;
        OverlapCallback _cb;
        bool DrawHolsters = false;
        int32_t next_ID = 1;
        RE::NiPoint3 palmoffset;

        // constants
        static constexpr const char* pluginName = "wearable"; // for determining ownership of temporary art objects
        static constexpr float hysteresis = 20; // squared distance threshold before changing to off state
        static constexpr float hysteresis_angular = helper::deg2radC(3);
        static constexpr RE::NiPoint3 NPCHandPalmNormal = { 0, -1, 0 };
        static constexpr RE::FormID dummyArt = 0x9405f;
        static constexpr const char* DrawNodeModelPath = "debug/debugsphere.nif";
        static constexpr const char* DrawNodeInitialName = "DEBUGDRAWSPHERE";
        static constexpr const char* DrawNodeName = "Z4K_DRAWSPHERE";
        static constexpr const char* DrawNodePointerName = "Z4K_OVERLAPNORMAL";

        RE::BGSArtObject* DrawNodeArt;
        RE::NiColor* TURNON;
        RE::NiColor* TURNOFF;

        OverlapSphereManager();
        ~OverlapSphereManager() = default;
        OverlapSphereManager(const OverlapSphereManager&) = delete;
        OverlapSphereManager(OverlapSphereManager&&) = delete;
        OverlapSphereManager& operator=(const OverlapSphereManager&) = delete;
        OverlapSphereManager& operator=(OverlapSphereManager&&) = delete;
    };
}
