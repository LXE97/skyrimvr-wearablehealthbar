#pragma once

namespace wearable_plugin
{
    using namespace RE;

    //void WearableOverlapHandler(const vrinput::OverlapEvent& e);
    
    class Wearable
    {
    public:
        //NiNode* parent;
        std::string parentName;
        std::vector<std::string> nodeNames;


        // false indicates this object is in the user configuration state
        bool locked;

        /** */
        virtual void Reattach(NiNode* newParent);

        /** Returns true if this object's 3D model(s) exists in the player's node tree */
        bool Has3D();
        /** Returns true if the 3D model was successfully added to the player */
        bool Create3D();
        /** Returns true if the 3D model was found and deleted */
        bool Delete3D();
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

        std::vector<std::unique_ptr<Wearable>> objects;

    private:
        bool configurationState;
    };

    /** A single meter with a single model */
    class Meter : Wearable
    {
    public:
        bool isLeft;

        void Reattach(NiNode* newParent) override;

    };

    /** Multiple meters in a single model*/
    class MultiMeter : Meter
    {

    };

    /** A single meter composed of multiple models*/
    class CompoundMeter : Meter
    {

    };

}