#pragma once

#include "PluginProcessor.h"

#include <array>

class ReaperianiBassAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ReaperianiBassAudioProcessorEditor(ReaperianiBassAudioProcessor&);
    ~ReaperianiBassAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class RetroLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;
    };

    ReaperianiBassAudioProcessor& processor;
    RetroLookAndFeel retroLookAndFeel;
    std::array<juce::Slider, 7> knobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 7> knobAttachments;
    juce::ToggleButton originalBassButton{"ORIGINAL BASS"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> originalBassAttachment;
    juce::TextButton licenseButton{"AGPL-3.0"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReaperianiBassAudioProcessorEditor)
};
