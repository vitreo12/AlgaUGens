// PluginAlgaAudioControl.cpp
// Francesco Cameli (vitreo12@site.com)

#include "SC_PlugIn.hpp"
#include "AlgaAudioControl.hpp"

static InterfaceTable* ft;

namespace AlgaAudioControl {

AlgaAudioControl::AlgaAudioControl() {
    mCalcFunc = make_calc_function<AlgaAudioControl, &AlgaAudioControl::next>();
    next(1);
}

void AlgaAudioControl::next(int nSamples) {
    const float* input = in(0);
    const float* gain = in(1);
    float* outbuf = out(0);

    // simple gain function
    for (int i = 0; i < nSamples; ++i) {
        outbuf[i] = input[i] * gain[i];
    }
}

} // namespace AlgaAudioControl

PluginLoad(AlgaAudioControlUGens) {
    // Plugin magic
    ft = inTable;
    registerUnit<AlgaAudioControl::AlgaAudioControl>(ft, "AlgaAudioControl", false);
}
