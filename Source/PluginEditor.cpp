#include "PluginEditor.h"

using namespace vintage;

namespace { juce::String u8 (const char* s) { return juce::String (juce::CharPointer_UTF8 (s)); } }

//==============================================================================
BassAudioProcessorEditor::BassAudioProcessorEditor (BassAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    compKnob = &addKnob ("comp", "KOMPRESSION", true);
    outKnob  = &addKnob ("out",  "AUSGANG",     true);
    driveKnobs = { &addKnob ("drive", "ZERRE", false), &addKnob ("blend", "MISCHUNG", false), &addKnob ("xover", "WEICHE", false) };
    toneKnobs  = { &addKnob ("bass", "BASS", false), &addKnob ("mid", "MITTEN", false),
                   &addKnob ("midfreq", "FREQUENZ", false), &addKnob ("treble", u8 ("H\xc3\x96HEN"), false) };
    toneKnobs[2]->slider.getProperties().set ("stepLabels", "250|500|800|1,5k|3k");
    driveKnobs[2]->slider.getProperties().set ("stepLabels", "80|110|180|280|400");

    setupSwitch (charSwitch, "char", "TRANSISTOR", u8 ("R\xc3\x96HRE"), lampAmber);
    setupSwitch (boxSwitch,  "box",  "BOX", "DI", lampRed);

    presetBox.getProperties().set ("plate", true);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    vuGr.setRestAtZeroVu (true);
    addAndMakeVisible (vuGr);
    addAndMakeVisible (vuOutMeter);

    setSize (1180, 780);
    startTimerHz (60);
}

BassAudioProcessorEditor::~BassAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

BassAudioProcessorEditor::Knob& BassAudioProcessorEditor::addKnob (const juce::String& paramID, const juce::String& name, bool big)
{
    auto k = std::make_unique<Knob>();
    k->name = name;
    k->big = big;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    k->slider.setMouseDragSensitivity (big ? 320 : 220);
    addAndMakeVisible (k->slider);
    k->attachment = std::make_unique<SliderAttachment> (proc.apvts, paramID, k->slider);
    if (auto* rp = proc.apvts.getParameter (paramID))
        k->slider.setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void BassAudioProcessorEditor::setupSwitch (juce::ToggleButton& b, const juce::String& paramID,
                                            const juce::String& onText, const juce::String& offText, juce::Colour lamp)
{
    b.setButtonText ("x");
    b.getProperties().set ("onText", onText);
    b.getProperties().set ("offText", offText);
    b.getProperties().set ("lamp", (int) lamp.getARGB());
    addAndMakeVisible (b);
    buttonAttachments.push_back (std::make_unique<ButtonAttachment> (proc.apvts, paramID, b));
}

void BassAudioProcessorEditor::timerCallback()
{
    constexpr float dt = 1.0f / 60.0f;
    // Pegelreduktion: ohne Kompression steht der Zeiger auf 0 VU und wandert nach links
    vuGr.update (juce::Decibels::decibelsToGain (-proc.grDb.load() - 18.0f) / 1.1107f, 0.0f, dt);
    vuOutMeter.update (proc.vuOut.load(), proc.peakOut.exchange (0.0f), dt);

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

//==============================================================================
void BassAudioProcessorEditor::renderTextures()
{
    tolexImage = juce::Image (juce::Image::RGB, juce::jmax (1, getWidth()), juce::jmax (1, getHeight()), false);
    {
        juce::Image::BitmapData data (tolexImage, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (11);
        for (int y = 0; y < tolexImage.getHeight(); ++y)
            for (int x = 0; x < tolexImage.getWidth(); ++x)
            {
                const int weave = ((x + y) % 4 == 0 || (x - y + 4096) % 4 == 0) ? -5 : 0;
                const int n = rnd.nextInt (13) - 6 + weave;
                data.setPixelColour (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, 34 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 28 + n),
                                                         (juce::uint8) juce::jlimit (0, 255, 24 + n)));
            }
    }
    if (faceplate.isEmpty())
        return;
    faceplateImage = juce::Image (juce::Image::ARGB, faceplate.getWidth(), faceplate.getHeight(), true);
    juce::Graphics g (faceplateImage);
    const auto r = faceplateImage.getBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (cream.brighter (0.06f), 0, 0, creamDark, 0, r.getHeight(), false));
    g.fillRoundedRectangle (r, 6.0f);
    juce::Random rnd (5);
    for (int y = 0; y < faceplate.getHeight(); ++y)
    {
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colour (0xff6b5534)).withAlpha (0.015f + rnd.nextFloat() * 0.035f));
        g.drawHorizontalLine (y, 0.0f, r.getWidth());
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                             juce::Colour (0x30694a1c), 0, 0, true));
    g.fillRoundedRectangle (r, 6.0f);
}

void BassAudioProcessorEditor::drawEngraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                                             juce::Font f, juce::Justification j)
{
    g.setFont (f);
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawFittedText (text, r.translated (0.0f, 1.0f).toNearestInt(), j, 2, 1.0f);
    g.setColour (ink);
    g.drawFittedText (text, r.toNearestInt(), j, 2, 1.0f);
}

