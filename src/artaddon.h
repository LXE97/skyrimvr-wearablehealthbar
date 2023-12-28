/** Library for sticking arbitrary .nifs to ObjectReferences.
 * It uses temporary BGSArtObject forms that are recycled on exit to main menu or desktop.
 * From my testing it's safe to create 0x400000 of these forms before the game hangs, but we'll
 * cache them anyway for repeated use of the same model.
 * To prevent orphaned NiObjects from getting stuck on the player, all the models added this way
 * are automatically deleted pre-savegame event and then re-applied.
 */
#pragma once

namespace helper
{
    using namespace RE;

    /** The user has ownership of the virtual art object, but the manager automates the initialization and maintenance
     * of the corresponding 3D objects. This class represents a request to the manager to create the 3D and
     * a storage for tracking its state.
     */
    class ArtAddon
    {
        friend class ArtAddonManager;
    public:
        ArtAddon(const char* modelPath, TESObjectREFR* target, NiAVObject* attachNode, NiTransform& local);
        ~ArtAddon();

        void SetTransform(NiTransform& local);

        /** Unlike the transform, any changes made through this will not be automatically preserved by the Manager */
        NiAVObject* Get3D();
    private:
        bool initialized;
        bool markedForUpdate;
        NiAVObject* root3D;
        BGSArtObject* AO;
        NiTransform desiredTransform;
        TESObjectREFR* target;
        NiAVObject* attachNode;

        ArtAddon(const ArtAddon&) = delete;
        ArtAddon(ArtAddon&&) = delete;
        ArtAddon& operator=(const ArtAddon&) = delete;
        ArtAddon& operator=(ArtAddon&&) = delete;
    };

    class ArtAddonManager
    {
        friend ArtAddon::ArtAddon(const char*, TESObjectREFR*, NiAVObject*, NiTransform&);
        friend ArtAddon::~ArtAddon();
    public:
        /** Must be called by some periodic function in the plugin file, e.g. PlayerCharacter::Update */
        void Update();

        /** Must be called on SKSE save event. */
        void PreSaveGame();

        static ArtAddonManager* GetSingleton()
        {
            static ArtAddonManager singleton;
            return &singleton;
        }

    private:
        ArtAddonManager();
        ~ArtAddonManager() = default;
        ArtAddonManager(const ArtAddonManager&) = delete;
        ArtAddonManager(ArtAddonManager&&) = delete;
        ArtAddonManager& operator=(const ArtAddonManager&) = delete;
        ArtAddonManager& operator=(ArtAddonManager&&) = delete;

        BGSArtObject* GetArtForm(const char* modelPath);

        BGSArtObject* baseAO;
        std::unordered_map<const char*, BGSArtObject*> AOcache;
        std::vector<ArtAddon*> virtualObjects;
        bool postSaveGameUpdate = false;
        const FormID baseAOID = 0x9405f;

        // to handle the case that an ArtAddon is destroyed before its 3D data is instantiated
        struct toCancel {
            toCancel(BGSArtObject* a, TESObjectREFR* t) : a(a), t(t){}
            BGSArtObject* a;
            TESObjectREFR* t;
        };
        std::vector<toCancel> cancel;

    };
}
