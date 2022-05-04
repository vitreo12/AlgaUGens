AlgaDynamicIEnvGenBuf : UGen {
	*ar { arg envelope, fadeTime = 0, isFadeIn = 0, isFadeOut = 0, maxSize = 256, release = 0;
		^this.multiNewList(['audio', envelope, fadeTime, isFadeIn, isFadeOut, maxSize, release])
	}

	*kr { arg envelope, fadeTime = 0, isFadeIn = 0, isFadeOut = 0, maxSize = 256, release = 0;
		^this.multiNewList(['control', envelope, fadeTime, isFadeIn, isFadeOut, maxSize, release])
	}
}
