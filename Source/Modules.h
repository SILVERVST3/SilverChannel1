#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSPUtils.h"
#include "Params.h"

//==============================================================================
// Modulos de processamento do SilverChannel 01
// Todos processam 2 canais (L/R) in-place. Mono e tratado pelo processador
// duplicando o canal.
//==============================================================================
namespace sc
{
using APVTS = juce::AudioProcessorValueTreeState;

inline std::atomic<float>* rawParam (APVTS& s, const char* id)
{
    auto* p = s.getRawParameterValue (id);
    jassert (p != nullptr);
    return p;
}
inline bool  pb (std::atomic<float>* p) { return p->load (std::memory_order_relaxed) > 0.5f; }
inline float pf (std::atomic<float>* p) { return p->load (std::memory_order_relaxed); }
inline int   pi (std::atomic<float>* p) { return (int) std::lround (p->load (std::memory_order_relaxed)); }

constexpr int kSub = 32; // tamanho de sub-bloco para atualizacao de coeficientes

//==============================================================================
// PRE: filtros HP/LP com ressonancia, Thump e saturacao (ODD/EVEN/HEAVY/CRUSH)
//==============================================================================
class PreModule
{
public:
    void bind (APVTS& s)
    {
        satOn = rawParam (s, "sat_on");   satAmt = rawParam (s, "sat_amt"); satType = rawParam (s, "sat_type");
        hpOn  = rawParam (s, "hp_on");    hpFreq = rawParam (s, "hp_freq"); hpSlope = rawParam (s, "hp_slope");
        lpOn  = rawParam (s, "lp_on");    lpFreq = rawParam (s, "lp_freq"); lpSlope = rawParam (s, "lp_slope");
        res   = rawParam (s, "flt_res");  thumpOn = rawParam (s, "thump_on"); thumpAmt = rawParam (s, "thump_amt");
    }

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1,
                juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        os->initProcessing ((size_t) maxBlock);
        latency = (int) std::lround (os->getLatencyInSamples());
        delayBuf.assign (2, std::vector<float> ((size_t) latency + 1, 0.0f));
        delayPos = 0;
        const double subRate = sr / kSub;
        for (auto* s : { &hpF, &lpF, &resS }) s->setTime (subRate, 25.0f);
        hpF.init = lpF.init = resS.init = false;
        for (auto& d : dc) d.prepare (sr);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            hp1[c].reset(); lp1[c].reset();
            for (auto& f : hpS[c]) f.reset();
            for (auto& f : lpS[c]) f.reset();
            th1[c].reset(); th2[c].reset(); dc[c].reset();
        }
        if (os) os->reset();
        for (auto& d : delayBuf) std::fill (d.begin(), d.end(), 0.0f);
        lastDrive = -1.0f;
    }

    int getLatency() const { return latency; }

    // Mantem o alinhamento quando o modulo ou a saturacao estao desligados
    void processDelayOnly (float* L, float* R, int n)
    {
        if (latency <= 0) return;
        float* ch[2] = { L, R };
        const int len = latency + 1;
        for (int i = 0; i < n; ++i)
        {
            const int rp = (delayPos + 1) % len;
            for (int c = 0; c < 2; ++c)
            {
                delayBuf[(size_t) c][(size_t) delayPos] = ch[c][i];
                ch[c][i] = delayBuf[(size_t) c][(size_t) rp];
            }
            delayPos = rp;
        }
    }

    void process (float* L, float* R, int n)
    {
        float* ch[2] = { L, R };
        const bool doHp = pb (hpOn), doLp = pb (lpOn), doThump = pb (thumpOn);
        const int  hs = pi (hpSlope), ls = pi (lpSlope);
        hpF.setTarget (std::log (pf (hpFreq)));
        lpF.setTarget (std::log (pf (lpFreq)));
        resS.setTarget (pf (res));

        const float thumpDb = pi (thumpAmt) == 0 ? 2.0f : 4.0f;
        if (doThump && thumpDb != lastThump)
        {
            lastThump = thumpDb;
            for (int c = 0; c < 2; ++c)
            {
                th1[c].setLowShelf (sr, 85.0, 1.35, thumpDb);       // shelf ressonante largo
                th2[c].setPeak (sr, 210.0, 1.1, -thumpDb * 0.35);   // leve recuo acima (efeito "ressonancia")
            }
        }

        for (int start = 0; start < n; start += kSub)
        {
            const int m = std::min (kSub, n - start);
            const float fh = std::exp (hpF.next()), fl = std::exp (lpF.next()), r = resS.next();
            if (doHp) setFilter (true,  hs, fh, r);
            if (doLp) setFilter (false, ls, fl, r);

            for (int c = 0; c < 2; ++c)
            {
                float* x = ch[c] + start;
                for (int i = 0; i < m; ++i)
                {
                    float v = x[i], a, b, h;
                    if (doHp)
                    {
                        switch (hs)
                        {
                            case 0: v = hp1[c].hp (v); break;
                            case 1: hpS[c][0].tick (v, a, b, h); v = h; break;
                            case 2: v = hp1[c].hp (v); hpS[c][0].tick (v, a, b, h); v = h; break;
                            default: hpS[c][0].tick (v, a, b, h); hpS[c][1].tick (h, a, b, h); v = h; break;
                        }
                    }
                    if (doLp)
                    {
                        switch (ls)
                        {
                            case 0: v = lp1[c].lp (v); break;
                            case 1: lpS[c][0].tick (v, a, b, h); v = a; break;
                            case 2: v = lp1[c].lp (v); lpS[c][0].tick (v, a, b, h); v = a; break;
                            default: lpS[c][0].tick (v, a, b, h); { float a2; lpS[c][1].tick (a, a2, b, h); v = a2; } break;
                        }
                    }
                    if (doThump) v = th2[c].process (th1[c].process (v));
                    x[i] = v;
                }
            }
        }

        if (! pb (satOn)) { processDelayOnly (L, R, n); return; }

        // ---- Saturacao com oversampling 2x ----
        const int type = pi (satType);
        const float amt = pf (satAmt) * 0.01f;
        const float driveDb = (type == 3) ? 6.0f + 36.0f * amt : 30.0f * amt;
        const float drive = dbToGain (driveDb);
        if (lastDrive < 0.0f) lastDrive = drive;

        juce::dsp::AudioBlock<float> block (ch, 2, (size_t) n);
        auto up = os->processSamplesUp (block);
        const int un = (int) up.getNumSamples();
        // compensacao: unitario em sinais baixos, ~mesma sonoridade em niveis tipicos de mix
        const float k = (type == 3) ? 0.8f : 1.0f;
        auto makeup = [k] (float g) { return k * std::sqrt (1.0f + (g / 5.0f) * (g / 5.0f)) / g; };
        const float comp1 = makeup (lastDrive);
        const float comp2 = makeup (drive);
        for (int c = 0; c < 2; ++c)
        {
            float* x = up.getChannelPointer ((size_t) c);
            for (int i = 0; i < un; ++i)
            {
                const float t = (float) i / (float) un;
                const float g = lastDrive + (drive - lastDrive) * t;
                const float cp = comp1 + (comp2 - comp1) * t;
                x[i] = shape (type, x[i] * g) * cp;
            }
        }
        os->processSamplesDown (block);
        lastDrive = drive;
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i) ch[c][i] = dc[c].process (ch[c][i]);
    }

    static inline float shape (int type, float u)
    {
        switch (type)
        {
            case 0: // ODD - simetrica, harmonicos impares, joelho macio
                return std::tanh (u);
            case 1: // EVEN - assimetrica, harmonicos pares (DC removido depois)
            {
                constexpr float b = 0.45f;
                static const float tb = std::tanh (b), d = 1.0f - tb * tb;
                return (std::tanh (u + b) - tb) / d;
            }
            case 2: // HEAVY - clipper de joelho firme (resposta "custom")
            {
                const float a = std::abs (u);
                const float a6 = a * a * a * a * a * a;
                return u / std::pow (1.0f + a6, 1.0f / 6.0f) * 0.98f + 0.02f * std::tanh (u);
            }
            default: // CRUSH - distorcao agressiva com dobra e assimetria
            {
                const float v = std::tanh (1.6f * u);
                float y = 1.15f * v + 0.28f * std::sin (2.6f * kPi * v) + 0.12f * v * v;
                return std::clamp (y, -1.0f, 1.0f);
            }
        }
    }