// eingravierter Rahmen mit Titel, gruppiert zusammengehörige Regler
void BassAudioProcessorEditor::drawGroup (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title)
{
    const auto a = area.toFloat();
    const auto f = lnf.labelFont (13.0f, 0.3f);
    const float tw = (float) juce::GlyphArrangement::getStringWidthInt (f, title) + 24.0f;
    juce::Path frame;
    frame.startNewSubPath (a.getX() + 20.0f, a.getY());
    frame.lineTo (a.getX() + 6.0f, a.getY());
    frame.quadraticTo (a.getX(), a.getY(), a.getX(), a.getY() + 6.0f);
    frame.lineTo (a.getX(), a.getBottom() - 6.0f);
    frame.quadraticTo (a.getX(), a.getBottom(), a.getX() + 6.0f, a.getBottom());
    frame.lineTo (a.getRight() - 6.0f, a.getBottom());
    frame.quadraticTo (a.getRight(), a.getBottom(), a.getRight(), a.getBottom() - 6.0f);
    frame.lineTo (a.getRight(), a.getY() + 6.0f);
    frame.quadraticTo (a.getRight(), a.getY(), a.getRight() - 6.0f, a.getY());
    frame.lineTo (a.getX() + 20.0f + tw, a.getY());
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.strokePath (frame, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.0f));
    g.setColour (ink.withAlpha (0.55f));
    g.strokePath (frame, juce::PathStrokeType (1.0f));
    drawEngraved (g, title, { a.getX() + 20.0f, a.getY() - 8.0f, tw, 16.0f }, f, juce::Justification::centred);
}

void BassAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (tolexImage, 0, 0);

    // Logo
    const auto scriptF = lnf.scriptFont (64.0f);
    const auto logoArea = juce::Rectangle<float> (40.0f, 8.0f, 400.0f, 84.0f);
    g.setFont (scriptF);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawText ("Klangbild", logoArea.translated (2.0f, 3.0f), juce::Justification::centredLeft);
    g.setColour (logo);
    g.drawText ("Klangbild", logoArea, juce::Justification::centredLeft);
    const float sw = (float) juce::GlyphArrangement::getStringWidthInt (scriptF, "Klangbild");
    g.setColour (gold);
    g.setFont (lnf.labelFont (26.0f, 0.35f));
    g.drawText ("BASS", juce::Rectangle<float> (logoArea.getX() + sw + 14.0f, logoArea.getY() + 30.0f, 140.0f, 40.0f), juce::Justification::centredLeft);
    g.setColour (logo.withAlpha (0.55f));
    g.setFont (lnf.labelFont (12.0f, 0.3f));
    g.drawText (u8 ("BASS-KANALZUG \xc2\xb7 KOMPRESSOR \xc2\xb7 ZERRE \xc2\xb7 KLANG \xc2\xb7 BOX"),
                juce::Rectangle<float> (44.0f, 90.0f, 600.0f, 16.0f), juce::Justification::centredLeft);

    const auto pa = presetArea.toFloat();
    g.setColour (logo.withAlpha (0.6f));
    g.setFont (lnf.labelFont (10.0f, 0.3f));
    g.drawText ("PROGRAMM", pa.withY (pa.getY() - 15.0f).withHeight (13.0f), juce::Justification::centredLeft);

    // Signalweg rechts oben
    g.setColour (logo.withAlpha (0.45f));
    g.setFont (lnf.labelFont (11.0f, 0.25f));
    g.drawText (u8 ("KOMPRESSOR \xe2\x86\x92 WEICHE \xe2\x86\x92 ZERRE (NUR MITTEN) \xe2\x86\x92 KLANG \xe2\x86\x92 BOX"),
                juce::Rectangle<float> ((float) getWidth() - 640.0f, 140.0f, 600.0f, 16.0f), juce::Justification::centredRight);

    // Frontplatte
    const auto fp = faceplate.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (fp.expanded (4.0f).translated (0, 2.0f), 9.0f);
    g.setGradientFill (juce::ColourGradient (gold.brighter (0.3f), 0, fp.getY(), gold.darker (0.6f), 0, fp.getBottom(), false));
    g.fillRoundedRectangle (fp.expanded (3.0f), 8.0f);
    g.drawImageAt (faceplateImage, faceplate.getX(), faceplate.getY());
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (fp, 6.0f, 1.0f);
    for (auto [x, y, a] : { std::tuple { fp.getX() + 12.0f, fp.getY() + 12.0f, 0.4f }, std::tuple { fp.getRight() - 12.0f, fp.getY() + 12.0f, 1.3f },
                            std::tuple { fp.getX() + 12.0f, fp.getBottom() - 12.0f, 2.2f }, std::tuple { fp.getRight() - 12.0f, fp.getBottom() - 12.0f, 0.9f } })
        VintageLookAndFeel::drawScrew (g, { x, y }, 5.0f, a);

    // Rahmen um die beiden VU-Meter
    for (auto vb : { vuBezelGr.toFloat(), vuBezelOut.toFloat() })
    {
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (vb.translated (2.0f, 3.0f), 12.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2520), 0, vb.getY(), juce::Colour (0xff120f0c), 0, vb.getBottom(), false));
        g.fillRoundedRectangle (vb, 12.0f);
    }

    drawEngraved (g, "PEGELREDUKTION", vuBezelGr.toFloat().withY ((float) vuBezelGr.getBottom() + 8.0f).withHeight (18.0f), lnf.labelFont (13.0f, 0.2f), juce::Justification::centred);
    drawEngraved (g, "AUSGANGSPEGEL", vuBezelOut.toFloat().withY ((float) vuBezelOut.getBottom() + 8.0f).withHeight (18.0f), lnf.labelFont (13.0f, 0.2f), juce::Justification::centred);

    drawGroup (g, groupDrive, u8 ("ZERRE \xc2\xb7 NUR AUF DEN MITTEN"));
    drawGroup (g, groupTone, "KLANG");

    for (auto& k : knobs)
    {
        const auto sb = k->slider.getBounds().toFloat();
        drawEngraved (g, k->name, { sb.getX() - 20.0f, sb.getBottom() - 4.0f, sb.getWidth() + 40.0f, k->big ? 24.0f : 18.0f },
                      lnf.labelFont (k->big ? 18.0f : 13.0f, k->big ? 0.2f : 0.14f), juce::Justification::centred);
    }

    const juce::String names[] = { "CHARAKTER", "BOX-SIMULATION" };
    for (size_t i = 0; i < 2; ++i)
        drawEngraved (g, names[i], switchLabels[i].toFloat(), lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);

    // Betriebslampe und Typenschild
    const auto pl = pilotArea.toFloat();
    VintageLookAndFeel::drawJewel (g, juce::Rectangle<float> (30.0f, 30.0f).withCentre ({ pl.getX() + 22.0f, pl.getCentreY() }), lampRed, true);
    drawEngraved (g, "BETRIEB", { pl.getX() + 44.0f, pl.getCentreY() - 8.0f, 80.0f, 16.0f }, lnf.labelFont (12.0f, 0.14f), juce::Justification::centredLeft);
    const auto tag = juce::Rectangle<float> (130.0f, 26.0f).withRightX (pl.getRight()).withCentre ({ pl.getRight() - 65.0f, pl.getCentreY() });
    g.setColour (ink.withAlpha (0.85f));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (gold);
    g.setFont (lnf.labelFont (11.0f, 0.2f));
    g.drawText ("MODELL KB-410", tag, juce::Justification::centred);
}

