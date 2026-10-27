#pragma once
#include "PluginProcessor.h"

class OrymyBassEditor : public juce::AudioProcessorEditor
{
public:
    explicit OrymyBassEditor (OrymyBassAudioProcessor&);
    ~OrymyBassEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    class Content;
    std::unique_ptr<Content> content;
    juce::ComponentBoundsConstrainer constrainer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrymyBassEditor)
};
