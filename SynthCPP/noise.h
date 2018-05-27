/*
ORIGINAL CODE BY JAVIDX9 ON YOUTUBE
WILL BE MODIFIED WHILE WORKING AND LEARNING ABOUT SOUND IN WINDOWS.
*/

#pragma once
#pragma comment(lib, "winmm.lib")
#include <iostream>
#include <Windows.h>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>

const double PI = 2.0 * std::acos(0.0); // acos(0) = pi/2.	
// Defaults
extern unsigned int
	SAMPLERATE{ 44100 },
	CHANNELS{ 1 },
	BLOCKS{ 8 },
	BLOCKSAMPLES{ 512 };

template <typename T>
class noiseMaker
{
private:
	// Variables
	double (*m_userFunction)(double);
	unsigned int 
		m_sampleRate, 
		m_channels, 
		m_blocks, 
		m_blockSamples, 
		m_blockCurrent;
	T* blockMemory;
	WAVEHDR * waveHeader;
	HWAVEOUT device;

	std::thread m_thread;
	std::atomic<bool> ready;
	std::atomic<unsigned int> blockFree;
	std::atomic<double> globalTime;
	std::condition_variable cvBlockNotzero;
	std::mutex mBlockNotZero;

	// Sound Card Request Handler
	void waveOutput(HWAVEOUT t_waveOut, UINT msg, DWORD param1, DWORD param2)
	{
		if (msg != WOM_DONE)
			return;

		blockFree++;
		std::unique_lock<std::mutex> lm(mBlockNotZero); // ?
		cvBlockNotzero.notify_one();
	}

	// SCRH Wrapper
	static void CALLBACK waveOutProcWrap(HWAVEOUT t_waveOut, UINT msg, DWORD instance, DWORD param1, DWORD param2)
	{
		((noiseMaker*)instance)->waveOutput(t_waveOut, msg, param1, param2);
	}

	bool Destroy()
	{
		return false;
	}

	void Stop()
	{
		ready = false;
		m_thread.join();
	}

	virtual double UserProcess(double time)
	{
		return 0.0;
	}

	double clip(double sample, double maxdB)
	{
		if (sample >= 0.0)
			return fmin(sample, maxdB);
		return fmax(sample, -maxdB);
	}

	// Main thread which waits to take input from user and request soundcard to fill 'blocks' with audio data.

public:

	noiseMaker(std::wstring t_outputDevice, unsigned int t_sampleRate, unsigned int t_channels, unsigned int t_blocks, unsigned int t_blockSamples)
	{
		//Set Defaults (For later use...)
		if (t_sampleRate == 0)
			t_sampleRate = SAMPLERATE;
		if (t_channels == 0)
			t_channels = CHANNELS;
		if (t_blocks == 0)
			t_blocks = BLOCKS;
		if (t_blockSamples == 0)
			t_blockSamples = BLOCKSAMPLES;

		Create(t_outputDevice, t_sampleRate, t_channels, t_blocks, t_blockSamples);
	}
	
	void SetUserFunction(double(*func)(double))
	{
		m_userFunction = func;
	}

	double GetTime()
	{
		return globalTime;
	}

