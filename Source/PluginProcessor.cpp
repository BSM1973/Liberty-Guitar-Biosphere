#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters/BiosphereParameters.h"
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
    return biosphere::params::createLayout();
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

    // Global bypass is already implemented in Phase 2:
    // no DSP is called, the original input passes unchanged.
    if (parameters.getRawParameterValue("global.bypass")->load() >= 0.5f)
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
    auto state = parameters.copyState();
    // UI-only selection; it must not become a host-automatable audio parameter.
    state.setProperty("editorPage", getSelectedEditorPage(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destination);
}

void GuitarBiosphereAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
    {
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto restored = juce::ValueTree::fromXml(*xml);
            // Older DAW sessions do not have editorPage and still open HARMONIC LIFE.
            const int page = static_cast<int>(restored.getProperty(
                "editorPage", biosphere::ui::EngineNavigationState::defaultPage));
            if (page >= 0 && page < biosphere::ui::EngineNavigationState::pageCount)
                setSelectedEditorPage(page);
            else
                setSelectedEditorPage(biosphere::ui::EngineNavigationState::defaultPage);
            parameters.replaceState(restored);
        }
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
