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

            //Phase needs to be reset regardless of t_release
            phase = 0.0f;

            //Triggered t_release
            if(release_)
                release = true;

            //Calculate increment according to freq (determined by fadeTime)
            if(freq > 0.0f)
                increment = 1.0f / freq;
            else {
                //This happens for fadeTime <= 0
                increment = 1.0f;
                phase = 1.1f; //Whatever value > 1.0f
            }
        }

        inline float advance() {
            float result;
            if(phase < 1.0f) {
                result = phase;
                phase += increment;
            }
            else
                result = 1.0f;

            if((release || runOnce) && phase > 1.0f) {
                if(!unit->mDone) {
                    unit->mDone = true;
                    DoneAction(2, unit);
		        }
                return 1.0f;
            }

            return result;
        }
};

