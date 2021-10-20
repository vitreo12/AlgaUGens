// PluginAlgaAudioControl.hpp
// Francesco Cameli (vitreo12@site.com)

#pragma once

#include "SC_PlugIn.hpp"

namespace AlgaAudioControl {

class AlgaAudioControl : public SCUnit {
public:
    AlgaAudioControl();

    // Destructor
    // ~AlgaAudioControl();

private:
    // Calc function
    void next(int nSamples);

    // Member variables
};

} // namespace AlgaAudioControl
