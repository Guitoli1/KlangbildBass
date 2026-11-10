#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ScaledEditor.h"

namespace
{
using APF = juce::AudioParameterFloat;
using Attr = juce::AudioParameterFloatAttributes;

Attr scale10()
{
    return Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
Attr decibels()
{
    return Attr().withStringFromValueFunction ([] (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB"; })
                 .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); });
}
juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); }
}

juce::AudioProcessorValueTreeState::ParameterLayout BassAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<APF> (juce::ParameterID { "comp", 1 }, "Kompression", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f, scale10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "drive", 1 }, "Zerre", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 0.0f, scale10()));
    l.add (std::make_unique<APF> (juce::ParameterID { "blend", 1 }, "Mischung", juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; })
              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    l.add (std::make_unique<APF> (juce::ParameterID { "xover", 1 }, "Weiche", juce::NormalisableRange<float> (80.0f, 400.0f, 1.0f, 0.6f), 160.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " Hz"; })
              .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue(); })));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "char", 1 }, "Charakter", juce::StringArray { u8 ("R\xc3\xb6hre"), "Transistor" }, 0));
    l.add (std::make_unique<APF> (juce::ParameterID { "bass", 1 }, "Bass", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, decibels()));
    l.add (std::make_unique<APF> (juce::ParameterID { "mid", 1 }, "Mitten", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, decibels()));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "midfreq", 1 }, "Mittenfrequenz",
        juce::StringArray { "250 Hz", "500 Hz", "800 Hz", "1,5 kHz", "3 kHz" }, 2));
    l.add (std::make_unique<APF> (juce::ParameterID { "treble", 1 }, u8 ("H\xc3\xb6hen"), juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, decibels()));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "box", 1 }, "Box", false));
    l.add (std::make_unique<APF> (juce::ParameterID { "out", 1 }, "Ausgang", juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f, decibels()));
    return l;
}

// Werte je Preset; "out" gleicht die Lautstärke an, damit beim Durchschalten der Klang zählt
const std::vector<BassAudioProcessor::Preset>& BassAudioProcessor::presets()
{
    static const std::vector<Preset> p = {
        { "Standard",                     { { "comp", 3 }, { "drive", 0 }, { "blend", 50 }, { "xover", 160 }, { "char", 0 }, { "bass", 0 },  { "mid", 0 },  { "midfreq", 2 }, { "treble", 0 },  { "box", 0 }, { "out", 0 } } },
        { "Motown \xe2\x80\x93 Flatwound", { { "comp", 5 }, { "drive", 1.5f }, { "blend", 35 }, { "xover", 180 }, { "char", 0 }, { "bass", 3 },  { "mid", 2 },  { "midfreq", 1 }, { "treble", -7 }, { "box", 1 }, { "out", 0 } } },
        { "Funk \xe2\x80\x93 Slap",        { { "comp", 6 }, { "drive", 0 }, { "blend", 0 },  { "xover", 160 }, { "char", 0 }, { "bass", 4 },  { "mid", -5 }, { "midfreq", 1 }, { "treble", 5 },  { "box", 0 }, { "out", 1.0f } } },
        { "Soul \xe2\x80\x93 warm",        { { "comp", 4 }, { "drive", 2.5f }, { "blend", 35 }, { "xover", 150 }, { "char", 0 }, { "bass", 2 },  { "mid", 1.5f }, { "midfreq", 0 }, { "treble", -3 }, { "box", 1 }, { "out", -0.5f } } },
        { "Blues \xe2\x80\x93 rau",        { { "comp", 3.5f }, { "drive", 4.5f }, { "blend", 45 }, { "xover", 180 }, { "char", 0 }, { "bass", 1 },  { "mid", 4 },  { "midfreq", 2 }, { "treble", 0 },  { "box", 1 }, { "out", -3.5f } } },
        { "Rock \xe2\x80\x93 Plektrum",    { { "comp", 4.5f }, { "drive", 5.5f }, { "blend", 55 }, { "xover", 200 }, { "char", 0 }, { "bass", 2 },  { "mid", 3 },  { "midfreq", 2 }, { "treble", 2 },  { "box", 1 }, { "out", -3.0f } } },
        { "Modern \xe2\x80\x93 Knurren",   { { "comp", 5 }, { "drive", 7 }, { "blend", 60 }, { "xover", 250 }, { "char", 1 }, { "bass", 3 },  { "mid", -2 }, { "midfreq", 1 }, { "treble", 3 },  { "box", 0 }, { "out", 0 } } },
        { "Dub \xe2\x80\x93 Tiefbass",     { { "comp", 5 }, { "drive", 0 }, { "blend", 0 },  { "xover", 120 }, { "char", 0 }, { "bass", 6 },  { "mid", -3 }, { "midfreq", 2 }, { "treble", -9 }, { "box", 1 }, { "out", 2.0f } } },
        { "Fingerstyle \xe2\x80\x93 natürlich", { { "comp", 3 }, { "drive", 1 }, { "blend", 25 }, { "xover", 160 }, { "char", 0 }, { "bass", 1 },  { "mid", 1 },  { "midfreq", 3 }, { "treble", 1 },  { "box", 0 }, { "out", -0.5f } } },
    };
    return p;
}

void BassAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets().size()))
        return;
    currentProgram = index;
    for (auto& [id, value] : presets()[(size_t) index].values)
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
}

const juce::String BassAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, (int) presets().size())
               ? juce::String (juce::CharPointer_UTF8 (presets()[(size_t) index].name)) : juce::String();
}

BassAudioProcessor::BassAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KlangbildBass", createLayout())
{
    pComp = apvts.getRawParameterValue ("comp");     pDrive = apvts.getRawParameterValue ("drive");
    pBlend = apvts.getRawParameterValue ("blend");   pXover = apvts.getRawParameterValue ("xover");
    pChar = apvts.getRawParameterValue ("char");     pBass = apvts.getRawParameterValue ("bass");
    pMid = apvts.getRawParameterValue ("mid");       pMidFreq = apvts.getRawParameterValue ("midfreq");
    pTreble = apvts.getRawParameterValue ("treble"); pBox = apvts.getRawParameterValue ("box");
    pOut = apvts.getRawParameterValue ("out");
}

bool BassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && in == out;
}

void BassAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    comp.prepare (sr);
    comp.setDetectorTime (0.012);
    for (auto& d : dc) d.prepare (sr * 2.0);
    for (auto& x : xover) x.reset();
    for (auto& t : toneState) t.reset();
    for (auto* arr : { &growlState, &postLpState1, &postLpState2 })
        for (auto& s : *arr) s.reset();

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) samplesPerBlock);
    // Tiefen und unverzerrter Anteil werden um dieselbe Zeit verzögert wie der Zerrweg
    delayLen = juce::jmax (0, juce::roundToInt (oversampling->getLatencyInSamples()));
    for (auto& d : delayLow)  d.assign ((size_t) delayLen + 1, 0.0f);
    for (auto& d : delayHigh) d.assign ((size_t) delayLen + 1, 0.0f);
    delayPos = 0;
    setLatencySamples (delayLen);

    lowBuf.setSize (2, samplesPerBlock);
    highBuf.setSize (2, samplesPerBlock);
    driveBuf.setSize (2, samplesPerBlock);

    lastFilterParams.fill (-1.0e9f);
    for (auto* s : { &smDriveGain, &smBlend, &smMakeup, &smOut, &smComp })
        s->reset (sr, 0.03);
    smDriveGain.setCurrentAndTargetValue (1.0f);
    smBlend.setCurrentAndTargetValue (0.0f);
    smMakeup.setCurrentAndTargetValue (1.0f);
    smOut.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    smComp.setCurrentAndTargetValue (1.0f);
    vuCoef = (float) (1.0 - std::exp (-1.0 / (0.01 * sr)));
    vuState = 0;
}

void BassAudioProcessor::updateFilters()
{
    const std::array<float, 9> now { pXover->load(), pDrive->load(), pChar->load(), pBass->load(), pMid->load(),
                                     pMidFreq->load(), pTreble->load(), pBox->load(), (float) sr };
    if (now == lastFilterParams)
        return;
    lastFilterParams = now;

    for (auto& x : xover) x.setFrequency (sr, now[0]);
    const bool transistor = now[2] > 0.5f;
    // Knurren: Mitten vor der Zerre leicht anheben, damit sie bissig wird statt matschig
    growl  = eq::makePeak (sr * 2.0, transistor ? 900.0 : 700.0, 0.8, 1.0 + 0.5 * now[1]);
    postLp = eq::makeLowPass (sr * 2.0, transistor ? 7000.0 : 5200.0, 0.70710678);
    const int mf = juce::jlimit (0, (int) midFrequencies.size() - 1, juce::roundToInt (now[5]));
    tone.set (sr, now[3], now[4], midFrequencies[(size_t) mf], now[6], now[7] > 0.5f);
}

void BassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0)
        return;
    if (n > lowBuf.getNumSamples())
    {
        lowBuf.setSize (2, n, false, false, true); highBuf.setSize (2, n, false, false, true); driveBuf.setSize (2, n, false, false, true);
        oversampling->initProcessing ((size_t) n);
    }

    updateFilters();

    const float amount = pComp->load(), drive = pDrive->load();
    const bool transistor = pChar->load() > 0.5f;
    comp.setAmount (amount);
    smMakeup.setTargetValue (juce::Decibels::decibelsToGain (juce::jmax (0.0f, amount - 2.0f) * 0.75f));   // Aufholverstärkung erst ab mittlerer Kompression
    // Vorverstärkung 0 .. +36 dB, Ausgleich, damit die Zerre nicht einfach nur lauter macht
    const float preDb = drive * 3.6f;
    smDriveGain.setTargetValue (juce::Decibels::decibelsToGain (preDb));
    smComp.setTargetValue (juce::Decibels::decibelsToGain (-preDb * 0.72f) * (transistor ? 1.0f : 1.1f));
    // Bei Zerre 0 bleibt der Klang sauber, egal wie die Mischung steht
    smBlend.setTargetValue (pBlend->load() * 0.01f * juce::jlimit (0.0f, 1.0f, drive * 2.0f));
    smOut.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    float maxGr = 0;

    // 1) Kompressor (gekoppelt) und Frequenzweiche
    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R ? R[i] : dl;
        const float gr = (float) comp.process (R ? 0.5f * (dl + dr) : dl);
        maxGr = juce::jmax (maxGr, gr);
        const float g = juce::Decibels::decibelsToGain (-gr) * smMakeup.getNextValue();
        const float dg = smDriveGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            float lo, hi;
            xover[(size_t) ch].process ((ch == 0 ? dl : dr) * g, lo, hi);
            lowBuf.setSample (ch, i, lo);
            highBuf.setSample (ch, i, hi);
            driveBuf.setSample (ch, i, hi * dg);
        }
    }

    // 2) Zerre auf den Mitten/Höhen, 2-fach überabgetastet
    {
        juce::dsp::AudioBlock<float> block (driveBuf.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
        auto up = oversampling->processSamplesUp (block);
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                float x = growlState[ch].process (growl, d[i]);
                x = transistor ? bass::shapeTransistor (x) : bass::shapeTube (x);
                x = dc[ch].process (x);
                x = postLpState2[ch].process (postLp, postLpState1[ch].process (postLp, x));
                d[i] = x;
            }
        }
        oversampling->processSamplesDown (block);
    }

    // 3) zusammenführen, Klangregelung, Box, Ausgang
    float pk = 0;
    for (int i = 0; i < n; ++i)
    {
        const float blend = smBlend.getNextValue(), cg = smComp.getNextValue(), og = smOut.getNextValue();
        const int rd = delayLen > 0 ? (delayPos + 1) % (delayLen + 1) : delayPos;
        float outS[2] = { 0, 0 };
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& dL = delayLow[(size_t) ch];
            auto& dH = delayHigh[(size_t) ch];
            dL[(size_t) delayPos] = lowBuf.getSample (ch, i);
            dH[(size_t) delayPos] = highBuf.getSample (ch, i);
            const float lo = dL[(size_t) rd], hiClean = dH[(size_t) rd];
            const float hiDrive = driveBuf.getSample (ch, i) * cg;
            const float x = lo + hiClean + blend * (hiDrive - hiClean);
            outS[ch] = toneState[(size_t) ch].process (tone, x) * og;
        }
        delayPos = rd;
        L[i] = outS[0];
        if (R) R[i] = outS[1];
        const float o = juce::jmax (std::abs (outS[0]), R ? std::abs (outS[1]) : 0.0f);
        vuState += (o - vuState) * vuCoef;
        pk = juce::jmax (pk, o);
    }

    grDb.store (maxGr);
    vuOut.store (vuState);
    peakOut.store (juce::jmax (pk, peakOut.load()));
}

void BassAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BassAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    auto tree = juce::ValueTree::fromXml (*xml);
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    apvts.replaceState (tree);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->convertTo0to1 (rp->convertFrom0to1 (rp->getValue())));
}

juce::AudioProcessorEditor* BassAudioProcessor::createEditor() { return new kb::ScaledEditor (*this, new BassAudioProcessorEditor (*this)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BassAudioProcessor(); }