	void mTHREAD()
	{
		globalTime = 0.0;
		double timeStep{ 1.0 / (double)m_sampleRate };

		// "Goofy hack to get maximum integer for a type at run-time"
		T maxSample{ (T)pow(2, (sizeof(T) * 8) - 1) - 1 };
		double dMaxSample{ (double)maxSample };
		T prevSample{ 0 };

		while (ready)
		{
			// Wait until free block
			if (blockFree == 0)
			{
				std::unique_lock<std::mutex> lm(mBlockNotZero);
				cvBlockNotzero.wait(lm);
			}

			// Use block
			blockFree--;

			// Prepare block for process
			if (waveHeader[m_blockCurrent].dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(device, &waveHeader[m_blockCurrent], sizeof(WAVEHDR));

			T newSample{ 0 };
			int currentBlock = m_blockCurrent * m_blockSamples;

			for (unsigned int i{ 0 }; i < m_blockSamples; i++)
			{
				// User Process
				if (m_userFunction == nullptr)
					newSample = (T)(clip(UserProcess(globalTime), 1.0) * dMaxSample);
				else
					newSample = (T)(clip(m_userFunction(globalTime), 1.0) * dMaxSample);

				blockMemory[currentBlock + i] = newSample;
				prevSample = newSample;
				globalTime = globalTime + timeStep;
			}

			//Send block to sound device
			waveOutPrepareHeader(device, &waveHeader[m_blockCurrent], sizeof(WAVEHDR));
			waveOutWrite(device, &waveHeader[m_blockCurrent], sizeof(WAVEHDR));
			m_blockCurrent++;
			m_blockCurrent %= m_blocks;
		}
	}

	static std::vector<std::wstring> DEVICES() // Vector of audio devices.
	{
		int numDevices = waveOutGetNumDevs();
		std::vector<std::wstring> devices;
		WAVEOUTCAPS woc;
		for (int i{ 0 }; i < numDevices; i++)
			if (waveOutGetDevCaps(i, &woc, sizeof(WAVEOUTCAPS)) == S_OK)
				devices.push_back(woc.szPname);

		return devices;
	}

	bool Create(std::wstring t_outputDevice, unsigned int t_sampleRate, unsigned int t_channels, unsigned int t_blocks, unsigned int t_blockSamples)
	{
		ready = false;
		m_sampleRate = t_sampleRate,
		m_channels = t_channels,
		m_blocks =  t_blocks,
		m_blockSamples = t_blockSamples,
		blockFree = t_blocks,
		m_blockCurrent = 0;
		blockMemory = nullptr,
		waveHeader = nullptr,
		m_userFunction = nullptr;

		// Devices
		std::vector<std::wstring> devices = DEVICES();
		auto c_device = std::find(devices.begin(), devices.end(), t_outputDevice);
		if (c_device != devices.end()) // If not at end of vector, meaning found
		{
			// Set Up Device Properties
			int deviceID{ std::distance(devices.begin(), c_device) };
			WAVEFORMATEX waveFormat; // Waveformatex up to 2 channels
			waveFormat.wFormatTag = WAVE_FORMAT_PCM; 
			waveFormat.nSamplesPerSec = m_sampleRate;
			waveFormat.wBitsPerSample = sizeof(T) * 8; // bytes to bits
			waveFormat.nChannels = m_channels;
			waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels; // number of channels * bytes/sample
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign; // samples/sec * totalbytes/sample
			waveFormat.cbSize =  0;

			// Open Device
			if (waveOutOpen(&device, deviceID, &waveFormat, (DWORD_PTR)waveOutProcWrap, (DWORD_PTR)this, CALLBACK_FUNCTION) != S_OK)
				return Destroy();
		}

		// Allocate Memory
		blockMemory = new T[m_blocks * m_blockSamples];
		if (blockMemory == nullptr)
			return Destroy();
		ZeroMemory(blockMemory, sizeof(T) * m_blocks * m_blockSamples);

		waveHeader = new WAVEHDR[m_blocks];
		if (waveHeader == nullptr)
			return Destroy();
		ZeroMemory(waveHeader, sizeof(WAVEHDR) * m_blocks);

		// Link headers to block
		for (unsigned int i{ 0 }; i < m_blocks; i++)
		{
			waveHeader[i].dwBufferLength = m_blockSamples * sizeof(T);
			waveHeader[i].lpData = (LPSTR)(blockMemory + (i * m_blockSamples));
		}

		ready = true;

		m_thread = std::thread(&noiseMaker::mTHREAD, this);

		std::unique_lock<std::mutex> lm(mBlockNotZero);
		cvBlockNotzero.notify_one();

		return true;
	}

	~noiseMaker()
	{
		Destroy();
	}
};