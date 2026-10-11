#include "BassEnhancerDSP.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace reaperiani
{
namespace
{
constexpr float pi = 3.14159265358979323846f;
constexpr int oversamplingFactor = 4;

float clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

float boundedFrequency(float frequency, double rate)
{
    const auto maximum = static_cast<float>(rate * 0.45);
    return std::clamp(frequency, 5.0f, maximum);
}
} // namespace

void BassEnhancerDSP::Biquad::setLowPass(double rate, float cutoffHz)
{
    const auto frequency = boundedFrequency(cutoffHz, rate);
    const auto omega = 2.0f * pi * frequency / static_cast<float>(rate);
    const auto cosine = std::cos(omega);
    const auto sine = std::sin(omega);
    constexpr float q = 0.70710678118f;
    const auto alpha = sine / (2.0f * q);
    const auto a0 = 1.0f + alpha;

    b0 = ((1.0f - cosine) * 0.5f) / a0;
    b1 = (1.0f - cosine) / a0;
    b2 = ((1.0f - cosine) * 0.5f) / a0;
    a1 = (-2.0f * cosine) / a0;
    a2 = (1.0f - alpha) / a0;
}

void BassEnhancerDSP::Biquad::setHighPass(double rate, float cutoffHz)
{
    const auto frequency = boundedFrequency(cutoffHz, rate);
    const auto omega = 2.0f * pi * frequency / static_cast<float>(rate);
    const auto cosine = std::cos(omega);
    const auto sine = std::sin(omega);
    constexpr float q = 0.70710678118f;
    const auto alpha = sine / (2.0f * q);
    const auto a0 = 1.0f + alpha;

    b0 = ((1.0f + cosine) * 0.5f) / a0;
    b1 = -(1.0f + cosine) / a0;
    b2 = ((1.0f + cosine) * 0.5f) / a0;
    a1 = (-2.0f * cosine) / a0;
    a2 = (1.0f - alpha) / a0;
}

void BassEnhancerDSP::Biquad::reset()
{
    z1 = 0.0f;
    z2 = 0.0f;
}

float BassEnhancerDSP::Biquad::process(float input)
{
    const auto output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return output;
}

void BassEnhancerDSP::prepare(double newSampleRate, int maximumChannels)
{
    sampleRate = std::clamp(newSampleRate, 8000.0, 384000.0);
    channelStates.resize(static_cast<std::size_t>(std::max(1, maximumChannels)));
    outputGainSmoothing = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * 0.02));
    envelopeAttack = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * 0.005));
    envelopeRelease = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * 0.12));
    reset();
}

void BassEnhancerDSP::reset()
{
    for (auto& state : channelStates)
    {
        state.bassLowPass.reset();
        state.bassHighPass.reset();
        state.bassHighPass2.reset();
        state.cleanHighPass.reset();
        state.harmonicHighPass.reset();
        state.harmonicLowPass.reset();
        state.previousShaperInput = 0.0f;
        state.envelope = 0.0f;
    }
    smoothedOutputGain = 1.0f;
    originalBassMeterDb.store(-60.0f);
    harmonicsMeterDb.store(-60.0f);
    outputLeftMeterDb.store(-60.0f);
    outputRightMeterDb.store(-60.0f);
    outputClipping.store(false);
}

MeterReadings BassEnhancerDSP::getMeterReadings() const noexcept
{
    return {originalBassMeterDb.load(), harmonicsMeterDb.load(),
            outputLeftMeterDb.load(), outputRightMeterDb.load(), outputClipping.load()};
}

