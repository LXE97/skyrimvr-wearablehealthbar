#pragma once
#include "art_addon.h"
#include "overlap_sphere.h"

namespace wearable
{
	void WearableOverlapHandler(const vrinput::OverlapEvent& e);

	class Wearable;
	using WearablePtr = std::shared_ptr<Wearable>;
	class Wearable
	{
	public:
		virtual WearablePtr Make() = 0;
	};

	class WearableManager
	{
	public:
		void Update();

		static WearableManager* GetSingleton()
		{
			static WearableManager singleton;
			return &singleton;
		}

		std::vector<std::weak_ptr<Wearable>> objects;

	private:
		bool configuration_state;
		int  configuration_index;
		int  update_index;
	};

	/** A single meter with a single model */
	class Meter : public Wearable
	{
	public:
		bool isLeft;
	};

	/** Multiple meters in a single model*/
	class MultiMeter : public Meter
	{};

	/** A single meter composed of multiple models*/
	class CompoundMeter : public Meter
	{
	public:
	private:
		
	};

}