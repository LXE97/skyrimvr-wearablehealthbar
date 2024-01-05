#include "mod_input.h"

namespace vrinput
{
	struct InputCallback
	{
		uint8_t           flags;
		InputCallbackFunc func;

		bool operator==(const InputCallback& a_rhs)
		{
			return (flags == a_rhs.flags) && (func == a_rhs.func);
		}
	};

	std::mutex callback_lock;

	// each button id is mapped to a list of callback funcs
	std::unordered_map<vr::EVRButtonId, std::vector<InputCallback>> callbacks;
	std::unordered_map<vr::EVRButtonId, bool>                       buttonStates;

	inline uint8_t packEventFlags(bool touch, bool buttonPress, bool isLeft)
	{
		return (0u | touch << 2 | buttonPress << 1 | (uint8_t)isLeft);
	}

	void processButtonChanges(uint64_t changedMask, uint64_t currentState, bool isLeft, bool touch,
		vr::VRControllerState_t* out)
	{
		// iterate through each of the button codes that we care about
		for (auto buttonID : all_buttons)
		{
			uint64_t bitmask = 1ull << buttonID;

			// check if this button's state has changed and it has any callbacks
			if (bitmask & changedMask && callbacks.contains(buttonID))
			{
				// check whether it was a press or release event
				uint64_t buttonPress = bitmask & currentState;

				uint8_t eventFlags = packEventFlags(touch, (bool)buttonPress, isLeft);

				// iterate through callbacks for this button and call if flags match
				for (auto cb : callbacks[buttonID])
				{
					if (cb.flags == eventFlags)
					{
						// the callback tells us if we should block the input
						if (cb.func())
						{
							if (buttonPress)  // clear the current state of the button
							{
								if (touch)
								{
									out->ulButtonTouched &= ~bitmask;
								} else
								{
									out->ulButtonPressed &= ~bitmask;
								}
							} else  // set the current state of the button
							{
								if (touch)
								{
									out->ulButtonTouched |= bitmask;
								} else
								{
									out->ulButtonPressed |= bitmask;
								}
							}
						}
					}
				}
			}
		}
	}

	void AddCallback(const vr::EVRButtonId a_button, InputCallbackFunc a_callback, bool isLeft,
		bool onTouch, bool onButtonDown)
	{
		std::scoped_lock lock(callback_lock);
		if (!a_callback) return;

		callbacks[a_button].push_back(
			InputCallback(packEventFlags(onTouch, onButtonDown, isLeft), a_callback));
	}

	void RemoveCallback(const vr::EVRButtonId a_button, InputCallbackFunc a_callback, bool isLeft,
		bool onTouch, bool onButtonDown)
	{
		std::scoped_lock lock(callback_lock);
		if (!a_callback) return;

		auto it = std::find(callbacks[a_button].begin(), callbacks[a_button].end(),
			InputCallback(packEventFlags(onTouch, onButtonDown, isLeft), a_callback));
		if (it != callbacks[a_button].end()) { callbacks[a_button].erase(it); }
	}

}
