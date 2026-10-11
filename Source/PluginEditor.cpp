#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
const juce::Colour nearBlack{0xff080d12};
const juce::Colour navy{0xff001426};
const juce::Colour bluePanel{0xff00243a};
const juce::Colour silver{0xff85898a};
const juce::Colour silverLight{0xffd0d0cb};
const juce::Colour silverDark{0xff292d30};
const juce::Colour labelColour{0xffd0d0d0};
const juce::Colour blueLed{0xff006bff};
const juce::Colour harmonicAmber{0xffffb000};
const juce::Colour peakRed{0xff840e00};

constexpr std::array<const char*, 7> parameterIds{
    "bassFrequency", "harmonicAmount", "character", "harmonicDecay", "dynamics", "clean", "outputGain"};
constexpr std::array<double, 7> parameterDefaults{90.0, 25.0, 35.0, 55.0, 25.0, 0.0, 0.0};

juce::String oneDecimal(float value)
{
    return juce::String(value, 1);
}
} // namespace

void ReaperianiBassAudioProcessorEditor::VintageLookAndFeel::drawLinearSlider(
    juce::Graphics& graphics, int x, int y, int width, int height, float sliderPos,
    float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    juce::ignoreUnused(minSliderPos, maxSliderPos);
    const auto vertical = style == juce::Slider::LinearVertical;
    const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

    graphics.setColour(nearBlack);
    if (vertical)
    {
        const auto centreX = bounds.getCentreX();
        const auto rail = juce::Rectangle<float>(centreX - 3.0f, bounds.getY() + 5.0f,
                                                  6.0f, bounds.getHeight() - 10.0f);
        graphics.fillRect(rail);
        graphics.setColour(silver.withAlpha(0.65f));
        graphics.drawLine(centreX - 1.0f, rail.getY() + 2.0f, centreX - 1.0f,
                          rail.getBottom() - 2.0f, 1.0f);

        auto thumb = juce::Rectangle<float>(bounds.getX() + 2.0f, sliderPos - 10.0f,
                                             bounds.getWidth() - 4.0f, 20.0f);
        juce::ColourGradient thumbGradient(juce::Colour{0xff62686b}, thumb.getX(), thumb.getY(),
                                           juce::Colour{0xff24282b}, thumb.getX(), thumb.getBottom(), false);
        graphics.setGradientFill(thumbGradient);
        graphics.fillRect(thumb);
        graphics.setColour(silverLight);
        graphics.drawLine(thumb.getX(), thumb.getY(), thumb.getRight(), thumb.getY(), 1.0f);
        graphics.setColour(nearBlack);
        graphics.drawLine(thumb.getX(), thumb.getBottom(), thumb.getRight(), thumb.getBottom(), 1.0f);
        graphics.setColour(labelColour);
        graphics.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        graphics.drawFittedText(slider.getTextFromValue(slider.getValue()), thumb.toNearestInt(),
                                juce::Justification::centred, 1);
        return;
    }

    const auto centreY = bounds.getCentreY();
    auto rail = juce::Rectangle<float>(bounds.getX() + 3.0f, centreY - 2.0f,
                                       bounds.getWidth() - 6.0f, 4.0f);
    graphics.fillRect(rail);
    const auto filledWidth = juce::jlimit(0.0f, rail.getWidth(), sliderPos - rail.getX());
    graphics.setColour(blueLed.withAlpha(0.76f));
    graphics.fillRect(rail.withWidth(filledWidth));
    graphics.setColour(silver.withAlpha(0.75f));
    graphics.drawLine(rail.getX(), centreY - 3.0f, rail.getRight(), centreY - 3.0f, 1.0f);

    auto thumb = juce::Rectangle<float>(sliderPos - 4.0f, centreY - 8.0f, 8.0f, 16.0f);
    graphics.setColour(silverLight);
    graphics.fillRect(thumb);
    graphics.setColour(nearBlack);
    graphics.drawRect(thumb, 1.0f);
}

