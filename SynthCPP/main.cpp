#include <iostream>
#include "noise.h"

class ADSREnvelope
{
private:
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
public:
	ADSREnvelope()
	{
		attackTime = 0.05; //10ms
		decayTime = 0.15;
		startAmplitude = 1.0;
		sustainAmplitude = 0.8;
		releaseTime = 0.05;
		triggerOn = 0.0;
		triggerOff = 0.0;
		notePressed = false;
	}

	double GetAmplitude(double time)
	{
		double Amplitude = 0.0;
		double currTime = time - triggerOn;

		if (notePressed) //ADS
		{
			// Attack
			if (currTime <= attackTime)
				Amplitude = (currTime / attackTime) * startAmplitude;
			// Decay
			if (currTime > attackTime && currTime <= (attackTime + decayTime))
				Amplitude = ((currTime - attackTime) / decayTime) * (sustainAmplitude - startAmplitude) + startAmplitude;
			// Sustain
			if (currTime > (attackTime + decayTime))
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

unsigned int
	smplrt{ SAMPLERATE },
	chnl{ CHANNELS },
	blck{ BLOCKS },
	blcksmp{ BLOCKSAMPLES };
// Get sound devices
std::vector<std::wstring> devices{ noiseMaker<short>::DEVICES() };
std::wstring
	dev{ devices[0] };
double
	OctaveBase{ 440.0 }, //A440
	KeyEdit{ pow(2.0, 1.0 / 12.0) };
std::atomic<double> FREQUENCY = 0.0;
ADSREnvelope envelope;

void CURR_SETUP()
{
	system("cls");
	std::wcout << "Current Setup: \n"
		<< "Sound Device: " << dev << '\n'
		<< "Sample Rate: " << smplrt << '\n'
		<< "Audio Channels: " << chnl << '\n'
		<< "Bits: " << blck << '\n'
		<< "Bit Samples: " << blcksmp << '\n';
}

double htv(double hertz)
{
	return hertz * 2 * PI;
}

double osc(double hertz, double time, int type)
{
	double baseSine{ sin(htv(hertz * time)) };
	switch (type)
	{
	case 0: // SINE
		return baseSine;
	case 1: // SQUARE
		return baseSine > 0.0 ? 1.0 : -1.0;
	case 2: // TRIANGLE
		return asin(baseSine) * 2.0 / PI;
	case 3: // SAWTOOTH (slower / analogue)
	{
		double output = 0.0;
		for (double i{ 1.0 }; i < 100.0; i++)
			output += (sin(i* htv(hertz) * time)) / i;
		return output * (2.0 / PI);
	}
	case 4: // SAWTOOTH (faster / harsh)
		return (2.0 / PI) * (hertz * PI * fmod(time, 1.0 / hertz) - (PI / 2.0));
	case 5: // NOISE
		return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;
	default:
		return 0;
	}
}

double noiz(double time)
{
	double output{ envelope.GetAmplitude(time) *  osc(FREQUENCY, time, 3) };

	return 0.3 * output; // Master Volume
}

int main()
{
	std::wcout << "SynthCPP\n";

	char
		mChoice,
		happy;
	unsigned int
		* samplerates = new unsigned int[3]{ 22050, 44100, 48000 },
		* bits = new unsigned int[3]{ 8, 16, 32 };

	// User Setup

	CURR_SETUP();
	std::wcout << "Would you like to manually set up values?\n (y/Y/n/N): ";
	std::cin >> mChoice;
	if (mChoice == 'y' || mChoice == 'Y')
	{
		do
		{
			CURR_SETUP();
			//Setup
				//Sound Device
			int devchoice;
			do
			{
				std::wcout << "Output Devices\n";
				for (unsigned int i{ 0 }; i < devices.size(); i++)
					std::wcout << '[' << i << ']' << ' ' << devices[i] << '\n';
				std::wcout << "Output Device [#]: ";
				std::cin >> devchoice;
			} while (devchoice < 0 || devchoice > devices.size() - 1);
			dev = devices[devchoice];
			//Sample Rate (accuracy of frequency)
			do
			{
				std::wcout << "Sample Rates\n";
				for (unsigned int i{ 0 }; i < 3; i++)
					std::wcout << '[' << i << ']' << ' ' << samplerates[i] << '\n';
				std::wcout << "Sample Rate [#]: ";
				std::cin >> smplrt;
			} while (smplrt < 0 || smplrt > 2);
			smplrt = samplerates[smplrt];
			//Audio Channels
			do
			{
				std::wcout << "Channel(s)\n [1]: Mono\n [2]: Stereo (2-channels)\n";
				std::wcout << "Channel(s) [#]: ";
				std::cin >> chnl;
			} while (chnl < 1 || chnl > 2);
			//Bits (accuracy of amplitude)
			do
			{
				std::wcout << "Bits\n";
				for (unsigned int i{ 0 }; i < 3; i++)
					std::wcout << '[' << i << ']' << ' ' << bits[i] << '\n';
				std::wcout << "Bits [#]: ";
				std::cin >> blck;
			} while (blck < 0 || blck > 2);
			blck = bits[blck];
			//Bit Samples -- TODO
			do
			{
				std::wcout << "Just using 512 bit samples for now.\n";
			} while (false);
			CURR_SETUP();
			std::wcout << "Are you okay with your current setup?\n (y/Y/n/N): ";
			std::cin >> happy;
		} while (happy == 'n' || happy == 'N');
	}
	// Create synth.
	noiseMaker<short> sound(dev, smplrt, chnl, blck, blcksmp);

	// Link noise function w/ synth
	sound.SetUserFunction(noiz);
	int currKey = -1;

	while (true)
	{
		// Keyboard!
		bool keyPressed{ false };
		for (int key{ 0 }; key < 17; key++)
		{
			if (GetAsyncKeyState((unsigned char)("AWSEDFTGYHUJKOLZX"[key])) & 0x8000)
			{
				if (currKey != key)
				{
					if (key != 15 && key != 16)
					{
						FREQUENCY = OctaveBase * pow(KeyEdit, key);
						std::wcout << "Note Pressed: " << FREQUENCY << '\n';
						envelope.noteOn(sound.GetTime());
					}
					else
					{
						double tmp{ OctaveBase };
						if (key == 15)
						{
							tmp = tmp / 2;
						}
						else
						{
							tmp = tmp * 2;
						}
						OctaveBase = (double)tmp;
						//std::cout << "OCTVBS: " << OctaveBase << '\n';
					}
					currKey = key;
				}
				keyPressed = true;
			}
		}

		if (!keyPressed)
		{
			if (currKey != -1)
			{
				std::wcout << "Note Released: " << FREQUENCY << '\n';
				envelope.noteOff(sound.GetTime());
				currKey = -1;
			}
			keyPressed = false;
		}
	}
	return 0;
}