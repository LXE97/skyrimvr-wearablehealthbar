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
	class ArtAddon : public std::enable_shared_from_this<ArtAddon>
	{
		friend class ArtAddonManager;

	public:
		/** Constructs a new ArtAddon, queues the creation of its 3D model, and adds it to the list
         * of objects to process during ArtAddonManager::Update()
         * 
         * modelPath:   path to the *.nif file relative to Data/meshes/
         * target:      object to attach the 3D to
         * attachNode:  parent node for the new 3D, must be a 3rd person skeleton node for the player
         * local:       transform relative to the attachNode
         */
		static std::shared_ptr<ArtAddon> Create(const char* modelPath, TESObjectREFR* target,
			NiAVObject* attachNode, NiTransform& local);

		/** This can be used immediately because it schedules the transform to be set on the next Update */
		void SetTransform(NiTransform& local);

		/** Unlike the transform, any changes made through this will not be automatically preserved
         *  by the Manager, so they must be re-applied after game save/load.
         */
		NiAVObject* Get3D();

		~ArtAddon();

	private:
		bool           initialized;
		bool           markedForUpdate;
		NiAVObject*    root3D;
		BGSArtObject*  AO;
		NiTransform    desiredTransform;
		TESObjectREFR* target;
		NiAVObject*    attachNode;

		ArtAddon() = default;
		ArtAddon(const ArtAddon&) = delete;
		ArtAddon(ArtAddon&&) = delete;
		ArtAddon& operator=(const ArtAddon&) = delete;
		ArtAddon& operator=(ArtAddon&&) = delete;
	};

	class ArtAddonManager
	{
		friend std::shared_ptr<ArtAddon> ArtAddon::Create(const char*, TESObjectREFR*, NiAVObject*,
			NiTransform&);
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
		void          Register(const std::weak_ptr<ArtAddon>& a);
		void          Deregister(const std::weak_ptr<ArtAddon>& a);

		std::mutex                                     vector_lock_;
		BGSArtObject*                                  baseAO;
		std::unordered_map<const char*, BGSArtObject*> AOcache;
		std::vector<std::weak_ptr<ArtAddon>>           process_list_;
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