ReaperianiBassAudioProcessorEditor::ReaperianiBassAudioProcessorEditor(
    ReaperianiBassAudioProcessor& audioProcessor)
    : AudioProcessorEditor(&audioProcessor), processor(audioProcessor)
{
    setLookAndFeel(&vintageLookAndFeel);
    setSize(720, 520);
    setResizable(false, false);

    for (std::size_t index = 0; index < sliders.size(); ++index)
    {
        auto& slider = sliders[index];
        slider.setSliderStyle(index == 1 || index == 6
                                  ? juce::Slider::LinearVertical
                                  : juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&vintageLookAndFeel);
        slider.setMouseDragSensitivity(150);
        slider.setDoubleClickReturnValue(true, parameterDefaults[index]);
        slider.setNumDecimalPlacesToDisplay(index == 0 || index == 6 ? 1 : 0);
        slider.setTextValueSuffix(index == 0 ? " Hz" : (index == 6 ? " dB" : "%"));
        addAndMakeVisible(slider);
        sliderAttachments[index] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, parameterIds[index], slider);
    }

    presetSelector.addItem("A: SUB FOUNDATION", 1);
    presetSelector.addItem("A: TIGHT & DEEP", 2);
    presetSelector.addItem("A: SMALL SPEAKERS", 3);
    presetSelector.addItem("A: CLEAN BODY", 4);
    presetSelector.setSelectedId(1, juce::dontSendNotification);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, navy);
    presetSelector.setColour(juce::ComboBox::textColourId, labelColour);
    presetSelector.setColour(juce::ComboBox::outlineColourId, silverDark);
    presetSelector.setColour(juce::ComboBox::arrowColourId, silverLight);
    presetSelector.onChange = [this] { applyPreset(presetSelector.getSelectedId()); };
    addAndMakeVisible(presetSelector);

    originalBassButton.setClickingTogglesState(true);
    originalBassButton.setColour(juce::ToggleButton::textColourId, labelColour);
    originalBassButton.setColour(juce::ToggleButton::tickColourId, blueLed);
    addAndMakeVisible(originalBassButton);
    originalBassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "originalBass", originalBassButton);

    aboutButton.setColour(juce::TextButton::buttonColourId, silverDark);
    aboutButton.setColour(juce::TextButton::textColourOffId, labelColour);
    aboutButton.onClick = []
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
    addAndMakeVisible(aboutButton);
    startTimerHz(30);
}

ReaperianiBassAudioProcessorEditor::~ReaperianiBassAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
    for (auto& slider : sliders)
        slider.setLookAndFeel(nullptr);
}

void ReaperianiBassAudioProcessorEditor::timerCallback()
{
    repaint();
}

