//These are actually only needed for SC < 3.13:
//https://github.com/supercollider/supercollider/pull/5601

+AudioControl {
    *ar { arg values;
        ^AlgaAudioControl.ar(values);
    }
}

+LagControl {
    *ar { arg values, lags;
        ^AlgaAudioControl.ar(values).lag(lags)
    }
}

