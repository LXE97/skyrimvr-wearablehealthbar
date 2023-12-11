#pragma once

namespace wearable
{
    using namespace RE;

    class Wearable
    {
    public:
        NiNode* parent;
        std::string parentName;
        std::vector<std::string> nodeNames;

        virtual void Reattach(NiNode* newParent);
    };

    class Meter : Wearable
    {

    };

}