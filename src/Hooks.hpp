
namespace Hooks
{
    static float g_detection_level = 2;
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

    static void Install()
    {
        stl::write_vfunc<0x1, StealthMeter_Update>(RE::VTABLE_StealthMeter[0]);
    }
}