#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "BassDsp.h"

class BassAudioProcessor : public juce::AudioProcessor
{
public:
    BassAudioProcessor();
    ~BassAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    static constexpr std::array<double, 5> midFrequencies { 250.0, 500.0, 800.0, 1500.0, 3000.0 };
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grDb { 0 };
    std::atomic<float> vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    bass::Compressor comp;
    std::array<bass::Crossover, 2> xover;
    std::array<bass::DcBlock, 2> dc;
    std::array<eq::BiquadState, 2> growlState, postLpState1, postLpState2;
    eq::Coeffs growl, postLp;
    bass::ToneStack tone;
    std::array<bass::ToneState, 2> toneState;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    juce::AudioBuffer<float> lowBuf, highBuf, driveBuf;
    std::array<std::vector<float>, 2> delayLow, delayHigh;
    int delayLen = 0, delayPos = 0;

    std::atomic<float>* pComp = nullptr;  std::atomic<float>* pDrive = nullptr; std::atomic<float>* pBlend = nullptr;
    std::atomic<float>* pXover = nullptr; std::atomic<float>* pChar = nullptr;
    std::atomic<float>* pBass = nullptr;  std::atomic<float>* pMid = nullptr;   std::atomic<float>* pMidFreq = nullptr;
    std::atomic<float>* pTreble = nullptr; std::atomic<float>* pBox = nullptr;  std::atomic<float>* pOut = nullptr;

    std::array<float, 9> lastFilterParams {};
    juce::SmoothedValue<float> smDriveGain, smBlend, smMakeup, smOut, smComp;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassAudioProcessor)
};
