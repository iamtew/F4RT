#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <cmath>

namespace f4rt
{

struct Analyser
{
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    static constexpr int bins = fftSize / 2 + 1;
    static constexpr int waveN = 1024;
    static constexpr int specCols = 96;
    static constexpr int specRows = 48;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    float fifo[fftSize] {};
    int fifoIdx = 0;
    float magDb[2][bins] {};
    int magBank = 0;
    float wave[waveN] {};
    int waveW = 0;
    float spec[specCols][specRows] {};
    int specCol = 0;
    float envMon[6] {};
    float fmVis[4][4] {};
    double sampleRate = 44100.0;  // bin -> Hz for the log-axis views

    void prepare (double sr)
    {
        sampleRate = sr > 0.0 ? sr : 44100.0;
        fifoIdx = 0;
        waveW = 0;
        specCol = 0;
        for (int s = 0; s < 2; ++s)
            for (int i = 0; i < bins; ++i)
                magDb[s][i] = -90.f;
        for (int c = 0; c < specCols; ++c)
            for (int r = 0; r < specRows; ++r)
                spec[c][r] = 0.f;
    }

    void push (const juce::AudioBuffer<float>& buffer)
    {
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() <= 0)
            return;
        const float* l = buffer.getReadPointer (0);
        for (int i = 0; i < n; ++i)
        {
            wave[waveW] = l[i];
            waveW = (waveW + 1) % waveN;
            fifo[fifoIdx++] = l[i];
            if (fifoIdx >= fftSize)
            {
                fifoIdx = 0;
                float work[fftSize * 2] {};
                for (int k = 0; k < fftSize; ++k)
                    work[k] = fifo[k];
                window.multiplyWithWindowingTable (work, (size_t) fftSize);
                fft.performFrequencyOnlyForwardTransform (work);
                const int dst = 1 - magBank;
                for (int b = 0; b < bins; ++b)
                {
                    const float m = work[b] / (float) fftSize;
                    magDb[dst][b] = juce::Decibels::gainToDecibels (m + 1.0e-8f);
                }
                magBank = dst;
                const int col = specCol % specCols;
                for (int r = 0; r < specRows; ++r)
                {
                    const int b = 1 + r * (bins - 2) / specRows;
                    spec[col][r] = juce::jlimit (0.f, 1.f, (magDb[dst][b] + 80.f) / 80.f);
                }
                specCol++;
            }
        }
    }

    void copyWave (float* dest, int n) const
    {
        for (int i = 0; i < n; ++i)
            dest[i] = wave[(waveW + i * (waveN / n)) % waveN];
    }
};

} // namespace f4rt
