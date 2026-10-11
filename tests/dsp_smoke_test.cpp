#include "BassEnhancerDSP.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

float rms(const std::array<float, 4096>& samples)
{
    double sum = 0.0;
    for (const auto sample : samples)
        sum += static_cast<double>(sample) * sample;
    return static_cast<float>(std::sqrt(sum / static_cast<double>(samples.size())));
}
} // namespace

int main()
{
    reaperiani::BassEnhancerDSP processor;
    processor.prepare(48000.0, 2);

    std::array<float, 4096> left{};
    std::array<float, 4096> right{};
    std::array<float*, 2> channels{left.data(), right.data()};
    reaperiani::BassEnhancerParameters parameters;

    processor.processBlock(channels.data(), 2, static_cast<int>(left.size()), parameters);
    require(rms(left) == 0.0f && rms(right) == 0.0f, "silence remains silent");

    constexpr float twoPi = 6.2831853071795864769f;
    for (std::size_t index = 0; index < left.size(); ++index)
    {
        const auto sample = 0.24f * std::sin(twoPi * 55.0f * static_cast<float>(index) / 48000.0f);
        left[index] = sample;
        right[index] = sample;
    }

    const auto originalRms = rms(left);
    parameters.harmonicAmount = 0.0f;
    processor.processBlock(channels.data(), 2, static_cast<int>(left.size()), parameters);
    require(std::abs(rms(left) - originalRms) < 0.001f, "zero harmonic amount is a transparent pass-through");

    const auto dryRms = rms(left);
    parameters.harmonicAmount = 0.82f;
    parameters.character = 0.55f;
    parameters.originalBass = false;
    processor.processBlock(channels.data(), 2, static_cast<int>(left.size()), parameters);
    require(rms(left) > 0.001f, "harmonic-only mode produces an output");
    require(rms(left) < dryRms * 3.0f, "harmonic output stays bounded");
    require(left == right, "stereo channels remain matched for a centered tone");

    for (const auto sample : left)
        require(std::isfinite(sample), "output contains no NaN or infinity");

    reaperiani::BassEnhancerDSP bassOutProcessor;
    bassOutProcessor.prepare(48000.0, 1);
    std::array<float, 4096> lowTone{};
    std::array<float*, 1> mono{lowTone.data()};
    for (std::size_t index = 0; index < lowTone.size(); ++index)
        lowTone[index] = 0.24f * std::sin(twoPi * 55.0f * static_cast<float>(index) / 48000.0f);
    const auto lowToneRms = rms(lowTone);
    parameters.harmonicAmount = 0.0f;
    parameters.originalBass = false;
    bassOutProcessor.processBlock(mono.data(), 1, static_cast<int>(lowTone.size()), parameters);
    require(rms(lowTone) < lowToneRms * 0.3f, "original-bass-out mode removes the selected bass band");

    std::cout << "DSP smoke tests passed\n";
    return EXIT_SUCCESS;
}
