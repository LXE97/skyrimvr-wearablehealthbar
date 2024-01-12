/** Library for sticking arbitrary nif files to ObjectReferences.
 * It uses temporary BGSArtObject forms that are recycled on exit to main menu or desktop.
 * From my testing it's safe to create 0x400000 of these forms before the game hangs, but we'll
 * cache them anyway for repeated use of the same model.
 */
#pragma once

#include <chrono>
#include <memory>

namespace art_addon
{
	class ArtAddon;
	using ArtAddonPtr = std::shared_ptr<ArtAddon>;
	class ArtAddon
	{
		friend class ArtAddonManager;

	public:
		/** Returns: shared_ptr, nullptr if modelPath or target is invalid
		 * 
		 * Constructs a new ArtAddon and queues the creation of its 3D model. The model is not 
		 * saved to the savefile but it will persist through game loads for the lifetime of the
		 * returned pointer.
         * 
         * a_model_path:	path to the *.nif file relative to Data/meshes/
         * a_target:		object to attach the 3D to
         * a_attach_node:	parent node for the new 3D, must be a 3rd person node for the player
         * a_local:      	transform relative to the attachNode
		 */
		static ArtAddonPtr Make(const char* a_model_path, RE::TESObjectREFR* a_target,
			RE::NiAVObject* a_attach_node, RE::NiTransform& a_local);

		~ArtAddon()
		{
			if (root3D) { root3D->parent->DetachChild(root3D); }
		}

		/** Returns: Pointer to the attached NiAVObject. nullptr if initialization hasn't finished. */
		RE::NiAVObject* const Get3D() { return root3D; }
		RE::NiAVObject* const GetParent() { return attach_node; }

	protected:
		ArtAddon() = default;
		ArtAddon(const ArtAddon&) = delete;
		ArtAddon(ArtAddon&&) = delete;
		ArtAddon& operator=(const ArtAddon&) = delete;
		ArtAddon& operator=(ArtAddon&&) = delete;

		virtual inline void OnModelCreation() {}

		RE::NiAVObject*    root3D = nullptr;
		RE::TESObjectREFR* target = nullptr;
		RE::BGSArtObject*  art_object = nullptr;
		RE::NiAVObject*    attach_node = nullptr;
		RE::NiTransform    local;
	};

	class ArtAddonManager
	{
		friend ArtAddon;
		friend class NifChar;

	public:
		/** Must be called every frame. It only takes 1 frame to create all the models and remove 
		 * them from the processing queue so this will usually do nothing. */
		void Update();

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

		RE::BGSArtObject* GetArtForm(const char* a_modelPath);
		int               GetNextId();

		std::unordered_map<int, std::weak_ptr<ArtAddon>>   new_objects;
		std::mutex                                         objects_lock;
		std::unordered_map<const char*, RE::BGSArtObject*> artobject_cache;
		RE::BGSArtObject*                                  base_artobject;
		int                                                next_id = -2;
	};

	class NifChar : public ArtAddon
	{
		friend class ArtAddonManager;

	public:
		static ArtAddonPtr Make(char a_ascii, RE::NiAVObject* a_parent, RE::NiTransform& a_local);

	private:
		static constexpr float       kUVOffset_x = 0.0625f;
		static constexpr float       kUVOffset_y = 0.125f;
		static constexpr const char* kFontModelPath = "ArtAddon/char.nif";

		void                OnModelCreation() override;
		inline RE::NiPoint2 AsciiToXY(char a_ascii)
		{
			char temp = a_ascii - ' ';
			return RE::NiPoint2((temp % 16) * kUVOffset_x, (temp / 16) * kUVOffset_y);
		}

		char ascii;
	};

	class NifTextBox
	{
	public:
		NifTextBox(const char* a_string, const float a_spacing, RE::NiAVObject* a_attach_to,
			RE::NiTransform& a_local) :
			string(a_string),
			spacing(a_spacing)
		{}
		~NifTextBox() { characters.clear(); }

	private:
		static constexpr const char* kEmpty = "meshes/effects/fxemptyobject.nif";

		void MakeString();

		ArtAddonPtr              root;
		std::vector<ArtAddonPtr> characters;
		const char*              string;
		const float              spacing;
	};

}
