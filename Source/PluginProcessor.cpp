#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace orymy;

OrymyBassAudioProcessor::OrymyBassAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ORYMY_BASS", createLayout())
{
    auto r = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    raw.gateOn = r ("gateOn"); raw.gateThr = r ("gateThr"); raw.gateRange = r ("gateRange");
    raw.compOn = r ("compOn"); raw.compThr = r ("compThr"); raw.compRatio = r ("compRatio"); raw.compMakeup = r ("compMakeup");
    raw.toneOn = r ("toneOn"); raw.tone = r ("tone"); raw.splitOn = r ("splitOn"); raw.splitFreq = r ("splitFreq");
    raw.subOn = r ("subOn"); raw.subLevel = r ("subLevel"); raw.eqMode = r ("eqMode");
    raw.ampModel = r ("ampModel"); raw.gain = r ("gain"); raw.master = r ("master"); raw.drive = r ("drive");
    raw.cabModel = r ("cabModel"); raw.cabMix = r ("cabMix");
    raw.modOn = r ("modOn"); raw.modType = r ("modType"); raw.modAmt = r ("modAmt");
    raw.delayOn = r ("delayOn"); raw.delayMs = r ("delayMs"); raw.delaySync = r ("delaySync");
    raw.reverbOn = r ("reverbOn"); raw.reverbType = r ("reverbType"); raw.reverbMix = r ("reverbMix");
    for (int i = 0; i < kNumEq; ++i) raw.eq[i] = r (kEqIds[i]);
    apvts.state.setProperty ("preset", 0, nullptr);
    loadPreset (0);
}

bool OrymyBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet(), i = l.getMainInputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) return false;
    return i == juce::AudioChannelSet::mono() || i == juce::AudioChannelSet::stereo();
}

void OrymyBassAudioProcessor::prepareToPlay (double sr, int block)
{
    maxBlock = juce::jmax (64, block);
    engine.prepare (sr, maxBlock);
    mono.assign ((size_t) maxBlock, 0.0f); outL.assign ((size_t) maxBlock, 0.0f); outR.assign ((size_t) maxBlock, 0.0f);
    setLatencySamples ((int) std::lround (engine.getLatency()));
}

EngineParams OrymyBassAudioProcessor::buildParams (double bpm) const
{
    EngineParams p;
    p.gateOn = raw.gateOn->load() > 0.5f; p.gateThr = raw.gateThr->load(); p.gateRange = raw.gateRange->load();
    p.compOn = raw.compOn->load() > 0.5f; p.compThr = raw.compThr->load(); p.compRatio = raw.compRatio->load(); p.compMakeup = raw.compMakeup->load();
    p.toneOn = raw.toneOn->load() > 0.5f; p.tone = raw.tone->load();
    p.splitOn = raw.splitOn->load() > 0.5f; p.splitFreq = raw.splitFreq->load();
    p.subOn = raw.subOn->load() > 0.5f; p.subLevel = raw.subLevel->load();
    for (int i = 0; i < kNumEq; ++i) p.eq[i] = raw.eq[i]->load();
    p.eqMode = (int) raw.eqMode->load();
    p.ampModel = (int) raw.ampModel->load(); p.gain = raw.gain->load(); p.master = raw.master->load(); p.drive = raw.drive->load();
    p.cabModel = (int) raw.cabModel->load(); p.cabMix = raw.cabMix->load();
    p.modOn = raw.modOn->load() > 0.5f; p.modType = (int) raw.modType->load(); p.modAmt = raw.modAmt->load();
    p.delayOn = raw.delayOn->load() > 0.5f; p.delayMs = raw.delayMs->load(); p.delaySync = (int) raw.delaySync->load();
    p.reverbOn = raw.reverbOn->load() > 0.5f; p.reverbType = (int) raw.reverbType->load(); p.reverbMix = raw.reverbMix->load();
    p.bpm = bpm;
    return p;
}

void OrymyBassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    const int nIn = getTotalNumInputChannels(), nOut = getTotalNumOutputChannels();

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    const EngineParams p = buildParams (bpm);

    for (int off = 0; off < total; off += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - off);
        const float* l = nIn > 0 ? buffer.getReadPointer (0, off) : nullptr;
        const float* r = nIn > 1 ? buffer.getReadPointer (1, off) : l;
        for (int i = 0; i < n; ++i) mono[(size_t) i] = l ? (r ? 0.5f * (l[i] + r[i]) : l[i]) : 0.0f;

        double e = 0; for (int i = 0; i < n; ++i) e += (double) mono[(size_t) i] * mono[(size_t) i];
        const float rms = (float) std::sqrt (e / n);
        inLevel.store (inLevel.load() * 0.8f + rms * 0.2f);
        inRing.push (mono.data(), n);

        engine.process (mono.data(), outL.data(), outR.data(), n, p);

        float pk = 0;
        for (int i = 0; i < n; ++i) pk = std::max (pk, std::max (std::abs (outL[(size_t) i]), std::abs (outR[(size_t) i])));
        outLevel.store (std::max (pk, outLevel.load() * 0.9f));
        compGr.store (engine.grDbMax);
        gateOpen.store (engine.gateOpen);

        for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (outL[(size_t) i] + outR[(size_t) i]);
        outRing.push (mono.data(), n);

        if (nOut > 0) juce::FloatVectorOperations::copy (buffer.getWritePointer (0, off), outL.data(), n);
        if (nOut > 1) juce::FloatVectorOperations::copy (buffer.getWritePointer (1, off), outR.data(), n);
        if (nOut == 1) for (int i = 0; i < n; ++i) buffer.getWritePointer (0, off)[i] = mono[(size_t) i];
    }
}

// ----------------------------------------------------------------- presets
void OrymyBassAudioProcessor::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void OrymyBassAudioProcessor::resetToDefaults()
{
    for (auto& id : allParamIds())
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->getDefaultValue());
}

juce::File OrymyBassAudioProcessor::userPresetFile()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("YMY Sounds").getChildFile ("ORYMY Bass").getChildFile ("Personalizado.xml");
}

void OrymyBassAudioProcessor::loadPreset (int index)
{
    const auto& list = presets();
    index = juce::jlimit (0, (int) list.size() - 1, index);
    currentPreset.store (index);
    apvts.state.setProperty ("preset", index, nullptr);
    resetToDefaults();

    if (index == kUserPresetIndex)
    {
        const auto f = userPresetFile();
        if (f.existsAsFile())
            if (auto xml = juce::XmlDocument::parse (f))
            {
                auto vt = juce::ValueTree::fromXml (*xml);
                for (auto c : vt)
                    if (c.hasProperty ("id") && c.hasProperty ("value"))
                        setParam (c["id"].toString(), (float) (double) c["value"]);
            }
        return;
    }
    const auto& d = list[(size_t) index];
    for (int i = 0; i < kNumEq; ++i) setParam (kEqIds[i], d.eq[(size_t) i]);
    for (auto& kv : d.v) setParam (kv.first, kv.second);
}

bool OrymyBassAudioProcessor::saveUserPreset()
{
    const auto f = userPresetFile();
    if (! f.getParentDirectory().createDirectory()) return false;
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr || ! xml->writeTo (f)) return false;
    currentPreset.store (kUserPresetIndex);
    apvts.state.setProperty ("preset", kUserPresetIndex, nullptr);
    return true;
}

// ----------------------------------------------------------------- estado
void OrymyBassAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    apvts.state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void OrymyBassAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPreset.store (juce::jlimit (0, (int) presets().size() - 1, (int) apvts.state.getProperty ("preset", 0)));
        }
}

juce::AudioProcessorEditor* OrymyBassAudioProcessor::createEditor() { return new OrymyBassEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrymyBassAudioProcessor(); }
