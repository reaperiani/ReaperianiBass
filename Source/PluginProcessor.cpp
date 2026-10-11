#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
using Parameter = juce::AudioProcessorValueTreeState;

std::unique_ptr<juce::RangedAudioParameter> makeFloatParameter(const juce::String& id,
                                                              const juce::String& name,
                                                              juce::NormalisableRange<float> range,
                                                              float initial,
                                                              const juce::String& label = {})
{
    const auto attributes = juce::AudioParameterFloatAttributes().withLabel(label);
    return std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1}, name, range, initial, attributes);
}
} // namespace

ReaperianiBassAudioProcessor::ReaperianiBassAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ReaperianiBassAudioProcessor::createParameterLayout()
{
    Parameter::ParameterLayout layout;
    layout.add(makeFloatParameter("bassFrequency", "Bass Frequency",
                                  juce::NormalisableRange<float>(35.0f, 180.0f, 0.1f, 0.45f), 90.0f, "Hz"));
    layout.add(makeFloatParameter("harmonicAmount", "Harmonics",
                                  juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f, "%"));
    layout.add(makeFloatParameter("character", "Odd / Even",
                                  juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f, "%"));
    layout.add(makeFloatParameter("harmonicDecay", "Harmonic Decay",
                                  juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 55.0f, "%"));
    layout.add(makeFloatParameter("dynamics", "Dynamics",
                                  juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f, "%"));
    layout.add(makeFloatParameter("clean", "Clean",
                                  juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"originalBass", 1}, "Original Bass", true));
    layout.add(makeFloatParameter("outputGain", "Output",
                                  juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f, "dB"));
    return layout;
}

const juce::String ReaperianiBassAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void ReaperianiBassAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    dsp.prepare(sampleRate, getTotalNumOutputChannels());
}

void ReaperianiBassAudioProcessor::releaseResources()
{
    dsp.reset();
}

bool ReaperianiBassAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo());
}

void ReaperianiBassAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    midiMessages.clear();

    const auto load = [this](const char* id)
    {
        return parameters.getRawParameterValue(id)->load();
    };

    reaperiani::BassEnhancerParameters settings;
    settings.bassFrequencyHz = load("bassFrequency");
    settings.harmonicAmount = load("harmonicAmount") * 0.01f;
    settings.character = load("character") * 0.01f;
    settings.harmonicDecay = load("harmonicDecay") * 0.01f;
    settings.dynamics = load("dynamics") * 0.01f;
    settings.clean = load("clean") * 0.01f;
    settings.originalBass = load("originalBass") >= 0.5f;
    settings.outputGainDb = load("outputGain");

    dsp.processBlock(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples(), settings);
}

juce::AudioProcessorEditor* ReaperianiBassAudioProcessor::createEditor()
{
    return new ReaperianiBassAudioProcessorEditor(*this);
}

void ReaperianiBassAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    const auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destination);
}

void ReaperianiBassAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReaperianiBassAudioProcessor();
}
