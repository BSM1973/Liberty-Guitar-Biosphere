#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

GuitarBiosphereAudioProcessor::GuitarBiosphereAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "BIOSPHERE", makeParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
GuitarBiosphereAudioProcessor::makeParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> values;
    auto add = [&](const char* id, const char* label, float min, float max, float step, float initial)
    {
        values.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), label, juce::NormalisableRange<float>(min, max, step), initial));
    };

    add("input", "Input (dB)", -24.0f, 18.0f, 0.1f, 0.0f);
    add("output", "Output (dB)", -24.0f, 12.0f, 0.1f, 0.0f);
    add("mix", "Master Mix", 0.0f, 1.0f, 0.01f, 0.65f);
    add("pulse", "Pulse", 0.0f, 1.0f, 0.01f, 0.35f);
    add("harmonic", "Harmonic Life", 0.0f, 1.0f, 0.01f, 0.30f);
    add("mutation", "Mutation", 0.0f, 1.0f, 0.01f, 0.20f);
    add("memory", "Memory", 0.0f, 1.0f, 0.01f, 0.20f);
    add("rate", "Pulse Rate (Hz)", 0.1f, 12.0f, 0.1f, 2.0f);
    add("root", "Resonance Root (Hz)", 40.0f, 500.0f, 1.0f, 110.0f);
    add("grain", "Grain Length (ms)", 35.0f, 220.0f, 1.0f, 95.0f);
    add("feedback", "Memory Feedback", 0.0f, 0.95f, 0.01f, 0.40f);
    return { values.begin(), values.end() };
}

bool GuitarBiosphereAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto input = layout.getMainInputChannelSet();
    const auto output = layout.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo())
           && input == output;
}

void GuitarBiosphereAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dsp_.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
}

void GuitarBiosphereAudioProcessor::releaseResources()
{
    dsp_.reset();
}

void GuitarBiosphereAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    const int samples = buffer.getNumSamples();
    for (int channel = channels; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, samples);
    if (channels < 1 || samples == 0)
        return;

    auto value = [&](const char* name) { return parameters.getRawParameterValue(name)->load(); };
    biosphere::Parameters p;
    p.inputGain = juce::Decibels::decibelsToGain(value("input"));
    p.outputGain = juce::Decibels::decibelsToGain(value("output"));
    p.mix = value("mix");
    p.pulse = value("pulse");
    p.harmonic = value("harmonic");
    p.mutation = value("mutation");
    p.memory = value("memory");
    p.rateHz = value("rate");
    p.rootHz = value("root");
    p.grainMs = value("grain");
    p.memoryFeedback = value("feedback");

    float* audio[2] = { buffer.getWritePointer(0), channels > 1 ? buffer.getWritePointer(1) : nullptr };
    dsp_.process(audio, channels, samples, p);
}

void GuitarBiosphereAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void GuitarBiosphereAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessorEditor* GuitarBiosphereAudioProcessor::createEditor()
{
    return new GuitarBiosphereAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarBiosphereAudioProcessor();
}