void ReaperianiBassAudioProcessorEditor::applyPreset(int presetId)
{
    struct Preset
    {
        float frequency;
        float amount;
        float character;
        float decay;
        float dynamics;
        float clean;
        bool originalBass;
        float output;
    };

    static constexpr std::array<Preset, 4> presets{{
        {90.0f, 25.0f, 35.0f, 55.0f, 25.0f, 0.0f, true, 0.0f},
        {70.0f, 20.0f, 20.0f, 35.0f, 42.0f, 15.0f, true, 0.0f},
        {82.0f, 42.0f, 65.0f, 72.0f, 28.0f, 0.0f, false, -1.5f},
        {105.0f, 18.0f, 28.0f, 62.0f, 60.0f, 32.0f, true, 0.0f}
    }};

    const auto index = juce::jlimit(0, static_cast<int>(presets.size()) - 1, presetId - 1);
    const auto& preset = presets[static_cast<std::size_t>(index)];
    const auto setValue = [this](const char* id, float value)
    {
        if (auto* parameter = processor.parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    };

    setValue("bassFrequency", preset.frequency);
    setValue("harmonicAmount", preset.amount);
    setValue("character", preset.character);
    setValue("harmonicDecay", preset.decay);
    setValue("dynamics", preset.dynamics);
    setValue("clean", preset.clean);
    setValue("originalBass", preset.originalBass ? 1.0f : 0.0f);
    setValue("outputGain", preset.output);
}

void ReaperianiBassAudioProcessorEditor::drawMeter(juce::Graphics& graphics,
                                                   juce::Rectangle<int> bounds,
                                                   float levelDb,
                                                   MeterKind kind,
                                                   bool showScale)
{
    auto frame = bounds.toFloat();
    graphics.setColour(nearBlack);
    graphics.fillRect(frame);
    graphics.setColour(silver.withAlpha(0.75f));
    graphics.drawLine(frame.getX(), frame.getY(), frame.getRight(), frame.getY(), 1.0f);
    graphics.drawLine(frame.getX(), frame.getY(), frame.getX(), frame.getBottom(), 1.0f);
    graphics.setColour(nearBlack);
    graphics.drawLine(frame.getX(), frame.getBottom(), frame.getRight(), frame.getBottom(), 1.0f);
    graphics.drawLine(frame.getRight(), frame.getY(), frame.getRight(), frame.getBottom(), 1.0f);

    auto interior = frame.reduced(3.0f, 3.0f);
    if (showScale)
        interior.setX(interior.getX() + 16.0f);
    graphics.setColour(juce::Colour{0xff000a13});
    graphics.fillRect(interior);

    constexpr int segmentCount = 24;
    constexpr float gap = 1.25f;
    const auto segmentHeight = (interior.getHeight() - gap * (segmentCount - 1)) / segmentCount;
    const auto normalized = juce::jlimit(0.0f, 1.0f, (levelDb + 36.0f) / 36.0f);
    const auto litSegments = static_cast<int>(std::round(normalized * segmentCount));

    for (int segment = 0; segment < segmentCount; ++segment)
    {
        const auto y = interior.getBottom() - (segment + 1) * segmentHeight - segment * gap;
        auto segmentBounds = juce::Rectangle<float>(interior.getX() + 1.0f, y,
                                                     interior.getWidth() - 2.0f, segmentHeight);
        const auto lit = segment < litSegments;
        juce::Colour colour;
        if (kind == MeterKind::originalBass)
            colour = lit ? juce::Colour{segment % 3 == 0 ? 0xff2d6cff : 0xff0013a8}
                         : juce::Colour{0xff00152d};
        else if (segment >= 22)
            colour = lit ? peakRed : juce::Colour{0xff270d0b};
        else if (segment >= 15)
            colour = lit ? juce::Colour{0xffffd43b} : juce::Colour{0xff3b2c08};
        else
            colour = lit ? (kind == MeterKind::output ? juce::Colour{0xffffe100} : harmonicAmber)
                         : juce::Colour{0xff30200a};

        graphics.setColour(colour);
        graphics.fillRect(segmentBounds);
        graphics.setColour(juce::Colours::white.withAlpha(lit ? 0.18f : 0.035f));
        graphics.drawLine(segmentBounds.getX(), segmentBounds.getY(),
                          segmentBounds.getRight(), segmentBounds.getY(), 0.5f);
    }

    if (showScale)
    {
        constexpr std::array<int, 5> marks{-3, -9, -15, -21, -27};
        graphics.setFont(juce::Font(juce::FontOptions(8.0f)));
        for (const auto mark : marks)
        {
            const auto y = interior.getY() + (static_cast<float>(-mark) / 36.0f) * interior.getHeight();
            graphics.setColour(labelColour.withAlpha(0.78f));
            graphics.drawText(juce::String(mark), bounds.getX() + 1,
                              static_cast<int>(y) - 5, 14, 10, juce::Justification::centredRight);
            graphics.setColour(labelColour.withAlpha(0.42f));
            graphics.drawLine(interior.getX() - 1.0f, y, interior.getX() + 3.0f, y, 0.7f);
        }
    }
}

void ReaperianiBassAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour{0xff15191c});

    auto chassis = getLocalBounds().toFloat().reduced(5.0f);
    juce::ColourGradient metal(juce::Colour{0xffa7aaab}, chassis.getX(), chassis.getY(),
                               juce::Colour{0xff383d40}, chassis.getX(), chassis.getBottom(), false);
    metal.addColour(0.12, juce::Colour{0xff74797b});
    metal.addColour(0.52, juce::Colour{0xff555a5c});
    graphics.setGradientFill(metal);
    graphics.fillRect(chassis);
    graphics.setColour(nearBlack);
    graphics.drawRect(chassis, 2.0f);
    graphics.setColour(silverLight.withAlpha(0.72f));
    graphics.drawLine(chassis.getX() + 3.0f, chassis.getY() + 3.0f,
                      chassis.getRight() - 3.0f, chassis.getY() + 3.0f, 1.0f);

    auto toolbar = juce::Rectangle<float>(13.0f, 12.0f, 694.0f, 21.0f);
    graphics.setColour(juce::Colour{0xff111518});
    graphics.fillRect(toolbar);
    graphics.setColour(silver.withAlpha(0.55f));
    graphics.drawRect(toolbar, 1.0f);
    graphics.setColour(bluePanel);
    graphics.fillRect(juce::Rectangle<float>(20.0f, 14.0f, 26.0f, 17.0f));
    graphics.setColour(blueLed.withAlpha(0.85f));
    graphics.drawRect(juce::Rectangle<float>(20.0f, 14.0f, 26.0f, 17.0f), 1.0f);
    graphics.setColour(silverLight);
    graphics.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    graphics.drawText("RB", 20, 15, 26, 15, juce::Justification::centred);
    graphics.setColour(labelColour);
    graphics.setFont(juce::Font(juce::FontOptions(9.0f)));
    graphics.drawText("REAPERIANI BASS   /   HARMONIC PROCESSOR", 55, 14, 340, 17,
                      juce::Justification::centredLeft);
    graphics.setColour(silver.withAlpha(0.72f));
    graphics.drawText("MONO / STEREO     V1.0.0", 428, 14, 264, 17,
                      juce::Justification::centredRight);

    auto presetBand = juce::Rectangle<float>(13.0f, 35.0f, 694.0f, 23.0f);
    graphics.setColour(nearBlack);
    graphics.fillRect(presetBand);
    graphics.setColour(silverDark);
    graphics.drawLine(presetBand.getX(), presetBand.getY(), presetBand.getRight(), presetBand.getY(), 1.0f);
    graphics.setColour(blueLed.withAlpha(0.25f));
    graphics.drawLine(presetBand.getX(), presetBand.getBottom(), presetBand.getRight(), presetBand.getBottom(), 1.0f);

    auto panel = juce::Rectangle<float>(17.0f, 62.0f, 686.0f, 384.0f);
    graphics.setColour(nearBlack);
    graphics.fillRect(panel.expanded(2.0f));
    graphics.setColour(silver.withAlpha(0.9f));
    graphics.drawRect(panel.expanded(1.0f), 1.0f);
    juce::ColourGradient blue(juce::Colour{0xff001426}, panel.getX(), panel.getY(),
                              juce::Colour{0xff00243a}, panel.getRight(), panel.getBottom(), false);
    blue.addColour(0.45, juce::Colour{0xff001c30});
    graphics.setGradientFill(blue);
    graphics.fillRect(panel);
    graphics.setColour(blueLed.withAlpha(0.18f));
    graphics.drawLine(panel.getX() + 2.0f, panel.getY() + 2.0f,
                      panel.getRight() - 2.0f, panel.getY() + 2.0f, 1.0f);

    const auto meters = processor.getMeterReadings();
    const auto led = meters.clipping ? peakRed : (meters.outputLeftDb > -58.0f ? blueLed : silverDark);
    graphics.setColour(nearBlack);
    graphics.fillEllipse(31.0f, 82.0f, 19.0f, 19.0f);
    graphics.setColour(silver);
    graphics.drawEllipse(32.0f, 83.0f, 17.0f, 17.0f, 1.0f);
    graphics.setColour(led);
    graphics.fillEllipse(35.0f, 86.0f, 11.0f, 11.0f);
    graphics.setColour(led.withAlpha(0.35f));
    graphics.fillEllipse(32.0f, 83.0f, 17.0f, 17.0f);
    graphics.setColour(labelColour);
    graphics.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    graphics.drawText(meters.clipping ? "CLIP" : "OUT", 52, 83, 42, 17,
                      juce::Justification::centredLeft);

    graphics.setColour(labelColour);
    graphics.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    graphics.drawText("FREQ. (Hz)", 316, 74, 110, 15, juce::Justification::centred);
    graphics.setColour(blueLed.withAlpha(0.28f));
    graphics.drawLine(276.0f, 92.0f, 468.0f, 92.0f, 1.0f);
    auto frequencyReadout = juce::Rectangle<int>(506, 96, 72, 23);
    graphics.setColour(nearBlack);
    graphics.fillRect(frequencyReadout);
    graphics.setColour(silverLight);
    graphics.drawLine(static_cast<float>(frequencyReadout.getX()), static_cast<float>(frequencyReadout.getY()),
                      static_cast<float>(frequencyReadout.getRight()), static_cast<float>(frequencyReadout.getY()), 1.0f);
    graphics.setColour(silverDark);
    graphics.drawRect(frequencyReadout, 1);
    graphics.setColour(labelColour);
    graphics.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    graphics.drawText(juce::String(static_cast<int>(std::round(sliders[0].getValue()))) + " Hz",
                      frequencyReadout, juce::Justification::centred);

    constexpr std::array<int, 5> columnCentres{102, 207, 339, 465, 593};
    constexpr std::array<const char*, 5> columnLabels{
        "ORIGINAL BASS", "INTENSITY", "HARMONICS", "GAIN", "OUTPUT"};
    for (std::size_t index = 0; index < columnCentres.size(); ++index)
    {
        graphics.setColour(labelColour);
        graphics.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        graphics.drawText(columnLabels[index], columnCentres[index] - 48, 145, 96, 14,
                          juce::Justification::centred);
        graphics.setColour(silver.withAlpha(0.28f));
        graphics.drawLine(static_cast<float>(columnCentres[index] - 38), 160.0f,
                          static_cast<float>(columnCentres[index] + 38), 160.0f, 0.7f);
    }

    drawMeter(graphics, {66, 168, 72, 165}, meters.originalBassDb, MeterKind::originalBass, true);
    drawMeter(graphics, {303, 168, 72, 165}, meters.harmonicsDb, MeterKind::harmonics, true);
    drawMeter(graphics, {574, 168, 16, 165}, meters.outputLeftDb, MeterKind::output, false);
    drawMeter(graphics, {596, 168, 16, 165}, meters.outputRightDb, MeterKind::output, false);

    graphics.setColour(silver.withAlpha(0.38f));
    graphics.drawLine(251.0f, 143.0f, 251.0f, 353.0f, 1.0f);
    graphics.drawLine(391.0f, 143.0f, 391.0f, 353.0f, 1.0f);
    graphics.drawLine(523.0f, 143.0f, 523.0f, 353.0f, 1.0f);

    const auto drawReadout = [&graphics](juce::Rectangle<int> area, const juce::String& text)
    {
        graphics.setColour(nearBlack);
        graphics.fillRect(area);
        graphics.setColour(silver.withAlpha(0.75f));
        graphics.drawLine(static_cast<float>(area.getX()), static_cast<float>(area.getY()),
                          static_cast<float>(area.getRight()), static_cast<float>(area.getY()), 1.0f);
        graphics.setColour(labelColour);
        graphics.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
        graphics.drawText(text, area, juce::Justification::centred);
    };
    drawReadout({73, 337, 58, 18}, oneDecimal(meters.originalBassDb));
    drawReadout({310, 337, 58, 18}, oneDecimal(meters.harmonicsDb));
    drawReadout({557, 337, 31, 18}, oneDecimal(meters.outputLeftDb));
    drawReadout({593, 337, 31, 18}, oneDecimal(meters.outputRightDb));

    graphics.setColour(silver.withAlpha(0.24f));
    graphics.drawLine(28.0f, 366.0f, 692.0f, 366.0f, 1.0f);
    constexpr std::array<const char*, 4> miniLabels{"ODD / EVEN", "HARMONIC DECAY", "DYNAMICS", "CLEAN"};
    constexpr std::array<int, 4> miniCentres{139, 300, 461, 620};
    for (std::size_t index = 0; index < miniLabels.size(); ++index)
    {
        graphics.setColour(labelColour.withAlpha(0.9f));
        graphics.setFont(juce::Font(juce::FontOptions(8.5f, juce::Font::bold)));
        graphics.drawText(miniLabels[index], miniCentres[index] - 66, 373, 132, 13,
                          juce::Justification::centred);
        const auto sliderIndex = index + 2;
        auto value = sliders[sliderIndex].getTextFromValue(sliders[sliderIndex].getValue());
        graphics.setColour(silverLight);
        graphics.setFont(juce::Font(juce::FontOptions(8.5f)));
        graphics.drawText(value, miniCentres[index] - 55, 419, 110, 12,
                          juce::Justification::centred);
    }

    auto footer = juce::Rectangle<float>(13.0f, 451.0f, 694.0f, 56.0f);
    juce::ColourGradient footerMetal(juce::Colour{0xff717678}, footer.getX(), footer.getY(),
                                     juce::Colour{0xff363b3e}, footer.getX(), footer.getBottom(), false);
    footerMetal.addColour(0.18, juce::Colour{0xff9da0a1});
    graphics.setGradientFill(footerMetal);
    graphics.fillRect(footer);
    graphics.setColour(nearBlack);
    graphics.drawLine(footer.getX(), footer.getY(), footer.getRight(), footer.getY(), 2.0f);
    graphics.setColour(silverLight.withAlpha(0.65f));
    graphics.drawLine(footer.getX() + 2.0f, footer.getY() + 3.0f,
                      footer.getRight() - 2.0f, footer.getY() + 3.0f, 1.0f);
    graphics.setColour(nearBlack.withAlpha(0.55f));
    graphics.setFont(juce::Font(juce::FontOptions(8.5f)));
    graphics.drawText("ORIGINAL BASS     •     HARMONIC SPECTRUM     •     STEREO OUTPUT", 27, 464, 430, 18,
                      juce::Justification::centredLeft);
    graphics.setColour(juce::Colour{0xfff0f0e9});
    graphics.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::italic)));
    graphics.drawText("Reaperiani Bass", 470, 462, 210, 25, juce::Justification::centredRight);
    graphics.setColour(nearBlack.withAlpha(0.6f));
    graphics.setFont(juce::Font(juce::FontOptions(8.0f)));
    graphics.drawText("BY REAPERIANI   /   V1.0.0", 480, 484, 200, 12, juce::Justification::centredRight);

    for (const auto point : {juce::Point<float>{23.0f, 478.0f}, juce::Point<float>{697.0f, 478.0f}})
    {
        graphics.setColour(nearBlack);
        graphics.fillEllipse(point.x - 4.0f, point.y - 4.0f, 8.0f, 8.0f);
        graphics.setColour(silverLight.withAlpha(0.65f));
        graphics.drawEllipse(point.x - 3.0f, point.y - 3.0f, 6.0f, 6.0f, 1.0f);
        graphics.drawLine(point.x - 2.0f, point.y, point.x + 2.0f, point.y, 0.7f);
    }
}

void ReaperianiBassAudioProcessorEditor::resized()
{
    presetSelector.setBounds(184, 35, 352, 21);
    aboutButton.setBounds(678, 12, 25, 21);
    originalBassButton.setBounds(34, 113, 138, 25);

    sliders[0].setBounds(265, 96, 232, 23);
    sliders[1].setBounds(181, 167, 53, 165);
    sliders[2].setBounds(85, 391, 108, 22);
    sliders[3].setBounds(246, 391, 108, 22);
    sliders[4].setBounds(408, 391, 106, 22);
    sliders[5].setBounds(571, 391, 98, 22);
    sliders[6].setBounds(439, 167, 53, 165);
}
