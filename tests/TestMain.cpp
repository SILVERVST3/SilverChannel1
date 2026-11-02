// Teste offline do SilverChannel 01 (somente para desenvolvimento/CI no Linux)
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>

static void setP (SilverChannelProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    if (prm == nullptr) { std::printf ("PARAM INEXISTENTE: %s\n", id); return; }
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static double rmsDb (const juce::AudioBuffer<float>& b, int ch, int from = 0)
{
    double s = 0; const int n = b.getNumSamples() - from;
    for (int i = from; i < b.getNumSamples(); ++i) s += (double) b.getSample (ch, i) * b.getSample (ch, i);
    return 10.0 * std::log10 (s / n + 1e-20);
}

static double harmonicDb (const juce::AudioBuffer<float>& b, double f, double sr, int from)
{
    double re = 0, im = 0; const int n = b.getNumSamples() - from;
    for (int i = from; i < b.getNumSamples(); ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (2 * juce::MathConstants<double>::pi * (i - from) / (n - 1));
        re += b.getSample (0, i) * w * std::cos (2 * juce::MathConstants<double>::pi * f * i / sr);
        im += b.getSample (0, i) * w * std::sin (2 * juce::MathConstants<double>::pi * f * i / sr);
    }
    return 20.0 * std::log10 (std::sqrt (re * re + im * im) / (n * 0.25) + 1e-12);
}

static bool finite (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (! std::isfinite (b.getSample (c, i)) || std::abs (b.getSample (c, i)) > 20.0f) return false;
    return true;
}

static juce::AudioBuffer<float> run (SilverChannelProcessor& p, int kind, double sr, int seconds = 2, float ampDb = -12.0f)
{
    const int N = (int) sr * seconds, bs = 480;
    juce::AudioBuffer<float> out (2, N);
    juce::Random rnd (42);
    const float a = juce::Decibels::decibelsToGain (ampDb);
    for (int i = 0; i < N; ++i)
    {
        float l, r;
        if (kind == 0) { l = r = a * (float) std::sin (2 * juce::MathConstants<double>::pi * 1000.0 * i / sr); }
        else if (kind == 1) { l = a * (rnd.nextFloat() * 2 - 1); r = a * (rnd.nextFloat() * 2 - 1); }
        else
        {   // "bateria": rajadas de ruido decaindo a cada 250 ms + sibilancia
            const int ph = i % (int) (sr * 0.25);
            const float env = std::exp (-ph / (float) (sr * 0.03));
            l = r = a * 2.0f * env * (rnd.nextFloat() * 2 - 1) + 0.003f * (rnd.nextFloat() * 2 - 1);
        }
        out.setSample (0, i, l); out.setSample (1, i, r);
    }
    juce::AudioBuffer<float> full (4, bs);
    juce::MidiBuffer midi;
    for (int s = 0; s < N; s += bs)
    {
        const int n = std::min (bs, N - s);
        juce::AudioBuffer<float> blk (full.getArrayOfWritePointers(), 4, n);
        blk.clear();
        blk.copyFrom (0, 0, out, 0, s, n); blk.copyFrom (1, 0, out, 1, s, n);
        p.processBlock (blk, midi);
        out.copyFrom (0, s, blk, 0, 0, n); out.copyFrom (1, s, blk, 1, 0, n);
    }
    return out;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double sr = 48000.0;
    int fails = 0;
    auto make = [&]
    {
        auto p = std::make_unique<SilverChannelProcessor>();
        auto layout = p->getBusesLayout();
        layout.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();
        p->setBusesLayout (layout);
        p->prepareToPlay (sr, 480);
        return p;
    };

    { auto p = make(); std::printf ("Latencia reportada: %d amostras\n", p->getLatencySamples());
      auto o = run (*p, 0, sr); std::printf ("Bypass (todos off): RMS %.2f dB (esperado ~ -15.05) | VU %.2f (esperado ~ +3)\n", rmsDb (o, 0, 4800), p->vuLevel[0].load()); }

    // ---- PRE: harmonicos por tipo ----
    const char* satNames[] = { "ODD", "EVEN", "HEAVY", "CRUSH" };
    for (int t = 0; t < 4; ++t)
    {
        auto p = make();
        setP (*p, "pre_on", 1); setP (*p, "sat_type", (float) t); setP (*p, "sat_amt", 60);
        auto o = run (*p, 0, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN em PRE %s\n", satNames[t]); ++fails; }
        const double h1 = harmonicDb (o, 1000, sr, 9600), h2 = harmonicDb (o, 2000, sr, 9600), h3 = harmonicDb (o, 3000, sr, 9600);
        std::printf ("PRE %-5s 60%%: RMS %6.2f dB | H2 %6.1f dBc | H3 %6.1f dBc\n", satNames[t], rmsDb (o, 0, 4800), h2 - h1, h3 - h1);
    }
    // ---- PRE: filtros + thump ----
    for (int sl = 0; sl < 4; ++sl)
    {
        auto p = make();
        setP (*p, "pre_on", 1); setP (*p, "sat_on", 0); setP (*p, "hp_on", 1); setP (*p, "hp_freq", 2000); setP (*p, "hp_slope", (float) sl);
        setP (*p, "lp_on", 1); setP (*p, "lp_freq", 400); setP (*p, "lp_slope", (float) sl); setP (*p, "flt_res", 3.0f);
        setP (*p, "thump_on", 1);
        auto o = run (*p, 1, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN em filtros slope %d\n", sl); ++fails; }
        std::printf ("PRE filtros slope %d (res 3): RMS ruido %6.2f dB\n", sl, rmsDb (o, 0, 4800));
    }

    // ---- EQ ----
    {
        auto p = make();
        setP (*p, "eq_on", 1);
        setP (*p, "eq_mid_gain", 12); setP (*p, "eq_mid_freq", 1000);
        auto o = run (*p, 0, sr);
        std::printf ("EQ MID WIDE +12 @1k: senoide 1k RMS %6.2f dB (esperado ~ -3)\n", rmsDb (o, 0, 4800));
        setP (*p, "eq_mid_gain", 0); setP (*p, "eq_tone_gain", 12); setP (*p, "eq_tone_freq", 1000);
        o = run (*p, 0, sr);
        std::printf ("EQ TONE WIDE +12 @1k (espalhado): RMS %6.2f dB\n", rmsDb (o, 0, 4800));
        for (int t = 0; t < 3; ++t)
        {
            setP (*p, "eq_tone_gain", 0); setP (*p, "eq_hi_gain", 10); setP (*p, "eq_hi_freq", 1000); setP (*p, "eq_hi_type", (float) t);
            o = run (*p, 1, sr);
            if (! finite (o)) { std::printf ("FALHA EQ\n"); ++fails; }
            std::printf ("EQ HIGH tipo %d +10 @1k: RMS ruido %6.2f dB\n", t, rmsDb (o, 0, 4800));
        }
    }

    // ---- COMP ----
    const char* cn[] = { "VCA", "FET", "OPT", "SOFT" };
    for (int t = 0; t < 4; ++t)
    {
        auto p = make();
        setP (*p, "cmp_on", 1); setP (*p, "cmp_type", (float) t); setP (*p, "cmp_thr", -30); setP (*p, "cmp_ratio", 4);
        auto o = run (*p, 0, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN COMP %s\n", cn[t]); ++fails; }
        std::printf ("COMP %-4s thr -30 4:1 senoide -12: RMS %6.2f dB, GR %.1f dB\n", cn[t], rmsDb (o, 0, 24000), p->comp.gr.load());
        p->comp.gr.store (0);
        setP (*p, "cmp_ratio", 100); setP (*p, "cmp_mode", 2); setP (*p, "cmp_mix", 50);
        o = run (*p, 2, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN COMP %s inf/MS\n", cn[t]); ++fails; }
    }
    // ---- COMP sidechain externo ----
    {
        auto p = make();
        setP (*p, "cmp_on", 1); setP (*p, "cmp_ext", 1); setP (*p, "cmp_thr", -30);
        auto o = run (*p, 0, sr); // sidechain silencioso -> sem compressao
        std::printf ("COMP EXT SC silencioso: RMS %6.2f dB (esperado ~ -15.05)\n", rmsDb (o, 0, 4800));
    }

    // ---- GATE ----
    for (int t = 0; t < 2; ++t)
    {
        auto p = make();
        setP (*p, "gt_on", 1); setP (*p, "gt_type", (float) t); setP (*p, "gt_thr", -30); setP (*p, "gt_floor", -40);
        setP (*p, "gt_scf", 1); setP (*p, "gt_hp", 200);
        auto o = run (*p, 2, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN GATE\n"); ++fails; }
        std::printf ("GATE %s: RMS %6.2f dB, atenuacao max %.1f dB\n", t ? "EXP " : "GATE", rmsDb (o, 0, 4800), p->gate.gr.load());
    }

    // ---- DS2 ----
    {
        auto p = make();
        setP (*p, "ds_on", 1); setP (*p, "ds1_type", 3); setP (*p, "ds1_freq", 5000); setP (*p, "ds1_thr", -40);
        setP (*p, "ds2_on", 1); setP (*p, "ds2_type", 1); setP (*p, "ds2_freq", 300); setP (*p, "ds2_thr", -40);
        auto o = run (*p, 1, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN DS2\n"); ++fails; }
        std::printf ("DS2 ruido: RMS %6.2f dB, GR1 %.1f dB, GR2 %.1f dB\n", rmsDb (o, 0, 4800), p->ds2.gr[0].load(), p->ds2.gr[1].load());
        setP (*p, "ds1_type", 2); setP (*p, "ds_mode", 2); setP (*p, "ds1_listen", 1);
        o = run (*p, 1, sr);
        if (! finite (o)) { std::printf ("FALHA: NaN DS2 listen\n"); ++fails; }
    }

    // ---- LIMITER ----
    {
        auto p = make();
        setP (*p, "lim_on", 1); setP (*p, "lim_thr", -20);
        auto o = run (*p, 1, sr);
        float pk = o.getMagnitude (0, o.getNumSamples());
        std::printf ("LIMITER -20: pico de saida %.4f (<= 0.9886)\n", pk);
        if (pk > 0.99f) ++fails;
    }

    // ---- Presets de fabrica + ordem ----
    {
        auto p = make();
        const auto& f = sc::PresetManager::factory();
        for (int i = 0; i < (int) f.size(); ++i)
        {
            p->presets.loadFactory (i);
            for (int k = 0; k < 3; ++k) p->moveModule (k, +1);
            auto o = run (*p, 2, sr, 1);
            if (! finite (o)) { std::printf ("FALHA preset %s\n", f[(size_t) i].name.toRawUTF8()); ++fails; }
            std::printf ("Preset %-28s RMS %6.2f dB\n", f[(size_t) i].name.toRawUTF8(), rmsDb (o, 0, 4800));
        }
        // estado: salva / restaura
        juce::MemoryBlock mb; p->getStateInformation (mb);
        auto q = make(); q->setStateInformation (mb.getData(), (int) mb.getSize());
        auto oa = p->getOrder(), ob = q->getOrder();
        std::printf ("Estado restaurado, ordem igual: %s\n", oa == ob ? "sim" : "NAO");
        if (oa != ob) ++fails;
        // undo
        p->undo.beginNewTransaction(); p->moveModule (sc::modEq, -1);
        auto before = p->getOrder(); p->undo.undo(); auto after = p->getOrder();
        std::printf ("Undo de reordenacao: %s\n", before != after ? "ok" : "FALHOU");
    }

    // ---- INSERT: hospeda um VST3 (usa o proprio build como teste) ----
    if (argc > 3)
    {
        auto p = make();
        juce::String err;
        const bool ok = p->insert.loadFromFile (juce::File (juce::String (argv[3])), err);
        std::printf ("INSERT carregou VST3: %s %s (latencia total %d)\n", ok ? "sim" : "NAO", err.toRawUTF8(), p->getLatencySamples());
        if (ok)
        {
            auto* inst = p->insert.getInstance();
            if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (inst->getParameters()[0])) (void) prm;
            auto o = run (*p, 0, sr);
            std::printf ("INSERT processando: RMS %.2f dB, finito %s\n", rmsDb (o, 0, 4800), finite (o) ? "sim" : "NAO");
            juce::MemoryBlock mb; p->getStateInformation (mb);
            auto q = make(); q->setStateInformation (mb.getData(), (int) mb.getSize());
            std::printf ("INSERT restaurado do estado: %s\n", q->insert.isLoaded() ? "sim" : "NAO");
            p->insert.unload();
        }
    }

    // ---- Snapshot da GUI ----
    if (argc > 1)
    {
        auto p = make();
        p->presets.loadFactory (1);
        run (*p, 2, sr, 1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        ed->setSize (SilverChannelEditor::baseW, SilverChannelEditor::baseH);
        for (int i = 0; i < 20; ++i) { run (*p, 2, sr, 1); juce::MessageManager::getInstance()->runDispatchLoopUntil (40); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out { juce::String (argv[1]) };
        out.deleteFile();
        juce::FileOutputStream fos (out);
        juce::PNGImageFormat().writeImageToStream (img, fos);
        std::printf ("Snapshot salvo: %s\n", argv[1]);
        if (argc > 2)
        {
            juce::Component holder;
            holder.setSize (SilverChannelEditor::baseW, SilverChannelEditor::baseH);
            SilverLookAndFeel lf; holder.setLookAndFeel (&lf);
            auto bgImg = img;
            int idx = 0;
            for (int m : { sc::modComp, sc::modGate, sc::modDs2 })
            {
                ui::ExpandedView ev (*p, m);
                holder.addAndMakeVisible (ev);
                ev.setBounds (holder.getLocalBounds());
                auto shot = ev.createComponentSnapshot (ev.getLocalBounds(), true, 1.0f);
                juce::Image composed (juce::Image::ARGB, shot.getWidth(), shot.getHeight(), true);
                { juce::Graphics g (composed); g.drawImageAt (bgImg, 0, 0); g.drawImageAt (shot, 0, 0); }
                juce::File o2 { juce::String (argv[2]) + juce::String (idx++) + ".png" };
                o2.deleteFile(); juce::FileOutputStream fo (o2); juce::PNGImageFormat().writeImageToStream (composed, fo);
                holder.removeChildComponent (&ev);
            }
            holder.setLookAndFeel (nullptr);
        }
    }

    std::printf ("\nFALHAS: %d\n", fails);
    return fails == 0 ? 0 : 1;
}