void BassAudioProcessorEditor::resized()
{
    presetArea = { 44, 128, 300, 32 };
    presetBox.setBounds (presetArea);

    faceplate = { 24, 178, getWidth() - 48, getHeight() - 202 };
    const auto in = faceplate.reduced (30, 20);

    // obere Reihe: Kompression | VU Kompression | VU Ausgang | Ausgang
    compKnob->slider.setBounds (in.getX(), in.getY() + 14, 220, 200);
    outKnob->slider.setBounds (in.getRight() - 220, in.getY() + 14, 220, 200);
    const int vuW = 280, vuH = 190, gap = (in.getWidth() - 440 - 2 * vuW) / 3;
    vuBezelGr  = { in.getX() + 220 + gap, in.getY() + 18, vuW, vuH };
    vuBezelOut = { vuBezelGr.getRight() + gap, in.getY() + 18, vuW, vuH };
    vuGr.setBounds (vuBezelGr.reduced (10));
    vuOutMeter.setBounds (vuBezelOut.reduced (10));

    // untere Reihe: Gruppen Zerre und Klang
    const int gy = in.getY() + 258, gh = in.getBottom() - gy;
    groupDrive = { in.getX(), gy, 470, gh };
    groupTone  = { groupDrive.getRight() + 18, gy, in.getRight() - groupDrive.getRight() - 18, gh };

    constexpr int kw = 116, kh = 112;
    for (int i = 0; i < 3; ++i)
    {
        const int cx = groupDrive.getX() + groupDrive.getWidth() * (2 * i + 1) / 6;
        driveKnobs[(size_t) i]->slider.setBounds (cx - kw / 2, gy + 24, kw, kh);
    }
    for (int i = 0; i < 4; ++i)
    {
        const int cx = groupTone.getX() + groupTone.getWidth() * (2 * i + 1) / 8;
        toneKnobs[(size_t) i]->slider.setBounds (cx - kw / 2, gy + 24, kw, kh);
    }

    const int swY = gy + 24 + kh + 30;
    switchLabels[0] = { groupDrive.getX() + 24, swY, 200, 16 };
    charSwitch.setBounds (groupDrive.getX() + 24, swY + 20, 190, 40);
    switchLabels[1] = { groupTone.getX() + 24, swY, 200, 16 };
    boxSwitch.setBounds (groupTone.getX() + 24, swY + 20, 150, 40);
    pilotArea = { groupTone.getRight() - 300, swY + 14, 280, 40 };

    renderTextures();
}
