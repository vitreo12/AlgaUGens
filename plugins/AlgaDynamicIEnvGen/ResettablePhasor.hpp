#include "SC_PlugIn.h"

static InterfaceTable *ft;

struct ResettablePhasor {
    private:
        float phase;
        float increment;
        bool  runOnce;
        Unit* unit;

    public:
        bool  release;
        bool  isFadeIn;
        bool  isFadeOut;

        inline void init(bool isFadeIn_ = false, bool isFadeOut_ = false) {
            phase = 0.0f;
            increment = 0.0f;
            release = false;
            runOnce = false;
            isFadeIn = isFadeIn_;
            isFadeOut = isFadeOut_;
            if(isFadeIn || isFadeOut)
                runOnce = true;
            //fadeIn reverses too (check in the .cpp file)
            if(isFadeIn)
                release = true;
        }

        inline void reset(float freq = 1.0f, Unit* unit_ = NULL, bool release_ = false) {
            if(unit_)
                unit = unit_;

            if(release_)
                release = release_;

            phase = 0.0f;

            if(freq > 0.0f)
                increment = 1.0f / freq;
            else
                increment = 1.0f;
        }

        inline float advance() {
            float result = phase < 1.0f ? phase : 1.0f;
            result = increment < 1.0f ? result : 1.0f;
            phase += increment;
            if((release || runOnce) && unit && phase > 1)
            {
                if(!unit->mDone)
                    DoneAction(2, unit);
                unit->mDone = true;
                return 1.0f;
            }
            if(phase > 1)
                phase = 1.0f;
            return result;
        }
};