private:
    void setFilter (bool high, int slope, float f, float r)
    {
        const float k = r / 0.7071f; // fator de ressonancia (1 = Butterworth)
        for (int c = 0; c < 2; ++c)
        {
            OnePole& op = high ? hp1[c] : lp1[c];
            SVF* s = high ? hpS[c] : lpS[c];
            switch (slope)
            {
                case 0: op.set (sr, f); break;
                case 1: s[0].set (sr, f, std::clamp (0.7071f * k, 0.1f, 12.0f)); break;
                case 2: op.set (sr, f); s[0].set (sr, f, std::clamp (1.0f * k, 0.1f, 12.0f)); break;
                default: s[0].set (sr, f, 0.5412f); s[1].set (sr, f, std::clamp (1.3066f * k, 0.1f, 14.0f)); break;
            }
        }
    }

    std::atomic<float> *satOn{}, *satAmt{}, *satType{}, *hpOn{}, *hpFreq{}, *hpSlope{}, *lpOn{}, *lpFreq{}, *lpSlope{}, *res{}, *thumpOn{}, *thumpAmt{};
    double sr = 44100.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    int latency = 0;
    std::vector<std::vector<float>> delayBuf; int delayPos = 0;
    OnePole hp1[2], lp1[2];
    SVF hpS[2][2], lpS[2][2];
    Biquad th1[2], th2[2];
    DCBlock dc[2];
    Smooth hpF, lpF, resS;
    float lastDrive = -1.0f, lastThump = 0.0f;
};

