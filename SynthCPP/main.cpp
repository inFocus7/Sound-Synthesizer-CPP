#include <iostream>
#include "noise.h"
#include "instrument.h"
instrument *voice = nullptr;

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

double noiz(double time)
{
	double output{ voice->sound(time, FREQUENCY) };
	return Master * output; // Master Volume
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
	voice = new harmonica(); // Instrument to be played

	// Link noise function w/ synth
	sound.SetUserFunction(noiz);
	int currKey = -1;

	// Keyboard
	while (true)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			std::wcout << "Thanks for playing!\n";
			return 0;
		}

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
						//std::wcout << "Pressed: " << FREQUENCY << "htz\n";
						voice->env.noteOn(sound.GetTime());
					}
					else
					{
						double tmp{ OctaveBase };
						if (key == 15 && OctaveBase != 13.75)
							tmp = tmp / 2;
						else if (key == 16 && OctaveBase != 1760)
							tmp = tmp * 2;
						OctaveBase = tmp;
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
				//std::wcout << "Released: " << FREQUENCY << '\n';
				voice->env.noteOff(sound.GetTime());
				currKey = -1;
			}
			keyPressed = false;
		}
	}
	return 0;
}