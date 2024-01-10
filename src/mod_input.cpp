#include "mod_input.h"

#include "menu_checker.h"

namespace vrinput
{
	using namespace vr;

	bool  block_all_inputs = false;
	int   smoothing_window = 0;
	float adjustable = 0.02f;

	std::mutex               callback_lock;
	vr::TrackedDeviceIndex_t g_leftcontroller;
	vr::TrackedDeviceIndex_t g_rightcontroller;

	std::deque<vr::TrackedDevicePose_t> pose_buffer[2];
	vr::TrackedDevicePose_t             simple_buffer[2] = {};

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

	TrackedDevicePose_t operator+(const TrackedDevicePose_t& lhs, const TrackedDevicePose_t& rhs)
	{
		TrackedDevicePose_t result;
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result.mDeviceToAbsoluteTracking.m[i][j] = lhs.mDeviceToAbsoluteTracking.m[i][j];
			}
			result.mDeviceToAbsoluteTracking.m[i][3] =
				lhs.mDeviceToAbsoluteTracking.m[i][3] + rhs.mDeviceToAbsoluteTracking.m[i][3];
		}

		return result;
	}

	vr::TrackedDevicePose_t& operator/=(vr::TrackedDevicePose_t& lhs, float divisor)
	{
		for (int i = 0; i < 3; ++i) { lhs.mDeviceToAbsoluteTracking.m[i][3] /= divisor; }
		return lhs;
	}

	RE::NiTransform HmdMatrixToNiTransform(const HmdMatrix34_t& hmdMatrix)
	{
		RE::NiTransform niTransform;

		// Copy rotation matrix
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j) { niTransform.rotate.entry[i][j] = hmdMatrix.m[i][j]; }
		}

		// Copy translation vector
		niTransform.translate.x = hmdMatrix.m[0][3];
		niTransform.translate.y = hmdMatrix.m[1][3];
		niTransform.translate.z = hmdMatrix.m[2][3];

		return niTransform;
	}

	HmdMatrix34_t NiTransformToHmdMatrix(const RE::NiTransform& niTransform)
	{
		HmdMatrix34_t hmdMatrix;

		// Copy rotation matrix
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j) { hmdMatrix.m[i][j] = niTransform.rotate.entry[i][j]; }
		}

		// Copy translation vector
		hmdMatrix.m[0][3] = niTransform.translate.x;
		hmdMatrix.m[1][3] = niTransform.translate.y;
		hmdMatrix.m[2][3] = niTransform.translate.z;

		return hmdMatrix;
	}

	// each button id is mapped to a list of callback funcs
	std::unordered_map<vr::EVRButtonId, std::vector<InputCallback>> callbacks;
	std::unordered_map<vr::EVRButtonId, bool>                       buttonStates;

	void StartBlockingAll() { block_all_inputs = true; }
	void StopBlockingAll() { block_all_inputs = false; }
	bool isBlockingAll() { return block_all_inputs; }

	void StartSmoothing() { smoothing_window = 1; }
	void StopSmoothing() { smoothing_window = 0; }

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
								if (touch) { out->ulButtonTouched &= ~bitmask; }
								else { out->ulButtonPressed &= ~bitmask; }
							}
							else  // set the current state of the button
							{
								if (touch) { out->ulButtonTouched |= bitmask; }
								else { out->ulButtonPressed |= bitmask; }
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
				}
				else { pOutputControllerState->ulButtonPressed = prev_Pressed_out[isLeft]; }

				if (touched_change)
				{
					vrinput::processButtonChanges(touched_change, pControllerState->ulButtonTouched,
						isLeft, true, pOutputControllerState);
					prev_touched[isLeft] = pControllerState->ulButtonTouched;
					prev_touched_out[isLeft] = pOutputControllerState->ulButtonTouched;
				}
				else { pOutputControllerState->ulButtonTouched = prev_touched_out[isLeft]; }

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

	// handles device poses
	vr::EVRCompositorError cbFuncGetPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount)
											  vr::TrackedDevicePose_t* pRenderPoseArray,
		uint32_t                                                       unRenderPoseArrayCount,
		VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t*  pGamePoseArray,
		uint32_t                                                       unGamePoseArrayCount)
	{
		for (auto isLeft : { true, false })
		{
			auto dev = isLeft ? g_leftcontroller : g_rightcontroller;
			if (smoothing_window)
			{
				auto pre = HmdMatrixToNiTransform(simple_buffer[isLeft].mDeviceToAbsoluteTracking);
				auto cur = HmdMatrixToNiTransform(pGamePoseArray[dev].mDeviceToAbsoluteTracking);

				// TODO: real filtering algorithm
				// simple noise gate
				constexpr float low_rate = 0.04f;
				constexpr float mid_rate = 0.14f;
				constexpr float high_rate = 0.2f;
				constexpr float low_threshold = 0.003f * 0.003f;
				constexpr float mid_threshold = 0.008f * 0.008f;
				constexpr float high_threshold = 0.05f * 0.05f;

				auto dist = pre.translate.GetSquaredDistance(cur.translate);

				if (dist > high_threshold)
				{
					pre.translate = helper::LinearInterp(pre.translate, cur.translate, high_rate);
				}
				else if (dist > mid_threshold)
				{
					pre.translate = helper::LinearInterp(pre.translate, cur.translate, mid_rate);
				}
				else if (dist > low_threshold)
				{
					pre.translate = helper::LinearInterp(pre.translate, cur.translate, low_rate);
				}

				//pre = HmdMatrixToNiTransform(simple_buffer[isLeft].mDeviceToAbsoluteTracking);

				// adaptive slerp?
				pre.rotate = helper::slerpMatrixAdaptive(pre.rotate, cur.rotate);

				pGamePoseArray[dev].mDeviceToAbsoluteTracking = NiTransformToHmdMatrix(pre);
				simple_buffer[isLeft] = pGamePoseArray[dev];
			}
			else { simple_buffer[isLeft] = pGamePoseArray[dev]; }
		}
		return vr::EVRCompositorError::VRCompositorError_None;
	}
}
