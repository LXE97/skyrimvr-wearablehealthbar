#include "helper_game.h"

namespace helper
{
    RE::TESForm *LookupByName(RE::FormType typeEnum, const char* name)
    {
        auto *data = RE::TESDataHandler::GetSingleton();
        auto &forms = data->GetFormArray(typeEnum);
        for (auto *&form : forms)
        {
            if (!strcmp(form->GetName(), name))
            {
                return form;
            }
        }
        return nullptr;
    }
}