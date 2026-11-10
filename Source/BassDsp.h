#pragma once

// Klangbild Bass – Bausteine
//  Kompressor  : weicher Feed-Forward-Kompressor (4:1, weiches Knie), Stereo gekoppelt
//  Weiche      : Linkwitz-Riley 4. Ordnung – Tiefen bleiben sauber, nur die Mitten/Höhen werden angezerrt
//  Zerre       : unsymmetrische Röhren-Sättigung oder härtere Transistor-Kennlinie, mit Knurr-Anhebung davor
//                und Tiefpass danach (gegen Kratzen); läuft 2-fach überabgetastet
//  Klang       : Bass-Shelf, Mitten-Glocke (5 Frequenzen), Höhen-Shelf
//  Box         : Bassbox-Simulation (8x10" / 1x15"-Charakter: Tiefbass-Bauch, Mitten-Senke, früher Höhenabfall)

#include <cmath>
#include <algorithm>
#include "EQDsp.h"

namespace bass
{
class Compressor
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        aAtt = coef (0.008);
        aRel = coef (0.140);
        aRelSlow = coef (0.9);
        det = env = slow = 0.0;
    }

    // amount 0..10
    void setAmount (float amount)
    {
        threshold = -4.0 - 3.4 * amount;    // -4 .. -38 dBFS
    }

    // liefert die Pegelreduktion in dB (>= 0)
    double process (float sidechain) noexcept
    {
        const double s = (double) sidechain * (double) sidechain;
        det = s + (det - s) * aDetRms;
        const double lvl = 10.0 * std::log10 (det + 1.0e-12) + 3.0;   // RMS -> etwa Spitzenpegel eines Sinus
        const double tgt = staticGr (lvl);
        env = tgt + (env - tgt) * (tgt > env ? aAtt : aRel);
        slow = env + (slow - env) * (env > slow ? aAtt : aRelSlow);
        return std::max (env, 0.6 * slow);                            // lange Töne regeln ruhiger aus
    }

    double staticGr (double lvl) const noexcept
    {
        constexpr double ratio = 4.0, knee = 10.0;
        const double over = lvl - threshold;
        if (over <= -knee * 0.5) return 0.0;
        if (over >= knee * 0.5)  return over * (1.0 - 1.0 / ratio);
        const double k = over + knee * 0.5;
        return (1.0 - 1.0 / ratio) * k * k / (2.0 * knee);
    }

    void setDetectorTime (double seconds) { aDetRms = coef (seconds); }

private:
    double coef (double seconds) const { return std::exp (-1.0 / (seconds * fs)); }
    double fs = 48000.0, aAtt = 0, aRel = 0, aRelSlow = 0, aDetRms = 0.999;
    double det = 0, env = 0, slow = 0, threshold = -10.0;
};

// Linkwitz-Riley-Weiche 4. Ordnung (zwei Butterworth-Stufen je Zweig): Summe ist amplitudenflach
class Crossover
{
public:
    void setFrequency (double fs, double f)
    {
        lp = eq::makeLowPass (fs, f, 0.70710678);
        hp = eq::makeHighPass (fs, f, 0.70710678);
    }
    void reset() { for (auto* s : { &l1, &l2, &h1, &h2 }) s->reset(); }
    inline void process (float x, float& low, float& high) noexcept
    {
        low  = l2.process (lp, l1.process (lp, x));
        high = h2.process (hp, h1.process (hp, x));
    }
private:
    eq::Coeffs lp, hp;
    eq::BiquadState l1, l2, h1, h2;
};

// Kennlinien der Zerre (Eingang ist bereits verstärkt)
inline float shapeTube (float x) noexcept
{
    constexpr float bias = 0.18f;
    return std::tanh (x + bias) - std::tanh (bias);              // unsymmetrisch -> gerade Obertöne, warm
}

inline float shapeTransistor (float x) noexcept
{
    const float y = x / std::pow (1.0f + std::pow (std::abs (x), 3.0f), 1.0f / 3.0f);   // härteres Knie
    return 0.92f * y + 0.08f * std::tanh (3.0f * x);
}

// Einfacher DC-Blocker (nach unsymmetrischer Sättigung)
struct DcBlock
{
    void prepare (double fs) { a = (float) std::exp (-2.0 * 3.14159265358979 * 12.0 / fs); x1 = y1 = 0; }
    inline float process (float x) noexcept { const float y = x - x1 + a * y1; x1 = x; y1 = y; return y; }
    float a = 0.999f, x1 = 0, y1 = 0;
};

// Klangregelung + Box in einer festen Kette aus Biquads
struct ToneStack
{
    enum { hpf, bassShelf, midBell, trebleShelf, box1, box2, box3, box4, box5, numStages };
    std::array<eq::Coeffs, numStages> c;
    bool boxOn = false;

    void set (double fs, float bassDb, float midDb, double midHz, float trebleDb, bool box)
    {
        c[hpf]         = eq::makeHighPass  (fs, 28.0, 0.70710678);
        c[bassShelf]   = eq::makeLowShelf  (fs, 90.0, 0.70710678, bassDb);
        c[midBell]     = eq::makePeak      (fs, midHz, 0.9, midDb);
        c[trebleShelf] = eq::makeHighShelf (fs, 2800.0, 0.70710678, trebleDb);
        boxOn = box;
        c[box1] = eq::makeHighPass (fs, 42.0, 0.9);
        c[box2] = eq::makePeak     (fs, 105.0, 1.1, 3.0);
        c[box3] = eq::makePeak     (fs, 520.0, 1.3, -4.0);
        c[box4] = eq::makePeak     (fs, 1500.0, 1.4, 2.0);
        c[box5] = eq::makeLowPass  (fs, 4200.0, 0.8);
    }
};

struct ToneState
{
    std::array<eq::BiquadState, ToneStack::numStages> s;
    void reset() { for (auto& x : s) x.reset(); }
    inline float process (const ToneStack& t, float x) noexcept
    {
        const int last = t.boxOn ? ToneStack::numStages : ToneStack::box1;
        for (int i = 0; i < last; ++i)
            x = s[(size_t) i].process (t.c[(size_t) i], x);
        return x;
    }
};
} // namespace bass
