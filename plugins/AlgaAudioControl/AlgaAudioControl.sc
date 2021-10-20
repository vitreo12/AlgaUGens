AlgaAudioControl : AudioControl {
    *ar { arg values;
        ^this.multiNewList(['audio'] ++ values.asArray)
    }
}

AlgaLagControl : LagControl {
    *ar { arg values, lags;
        ^AlgaAudioControl.ar(values).lag(lags)
    }
}

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

