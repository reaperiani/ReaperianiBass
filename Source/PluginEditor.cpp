#include "PluginEditor.h"

#include <cmath>

namespace
{
const juce::Colour background{0xff211719};
const juce::Colour panel{0xff26282a};
const juce::Colour inset{0xff17191b};
const juce::Colour brass{0xffc9a86b};
const juce::Colour brassLight{0xffead7ac};
const juce::Colour wine{0xff55252b};
const juce::Colour ink{0xffeee4d2};

constexpr std::array<const char*, 7> parameterIds{
    "bassFrequency", "harmonicAmount", "character", "harmonicDecay", "dynamics", "clean", "outputGain"};
constexpr std::array<const char*, 7> parameterNames{
    "BASS FREQUENCY", "HARMONICS", "ODD / EVEN", "HARMONIC DECAY", "DYNAMICS", "CLEAN", "OUTPUT"};
constexpr std::array<double, 7> parameterDefaults{90.0, 25.0, 35.0, 55.0, 25.0, 0.0, 0.0};
} // namespace

void ReaperianiBassAudioProcessorEditor::RetroLookAndFeel::drawRotarySlider(
    juce::Graphics& graphics, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));
    const auto diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 10.0f;
    const auto radius = diameter * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    graphics.setColour(inset);
    graphics.fillEllipse(centre.x - radius - 2.0f, centre.y - radius - 2.0f,
                         diameter + 4.0f, diameter + 4.0f);

    juce::ColourGradient knobGradient(juce::Colour{0xff68615a}, centre.x, centre.y - radius,
                                      juce::Colour{0xff27282a}, centre.x, centre.y + radius, false);
    knobGradient.addColour(0.46, juce::Colour{0xff44413d});
    graphics.setGradientFill(knobGradient);
    graphics.fillEllipse(centre.x - radius, centre.y - radius, diameter, diameter);
    graphics.setColour(brass.withAlpha(0.9f));
    graphics.drawEllipse(centre.x - radius, centre.y - radius, diameter, diameter, 1.2f);

    const auto tickRadius = radius + 5.0f;
    constexpr int ticks = 25;
    for (int tick = 0; tick < ticks; ++tick)
    {
        const auto proportion = static_cast<float>(tick) / static_cast<float>(ticks - 1);
        const auto tickAngle = rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle);
        const auto inner = radius + (tick % 4 == 0 ? 3.0f : 5.0f);
        const auto outer = tickRadius + (tick % 4 == 0 ? 1.0f : -1.0f);
        juce::Line<float> line(centre.x + inner * std::cos(tickAngle), centre.y + inner * std::sin(tickAngle),
                               centre.x + outer * std::cos(tickAngle), centre.y + outer * std::sin(tickAngle));
        graphics.setColour(tick % 4 == 0 ? brassLight : brass.withAlpha(0.65f));
        graphics.drawLine(line, tick % 4 == 0 ? 1.6f : 1.0f);
    }

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius + 1.0f, radius + 1.0f, 0.0f,
                      rotaryStartAngle, angle, true);
    graphics.setColour(brass);
    graphics.strokePath(arc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle(-1.8f, -radius + 10.0f, 3.6f, radius * 0.49f, 1.8f);
    graphics.setColour(brassLight);
    graphics.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    graphics.setColour(juce::Colour{0xff151618});
    graphics.fillEllipse(centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

ReaperianiBassAudioProcessorEditor::ReaperianiBassAudioProcessorEditor(
    ReaperianiBassAudioProcessor& audioProcessor)
    : AudioProcessorEditor(&audioProcessor), processor(audioProcessor)
{
    setLookAndFeel(&retroLookAndFeel);
    setSize(920, 570);
    setResizable(false, false);

    for (std::size_t index = 0; index < knobs.size(); ++index)
    {
        auto& knob = knobs[index];
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters(juce::MathConstants<float>::pi * 1.22f,
                                 juce::MathConstants<float>::pi * 2.78f, true);
        knob.setDoubleClickReturnValue(true, parameterDefaults[index]);
        knob.setTextValueSuffix(index == 0 ? " Hz" : (index == 6 ? " dB" : "%"));
        knob.setNumDecimalPlacesToDisplay(index == 0 ? 0 : 1);
        knob.setLookAndFeel(&retroLookAndFeel);
        addAndMakeVisible(knob);
        knobAttachments[index] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, parameterIds[index], knob);
    }

    originalBassButton.setClickingTogglesState(true);
    originalBassButton.setColour(juce::ToggleButton::textColourId, ink);
    originalBassButton.setColour(juce::ToggleButton::tickColourId, brassLight);
    addAndMakeVisible(originalBassButton);
    originalBassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "originalBass", originalBassButton);

    licenseButton.setColour(juce::TextButton::buttonColourId, wine);
    licenseButton.setColour(juce::TextButton::textColourOffId, brassLight);
    licenseButton.onClick = []
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::MessageBoxIconType::InfoIcon,
            "About ReaperianiBass",
            "ReaperianiBass Copyright (C) 2026 Reaperiani.\n\n"
            "Temporary project license: GNU Affero General Public License, version 3 (AGPLv3). "
            "You may redistribute and modify this software under those terms.\n\n"
            "This program comes with NO WARRANTY. The full license is in LICENSE in the source repository, "
            "and at https://www.gnu.org/licenses/agpl-3.0.html.",
            "Close");
    };
    addAndMakeVisible(licenseButton);
}

