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

