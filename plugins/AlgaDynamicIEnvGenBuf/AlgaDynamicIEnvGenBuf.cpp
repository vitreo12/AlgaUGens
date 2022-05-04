#include "SC_PlugIn.h"
#include "ResettablePhasor.hpp"

struct AlgaDynamicIEnvGenBuf : public Unit {
    ResettablePhasor phasor;
    float m_resetVal;
    float m_level;
    float m_pointin;
    float* m_envvals;
    float m_totalDur;
    int   m_numStages;
    float m_fbufnum;
    bool  m_validEnv;
};

extern "C" {
void AlgaDynamicIEnvGenBuf_next_a(AlgaDynamicIEnvGenBuf* unit, int inNumSamples);
void AlgaDynamicIEnvGenBuf_next_k(AlgaDynamicIEnvGenBuf* unit, int inNumSamples);
void AlgaDynamicIEnvGenBuf_Ctor(AlgaDynamicIEnvGenBuf* unit);
void AlgaDynamicIEnvGenBuf_Dtor(AlgaDynamicIEnvGenBuf* unit);
}

#define UPDATE_BUFFER \
    float fbufnum = IN0(0); \
    if (fbufnum >= 0 && fbufnum != unit->m_fbufnum) { \
        uint32 bufnum = (int)fbufnum; \
        World* world = unit->mWorld; \
        if (bufnum >= world->mNumSndBufs) \
            bufnum = world->mNumSndBufs; \
        unit->m_fbufnum = fbufnum; \
        const SndBuf* buf = world->mSndBufs + bufnum; \
        ACQUIRE_SNDBUF_SHARED(buf); \
        const float* bufData __attribute__((__unused__)) = buf->data; \
        if(bufData) { \
            uint32 bufFrames = buf->frames; \
            UPDATE_ENVVALS \
            unit->m_validEnv = true; \
        } \
        else { \
            unit->m_validEnv = false; \
        } \
        RELEASE_SNDBUF_SHARED(buf); \
    }

#define UPDATE_ENVVALS \
    unit->m_envvals[0] = bufData[1]; \
    unit->m_numStages = (int)bufData[2]; \
    unit->m_totalDur = bufData[3]; \
    for (int i = 4; i < bufFrames; i++) { \
        unit->m_envvals[i-3] = bufData[i]; \
    }

#define PHASOR_RESET_AR(RELEASE) \
    float fadeTime = IN0(1); \
    unit->phasor.reset(fadeTime * unit->mWorld->mSampleRate, unit, RELEASE);

#define PHASOR_RESET_KR(RELEASE) \
    float fadeTime = IN0(1); \
    unit->phasor.reset((fadeTime * unit->mWorld->mSampleRate) / unit->mWorld->mBufLength, unit, RELEASE);

void AlgaDynamicIEnvGenBuf_Ctor(AlgaDynamicIEnvGenBuf* unit) {
    bool isFadeIn = (bool)IN0(2);
    bool isFadeOut = (bool)IN0(3);
    unit->phasor.init(isFadeIn, isFadeOut);

    unit->m_resetVal = 1.0f; //Start from highest point (for fadeTime == 0)
    unit->m_pointin = 0.0f;
    unit->m_fbufnum = -1e9f;
    unit->m_validEnv = false;
    unit->m_numStages = 0.0f;
    unit->m_totalDur = 0.0f;
    unit->m_level = 0.0f;

    int maxSize = (int)IN0(4);
    int numVals = maxSize * 4; 
    int allocSize = (int)(numVals + 1.) * sizeof(float);
    unit->m_envvals = (float*)RTAlloc(unit->mWorld, allocSize);
    if(unit->m_envvals) {
        if (unit->mCalcRate == calc_FullRate) {
            PHASOR_RESET_AR(false)
            SETCALC(AlgaDynamicIEnvGenBuf_next_a);
        }
        else {
            PHASOR_RESET_KR(false)
            SETCALC(AlgaDynamicIEnvGenBuf_next_k);
        }

        //Update Buffer
        UPDATE_BUFFER
            
        //Always start from 0 (fully on)
        OUT0(0) = 1.0f;
    }
    else
        SETCALC(ClearUnitOutputs);
}

