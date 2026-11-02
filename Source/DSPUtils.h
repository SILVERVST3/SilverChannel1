#pragma once
#include <cmath>
#include <algorithm>
#include <atomic>
#include <juce_core/juce_core.h>

namespace sc
{
constexpr float kPi = 3.14159265358979323846f;

inline float dbToGain (float db)  { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)   { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float timeCoef (float ms, double sr)
{
    return ms <= 0.0f ? 0.0f : (float) std::exp (-1.0 / (0.001 * (double) ms * sr));
}
inline void atomicMax (std::atomic<float>& a, float v)
{
    float cur = a.load (std::memory_order_relaxed);
    while (v > cur && ! a.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
}
inline float flushDenormal (float v) { return std::abs (v) < 1.0e-15f ? 0.0f : v; }

//==============================================================================
// Biquad RBJ (Direct Form II transposta) - sem alocacao no thread de audio
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0.0f; }
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        z1 = flushDenormal (z1); z2 = flushDenormal (z2);
        return y;
    }
    void setNorm (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
    void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    static double clampF (double f, double sr) { return std::clamp (f, 5.0, sr * 0.49); }

    void setPeak (double sr, double f, double q, double gainDb)
    {
        f = clampF (f, sr);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q);
        setNorm (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
    void setLowShelf (double sr, double f, double q, double gainDb)
    {
        f = clampF (f, sr);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q), sA = 2.0 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) - (A - 1) * cs + sA), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - sA),
                 (A + 1) + (A - 1) * cs + sA, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - sA);
    }
    void setHighShelf (double sr, double f, double q, double gainDb)
    {
        f = clampF (f, sr);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q), sA = 2.0 * std::sqrt (A) * al;
        setNorm (A * ((A + 1) + (A - 1) * cs + sA), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - sA),
                 (A + 1) - (A - 1) * cs + sA, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - sA);
    }
    void setHighPass (double sr, double f, double q)
    {
        f = clampF (f, sr);
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr, cs = std::cos (w), al = std::sin (w) / (2.0 * q);
        setNorm ((1 + cs) / 2, -(1 + cs), (1 + cs) / 2, 1 + al, -2 * cs, 1 - al);
    }
    void setLowPass (double sr, double f, double q)
    {
        f = clampF (f, sr);
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr, cs = std::cos (w), al = std::sin (w) / (2.0 * q);
        setNorm ((1 - cs) / 2, 1 - cs, (1 - cs) / 2, 1 + al, -2 * cs, 1 - al);
    }
};

//==============================================================================
// State Variable Filter TPT (Simper/Zavalishin): x = lp + k*bp + hp
struct SVF
{
    float g = 0, k = 1.414f, a1 = 0, a2 = 0, a3 = 0;
    float ic1 = 0, ic2 = 0;

    void reset() { ic1 = ic2 = 0.0f; }
    void set (double sr, double f, double q)
    {
        f = std::clamp (f, 5.0, sr * 0.49);
        g  = (float) std::tan (juce::MathConstants<double>::pi * f / sr);
        k  = (float) (1.0 / q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    inline void tick (float v0, float& lp, float& bp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flushDenormal (2.0f * v1 - ic1);
        ic2 = flushDenormal (2.0f * v2 - ic2);
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
};

// Filtro de 1 polo TPT
struct OnePole
{
    float G = 0, s = 0;
    void reset() { s = 0; }
    void set (double sr, double f)
    {
        f = std::clamp (f, 5.0, sr * 0.49);
        const double g = std::tan (juce::MathConstants<double>::pi * f / sr);
        G = (float) (g / (1.0 + g));
    }
    inline float lp (float x)
    {
        const float v = (x - s) * G;
        const float y = v + s;
        s = flushDenormal (y + v);
        return y;
    }
    inline float hp (float x) { return x - lp (x); }
};

// Bloqueador de DC
struct DCBlock
{
    float x1 = 0, y1 = 0, R = 0.9995f;
    void prepare (double sr) { R = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * 8.0 / sr); reset(); }
    void reset() { x1 = y1 = 0; }
    inline float process (float x)
    {
        const float y = x - x1 + R * y1;
        x1 = x; y1 = flushDenormal (y);
        return y;
    }
};

// Suavizador exponencial simples (para parametros de filtro por sub-bloco)
struct Smooth
{
    float v = 0, target = 0, c = 0.9f; bool init = false;
    void setTime (double ratePerSec, float ms) { c = (float) std::exp (-1.0 / (0.001 * ms * ratePerSec)); }
    void setTarget (float t) { target = t; if (! init) { v = t; init = true; } }
    float next() { v = target + c * (v - target); if (std::abs (v - target) < 1.0e-6f * std::max (1.0f, std::abs (target))) v = target; return v; }
};

// Mid/Side
inline void msEncode (float* L, float* R, int n)
{
    for (int i = 0; i < n; ++i) { const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]); L[i] = m; R[i] = s; }
}
inline void msDecode (float* M, float* S, int n)
{
    for (int i = 0; i < n; ++i) { const float l = M[i] + S[i], r = M[i] - S[i]; M[i] = l; S[i] = r; }
}

// Curva do computador de ganho com joelho (retorna reducao em dB, positiva)
inline float kneeGR (float over, float slope, float knee)
{
    if (knee <= 0.0f) return over > 0.0f ? over * slope : 0.0f;
    if (2.0f * over < -knee) return 0.0f;
    if (2.0f * std::abs (over) <= knee) { const float t = over + knee * 0.5f; return slope * t * t / (2.0f * knee); }
    return over * slope;
}
} // namespace sc
