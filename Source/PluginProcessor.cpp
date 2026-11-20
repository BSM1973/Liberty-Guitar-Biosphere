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
    // Initialized once; route automation reads these atomics directly in the
    // audio callback without building JUCE Strings or allocating memory.
    static constexpr const char* routeIds[8][4] = {
        {"react.route1.active", "react.route1.source", "react.route1.destination", "react.route1.amount"},
        {"react.route2.active", "react.route2.source", "react.route2.destination", "react.route2.amount"},
        {"react.route3.active", "react.route3.source", "react.route3.destination", "react.route3.amount"},
        {"react.route4.active", "react.route4.source", "react.route4.destination", "react.route4.amount"},
        {"react.route5.active", "react.route5.source", "react.route5.destination", "react.route5.amount"},
        {"react.route6.active", "react.route6.source", "react.route6.destination", "react.route6.amount"},
        {"react.route7.active", "react.route7.source", "react.route7.destination", "react.route7.amount"},
        {"react.route8.active", "react.route8.source", "react.route8.destination", "react.route8.amount"}
    };
    for (std::size_t i = 0; i < reactionHandles_.size(); ++i)
    {
        auto& handles = reactionHandles_[i];
        handles.active = parameters.getRawParameterValue(routeIds[i][0]);
        handles.source = parameters.getRawParameterValue(routeIds[i][1]);
        handles.destination = parameters.getRawParameterValue(routeIds[i][2]);
        handles.amount = parameters.getRawParameterValue(routeIds[i][3]);
        jassert(handles.active && handles.source && handles.destination && handles.amount);
    }

    // Resolve the 97 stable host-parameter pointers at construction time.
    // The audio callback never builds juce::Strings or touches host setters.
    for (auto* parameter : getParameters())
        if (auto* identified = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter))
            {
                if (midiParameterCount_ >= biosphere::midi::LearnRouter::maximumParameters)
                    break;
                midiParameterIds_[static_cast<std::size_t>(midiParameterCount_)] =
                    identified->getParameterID();
                midiParameters_[static_cast<std::size_t>(midiParameterCount_)] = ranged;
                ++midiParameterCount_;
            }
    jassert(midiParameterCount_ == 97);
    startTimerHz(60);
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

void GuitarBiosphereAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    // Capture MIDI CC in the audio callback WITHOUT setting APVTS parameters
    // from the real-time thread. The 60 Hz processor timer commits values
    // on the JUCE message thread, even when the editor is closed.
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        if (message.isController())
            midiRouter_.receive(message.getChannel(), message.getControllerNumber(),
                                message.getControllerValue());
    }
    const int channels = getTotalNumInputChannels();
    const int samples = buffer.getNumSamples();
    for (int channel = channels; channel < buffer.getNumChannels(); ++channel)
        buffer.clear(channel, 0, samples);
    if (channels < 1 || samples == 0)
        return;

    float inputPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel)
        inputPeak = std::max(inputPeak, buffer.getMagnitude(channel, 0, samples));

    // One atomic APVTS snapshot per host block; no allocations or locks.
    // The legacy 11 parameter IDs must remain unchanged for DAW compatibility.
    auto value = [&](const char* name) noexcept
    {
        return parameters.getRawParameterValue(name)->load(std::memory_order_relaxed);
    };
    auto enabled = [&](const char* id) noexcept { return value(id) >= 0.5f; };
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

    p.globalIntensity = value("global.intensity");
    p.globalSpeed = value("global.speed");
    p.globalWidth = value("global.width");
    p.globalSync = enabled("global.sync");
    p.globalFreeze = enabled("global.freeze");
    p.bypass = enabled("global.bypass");
    p.macroGrowth = value("macro.growth");
    p.macroMotion = value("macro.motion");
    p.macroSpace = value("macro.space");
    p.macroEnergy = value("macro.energy");

    p.pulseActive = enabled("pulse.active");
    p.pulseSensitivity = value("pulse.sensitivity");
    p.pulseDivision = static_cast<int>(value("pulse.division"));
    p.pulseSync = enabled("pulse.sync");
    p.pulseSwing = value("pulse.swing");
    p.pulseFeedback = value("pulse.feedback");
    p.pulseWidth = value("pulse.width");
    p.pulseMix = value("pulse.mix");

    p.harmonicActive = enabled("harm.active");
    p.harmonicMode = static_cast<int>(value("harm.mode"));
    p.harmonicResonance = value("harm.resonance");
    p.harmonicDecay = value("harm.decay");
    p.harmonicDamping = value("harm.damping");
    p.harmonicMotion = value("harm.motion");
    p.harmonicSpread = value("harm.spread");
    p.harmonicMono = enabled("harm.mono");

    p.mutationActive = enabled("mut.active");
    p.mutationDensity = value("mut.density");
    p.mutationCharacter = static_cast<int>(value("mut.character"));
    p.mutationMotion = value("mut.motion");
    p.mutationPitch = value("mut.pitch");
    p.mutationRandom = value("mut.random");
    p.mutationStereo = value("mut.stereo");

    p.memoryActive = enabled("mem.active");
    p.memoryLength = value("mem.length");
    p.memoryCapture = value("mem.capture");
    p.memoryRecall = value("mem.recall");
    p.memoryRepeat = static_cast<int>(value("mem.repeat"));
    p.memoryReverse = value("mem.reverse");
    p.memoryFade = value("mem.fade");
    p.memoryFreeze = enabled("mem.freeze");

    // The host tempo is applied only when the host actually supplies it.
    // Otherwise the free-running rate remains in charge.
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
            if (auto bpm = position->getBpm())
                if (std::isfinite(*bpm) && *bpm > 0)
                {
                    p.hasHostBpm = true;
                    p.hostBpm = static_cast<float>(*bpm);
                }

    // REACTION: real eight-route APVTS -> DSP matrix. The choice indices
    // remain byte-for-byte compatible with existing factory/user sessions.
    p.reaction.active = enabled("react.active");
    p.reaction.reactivity = value("react.reactivity");
    p.reaction.interaction = value("react.interaction");
    p.reaction.sensitivity = value("react.sensitivity");
    p.reaction.responseMs = value("react.response");
    p.reaction.complexity = value("react.complexity");
    for (std::size_t index = 0; index < reactionHandles_.size(); ++index)
    {
        const auto& handles = reactionHandles_[index];
        auto& route = p.reaction.routes[index];
        route.active = handles.active->load(std::memory_order_relaxed) >= 0.5f;
        route.source = static_cast<int>(handles.source->load(std::memory_order_relaxed));
        route.destination = static_cast<int>(handles.destination->load(std::memory_order_relaxed));
        route.amount = handles.amount->load(std::memory_order_relaxed);
    }

    // EVOLUTION: six existing APVTS controls — no new IDs and no per-block
    // allocation; its state machine is driven by live guitar signal levels.
    p.evolution.active = enabled("evo.active");
    p.evolution.rate = value("evo.rate");
    p.evolution.intensity = value("evo.intensity");
    p.evolution.sensitivity = value("evo.sensitivity");
    p.evolution.stability = value("evo.stability");
    p.evolution.complexity = value("evo.complexity");
    p.evolution.recovery = value("evo.recovery");

    float* audio[2] = { buffer.getWritePointer(0), channels > 1 ? buffer.getWritePointer(1) : nullptr };
    dsp_.process(audio, channels, samples, p);

    // Single writer (audio thread) to atomic snapshots, consumed by UI at 30 Hz.
    const auto values = dsp_.getAnalysisValues();
    analysis.envelope.store(values.envelope, std::memory_order_relaxed);
    analysis.attacks.store(values.attacks, std::memory_order_relaxed);
    analysis.rhythmicActivity.store(values.rhythmicActivity, std::memory_order_relaxed);
    analysis.memoryActivity.store(values.memoryActivity, std::memory_order_relaxed);
    analysis.tonalEnergy.store(values.tonalEnergy, std::memory_order_relaxed);
    analysis.rootHz.store(values.rootHz, std::memory_order_relaxed);
    analysis.pitchConfidence.store(values.pitchConfidence, std::memory_order_relaxed);
    analysis.midiNote.store(values.midiNote, std::memory_order_relaxed);
    analysis.evolutionEnergy.store(values.evolutionEnergy, std::memory_order_relaxed);
    analysis.evolutionState.store(values.evolutionState, std::memory_order_relaxed);
    analysis.hostBpm.store(p.hasHostBpm ? p.hostBpm : 0.0f, std::memory_order_relaxed);
    analysis.inputPeak.store(inputPeak, std::memory_order_relaxed);
    float outputPeak = 0.0f;
    for (int channel = 0; channel < channels; ++channel)
        outputPeak = std::max(outputPeak, buffer.getMagnitude(channel, 0, samples));
    analysis.outputPeak.store(outputPeak, std::memory_order_relaxed);
}