//==============================================================================
// EQ: HIGH / MID / TONE / LOW
//==============================================================================
class EqModule
{
public:
    void bind (APVTS& s)
    {
        const char* keys[4] = { "eq_hi", "eq_mid", "eq_tone", "eq_lo" };
        for (int b = 0; b < 4; ++b)
        {
            const juce::String k (keys[b]);
            bands[b].on   = rawParam (s, (k + "_on").toRawUTF8());
            bands[b].gain = rawParam (s, (k + "_gain").toRawUTF8());
            bands[b].freq = rawParam (s, (k + "_freq").toRawUTF8());
            bands[b].type = rawParam (s, (k + "_type").toRawUTF8());
            bands[b].q    = rawParam (s, (k + "_q").toRawUTF8());
        }
    }
    void prepare (double sampleRate, int)
    {
        sr = sampleRate;
        for (auto& b : bands)
        {
            for (auto* sm : { &b.sg, &b.sf, &b.sq }) { sm->setTime (sr / kSub, 20.0f); sm->init = false; }
            b.lastG = b.lastF = b.lastQ = -999.0f; b.lastT = -1;
        }
        reset();
    }
    void reset() { for (auto& b : bands) for (auto& c : b.f) for (auto& q : c) q.reset(); }

    void process (float* L, float* R, int n)
    {
        float* ch[2] = { L, R };
        for (int bi = 0; bi < 4; ++bi)
        {
            auto& b = bands[bi];
            b.active = pb (b.on);
            b.sg.setTarget (pf (b.gain));
            b.sf.setTarget (std::log (pf (b.freq)));
            b.sq.setTarget (std::log (pf (b.q)));
        }
        for (int start = 0; start < n; start += kSub)
        {
            const int m = std::min (kSub, n - start);
            for (int bi = 0; bi < 4; ++bi)
            {
                auto& b = bands[bi];
                const float g = b.sg.next(), f = std::exp (b.sf.next()), q = std::exp (b.sq.next());
                if (! b.active || std::abs (g) < 0.01f) { b.bypass = true; continue; }
                b.bypass = false;
                const int t = pi (b.type);
                if (g != b.lastG || f != b.lastF || q != b.lastQ || t != b.lastT)
                {
                    b.lastG = g; b.lastF = f; b.lastQ = q; b.lastT = t;
                    computeBand (bi, t, g, f, q);
                }
                for (int c = 0; c < 2; ++c)
                {
                    float* x = ch[c] + start;
                    auto& f0 = b.f[c][0]; auto& f1 = b.f[c][1];
                    if (b.stages == 2) for (int i = 0; i < m; ++i) x[i] = f1.process (f0.process (x[i]));
                    else               for (int i = 0; i < m; ++i) x[i] = f0.process (x[i]);
                }
            }
        }
    }

private:
    struct Band
    {
        std::atomic<float> *on{}, *gain{}, *freq{}, *type{}, *q{};
        Smooth sg, sf, sq;
        Biquad f[2][2];
        int stages = 1, lastT = -1;
        float lastG = -999, lastF = -999, lastQ = -999;
        bool active = false, bypass = true;
    };

