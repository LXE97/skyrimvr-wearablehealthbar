/** Library for sticking arbitrary .nifs to ObjectReferences.
 * It uses temporary BGSArtObject forms that are recycled on exit to main menu or desktop.
 * From my testing it's safe to create 0x400000 of these forms before the game hangs, but we'll
 * cache them anyway for repeated use of the same model.
 */
#pragma once

#include <chrono>
#include <future>
#include <memory>

namespace helper
{
	class ArtAddon
	{
	public:
		/** Constructs a new ArtAddon and queues the creation of its 3D model.
         * 
         * modelPath:   path to the *.nif file relative to Data/meshes/
         * target:      object to attach the 3D to
         * attachNode:  parent node for the new 3D, must be a 3rd person skeleton node for the player
         * local:       transform relative to the attachNode */
		static std::shared_ptr<ArtAddon> Make(const char* modelPath, RE::TESObjectREFR* target,
			RE::NiAVObject* attachNode, RE::NiTransform& local);

		/** Returns: Pointer to the created NiAVObject. Will be nullptr until initialization finishes. */
		RE::NiAVObject* Get3D();

		~ArtAddon();

	private:
		std::future<RE::NiAVObject*> root3D_;
		RE::TESObjectREFR*           target_;

		ArtAddon() = default;
		ArtAddon(const ArtAddon&) = delete;
		ArtAddon(ArtAddon&&) = delete;
		ArtAddon& operator=(const ArtAddon&) = delete;
		ArtAddon& operator=(ArtAddon&&) = delete;
	};

	class ArtAddonManager
	{
	public:
		/** Must be called on SKSE Savegame event.
         * It forces any in-progress ArtAddon initializations to finish and cleans up ophaned
         * resources in the unlikely event that a game save happened in the same frame that an
         * ArtAddon was created. */
		static void PreSaveGame();

		static ArtAddonManager* GetSingleton()
		{
			static ArtAddonManager singleton;
			return &singleton;
		}

	private:
		std::vector<RE::ModelReferenceEffect*>             model_pool_;
		std::mutex                                         model_pool_lock_;
		std::unordered_map<const char*, RE::BGSArtObject*> artobject_cache_;
		std::mutex                                         artobject_cache_lock_;
		std::vector<std::weak_ptr<ArtAddon>>               process_list_;
		std::mutex                                         process_list_lock_;
		RE::BGSArtObject*                                  base_artobject_;
		const RE::FormID                                   base_artobject_Id_ = 0x9405f;

		RE::BGSArtObject*            GetArtForm(const char* modelPath);
		std::future<RE::NiAVObject*> AcquireResource();

		ArtAddonManager() = default;
		~ArtAddonManager() = default;
		ArtAddonManager(const ArtAddonManager&) = delete;
		ArtAddonManager(ArtAddonManager&&) = delete;
		ArtAddonManager& operator=(const ArtAddonManager&) = delete;
		ArtAddonManager& operator=(ArtAddonManager&&) = delete;
	};

}
