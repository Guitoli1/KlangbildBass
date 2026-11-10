#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VintageUI.h"

class BassAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BassAudioProcessorEditor (BassAudioProcessor&);
    ~BassAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob { juce::Slider slider; juce::String name; bool big = false; std::unique_ptr<SliderAttachment> attachment; };
    Knob& addKnob (const juce::String& paramID, const juce::String& name, bool big);
    void setupSwitch (juce::ToggleButton&, const juce::String& paramID, const juce::String& onText,
                      const juce::String& offText, juce::Colour lamp);
    void timerCallback() override;
    void renderTextures();
    void drawEngraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Justification);
    void drawGroup (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);

    BassAudioProcessor& proc;
    VintageLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob* compKnob = nullptr; Knob* outKnob = nullptr;
    std::array<Knob*, 3> driveKnobs {};
    std::array<Knob*, 4> toneKnobs {};
    juce::ToggleButton charSwitch, boxSwitch;
    std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;
    juce::ComboBox presetBox;
    VuMeter vuGr { lnf, "GR" }, vuOutMeter { lnf, "L+R" };

    juce::Rectangle<int> faceplate, presetArea, vuBezelGr, vuBezelOut, groupDrive, groupTone, pilotArea;
    std::array<juce::Rectangle<int>, 2> switchLabels;
    juce::Image tolexImage, faceplateImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassAudioProcessorEditor)
};
