#pragma once
#include "VR/PapyrusVRAPI.h"
#include "helper_math.h"

namespace vrinput
{
	constexpr const char* kLeftHandNodeName = "NPC L Hand [LHnd]";
	constexpr const char* kRightHandNodeName = "NPC R Hand [RHnd]";
	constexpr const char* kControllerNodeName[2] = { kRightHandNodeName, kLeftHandNodeName };

	extern vr::TrackedDeviceIndex_t g_leftcontroller;
	extern vr::TrackedDeviceIndex_t g_rightcontroller;
	extern float                    adjustable;

	enum class Hand
	{
		kRight = 0,
		kLeft,
		kBoth
	};
	enum class ActionType
	{
		kPress = 0,
		kTouch
	};
	enum class ButtonState
	{
		kButtonUp = 0,
		kButtonDown
	};

	inline Hand GetOtherHand(Hand a)
	{
		switch (a)
		{
		case Hand::kRight:
			return Hand::kLeft;
		case Hand::kLeft:
			return Hand::kRight;
		default:
			return Hand::kBoth;
		}
	}

	inline RE::NiAVObject* GetHandNode(Hand a_hand, bool a_first_person)
	{
		return RE::PlayerCharacter::GetSingleton()
			->Get3D(a_first_person)
			->GetObjectByName(a_hand == Hand::kRight ? kRightHandNodeName : kLeftHandNodeName);
	}

	constexpr std::array all_buttons{ vr::k_EButton_System,
		vr::k_EButton_ApplicationMenu,  // aka B button
		vr::k_EButton_Grip, vr::k_EButton_DPad_Left, vr::k_EButton_DPad_Up,
		vr::k_EButton_DPad_Right, vr::k_EButton_DPad_Down, vr::k_EButton_A,
		vr::k_EButton_ProximitySensor, vr::k_EButton_SteamVR_Touchpad,
		vr::k_EButton_SteamVR_Trigger };

	struct ModInputEvent
	{
		Hand        device;
		ActionType  touch_or_press;
		ButtonState button_state;

		bool operator==(const ModInputEvent& a_rhs)
		{
			return (device == a_rhs.device) && (touch_or_press == a_rhs.touch_or_press) &&
				(button_state == a_rhs.button_state);
		}
	};

	// returns: true if input that triggered the event should be blocked
	// Decision to block controller input to the game is made inside the callback, because we don't want to block it
	// if e.g. a menu is open or the action bound to this input is not applicable.
	// If the press is blocked, then we don't need to block the release
	typedef bool (*InputCallbackFunc)(const ModInputEvent& e);

	void AddCallback(const vr::EVRButtonId a_button, const InputCallbackFunc a_callback,
		const Hand hand, const ActionType touch_or_press);
	void RemoveCallback(const vr::EVRButtonId a_button, const InputCallbackFunc a_callback,
		const Hand hand, const ActionType touch_or_press);

	void StartBlockingAll();
	void StopBlockingAll();
	bool isBlockingAll();

	void StartSmoothing();
	void StopSmoothing();

	bool ControllerInput_CB(vr::TrackedDeviceIndex_t unControllerDeviceIndex,
		const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize,
		vr::VRControllerState_t* pOutputControllerState);

	vr::EVRCompositorError cbFuncGetPoses(VR_ARRAY_COUNT(unRenderPoseArrayCount)
											  vr::TrackedDevicePose_t* pRenderPoseArray,
		uint32_t                                                       unRenderPoseArrayCount,
		VR_ARRAY_COUNT(unGamePoseArrayCount) vr::TrackedDevicePose_t*  pGamePoseArray,
		uint32_t                                                       unGamePoseArrayCount);
}