    void computeBand (int bi, int t, float g, float f, float q)
    {
        auto& b = bands[bi];
        b.stages = 1;
        const float ag = std::min (std::abs (g), 18.0f) / 18.0f;
        for (int c = 0; c < 2; ++c)
        {
            Biquad& x = b.f[c][0]; Biquad& y = b.f[c][1];
            if (bi == 0 || bi == 3) // shelves
            {
                if (t == 2) x.setPeak (sr, f, q, g);
                else
                {
                    const double sq = (t == 0) ? 1.55 : 0.7071; // RES: shelf com sobressinal na transicao
                    if (bi == 0) x.setHighShelf (sr, f, sq, g); else x.setLowShelf (sr, f, sq, g);
                }
            }
            else if (bi == 1) // MID: limpo, Q proporcional ao ganho
            {
                if (t == 0)      x.setPeak (sr, f, 0.55 + 0.35 * ag, g);
                else if (t == 1) x.setPeak (sr, f, 1.0 + 1.0 * ag, g);
                else             x.setPeak (sr, f, q, g);
            }
            else // TONE: no modo WIDE e "espalhado" (dois sinos escalonados)
            {
                if (t == 0)
                {
                    b.stages = 2;
                    x.setPeak (sr, f / 1.35, 0.85, g * 0.5);
                    y.setPeak (sr, f * 1.35, 0.85, g * 0.5);
                }
                else if (t == 1) x.setPeak (sr, f, 0.8 + 0.8 * ag, g);
                else             x.setPeak (sr, f, q, g);
            }
        }
    }

    double sr = 44100.0;
    Band bands[4];
};

//==============================================================================
// Utilitario de modo estereo para modulos dinamicos
//==============================================================================
struct StereoScope
{
    float* L; float* R; float* sL; float* sR; int n; bool ms;
    StereoScope (float* l, float* r, float* scl, float* scr, int num, bool isMs, bool encodeSc)
        : L (l), R (r), sL (scl), sR (scr), n (num), ms (isMs)
    {
        if (ms) { msEncode (L, R, n); if (encodeSc && sL && sR) msEncode (sL, sR, n); }
    }
    ~StereoScope() { if (ms) msDecode (L, R, n); }
};

//==============================================================================
// DS2: dois de-essers de qualquer frequencia (EQ dinamico por subtracao de banda)
//==============================================================================
class Ds2Module
{
public:
    std::atomic<float> gr[2] { 0.0f, 0.0f };
    float blockGr = 0.0f; // GR do ultimo bloco (thread de audio)