void AlgaDynamicIEnvGenBuf_Dtor(AlgaDynamicIEnvGenBuf* unit) { 
    if(unit->m_envvals)
        RTFree(unit->mWorld, unit->m_envvals); 
}

enum {
    shape_Step,
    shape_Linear,
    shape_Exponential,
    shape_Sine,
    shape_Welch,
    shape_Curve,
    shape_Squared,
    shape_Cubed,
    shape_Hold,
    shape_Sustain = 9999
};

#define GET_ENV_VAL                                                                                                    \
    switch (shape) {                                                                                                   \
    case shape_Step:                                                                                                   \
        level = unit->m_level = endLevel;                                                                              \
        break;                                                                                                         \
    case shape_Hold:                                                                                                   \
        level = unit->m_level;                                                                                         \
        unit->m_level = endLevel;                                                                                      \
        break;                                                                                                         \
    case shape_Linear:                                                                                                 \
    default:                                                                                                           \
        level = unit->m_level = pos * (endLevel - begLevel) + begLevel;                                                \
        break;                                                                                                         \
    case shape_Exponential:                                                                                            \
        level = unit->m_level = begLevel * pow(endLevel / begLevel, pos);                                              \
        break;                                                                                                         \
    case shape_Sine:                                                                                                   \
        level = unit->m_level = begLevel + (endLevel - begLevel) * (-cos(pi * pos) * 0.5 + 0.5);                       \
        break;                                                                                                         \
    case shape_Welch: {                                                                                                \
        if (begLevel < endLevel)                                                                                       \
            level = unit->m_level = begLevel + (endLevel - begLevel) * sin(pi2 * pos);                                 \
        else                                                                                                           \
            level = unit->m_level = endLevel - (endLevel - begLevel) * sin(pi2 - pi2 * pos);                           \
        break;                                                                                                         \
    }                                                                                                                  \
    case shape_Curve:                                                                                                  \
        if (fabs((float)curve) < 0.0001) {                                                                             \
            level = unit->m_level = pos * (endLevel - begLevel) + begLevel;                                            \
        } else {                                                                                                       \
            double denom = 1. - exp((float)curve);                                                                     \
            double numer = 1. - exp((float)(pos * curve));                                                             \
            level = unit->m_level = begLevel + (endLevel - begLevel) * (numer / denom);                                \
        }                                                                                                              \
        break;                                                                                                         \
    case shape_Squared: {                                                                                              \
        double sqrtBegLevel = sqrt(begLevel);                                                                          \
        double sqrtEndLevel = sqrt(endLevel);                                                                          \
        double sqrtLevel = pos * (sqrtEndLevel - sqrtBegLevel) + sqrtBegLevel;                                         \
        level = unit->m_level = sqrtLevel * sqrtLevel;                                                                 \
        break;                                                                                                         \
    }                                                                                                                  \
    case shape_Cubed: {                                                                                                \
        double cbrtBegLevel = pow(begLevel, 0.3333333f);                                                               \
        double cbrtEndLevel = pow(endLevel, 0.3333333f);                                                               \
        double cbrtLevel = pos * (cbrtEndLevel - cbrtBegLevel) + cbrtBegLevel;                                         \
        level = unit->m_level = cbrtLevel * cbrtLevel * cbrtLevel;                                                     \
        break;                                                                                                         \
    }                                                                                                                  \
    }

