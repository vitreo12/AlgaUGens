AlgaDynamicIEnvGen : UGen {

	*ar { arg envelope, fadeTime = 0, updateEnv = 0, isFadeIn = 0, isFadeOut = 0;
		envelope = this.convertEnv(envelope, updateEnv, isFadeIn, isFadeOut);
		^this.multiNewList(['audio', fadeTime, envelope]);
	}

	*kr { arg envelope, fadeTime = 0, updateEnv = 0, isFadeIn = 0, isFadeOut = 0;
		envelope = this.convertEnv(envelope, updateEnv, isFadeIn, isFadeOut);
		^this.multiNewList(['control', fadeTime, envelope]);
	}

	*convertEnv { arg env, updateEnv, isFadeIn, isFadeOut;
		if(env.isSequenceableCollection) {
            env = env.add(updateEnv);
			env = env.add(isFadeIn);
			env = env.add(isFadeOut);
            ^env.reference
        }; // raw envelope data
		env = env.asArrayForInterpolation;
        env = env.add(updateEnv);
		env = env.add(isFadeIn);
		env = env.add(isFadeOut);
        ^env.collect(_.reference).unbubble;
	}

	*new1 { arg rate, index, envArray;
		^super.new.rate_(rate).addToSynth.init([index] ++ envArray.dereference)
	}

	init { arg theInputs;
		// store the inputs as an array
		inputs = theInputs;
	}

	argNamesInputsOffset { ^2 }

}
