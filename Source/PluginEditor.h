#pragma once

#include "PluginProcessor.h"

#include <array>

class ReaperianiBassAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit ReaperianiBassAudioProcessorEditor(ReaperianiBassAudioProcessor&);
    ~ReaperianiBassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class VintageLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle, juce::Slider&) override;
    };

    enum class MeterKind
    {
        originalBass,
        harmonics,
        output
    };

    void timerCallback() override;
    void drawMeter(juce::Graphics&, juce::Rectangle<int>, float levelDb,
                   MeterKind, bool showScale);
    void applyPreset(int presetId);

    ReaperianiBassAudioProcessor& processor;
    VintageLookAndFeel vintageLookAndFeel;
    std::array<juce::Slider, 7> sliders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 7> sliderAttachments;
    juce::ComboBox presetSelector;
    juce::ToggleButton originalBassButton{"ORIGINAL BASS"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> originalBassAttachment;
    juce::TextButton aboutButton{"?"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReaperianiBassAudioProcessorEditor)
};
