#include "SC_PlugIn.h"

static InterfaceTable *ft;

struct AlgaAudioControl : public Unit 
{
    float* prevVal;
    float* m_prevBus;
    bool m_busUsedInPrevCycle;
};

static void AlgaAudioControl_next_1(AlgaAudioControl* unit, int inNumSamples);
static void AlgaAudioControl_next_k(AlgaAudioControl* unit, int inNumSamples);
static void AlgaAudioControl_Ctor(AlgaAudioControl* unit);

void AlgaAudioControl_Ctor(AlgaAudioControl* unit) 
{
    unit->prevVal = (float*)RTAlloc(unit->mWorld, unit->mNumOutputs * sizeof(float));
    unit->m_prevBus = NULL;
    unit->m_busUsedInPrevCycle = false;
    for (int i = 0; i < unit->mNumOutputs; i++) {
        unit->prevVal[i] = 0.0;
    }
    if (unit->mNumOutputs == 1) {
        SETCALC(AlgaAudioControl_next_1);
        AlgaAudioControl_next_1(unit, 1);
    } else {
        SETCALC(AlgaAudioControl_next_k);
        AlgaAudioControl_next_k(unit, 1);
    }
}

void AlgaAudioControl_next_k(AlgaAudioControl* unit, int inNumSamples) 
{
    uint32 numChannels = unit->mNumOutputs;
    float* prevVal = unit->prevVal;
    float** mapin = unit->mParent->mMapControls + unit->mSpecialIndex;
    World* world = unit->mWorld;
    int32 bufCounter = world->mBufCounter;

    int32* touched = world->mAudioBusTouched;
    int32* channelOffsets = unit->mParent->mAudioBusOffsets;

    //Changing buffer: reset m_busUsedInPrevCycle
    if(*mapin != unit->m_prevBus)
    {
        unit->m_busUsedInPrevCycle = false;
        unit->m_prevBus = *mapin;
    }

    for (uint32 i = 0; i < numChannels; ++i, mapin++) {
        float* out = OUT(i);
        int* mapRatep;
        int mapRate;
        float nextVal, curVal, valSlope;
        mapRatep = unit->mParent->mControlRates + unit->mSpecialIndex;
        mapRate = mapRatep[i];
        switch (mapRate) {
            case 0: {
                for (int j = 0; j < inNumSamples; j++) {
                    out[j] = *mapin[0];
                }
            } break;
            case 1: {
                nextVal = *mapin[0];
                curVal = prevVal[i];
                valSlope = CALCSLOPE(nextVal, curVal);
                for (int j = 0; j < inNumSamples; j++) {
                    out[j] = curVal; // should be prevVal
                    curVal += valSlope;
                }
                unit->prevVal[i] = curVal;
            } break;
                // case 2 - AudioControl is in effect
            case 2: {
                /*
                    the graph / unit stores which controls (based on special index) are mapped
                    to which audio buses this is needed to access the touched values for when
                    an audio bus has been written to last. bufCounter is the current value for the
                    control period (basically, the number of control periods that have elapsed
                    since the server started). We check the touched value for the mapped audio
                    bus to see if it has been written to in the current control period or the
                    previous control period (to enable an InFeedback type of mapping)...
                */
                int thisChannelOffset = channelOffsets[unit->mSpecialIndex + i];
                bool validOffset = thisChannelOffset >= 0;
                int diff = bufCounter - touched[thisChannelOffset];
                if (validOffset && diff == 0) {
                    Copy(inNumSamples, out, *mapin);
                    unit->m_busUsedInPrevCycle = true;
                }
                else if(validOffset && diff == 1) {
                    if(unit->m_busUsedInPrevCycle){
                        Fill(inNumSamples, out, 0.f);
                        unit->m_busUsedInPrevCycle = false;
                    }
                    else
                        Copy(inNumSamples, out, *mapin);
                }
                else {
                    Fill(inNumSamples, out, 0.f);
                    unit->m_busUsedInPrevCycle = false;
                }
            } break;
        }
    }
}

void AlgaAudioControl_next_1(AlgaAudioControl* unit, int inNumSamples) {
    float* mapin = *(unit->mParent->mMapControls + unit->mSpecialIndex);
    float* out = OUT(0);
    int* mapRatep;
    int mapRate;
    float nextVal, curVal, valSlope;
    float* prevVal;
    prevVal = unit->prevVal;
    curVal = prevVal[0];
    mapRatep = unit->mParent->mControlRates + unit->mSpecialIndex;
    mapRate = mapRatep[0];
    World* world = unit->mWorld;
    int32* touched = world->mAudioBusTouched;
    int32 bufCounter = world->mBufCounter;
    int32* channelOffsets = unit->mParent->mAudioBusOffsets;

    //Changing buffer: reset m_busUsedInPrevCycle
    if(mapin != unit->m_prevBus)
    {
        unit->m_busUsedInPrevCycle = false;
        unit->m_prevBus = mapin;
    }

    switch (mapRate) {
        case 0: {
            for (int i = 0; i < inNumSamples; i++) {
                out[i] = mapin[0];
            }
        } break;
        case 1: {
            nextVal = mapin[0];
            valSlope = CALCSLOPE(nextVal, curVal);
            for (int i = 0; i < inNumSamples; i++) {
                out[i] = curVal;
                curVal += valSlope;
            }
            unit->prevVal[0] = curVal;
        } break;
            /*
            see case 2 comments above in definition for AudioControl_next_k
            */
        case 2: {
            int thisChannelOffset = channelOffsets[unit->mSpecialIndex];
            bool validOffset = thisChannelOffset >= 0;
            int diff = bufCounter - touched[thisChannelOffset];
            if (validOffset && diff == 0) {
                Copy(inNumSamples, out, mapin);
                unit->m_busUsedInPrevCycle = true;
            }
            else if(validOffset && diff == 1) {
                if(unit->m_busUsedInPrevCycle){
                    Fill(inNumSamples, out, 0.f);
                    unit->m_busUsedInPrevCycle = false;
                }
                else
                    Copy(inNumSamples, out, mapin);
            }
            else {
                Fill(inNumSamples, out, 0.f);
                unit->m_busUsedInPrevCycle = false;
            } break;
        }
    }
}

PluginLoad(AlgaAudioControlUGens) 
{
    ft = inTable; 
    DefineSimpleUnit(AlgaAudioControl);
}
