#include "SC_PlugIn.h"

static InterfaceTable *ft;

struct ResettablePhasor {
    private:
        float phase;
        float phaseResetVal;
        float phaseResetValScale;
        float increment;
        bool  runOnce;
        Unit* unit;

    public:
        bool  release;
        bool  releaseTriggered;
        bool  isFadeIn;
        bool  isFadeOut;

        inline void init(bool isFadeIn_ = false, bool isFadeOut_ = false) {
            phase = 0.0f;
            phaseResetVal = 0.0f;
            phaseResetValScale = 0.0f;
            increment = 0.0f;
            release = false;
            releaseTriggered = false;
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

            //Retrieve peak and calculate scaling
            phaseResetVal = phase;
            if(phaseResetVal > 0.0f && phaseResetVal < 1.0)
                phaseResetValScale = 1.0 / (1.0 - phaseResetVal);

            //Only trigger the phase reset ONCE
            if(release_ && !releaseTriggered)
            {
                release = true;
                releaseTriggered = true;
                phase = 0.0f;
                phaseResetVal = 0.0f;
                phaseResetValScale = 0.0f;
            }

            if(freq > 0.0f)
                increment = 1.0f / freq;
            else
                increment = 1.0f;
        }

        inline float advance() {
            float result;
            if(phase < 1.0f) {
                result = phase;
                phase += increment;
            }
            else {
                result = 1.0f;
            }

            if((release || runOnce) && unit && phase > 1.0f) {
                if(!unit->mDone) {
                    unit->mDone = true;
                    DoneAction(2, unit);
		        }
                return 1.0f;
            }

            //Scale by the last reset to have 0-1 range at all times (with varying speed according to fadeTime)
            if(phaseResetVal > 0.0f && phaseResetVal < 1.0f)
                result = (result - phaseResetVal) * phaseResetValScale; //a.k.a optimized linlin
            
            return result;
        }
};