void BassEnhancerDSP::processBlock(float* const* channels,
                                  int channelCount,
                                  int sampleCount,
                                  const BassEnhancerParameters& rawParameters)
{
    if (channels == nullptr || sampleCount <= 0 || channelCount <= 0)
        return;

    const auto activeChannels = std::min(channelCount, static_cast<int>(channelStates.size()));
    if (activeChannels <= 0)
        return;

    const auto frequency = std::clamp(rawParameters.bassFrequencyHz, 35.0f, 180.0f);
    const auto amount = clamp01(rawParameters.harmonicAmount);
    const auto character = clamp01(rawParameters.character);
    const auto decay = clamp01(rawParameters.harmonicDecay);
    const auto dynamics = clamp01(rawParameters.dynamics);
    const auto clean = clamp01(rawParameters.clean);
    const auto targetOutputGain = std::pow(10.0f, std::clamp(rawParameters.outputGainDb, -24.0f, 12.0f) / 20.0f);

    const auto cleanCutoff = 6.0f + clean * 74.0f;
    const auto harmonicHighPassCutoff = frequency * 0.72f;
    const auto harmonicLowPassCutoff = frequency * (1.4f + 2.5f * decay);

    for (int channel = 0; channel < activeChannels; ++channel)
    {
        auto& state = channelStates[static_cast<std::size_t>(channel)];
        state.bassLowPass.setLowPass(sampleRate, frequency);
        state.bassHighPass.setHighPass(sampleRate, frequency);
        state.bassHighPass2.setHighPass(sampleRate, frequency);
        state.cleanHighPass.setHighPass(sampleRate, cleanCutoff);
        state.harmonicHighPass.setHighPass(sampleRate, harmonicHighPassCutoff);
        state.harmonicLowPass.setLowPass(sampleRate, harmonicLowPassCutoff);
    }

    const auto drive = 1.5f + amount * 5.5f;
    const auto normalizer = std::max(0.001f, std::tanh(drive));
    const auto linearSlope = drive / normalizer;
    double bassEnergy = 0.0;
    double harmonicEnergy = 0.0;
    std::array<float, 2> outputPeaks{0.0f, 0.0f};

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        smoothedOutputGain += (targetOutputGain - smoothedOutputGain) * outputGainSmoothing;

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto& state = channelStates[static_cast<std::size_t>(channel)];
            const auto input = channels[channel][sample];
            const auto low = state.bassLowPass.process(input);
            const auto high = state.bassHighPass2.process(state.bassHighPass.process(input));
            const auto shaperInput = state.cleanHighPass.process(low);
            bassEnergy += static_cast<double>(low) * low;

            const auto detector = std::abs(shaperInput);
            const auto detectorCoefficient = detector > state.envelope ? envelopeAttack : envelopeRelease;
            state.envelope += (detector - state.envelope) * detectorCoefficient;
            const auto levelAboveThreshold = std::max(0.0f, state.envelope - 0.045f);
            const auto dynamicGain = 1.0f / (1.0f + dynamics * levelAboveThreshold * 4.0f);

            float shaped = 0.0f;
            for (int step = 1; step <= oversamplingFactor; ++step)
            {
                const auto fraction = static_cast<float>(step) / static_cast<float>(oversamplingFactor);
                const auto oversampledInput = state.previousShaperInput
                    + (shaperInput - state.previousShaperInput) * fraction;
                const auto normalized = std::tanh(drive * oversampledInput) / normalizer;
                const auto oddHarmonics = normalized - linearSlope * oversampledInput;
                const auto evenHarmonics = normalized * normalized;
                const auto characterBlend = oddHarmonics * (1.0f - character)
                    + evenHarmonics * character;
                shaped += characterBlend;
            }
            state.previousShaperInput = shaperInput;
            shaped /= static_cast<float>(oversamplingFactor);

            const auto harmonics = state.harmonicHighPass.process(
                state.harmonicLowPass.process(shaped * amount * dynamicGain * 1.35f));
            harmonicEnergy += static_cast<double>(harmonics) * harmonics;
            const auto original = rawParameters.originalBass ? input : high;
            const auto output = (original + harmonics) * smoothedOutputGain;
            channels[channel][sample] = output;
            if (channel < static_cast<int>(outputPeaks.size()))
                outputPeaks[static_cast<std::size_t>(channel)] =
                    std::max(outputPeaks[static_cast<std::size_t>(channel)], std::abs(output));
        }
    }

    // Preserve the input if a host provides more output channels than the prepared layout.
    for (int channel = activeChannels; channel < channelCount; ++channel)
        if (channels[channel] != nullptr)
            for (int sample = 0; sample < sampleCount; ++sample)
                channels[channel][sample] *= smoothedOutputGain;

    const auto sampleTotal = static_cast<double>(activeChannels) * sampleCount;
    const auto rmsDb = [sampleTotal](double energy)
    {
        const auto rmsValue = std::sqrt(energy / sampleTotal);
        return static_cast<float>(std::clamp(20.0 * std::log10(std::max(rmsValue, 1.0e-6)), -60.0, 6.0));
    };
    const auto peakDb = [](float peak)
    {
        return std::clamp(20.0f * std::log10(std::max(peak, 1.0e-6f)), -60.0f, 6.0f);
    };
    const auto smooth = [](std::atomic<float>& meter, float nextValue)
    {
        const auto previous = meter.load();
        const auto coefficient = nextValue > previous ? 0.72f : 0.16f;
        meter.store(previous + (nextValue - previous) * coefficient);
    };

    smooth(originalBassMeterDb, rmsDb(bassEnergy));
    smooth(harmonicsMeterDb, rmsDb(harmonicEnergy));
    smooth(outputLeftMeterDb, peakDb(outputPeaks[0]));
    smooth(outputRightMeterDb, peakDb(activeChannels > 1 ? outputPeaks[1] : outputPeaks[0]));
    outputClipping.store(outputPeaks[0] >= 0.999f || outputPeaks[1] >= 0.999f);
}
} // namespace reaperiani
