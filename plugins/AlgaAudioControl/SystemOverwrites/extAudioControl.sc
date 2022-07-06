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

