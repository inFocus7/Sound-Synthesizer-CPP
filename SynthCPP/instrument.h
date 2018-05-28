#pragma once
#define OSC_SINE 0
#define OSC_SQUARE 1
#define OSC_TRIANGLE 2
#define OSC_SAW_ANA 3
#define OSC_SAW_DIG 4
#define OSC_NOISE 5

// Globals
extern unsigned int
	smplrt{ SAMPLERATE },
	chnl{ CHANNELS },
	blck{ BLOCKS },
	blcksmp{ BLOCKSAMPLES };
// Get sound devices
std::vector<std::wstring> devices{ noiseMaker<short>::DEVICES() };
std::wstring
dev{ devices[0] };
extern double
	OctaveBase{ 440.0 }, //A440
	KeyEdit{ pow(2.0, 1.0 / 12.0) },
	Master{ 0.5 };
extern std::atomic<double> FREQUENCY = 0.0;

// ADSR
class ADSREnvelope
{
public:
	double
		attackTime,
		decayTime,
		releaseTime,
		sustainAmplitude,
		startAmplitude,
		triggerOn,
		triggerOff;
	bool
		notePressed;

	ADSREnvelope()
	{
		attackTime = 0.01;
		decayTime = 1.0;
		startAmplitude = 1.0;
		sustainAmplitude = 0.0;
		releaseTime = 1.0;
		triggerOn = 0.0;
		triggerOff = 0.0;
		notePressed = false;
	}

	double GetAmplitude(double time)
	{
		double Amplitude = 0.0;
		double lifeTime = time - triggerOn;

		if (notePressed) //ADS
		{
			// Attack
			if (lifeTime <= attackTime)
				Amplitude = (lifeTime / attackTime) * startAmplitude;
			// Decay
			if (lifeTime > attackTime && lifeTime <= (attackTime + decayTime))
				Amplitude = ((lifeTime - attackTime) / decayTime) * (sustainAmplitude - startAmplitude) + startAmplitude;
			// Sustain
			if (lifeTime > (attackTime + decayTime))
				Amplitude = sustainAmplitude;
		}
		else //R
		{
			// Release
			Amplitude = ((time - triggerOff) / releaseTime) * (0.0 - sustainAmplitude) + sustainAmplitude;
		}

		if (Amplitude <= 0.0001)
		{
			Amplitude = 0.00;
		}

		return Amplitude;
	}

	void noteOn(double time)
	{
		triggerOn = time;
		notePressed = true;
	}

	void noteOff(double time)
	{
		triggerOff = time;
		notePressed = false;
	}
};

double htv(double hertz)
{
	return hertz * 2 * PI;
}

double osc(double hertz, double time, int type = OSC_SINE, double LFOHertz = 0.0, double LFOAmpltude = 0.0)
{
	double baseFreq{ htv(hertz) * time + LFOAmpltude * hertz * sin(htv(LFOHertz) * time) };
	switch (type)
	{
	case OSC_SINE: // SINE
		return sin(baseFreq);
	case OSC_SQUARE: // SQUARE
		return sin(baseFreq) > 0.0 ? 1.0 : -1.0;
	case OSC_TRIANGLE: // TRIANGLE
		return asin(sin(baseFreq)) * (2.0 / PI);
	case OSC_SAW_ANA: // SAWTOOTH (analog / slower / analogue)
	{
		double output = 0.0;
		for (double i{ 1.0 }; i < 100.0; i++)
			output += (sin(i * baseFreq) / i);
		return output * (2.0 / PI);
	}
	case OSC_SAW_DIG: // SAWTOOTH (digital / faster / harsh)
		return (2.0 / PI) * (hertz * PI * fmod(time, 1.0 / hertz) - (PI / 2.0));
	case OSC_NOISE: // NOISE
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
	default:
		return 0;
	}
}

// INSTRUMENTS
class instrument
{
public:
	double volume;
	ADSREnvelope env;
	virtual double sound(double time, double freq) = 0;
};

class bell : public instrument
{
public:
	bell()
	{
		env.attackTime = 0.01;
		env.decayTime = 1.0;
		env.startAmplitude = 1.0;
		env.sustainAmplitude = 0.0;
		env.releaseTime = 1.0;
	}

	double sound(double time, double freq)
	{
		double output{
		env.GetAmplitude(time) *
		(
			/* Beautiful Harmonic Sound "Harmonica"
			+ 1.0 * osc(FREQUENCY, time, OSC_SQUARE, 5.0, 0.001)
			+ 0.5 * osc(FREQUENCY * 1.5, time, OSC_SQUARE)
			+ 0.25 * osc(FREQUENCY * 2.0, time, OSC_SQUARE)
			+ 0.05 * osc(0, time, OSC_NOISE)
			*/
			+ 1.0 * osc(FREQUENCY * 2.0, time, OSC_SINE, 5.0, 0.001)
			+ 0.5 * osc(FREQUENCY * 3.0, time, OSC_SINE)
			+ 0.25 * osc(FREQUENCY * 4.0, time, OSC_SINE)
			)
		};

		return output;
	};
};

class harmonica : public instrument
{
public:
	harmonica()
	{
		env.attackTime = 0.5;
		env.decayTime = 0.5;
		env.startAmplitude = 1.0;
		env.sustainAmplitude = 0.8;
		env.releaseTime = 1.0;
	}

	double sound(double time, double freq)
	{
		double output{
			env.GetAmplitude(time) *
			(
				+ 1.0 * osc(FREQUENCY, time, OSC_SQUARE, 5.0, 0.001)
				+ 0.5 * osc(FREQUENCY * 1.5, time, OSC_SQUARE)
				+ 0.25 * osc(FREQUENCY * 2.0, time, OSC_SQUARE)
				+ 0.05 * osc(0, time, OSC_NOISE)
				)
		};

		return output;
	};
};

// EFFECTS

	// Reverb

	// Delay

	// Compression

	// Chorus

	// Flanger

	// EQ

	// Looper

	// Phaser