int GuitarBiosphereAudioProcessor::midiParameterIndex(const juce::String& id) const
{
    for (int i = 0; i < midiParameterCount_; ++i)
        if (midiParameterIds_[static_cast<std::size_t>(i)] == id)
            return i;
    return -1;
}

void GuitarBiosphereAudioProcessor::armMidiLearn(const juce::String& parameterId)
{
    midiRouter_.arm(midiParameterIndex(parameterId));
}

void GuitarBiosphereAudioProcessor::clearMidiLearn(const juce::String& parameterId)
{
    midiRouter_.clear(midiParameterIndex(parameterId));
}

int GuitarBiosphereAudioProcessor::midiAssignment(const juce::String& parameterId) const
{
    return midiRouter_.assignment(midiParameterIndex(parameterId));
}

juce::String GuitarBiosphereAudioProcessor::midiParameterName(int index) const
{
    if (index >= 0 && index < midiParameterCount_)
        return midiParameterIds_[static_cast<std::size_t>(index)];
    return {};
}

void GuitarBiosphereAudioProcessor::timerCallback()
{
    // The host's parameter notification occurs here on the message thread,
    // not in processBlock. A learnt CC therefore works without any editor.
    for (int i = 0; i < midiParameterCount_; ++i)
    {
        const int value = midiRouter_.consume(i);
        if (value < 0) continue;
        if (auto* parameter = midiParameters_[static_cast<std::size_t>(i)])
            parameter->setValueNotifyingHost(static_cast<float>(value) / 127.0f);
    }
}

void GuitarBiosphereAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto state = parameters.copyState();
    // MIDI mappings are per DAW project, never new audio parameters.
    if (auto previous = state.getChildWithName("MIDI_LEARN"); previous.isValid())
        state.removeChild(previous, nullptr);
    juce::ValueTree midi("MIDI_LEARN");
    for (int index = 0; index < midiParameterCount_; ++index)
    {
        const int value = midiRouter_.assignment(index);
        if (value < 0) continue;
        juce::ValueTree link("CC");
        link.setProperty("parameter",
                         midiParameterIds_[static_cast<std::size_t>(index)], nullptr);
        link.setProperty("channel",
                         biosphere::midi::LearnRouter::channelOf(value), nullptr);
        link.setProperty("controller",
                         biosphere::midi::LearnRouter::controllerOf(value), nullptr);
        midi.addChild(link, -1, nullptr);
    }
    state.addChild(midi, -1, nullptr);
    // UI-only selection; it must not become a host-automatable audio parameter.
    state.setProperty("editorPage", getSelectedEditorPage(), nullptr);
    state.setProperty("editorLanguage", isEnglish() ? "en" : "fr", nullptr);
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
            setEnglish(restored.getProperty("editorLanguage", "fr").toString() == "en");
            midiRouter_.clearAll();
            const auto mappings = restored.getChildWithName("MIDI_LEARN");
            if (mappings.isValid())
            {
                for (int index = 0; index < mappings.getNumChildren(); ++index)
                {
                    const auto entry = mappings.getChild(index);
                    if (!entry.hasType("CC")) continue;
                    const int parameterIndex = midiParameterIndex(
                        entry.getProperty("parameter").toString());
                    const int channel = static_cast<int>(entry.getProperty("channel", -1));
                    const int controller = static_cast<int>(entry.getProperty("controller", -1));
                    midiRouter_.assign(parameterIndex, channel, controller);
                }
            }
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