void AlgaDynamicIEnvGenBuf_next_a(AlgaDynamicIEnvGenBuf* unit, int inNumSamples) {
    float* out = OUT(0);
    float point; // = unit->m_pointin;
    float level = unit->m_level;
    int stagemul;
    
    // pointer, offset
    // level0, numstages, totaldur,
    // [initval, [dur, shape, curve, level] * N ]
    
    //Release
    if(IN0(5))
    {
        if(!unit->phasor.release)
            unit->m_resetVal = level;
        else
            unit->m_resetVal = (1.0f - level) * unit->m_resetVal;
        PHASOR_RESET_AR(true)
        
        //Should this be outside of the Release check?
        //The CPU gain of having it here is pretty neglegible
        UPDATE_BUFFER
    }

    //Processing
    if(unit->m_validEnv) {
        for (int i = 0; i < inNumSamples; i++) {
            float phase = unit->phasor.advance() * unit->m_totalDur;
            if (phase != unit->m_pointin) {
                unit->m_pointin = point = phase;
                float newtime = 0.f;
                int stage = 0;
                float seglen = 0.f;
                if (point >= unit->m_totalDur) {
                    unit->m_level = level = unit->m_envvals[unit->m_numStages * 4]; // grab the last value
                } else {
                    if (point <= 0.0) {
                        unit->m_level = level = unit->m_envvals[0];
                    } else {
                        float segpos = point;
                        // determine which segment the current time pointer needs
                        for (int j = 0; point >= newtime; j++) {
                            seglen = unit->m_envvals[(j * 4) + 1];
                            newtime += seglen;
                            segpos -= seglen;
                            stage = j;
                        }
                        stagemul = stage * 4;
                        segpos = segpos + seglen;
                        float begLevel = unit->m_envvals[stagemul];
                        int shape = (int)unit->m_envvals[stagemul + 2];
                        int curve = (int)unit->m_envvals[stagemul + 3];
                        float endLevel = unit->m_envvals[stagemul + 4];
                        float pos = (segpos / seglen);

                        GET_ENV_VAL
                    }
                }
            }
            
            //Invert on reset and scale according to resetVal
            if(unit->phasor.release)
                level = (1.0f - level) * unit->m_resetVal;

            //If done, use 0.0f, or it will click at the end
            if(unit->mDone)
                level = 0.0f;

            out[i] = level;
        }
    }
    else 
        ClearUnitOutputs(unit, inNumSamples);
}

void AlgaDynamicIEnvGenBuf_next_k(AlgaDynamicIEnvGenBuf* unit, int inNumSamples) {
    float* out = OUT(0);
    float level = unit->m_level;
    float point; // = unit->m_pointin;
    int stagemul;

    /* // pointer, offset */
    /* // level0, numstages, totaldur, */
    /* // [initval, [dur, shape, curve, level] * N ] */
    
    //Release
    if(IN0(5))
    {
        if(!unit->phasor.release)
            unit->m_resetVal = level;
        else
            unit->m_resetVal = (1.0f - level) * unit->m_resetVal;
        PHASOR_RESET_KR(true)
            
        //Should this be outside of the Release check?
        //The CPU gain of having it here is pretty neglegible
        UPDATE_BUFFER
    }

    //Processing
    if(unit->m_validEnv) {
        float phase = unit->phasor.advance() * unit->m_totalDur;
        if (phase != unit->m_pointin) {
            unit->m_pointin = point = sc_max(phase, 0.0f);
            float newtime = 0.f;
            int stage = 0;
            float seglen = 0.f;
            if (point >= unit->m_totalDur) {
                unit->m_level = level = unit->m_envvals[unit->m_numStages * 4]; // grab the last value
            } else {
                if (point <= 0.0) {
                    unit->m_level = level = unit->m_envvals[0];
                } else {
                    float segpos = point;
                    // determine which segment the current time pointer needs
                    for (int j = 0; point >= newtime; j++) {
                        seglen = unit->m_envvals[(j * 4) + 1];
                        newtime += seglen;
                        segpos -= seglen;
                        stage = j;
                    }
                    stagemul = stage * 4;
                    segpos = segpos + seglen;
                    float begLevel = unit->m_envvals[stagemul];
                    int shape = (int)unit->m_envvals[stagemul + 2];
                    int curve = (int)unit->m_envvals[stagemul + 3];
                    float endLevel = unit->m_envvals[stagemul + 4];
                    float pos = (segpos / seglen);

                    GET_ENV_VAL
                }
            }
        }
        
        //Invert on reset and scale according to resetVal
        if(unit->phasor.release)
            level = (1.0f - level) * unit->m_resetVal;

        //If done, use 0.0f, or it will click at the end
        if(unit->mDone)
            level = 0.0f;

        out[0] = level;
    }
    else
        out[0] = 0.0f;
}

PluginLoad(AlgaDynamicIEnvGenBufUGens) 
{
    ft = inTable; 
    DefineDtorUnit(AlgaDynamicIEnvGenBuf);
}