    void bind (APVTS& s)
    {
        mode = rawParam (s, "ds_mode"); ext = rawParam (s, "ds_ext"); out = rawParam (s, "ds_out");
        for (int i = 0; i < 2; ++i)
        {
            const juce::String k = "ds" + juce::String (i + 1);
            sec[i].on     = rawParam (s, (k + "_on").toRawUTF8());
            sec[i].type   = rawParam (s, (k + "_type").toRawUTF8());
            sec[i].freq   = rawParam (s, (k + "_freq").toRawUTF8());
            sec[i].thr    = rawParam (s, (k + "_thr").toRawUTF8());
            sec[i].listen = rawParam (s, (k + "_listen").toRawUTF8());
        }
    }
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        envAtt = timeCoef (0.4f, sr); envRel = timeCoef (50.0f, sr);
        grAtt  = timeCoef (1.0f, sr); grRel  = timeCoef (90.0f, sr);
        outS.reset (sr, 0.02); outS.setCurrentAndTargetValue (dbToGain (pf (out)));
        for (auto& s : sec) { s.fS.setTime (sr / kSub, 25.0f); s.fS.init = false; }
        listenBuf.setSize (2, maxBlock, false, true, true);
        reset();
    }
    void reset()
    {
        for (auto& s : sec) { for (auto& f : s.main) f.reset(); for (auto& f : s.side) f.reset(); s.env[0] = s.env[1] = 0; s.g[0] = s.g[1] = 0; }
    }

    void process (float* L, float* R, float* sL, float* sR, int n, bool hasExt)
    {
        const bool useExt = hasExt && pb (ext);
        const int  md = pi (mode);
        StereoScope scope (L, R, sL, sR, n, md == 2, useExt);
        float* ch[2] = { L, R };
        float* sc[2] = { sL, sR };
        bool anyListen = false;
        blockGr = 0.0f;
        listenBuf.clear (0, n);

        for (int si = 0; si < 2; ++si)
        {
            auto& s = sec[si];
            if (! pb (s.on)) continue;
            const int t = pi (s.type);
            const float thr = pf (s.thr);
            const bool lis = pb (s.listen);
            anyListen |= lis;
            s.fS.setTarget (std::log (pf (s.freq)));
            const double q = (t == 1) ? 1.0 : (t == 2 ? 4.0 : 0.7071);
            float grMax = 0.0f;

            for (int start = 0; start < n; start += kSub)
            {
                const int m = std::min (kSub, n - start);
                const float f = std::exp (s.fS.next());
                for (int c = 0; c < 2; ++c) { s.main[c].set (sr, f, q); s.side[c].set (sr, f, q); }

                for (int i = start; i < start + m; ++i)
                {
                    float comp[2], det[2];
                    for (int c = 0; c < 2; ++c)
                    {
                        float lp, bp, hp;
                        s.main[c].tick (ch[c][i], lp, bp, hp);
                        comp[c] = select (t, lp, bp, hp, s.main[c].k);
                        if (useExt)
                        {
                            s.side[c].tick (sc[c][i], lp, bp, hp);
                            det[c] = select (t, lp, bp, hp, s.side[c].k);
                        }
                        else det[c] = comp[c];

                        const float a = std::abs (det[c]);
                        s.env[c] = a > s.env[c] ? a + envAtt * (s.env[c] - a) : a + envRel * (s.env[c] - a);
                    }
                    float lv[2] = { s.env[0], s.env[1] };
                    if (md == 0) lv[0] = lv[1] = std::max (s.env[0], s.env[1]);
                    for (int c = 0; c < 2; ++c)
                    {
                        const float target = std::min (20.0f, kneeGR (gainToDb (lv[c]) - thr, 0.8f, 6.0f));
                        s.g[c] = target > s.g[c] ? target + grAtt * (s.g[c] - target) : target + grRel * (s.g[c] - target);
                        const float gain = dbToGain (-s.g[c]);
                        ch[c][i] -= comp[c] * (1.0f - gain);
                        if (lis) listenBuf.getWritePointer (c)[i] += det[c];
                    }
                    grMax = std::max (grMax, std::max (s.g[0], s.g[1]));
                }
            }
            atomicMax (gr[si], grMax);
            blockGr = std::max (blockGr, grMax);
        }

        if (anyListen)
            for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::copy (ch[c], listenBuf.getReadPointer (c), n);

        outS.setTargetValue (dbToGain (pf (out)));
        for (int i = 0; i < n; ++i) { const float g = outS.getNextValue(); L[i] *= g; R[i] *= g; }
    }

private:
    static inline float select (int t, float lp, float bp, float hp, float k)
    {
        switch (t) { case 0: return lp; case 1: case 2: return k * bp; default: return hp; }
    }
    struct Section
    {
        std::atomic<float> *on{}, *type{}, *freq{}, *thr{}, *listen{};
        SVF main[2], side[2];
        float env[2] { 0, 0 }, g[2] { 0, 0 };
        Smooth fS;
    };
    std::atomic<float> *mode{}, *ext{}, *out{};
    Section sec[2];
    double sr = 44100.0;
    float envAtt = 0, envRel = 0, grAtt = 0, grRel = 0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outS;
    juce::AudioBuffer<float> listenBuf;
};

//==============================================================================
// COMP: VCA / FET / OPT / SOFT
//==============================================================================
class CompModule
{
public:
    std::atomic<float> gr { 0.0f };
    float blockGr = 0.0f;

