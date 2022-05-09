#include "SC_PlugIn.h"
#include "../ResettablePhasor.hpp"

struct AlgaDynamicIEnvGen : public Unit {
    ResettablePhasor phasor;
    float m_resetVal;
    float m_level, m_offset;
    float m_numvals, m_pointin;
    float* m_envvals;
};

extern "C" {
void AlgaDynamicIEnvGen_next_a(AlgaDynamicIEnvGen* unit, int inNumSamples);
void AlgaDynamicIEnvGen_next_k(AlgaDynamicIEnvGen* unit, int inNumSamples);
void AlgaDynamicIEnvGen_Ctor(AlgaDynamicIEnvGen* unit);
void AlgaDynamicIEnvGen_Dtor(AlgaDynamicIEnvGen* unit);
}

#define UPDATE_ENVVALS \
    for (int i = 1; i <= numStages * 4; i++) { \
        unit->m_envvals[i] = IN0(4 + i); \
    }

#define PHASOR_RESET_AR(RELEASE) \
    float fadeTime = IN0(0); \
    unit->phasor.reset(fadeTime * unit->mWorld->mSampleRate, unit, RELEASE);

#define PHASOR_RESET_KR(RELEASE) \
    float fadeTime = IN0(0); \
    unit->phasor.reset((fadeTime * unit->mWorld->mSampleRate) / unit->mWorld->mBufLength, unit, RELEASE);

void AlgaDynamicIEnvGen_Ctor(AlgaDynamicIEnvGen* unit) {

    bool isFadeIn = (bool)IN0(unit->mNumInputs - 2);
    bool isFadeOut = (bool)IN0(unit->mNumInputs - 1);

    unit->m_level = 0.0f;
    unit->m_resetVal = 1.0f; //Start from highest point (for fadeTime == 0)
    unit->phasor.init(isFadeIn, isFadeOut);

    // pointer, offset
    // initlevel, numstages, totaldur,
    // [dur, shape, curve, level] * numvals
    int numStages = (int)IN0(3);
    int numvals = numStages * 4; // initlevel + (levels, dur, shape, curves) * stages
    float offset = unit->m_offset = IN0(1);
    float point = unit->m_pointin = 0.0f - offset;
    unit->m_envvals = (float*)RTAlloc(unit->mWorld, (int)(numvals + 1.) * sizeof(float));

    if(unit->m_envvals)
    {
        unit->m_envvals[0] = IN0(2);

        UPDATE_ENVVALS

        if (unit->mCalcRate == calc_FullRate) {
            PHASOR_RESET_AR(false)
            SETCALC(AlgaDynamicIEnvGen_next_a);
        } else {
            PHASOR_RESET_KR(false)
            SETCALC(AlgaDynamicIEnvGen_next_k);
        }
        
        //Always start from 1 (fully on)
        OUT0(0) = 1.0f;
    }
    else
        SETCALC(ClearUnitOutputs);
}

void AlgaDynamicIEnvGen_Dtor(AlgaDynamicIEnvGen* unit) { 
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

void AlgaDynamicIEnvGen_next_a(AlgaDynamicIEnvGen* unit, int inNumSamples) {
    float* out = OUT(0);
    float level = unit->m_level;
    float offset = unit->m_offset;
    int numStages = (int)IN0(3);
    float point; // = unit->m_pointin;

    float totalDur = IN0(4);

    int stagemul;
    // pointer, offset
    // level0, numstages, totaldur,
    // [initval, [dur, shape, curve, level] * N ]
    
    bool updateEnv = (bool)IN0(unit->mNumInputs-3);
    if(updateEnv)
    {
        if(!unit->phasor.release)
            unit->m_resetVal = level;
        else
            unit->m_resetVal = (1.0f - level) * unit->m_resetVal;
        PHASOR_RESET_AR(true)
        UPDATE_ENVVALS
    }

    for (int i = 0; i < inNumSamples; i++) {
        float phase = unit->phasor.advance() * totalDur;
        if (phase != unit->m_pointin) {
            unit->m_pointin = point = sc_max(phase - offset, 0.0);
            float newtime = 0.f;
            int stage = 0;
            float seglen = 0.f;
            if (point >= totalDur) {
                unit->m_level = level = unit->m_envvals[numStages * 4]; // grab the last value
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

void AlgaDynamicIEnvGen_next_k(AlgaDynamicIEnvGen* unit, int inNumSamples) {
    float* out = OUT(0);
    float level = unit->m_level;
    float offset = unit->m_offset;
    int numStages = (int)IN0(3);
    float point; // = unit->m_pointin;

    float totalDur = IN0(4);

    int stagemul;
    // pointer, offset
    // level0, numstages, totaldur,
    // [initval, [dur, shape, curve, level] * N ]

    bool updateEnv = (bool)IN0(unit->mNumInputs-3);
    if(updateEnv)
    {
        if(!unit->phasor.release)
            unit->m_resetVal = level;
        else
            unit->m_resetVal = (1.0f - level) * unit->m_resetVal;
        PHASOR_RESET_KR(true)
        UPDATE_ENVVALS
    }

    float phase = unit->phasor.advance() * totalDur;

    if (phase != unit->m_pointin) {
        unit->m_pointin = point = sc_max(phase - offset, 0.0);
        float newtime = 0.f;
        int stage = 0;
        float seglen = 0.f;
        if (point >= totalDur) {
            unit->m_level = level = unit->m_envvals[numStages * 4]; // grab the last value
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

PluginLoad(AlgaDynamicIEnvGenUGens) 
{
    ft = inTable; 
    DefineDtorUnit(AlgaDynamicIEnvGen);
}
