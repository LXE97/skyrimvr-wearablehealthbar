#include "mod_input.h"

#include "menu_checker.h"

namespace vrinput
{
	struct InputCallback
	{
		Hand              device;
		ActionType        type;
		InputCallbackFunc func;

		bool operator==(const InputCallback& a_rhs)
		{
			return (device == a_rhs.device) && (type == a_rhs.type) && (func == a_rhs.func);
		}
	};

	std::mutex               callback_lock;
	bool                     block_all_inputs;
	vr::TrackedDeviceIndex_t g_leftcontroller;
	vr::TrackedDeviceIndex_t g_rightcontroller;

	// each button id is mapped to a list of callback funcs
	std::unordered_map<vr::EVRButtonId, std::vector<InputCallback>> callbacks;
	std::unordered_map<vr::EVRButtonId, bool>                       buttonStates;

	void StartBlockingAll() { block_all_inputs = true; }
	void StopBlockingAll() { block_all_inputs = false; }
	bool isBlockingAll() { return block_all_inputs; }

	void AddCallback(const vr::EVRButtonId a_button, const InputCallbackFunc a_callback,
		const Hand hand, const ActionType touch_or_press)
	{
		std::scoped_lock lock(callback_lock);
		if (!a_callback) return;

		callbacks[a_button].push_back(InputCallback(hand, touch_or_press, a_callback));
	}

	void RemoveCallback(const vr::EVRButtonId a_button, const InputCallbackFunc a_callback,
		const Hand hand, const ActionType touch_or_press)
	{
		std::scoped_lock lock(callback_lock);
		if (!a_callback) return;

		auto it = std::find(callbacks[a_button].begin(), callbacks[a_button].end(),
			InputCallback(hand, touch_or_press, a_callback));
		if (it != callbacks[a_button].end()) { callbacks[a_button].erase(it); }
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
				bool buttonPress = bitmask & currentState;

				const ModInputEvent event_flags = ModInputEvent(static_cast<Hand>(isLeft),
					static_cast<ActionType>(touch), static_cast<ButtonState>(buttonPress));

				// iterate through callbacks for this button and call if flags match
				for (auto& cb : callbacks[buttonID])
				{
					if (cb.device == event_flags.device && cb.type == event_flags.touch_or_press)
					{
						// the callback tells us if we should block the input
						if (cb.func(event_flags))
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

	// handles low level button/trigger events
	bool ControllerInput_CB(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
		const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize,
		vr::VRControllerState_t* pOutputControllerState)
	{
		// save last controller input to only do processing on button changes
		static uint64_t prev_pressed[2] = {};
		static uint64_t prev_touched[2] = {};

		// need to remember the last output sent to the game in order to maintain input blocking
		static uint64_t prev_Pressed_out[2] = {};
		static uint64_t prev_touched_out[2] = {};

		if (pControllerState && !menuchecker::isGameStopped())
		{
			bool isLeft = unControllerDeviceIndex == g_leftcontroller;
			if (isLeft || unControllerDeviceIndex == g_rightcontroller)
			{
				uint64_t pressed_change = prev_pressed[isLeft] ^ pControllerState->ulButtonPressed;
				uint64_t touched_change = prev_touched[isLeft] ^ pControllerState->ulButtonTouched;

				if (pressed_change)
				{
					vrinput::processButtonChanges(pressed_change, pControllerState->ulButtonPressed,
						isLeft, false, pOutputControllerState);
					prev_pressed[isLeft] = pControllerState->ulButtonPressed;
					prev_Pressed_out[isLeft] = pOutputControllerState->ulButtonPressed;
				} else
				{
					pOutputControllerState->ulButtonPressed = prev_Pressed_out[isLeft];
				}

				if (touched_change)
				{
					vrinput::processButtonChanges(touched_change, pControllerState->ulButtonTouched,
						isLeft, true, pOutputControllerState);
					prev_touched[isLeft] = pControllerState->ulButtonTouched;
					prev_touched_out[isLeft] = pOutputControllerState->ulButtonTouched;
				} else
				{
					pOutputControllerState->ulButtonTouched = prev_touched_out[isLeft];
				}

				if (vrinput::isBlockingAll())
				{
					pOutputControllerState->ulButtonPressed = 0;
					pOutputControllerState->ulButtonTouched = 0;
					pOutputControllerState->rAxis->x = 0.0f;
					pOutputControllerState->rAxis->y = 0.0f;
				}
			}
		}
		return true;
	}
}
