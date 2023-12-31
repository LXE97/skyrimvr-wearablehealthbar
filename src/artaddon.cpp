#include "artaddon.h"
#include "helper_game.h"

namespace helper
{
	std::shared_ptr<ArtAddon> ArtAddon::Make(const char* modelPath, RE::TESObjectREFR* target,
		RE::NiAVObject* attachNode, RE::NiTransform& local)
	{}
	RE::NiAVObject* ArtAddon::Get3D() {}
	ArtAddon::~ArtAddon() {}

	void ArtAddonManager::PreSaveGame() {}
	RE::BGSArtObject*            ArtAddonManager::GetArtForm(const char* modelPath) {}
	std::future<RE::NiAVObject*> ArtAddonManager::AcquireResource() {}

}
