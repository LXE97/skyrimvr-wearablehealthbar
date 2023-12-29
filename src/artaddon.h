/** Library for sticking arbitrary .nifs to ObjectReferences.
 * It uses temporary BGSArtObject forms that are recycled on exit to main menu or desktop.
 * From my testing it's safe to create 0x400000 of these forms before the game hangs, but the
 * ArtAddonManager caches them anyway for repeated use of the same model.
 */
#pragma once

namespace helper
{
	using namespace RE;

	/** The user has ownership of the virtual art object, but the manager automates the
     *  initialization and maintenance of the corresponding 3D objects. This class represents a
     *  request to the manager to create the 3D model and a storage for tracking its state.
     */
	class ArtAddon
	{
		friend class ArtAddonManager;

	public:
		ArtAddon(const char* modelPath, TESObjectREFR* target, NiAVObject* attachNode,
			NiTransform& local);
		~ArtAddon();

		void SetTransform(NiTransform& local);

		/** Unlike the transform, any changes made through this will not be automatically preserved
         *  by the Manager, so they must be re-applied after game save/load.
         */
		NiAVObject* Get3D();

	private:
		bool           initialized;
		bool           markedForUpdate;
		NiAVObject*    root3D;
		BGSArtObject*  AO;
		NiTransform    desiredTransform;
		TESObjectREFR* target;
		NiAVObject*    attachNode;

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
		/** Must be called by some periodic function in the plugin file,
         *  e.g. PlayerCharacter::Update
         */
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
		void          addChild(ArtAddon* a);
		void          removeChild(ArtAddon* a);

		BGSArtObject*                                  baseAO;
		std::unordered_map<const char*, BGSArtObject*> AOcache;
		std::vector<ArtAddon*>                         virtualObjects;
		bool                                           postSaveGameUpdate = false;
		const FormID                                   baseAOID = 0x9405f;

		struct toCancel
		{
			BGSArtObject*  a;
			TESObjectREFR* t;
		};
		std::vector<toCancel> cancel;
	};
}
