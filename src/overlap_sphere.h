#pragma once

#include "artaddon.h"
#include "helper_game.h"
#include "helper_math.h"

namespace vrinput
{
	struct OverlapEvent
	{
		bool entered;
		bool isLeft;
	};

	using OverlapCallback = void (*)(const OverlapEvent& e);

	class OverlapSphere
	{
		friend class OverlapSphereManager;

	public:
		OverlapSphere(const char* node_name, OverlapCallback callback, float radius,
			float max_angle_deg = 0.0f, const RE::NiPoint3& offset = RE::NiPoint3(0.0, 0.0, 0.0),
			const RE::NiPoint3& normal = RE::NiPoint3(0.0, 0.0, 0.0));
		~OverlapSphere();

		const bool is_overlapping(bool isLeft);

	private:
		OverlapCallback                   callback_;
		bool                              overlap_state_[2] = { false, false };
		RE::NiPoint3                      local_position_;
		RE::NiPoint3                      normal_;
		const char*                       attach_node_name_;
		float                             max_angle_;
		float                             squared_radius_;
		std::unique_ptr<helper::ArtAddon> visible_debug_sphere_;
	};

	class OverlapSphereManager
	{
		friend OverlapSphere;

	public:
		static OverlapSphereManager* GetSingleton()
		{
			static OverlapSphereManager singleton;
			return &singleton;
		}

		void Update();
		void Reset();
		void ShowDebugSpheres();
		void HideDebugSpheres();
		void set_palm_offset(RE::NiPoint3& offset);

	private:
		static constexpr float        kHysteresis = 20.f;
		static constexpr RE::NiPoint3 kHandPalmNormal = { 0, -1, 0 };
		static constexpr const char*  kDebugModelPath = "debug/debugsphere.nif";
		static constexpr const char*  kControllerNodeName[2] = { "NPC R Hand [RHnd]",
			 "NPC L Hand [LHnd]" };
		const float                   kHysteresisAngular = helper::deg2rad(3);
		const float                   kControllerDebugModelScale = 1.0f;
		const int                     kOnHexColor = 0xffdc00;
		const int                     kOffHexColor = 0xff00ff;

		const void                        Register(OverlapSphere* s);
		const void                        UnRegister(OverlapSphere* s);
		std::unique_ptr<helper::ArtAddon> CreateDebugSphere(OverlapSphere* s);
		bool                              SendEvent(OverlapSphere* s, bool entered, bool isLeft);

		OverlapSphereManager();
		OverlapSphereManager(const OverlapSphereManager&) = delete;
		OverlapSphereManager(OverlapSphereManager&&) = delete;
		OverlapSphereManager& operator=(const OverlapSphereManager&) = delete;
		OverlapSphereManager& operator=(OverlapSphereManager&&) = delete;

		std::vector<OverlapSphere*>       process_list_;
		std::unique_ptr<helper::ArtAddon> controller_debug_models_[2];
		RE::NiPoint3                      palm_offset_ = RE::NiPoint3::Zero();
		bool                              draw_spheres_ = false;
		RE::NiColor*                      on_;
		RE::NiColor*                      off_;
	};

}