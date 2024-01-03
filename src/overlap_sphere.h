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
		int  id;
	};

	using OverlapCallback = void (*)(const OverlapEvent& e);

	class OverlapSphere
	{
		friend class OverlapSphereManager;

	public:
		static std::shared_ptr<OverlapSphere> Make(RE::NiAVObject* a_attach_to,
			OverlapCallback a_callback, float a_radius, float a_max_angle_deg = 0.0f,
			const RE::NiPoint3& a_offset = RE::NiPoint3(0.0, 0.0, 0.0),
			const RE::NiPoint3& a_normal = RE::NiPoint3(0.0, 0.0, 0.0));

		~OverlapSphere() = default;

		const bool is_overlapping(bool a_isLeft);

	private:
		OverlapSphere() = default;
		OverlapSphere(const OverlapSphere&) = delete;
		OverlapSphere(OverlapSphere&&) = delete;
		OverlapSphere& operator=(const OverlapSphere&) = delete;
		OverlapSphere& operator=(OverlapSphere&&) = delete;

		OverlapCallback                      callback;
		bool                                 overlap_state[2] = { false, false };
		RE::NiPoint3                         local_position;
		RE::NiPoint3                         normal;
		RE::NiAVObject*                      attach_node;
		float                                max_angle;
		float                                squared_radius;
		std::shared_ptr<art_addon::ArtAddon> visible_debug_sphere;
		int                                  id;
	};
	using OverlapSpherePtr = std::shared_ptr<OverlapSphere>;

	class OverlapSphereManager
	{
		friend OverlapSphere;

	public:
		static OverlapSphereManager* GetSingleton()
		{
			static OverlapSphereManager singleton;
			return &singleton;
		}

		float Update();
		void  ShowDebugSpheres();
		void  HideDebugSpheres();
		void  SetPalmOffset(const RE::NiPoint3& a_offset);

	private:
		static constexpr float        kHysteresis = 20.f;
		static constexpr RE::NiPoint3 kHandPalmNormal = { 0, -1, 0 };
		static constexpr const char*  kModelPath = "debug/debugsphere.nif";
		static constexpr const char*  kControllerNodeName[2] = { "NPC R Hand [RHnd]",
			 "NPC L Hand [LHnd]" };
		const float                   kHysteresisAngular = helper::deg2rad(3);
		const float                   kModelScale = 1.0f;
		const int                     kOnHexColor = 0xffdc00;
		const int                     kOffHexColor = 0xff00ff;

		OverlapSphereManager();
		OverlapSphereManager(const OverlapSphereManager&) = delete;
		OverlapSphereManager(OverlapSphereManager&&) = delete;
		OverlapSphereManager& operator=(const OverlapSphereManager&) = delete;
		OverlapSphereManager& operator=(OverlapSphereManager&&) = delete;

		art_addon::ArtAddonPtr CreateVisibleSphere(const OverlapSphere& a_s);
		int                    GetNextId();
		bool                   SendEvent(const OverlapSphere& a_s, bool a_entered, bool isLeft);

		std::vector<std::weak_ptr<OverlapSphere>> process_list;
		std::mutex                                process_lock;
		art_addon::ArtAddonPtr                    controller_models[2];
		RE::NiPoint3                              palm_offset = RE::NiPoint3::Zero();
		bool                                      draw_spheres = false;
		RE::NiColor*                              on;
		RE::NiColor*                              off_;
		int                                       nextId = 0;
	};

}