#include "Relocation.h"
#include "SKSE/Impl/Stubs.h"
#include "main_plugin.h"
#include "xbyak/xbyak.h"

namespace hooks
{
	float g_detection_level = -1.0f;
	// hook from Doodlum/Contextual Crosshair
	struct StealthMeter_Update
	{
		static char thunk(RE::StealthMeter* a1, int64_t a2, int64_t a3, int64_t a4)
		{
			auto result = func(a1, a2, a3, a4);

			g_detection_level = static_cast<float>(a1->unk88);

			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void PostWandUpdateHook() { wearable_plugin::Update(); }

	// thanks to FlyingParticle for this hook
	uintptr_t postWandUpdateHookedFuncAddr = 0;
	// A call shortly after the wand nodes are updated as part of Main::Draw()
	auto postWandUpdateHookLoc = RelocAddr<uintptr_t>(0x13233AA);
	auto postWandUpdateHookedFunc = RelocAddr<uintptr_t>(0xDCF900);

	static void Install()
	{
		stl::write_vfunc<0x1, StealthMeter_Update>(RE::VTABLE_StealthMeter[0]);

		postWandUpdateHookedFuncAddr = postWandUpdateHookedFunc.GetUIntPtr();
		{
			struct Code : Xbyak::CodeGenerator
			{
				Code()
				{
					Xbyak::Label jumpBack;

					// Original code
					mov(rax, postWandUpdateHookedFuncAddr);
					call(rax);

					push(rax);
					// Need to keep the stack 16 byte aligned, and an additional 0x20 bytes for scratch space
					sub(rsp, 0x38);
					movsd(ptr[rsp + 0x20], xmm0);

					// Call our hook
					mov(rax, (uintptr_t)PostWandUpdateHook);
					call(rax);

					movsd(xmm0, ptr[rsp + 0x20]);
					add(rsp, 0x38);
					pop(rax);

					// Jump back to whence we came (+ the size of the initial branch instruction)
					jmp(ptr[rip + jumpBack]);

					L(jumpBack);
					dq(postWandUpdateHookLoc.GetUIntPtr() + 5);
				}
			};
			Code code;
			code.ready();

			auto& trampoline = SKSE::GetTrampoline();

			trampoline.write_branch<5>(
				postWandUpdateHookLoc.GetUIntPtr(), trampoline.allocate(code));

			SKSE::log::trace("Post Wand Update hook complete");
			
		}
	}
}