    void bind (APVTS& s)
    {
        mode = rawParam (s, "cmp_mode"); ext = rawParam (s, "cmp_ext"); type = rawParam (s, "cmp_type");
        thr = rawParam (s, "cmp_thr"); ratio = rawParam (s, "cmp_ratio"); att = rawParam (s, "cmp_att");
        rel = rawParam (s, "cmp_rel"); mix = rawParam (s, "cmp_mix"); out = rawParam (s, "cmp_out");
        scf = rawParam (s, "cmp_scf"); listen = rawParam (s, "cmp_listen");
        lsOn = rawParam (s, "cmp_ls_on"); lsGain = rawParam (s, "cmp_ls_gain"); lsFreq = rawParam (s, "cmp_ls_freq");
        bOn = rawParam (s, "cmp_bell_on"); bGain = rawParam (s, "cmp_bell_gain"); bFreq = rawParam (s, "cmp_bell_freq");
        hOn = rawParam (s, "cmp_hp_on"); hFreq = rawParam (s, "cmp_hp_freq");
    }
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        dry.setSize (2, maxBlock, false, true, true);
        side.setSize (2, maxBlock, false, true, true);
        mixS.reset (sr, 0.03); outS.reset (sr, 0.03);
        mixS.setCurrentAndTargetValue (pf (mix) * 0.01f);
        outS.setCurrentAndTargetValue (dbToGain (pf (out)));
        rmsOpt = timeCoef (12.0f, sr); rmsSoft = timeCoef (4.0f, sr); memC = timeCoef (1500.0f, sr);
        for (auto& d : dcb) d.prepare (sr);
        reset();
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            g[c] = 0; ms[c] = 0; fb[c] = 1; mem[c] = 0;
            ls[c].reset(); bell[c].reset(); hpf[c].reset(); dcb[c].reset();
        }
    }

    void process (float* L, float* R, float* sL, float* sR, int n, bool hasExt)
    {
        const bool useExt = hasExt && pb (ext);
        const int  md = pi (mode);
        // copia do sidechain (interno = entrada do modulo)
        side.copyFrom (0, 0, useExt ? sL : L, n);
        side.copyFrom (1, 0, useExt ? sR : R, n);
        float* s0 = side.getWritePointer (0); float* s1 = side.getWritePointer (1);

        StereoScope scope (L, R, s0, s1, n, md == 2, true);
        float* ch[2] = { L, R };
        float* sc[2] = { s0, s1 };

        // filtros de sidechain
        if (pb (scf))
        {
            const bool dl = pb (lsOn), db = pb (bOn), dh = pb (hOn);
            for (int c = 0; c < 2; ++c)
            {
                if (dl) ls[c].setLowShelf (sr, pf (lsFreq), 0.7071, pf (lsGain));
                if (db) bell[c].setPeak (sr, pf (bFreq), 1.0, pf (bGain));
                if (dh) hpf[c].setHighPass (sr, pf (hFreq), 0.7071);
                for (int i = 0; i < n; ++i)
                {
                    float v = sc[c][i];
                    if (dh) v = hpf[c].process (v);
                    if (dl) v = ls[c].process (v);
                    if (db) v = bell[c].process (v);
                    sc[c][i] = v;
                }
            }
        }

        if (pb (listen))
        {
            for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::copy (ch[c], sc[c], n);
            return;
        }

        dry.copyFrom (0, 0, L, n); dry.copyFrom (1, 0, R, n);

        const int   t  = pi (type);
        const float T  = pf (thr);
        const float r  = pf (ratio);
        const bool  inf = r >= 99.5f;
        const float slope = inf ? 1.0f : 1.0f - 1.0f / r;
        const float fbSlope = inf ? 30.0f : std::min (r - 1.0f, 30.0f);
        const float knee = t == 0 ? 2.0f : (t == 1 ? 4.0f : (t == 2 ? 8.0f : 18.0f));
        float a = pf (att), rl = pf (rel);
        if (t == 1) a *= 1.25f;
        if (t == 2) { a = std::max (a * 2.5f, 10.0f); }
        const float cA = timeCoef (a, sr), cR = timeCoef (rl, sr), cR3 = timeCoef (rl * 3.0f, sr);
        float grMax = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float d[2] = { std::abs (sc[0][i]), std::abs (sc[1][i]) };
            if (t == 1) { d[0] *= fb[0]; d[1] *= fb[1]; } // FET: feedback (detecta a saida)
            if (md == 0) d[0] = d[1] = std::max (d[0], d[1]);

            for (int c = 0; c < 2; ++c)
            {
                float lvl;
                if (t == 2)      { ms[c] = d[c] * d[c] + rmsOpt  * (ms[c] - d[c] * d[c]); lvl = std::sqrt (ms[c]) * 1.41f; }
                else if (t == 3) { ms[c] = d[c] * d[c] + rmsSoft * (ms[c] - d[c] * d[c]); lvl = std::sqrt (ms[c]) * 1.41f; }
                else lvl = d[c];

                const float over = gainToDb (lvl) - T;
                const float target = std::min (60.0f, t == 1 ? kneeGR (over, fbSlope, knee) : kneeGR (over, slope, knee));
                float rc = cR;
                if (t == 2) // OPT: memoria longa -> release mais lento apos compressao sustentada
                {
                    mem[c] = (g[c] > 2.0f ? 1.0f : 0.0f) + memC * (mem[c] - (g[c] > 2.0f ? 1.0f : 0.0f));
                    rc = cR + (cR3 - cR) * mem[c];
                }
                g[c] = target > g[c] ? target + cA * (g[c] - target) : target + rc * (g[c] - target);
                fb[c] = dbToGain (-g[c]);
            }

            for (int c = 0; c < 2; ++c)
            {
                float y = ch[c][i] * fb[c];
                if (t == 1 || t == 2) // cor harmonica (FET/OPT colorem mesmo sem compressao)
                {
                    const float a2 = t == 1 ? 0.025f : 0.04f;
                    const float a3 = t == 1 ? 0.035f + 0.004f * std::min (g[c], 20.0f) : 0.015f;
                    const float yc = std::clamp (y, -1.25f, 1.25f);
                    y = y + a2 * yc * yc - a3 * yc * yc * yc;
                    y = dcb[c].process (y);
                }
                ch[c][i] = y;
            }
            grMax = std::max (grMax, std::max (g[0], g[1]));
        }
        atomicMax (gr, grMax);
        blockGr = grMax;

        // mix paralelo e ganho de saida no dominio L/R: decodifica aqui e desativa o destrutor
        if (scope.ms) { msDecode (L, R, n); scope.ms = false; }
        mixS.setTargetValue (pf (mix) * 0.01f);
        outS.setTargetValue (dbToGain (pf (out)));
        const float* d0 = dry.getReadPointer (0); const float* d1 = dry.getReadPointer (1);
        // o dry foi copiado no dominio M/S quando md==2 -> decodifica a copia
        if (md == 2) msDecode (dry.getWritePointer (0), dry.getWritePointer (1), n);
        for (int i = 0; i < n; ++i)
        {
            const float mx = mixS.getNextValue(), og = outS.getNextValue();
            L[i] = (d0[i] + mx * (L[i] - d0[i])) * og;
            R[i] = (d1[i] + mx * (R[i] - d1[i])) * og;
        }
    }