ReaperianiBassAudioProcessorEditor::~ReaperianiBassAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    for (auto& knob : knobs)
        knob.setLookAndFeel(nullptr);
}

void ReaperianiBassAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(background);

    auto outer = getLocalBounds().toFloat().reduced(12.0f);
    graphics.setColour(brass.withAlpha(0.95f));
    graphics.fillRoundedRectangle(outer, 12.0f);

    auto face = outer.reduced(5.0f);
    graphics.setColour(panel);
    graphics.fillRoundedRectangle(face, 9.0f);
    graphics.setColour(brass.withAlpha(0.38f));
    graphics.drawRoundedRectangle(face, 9.0f, 1.0f);

    auto header = face.removeFromTop(80.0f).reduced(1.0f);
    graphics.setColour(wine);
    graphics.fillRoundedRectangle(header, 7.0f);
    graphics.setColour(brass.withAlpha(0.8f));
    graphics.drawLine(header.getX() + 18.0f, header.getBottom() - 1.0f,
                      header.getRight() - 18.0f, header.getBottom() - 1.0f, 1.0f);

    graphics.setColour(brassLight);
    graphics.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    graphics.drawText("REAPERIANI", 36, 31, 185, 27, juce::Justification::centredLeft);
    graphics.setColour(ink);
    graphics.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    graphics.drawText("BASS", 218, 25, 130, 39, juce::Justification::centredLeft);
    graphics.setColour(brassLight.withAlpha(0.86f));
    graphics.setFont(juce::Font(juce::FontOptions(11.0f)));
    graphics.drawText("HARMONIC BASS ENHANCER   •   BY REAPERIANI", 539, 37, 342, 25,
                      juce::Justification::centredRight);

    graphics.setColour(brass.withAlpha(0.22f));
    graphics.drawLine(50.0f, 314.0f, 870.0f, 314.0f, 1.0f);

    graphics.setColour(brass.withAlpha(0.55f));
    graphics.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    graphics.drawText("LOW-END SELECT", 40, 296, 170, 15, juce::Justification::centredLeft);
    graphics.drawText("HARMONIC SHAPING", 39, 510, 260, 15, juce::Justification::centredLeft);

    for (std::size_t index = 0; index < knobs.size(); ++index)
    {
        const auto topRow = index < 4;
        const auto centreX = topRow ? 132 + static_cast<int>(index) * 219
                                    : 242 + static_cast<int>(index - 4) * 219;
        const auto labelY = topRow ? 120 : 339;
        const auto valueY = topRow ? 268 : 460;
        graphics.setColour(ink);
        graphics.setFont(juce::Font(juce::FontOptions(topRow ? 12.0f : 11.0f, juce::Font::bold)));
        graphics.drawText(parameterNames[index], centreX - 85, labelY, 170, 17,
                          juce::Justification::centred);

        auto value = knobs[index].getTextFromValue(knobs[index].getValue());
        if (index == 2)
            value = juce::String(static_cast<int>(std::round(knobs[index].getValue()))) + "% EVEN";
        graphics.setColour(brassLight);
        graphics.setFont(juce::Font(juce::FontOptions(11.0f)));
        graphics.drawText(value, centreX - 75, valueY, 150, 17, juce::Justification::centred);
    }

    graphics.setColour(brass.withAlpha(0.45f));
    graphics.setFont(juce::Font(juce::FontOptions(9.0f)));
    graphics.drawText("ODD", 520, 289, 42, 13, juce::Justification::centredLeft);
    graphics.drawText("EVEN", 578, 289, 42, 13, juce::Justification::centredRight);

    graphics.setColour(brass.withAlpha(0.22f));
    graphics.drawLine(38.0f, 535.0f, 882.0f, 535.0f, 1.0f);
    graphics.setColour(brass.withAlpha(0.72f));
    graphics.setFont(juce::Font(juce::FontOptions(9.0f)));
    graphics.drawText("MISSING FUNDAMENTAL  /  TONE  •  DYNAMICS  •  CLEAN", 40, 538, 690, 13,
                      juce::Justification::centredLeft);
}

void ReaperianiBassAudioProcessorEditor::resized()
{
    for (std::size_t index = 0; index < knobs.size(); ++index)
    {
        const auto topRow = index < 4;
        const auto centreX = topRow ? 132 + static_cast<int>(index) * 219
                                    : 242 + static_cast<int>(index - 4) * 219;
        const auto size = topRow ? 128 : 108;
        const auto top = topRow ? 137 : 350;
        knobs[index].setBounds(centreX - size / 2, top, size, size);
    }

    originalBassButton.setBounds(754, 430, 128, 30);
    licenseButton.setBounds(773, 536, 102, 18);
}
