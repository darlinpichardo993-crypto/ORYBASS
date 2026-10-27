#pragma once
#include "Common.h"
#include "Params.h"
#include "DSP.h"
#include "Analyzer.h"

class OrymyBassAudioProcessor : public juce::AudioProcessor
{
public:
    OrymyBassAudioProcessor();
    ~OrymyBassAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ORYMY Bass"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "ORYMY Bass"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // presets
    void loadPreset (int index);
    bool saveUserPreset();
    int getCurrentPreset() const { return currentPreset.load(); }
    static juce::File userPresetFile();

    juce::AudioProcessorValueTreeState apvts;

    // medidores / analisis (audio -> GUI)
    orymy::Ring inRing, outRing;
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f }, compGr { 0.0f };
    std::atomic<bool> gateOpen { true };

private:
    void setParam (const juce::String& id, float realValue);
    void resetToDefaults();
    orymy::EngineParams buildParams (double bpm) const;

    orymy::BassEngine engine;
    std::vector<float> mono, outL, outR;
    int maxBlock = 512;
    std::atomic<int> currentPreset { 0 };

    struct Raw
    {
        std::atomic<float> *gateOn, *gateThr, *gateRange, *compOn, *compThr, *compRatio, *compMakeup,
            *toneOn, *tone, *splitOn, *splitFreq, *subOn, *subLevel, *eqMode, *ampModel, *gain, *master, *drive,
            *cabModel, *cabMix, *modOn, *modType, *modAmt, *delayOn, *delayMs, *delaySync,
            *reverbOn, *reverbType, *reverbMix, *eq[orymy::kNumEq];
    } raw {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrymyBassAudioProcessor)
};