private:
    std::atomic<float> *mode{}, *ext{}, *type{}, *thr{}, *ratio{}, *att{}, *rel{}, *mix{}, *out{}, *scf{}, *listen{};
    std::atomic<float> *lsOn{}, *lsGain{}, *lsFreq{}, *bOn{}, *bGain{}, *bFreq{}, *hOn{}, *hFreq{};
    double sr = 44100.0;
    juce::AudioBuffer<float> dry, side;
    juce::SmoothedValue<float> mixS;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outS;
    float g[2] { 0, 0 }, ms[2] { 0, 0 }, fb[2] { 1, 1 }, mem[2] { 0, 0 };
    float rmsOpt = 0, rmsSoft = 0, memC = 0;
    Biquad ls[2], bell[2], hpf[2];
    DCBlock dcb[2];
};

//==============================================================================
// GATE / EXPANDER
//==============================================================================
class GateModule
{
public:
    std::atomic<float> gr { 0.0f };
    float blockGr = 0.0f;

    void bind (APVTS& s)
    {
        mode = rawParam (s, "gt_mode"); ext = rawParam (s, "gt_ext"); type = rawParam (s, "gt_type");
        thr = rawParam (s, "gt_thr"); floorP = rawParam (s, "gt_floor"); closeP = rawParam (s, "gt_close");
        att = rawParam (s, "gt_att"); rel = rawParam (s, "gt_rel"); scf = rawParam (s, "gt_scf");
        hp = rawParam (s, "gt_hp"); lp = rawParam (s, "gt_lp"); listen = rawParam (s, "gt_listen");
    }
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        side.setSize (2, maxBlock, false, true, true);
        detRel = timeCoef (8.0f, sr);
        reset();
    }
    void reset()
    {
        for (int c = 0; c < 2; ++c) { env[c] = 0; gDb[c] = 0; open[c] = true; fh[c].reset(); fl[c].reset(); }
    }

    void process (float* L, float* R, float* sL, float* sR, int n, bool hasExt)
    {
        const float T = pf (thr);
        const bool useExt = hasExt && pb (ext);
        const int md = pi (mode);
        side.copyFrom (0, 0, useExt ? sL : L, n);
        side.copyFrom (1, 0, useExt ? sR : R, n);
        float* sc[2] = { side.getWritePointer (0), side.getWritePointer (1) };
        StereoScope scope (L, R, sc[0], sc[1], n, md == 2, true);
        float* ch[2] = { L, R };

        if (pb (scf))
        {
            for (int c = 0; c < 2; ++c)
            {
                fh[c].setHighPass (sr, pf (hp), 0.7071);
                fl[c].setLowPass (sr, pf (lp), 0.7071);
                for (int i = 0; i < n; ++i) sc[c][i] = fl[c].process (fh[c].process (sc[c][i]));
            }
        }
        if (pb (listen))
        {
            for (int c = 0; c < 2; ++c) juce::FloatVectorOperations::copy (ch[c], sc[c], n);
            return;
        }
        if (T <= -79.9f) { for (auto& v : gDb) v = 0; return; } // -inf = sempre aberto

        const bool isExp = pi (type) == 1;
        const float fl0 = pf (floorP);
        const float cl = pf (closeP);
        const float closeT = (cl < -0.05f && cl < T) ? cl : T - 3.0f; // histerese
        const float cA = timeCoef (pf (att), sr), cR = timeCoef (pf (rel), sr);
        float grMax = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            float d[2];
            for (int c = 0; c < 2; ++c)
            {
                const float a = std::abs (sc[c][i]);
                env[c] = a > env[c] ? a : a + detRel * (env[c] - a);
                d[c] = env[c];
            }
            if (md == 0) d[0] = d[1] = std::max (d[0], d[1]);
            for (int c = 0; c < 2; ++c)
            {
                const float lv = gainToDb (d[c]);
                float target;
                if (! isExp)
                {
                    if (open[c]) { if (lv < closeT) open[c] = false; }
                    else if (lv > T) open[c] = true;
                    target = open[c] ? 0.0f : fl0;
                }
                else
                {
                    const float over = lv - T;
                    target = over >= 0.0f ? 0.0f : std::max (fl0, over * 1.5f);
                }
                gDb[c] = target > gDb[c] ? target + cA * (gDb[c] - target) : target + cR * (gDb[c] - target);
                ch[c][i] *= dbToGain (gDb[c]);
            }
            grMax = std::max (grMax, -std::min (gDb[0], gDb[1]));
        }
        atomicMax (gr, grMax);
        blockGr = grMax;
    }

