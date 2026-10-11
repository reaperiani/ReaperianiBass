#pragma once

#include <atomic>
#include <vector>

namespace reaperiani
{
struct MeterReadings
{
    float originalBassDb = -60.0f;
    float harmonicsDb = -60.0f;
    float outputLeftDb = -60.0f;
    float outputRightDb = -60.0f;
    bool clipping = false;
};

struct BassEnhancerParameters
{
    float bassFrequencyHz = 90.0f;
    float harmonicAmount = 0.25f;
    float character = 0.35f;
    float harmonicDecay = 0.55f;
    float dynamics = 0.25f;
    float clean = 0.0f;
    bool originalBass = true;
    float outputGainDb = 0.0f;
};

class BassEnhancerDSP
{
public:
    void prepare(double newSampleRate, int maximumChannels);
    void reset();
    void processBlock(float* const* channels,
                      int channelCount,
                      int sampleCount,
                      const BassEnhancerParameters& parameters);
    MeterReadings getMeterReadings() const noexcept;

private:
    struct Biquad
    {
        void setLowPass(double sampleRate, float cutoffHz);
        void setHighPass(double sampleRate, float cutoffHz);
        void reset();
        float process(float input);

        float b0 = 1.0f;
        float b1 = 0.0f;
        float b2 = 0.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;
        float z1 = 0.0f;
        float z2 = 0.0f;
    };

    struct ChannelState
    {
        Biquad bassLowPass;
        Biquad bassHighPass;
        Biquad bassHighPass2;
        Biquad cleanHighPass;
        Biquad harmonicHighPass;
        Biquad harmonicLowPass;
        float previousShaperInput = 0.0f;
        float envelope = 0.0f;
    };

    double sampleRate = 44100.0;
    float smoothedOutputGain = 1.0f;
    float outputGainSmoothing = 0.01f;
    float envelopeAttack = 0.01f;
    float envelopeRelease = 0.001f;
    std::vector<ChannelState> channelStates;
    std::atomic<float> originalBassMeterDb{-60.0f};
    std::atomic<float> harmonicsMeterDb{-60.0f};
    std::atomic<float> outputLeftMeterDb{-60.0f};
    std::atomic<float> outputRightMeterDb{-60.0f};
    std::atomic<bool> outputClipping{false};
};
} // namespace reaperiani