private:
    std::atomic<float> *mode{}, *ext{}, *type{}, *thr{}, *floorP{}, *closeP{}, *att{}, *rel{}, *scf{}, *hp{}, *lp{}, *listen{};
    double sr = 44100.0;
    juce::AudioBuffer<float> side;
    float env[2] { 0, 0 }, gDb[2] { 0, 0 }, detRel = 0;
    bool open[2] { true, true };
    Biquad fh[2], fl[2];
};

//==============================================================================
// LIMITER (master): estilo "brickwall" zero latencia - distorce se for empurrado
//==============================================================================
class Limiter
{
public:
    std::atomic<float> gr { 0.0f };
    float blockGr = 0.0f;
    void prepare (double sampleRate) { sr = sampleRate; relC = timeCoef (60.0f, sr); env = 1.0f; driveS.reset (sr, 0.03); }
    void process (float* L, float* R, int n, float thrDb)
    {
        constexpr float ceil = 0.9886f; // -0.1 dBFS
        driveS.setTargetValue (dbToGain (-thrDb));
        float grMax = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float dr = driveS.getNextValue();
            float l = L[i] * dr, r = R[i] * dr;
            const float pk = std::max (std::abs (l), std::abs (r));
            const float target = pk > ceil ? ceil / pk : 1.0f;
            env = target < env ? target : target + relC * (env - target);
            l *= env; r *= env;
            L[i] = std::clamp (l, -ceil, ceil);
            R[i] = std::clamp (r, -ceil, ceil);
            grMax = std::max (grMax, -gainToDb (env));
        }
        atomicMax (gr, grMax);
        blockGr = grMax;
    }
    void reset() { env = 1.0f; }
private:
    double sr = 44100.0; float relC = 0, env = 1.0f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveS;
};
} // namespace sc
