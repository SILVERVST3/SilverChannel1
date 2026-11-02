#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

//==============================================================================
// SilverChannel 01 - definicao de parametros
// Layout de modulos inspirado no Scheps Omni Channel 2:
// PRE | GATE | DS2 | EQ | COMP | INSERT  ->  MASTER (entrada/saida, VU, limiter)
//==============================================================================

namespace sc
{
enum ModuleId { modPre = 0, modGate, modDs2, modEq, modComp, modInsert, numModules };

inline const char* moduleName (int m)
{
    static const char* names[] = { "PRE", "GATE", "DS2", "EQ", "COMP", "INSERT" };
    return (m >= 0 && m < numModules) ? names[m] : "?";
}

inline juce::String defaultOrderString() { return "0,1,2,3,4,5"; }

// Escolhas
inline juce::StringArray stereoModes()  { return { "ST", "DUO", "M/S" }; }
inline juce::StringArray satTypes()     { return { "ODD", "EVEN", "HEAVY", "CRUSH" }; }
inline juce::StringArray slopes()       { return { "6", "12", "18", "24R" }; }
inline juce::StringArray thumpAmts()    { return { "+2", "+4" }; }
inline juce::StringArray shelfTypes()   { return { "RES", "SHELF", "PARA" }; }
inline juce::StringArray bellTypes()    { return { "WIDE", "NARROW", "PARA" }; }
inline juce::StringArray dsTypes()      { return { "LO SHF", "BELL", "NOTCH", "HI SHF" }; }
inline juce::StringArray compTypes()    { return { "VCA", "FET", "OPT", "SOFT" }; }
inline juce::StringArray gateTypes()    { return { "GATE", "EXP" }; }
inline juce::StringArray vuSources()    { return { "IN", "OUT", "GR" }; }
inline juce::StringArray monitorModes() { return { "ST", "MONO", "L", "R", "M", "S" }; }

//------------------------------------------------------------------------------
// Faixas e textos
inline juce::NormalisableRange<float> logRange (float mn, float mx)
{
    return juce::NormalisableRange<float> (mn, mx,
        [] (float start, float end, float n) { return start * std::pow (end / start, n); },
        [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); },
        [] (float start, float end, float v) { return juce::jlimit (start, end, v); });
}

inline juce::String freqText (float v, int)
{
    if (v < 1000.0f) return juce::String (v, v < 100.0f ? 1 : 0) + " Hz";
    return juce::String (v / 1000.0f, v < 10000.0f ? 2 : 1) + " kHz";
}
inline juce::String dbText (float v, int)
{
    return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
}
inline juce::String msText (float v, int)
{
    if (v < 10.0f)   return juce::String (v, 1) + " ms";
    if (v < 1000.0f) return juce::String (v, 0) + " ms";
    return juce::String (v / 1000.0f, 2) + " s";
}
inline juce::String pctText (float v, int)  { return juce::String (v, 0) + " %"; }
inline juce::String qText (float v, int)    { return "Q " + juce::String (v, 2); }
inline juce::String ratioText (float v, int){ return v >= 99.5f ? juce::String ("inf:1") : juce::String (v, v < 10.0f ? 1 : 0) + ":1"; }
inline juce::String gateThrText (float v, int) { return v <= -79.9f ? juce::String ("-inf") : dbText (v, 0); }
inline juce::String closeText (float v, int)   { return v >= -0.05f ? juce::String ("OFF") : gateThrText (v, 0); }

//------------------------------------------------------------------------------
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto fl = [&p] (const String& id, const String& name, NormalisableRange<float> r, float def,
                    std::function<String (float, int)> txt)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                        AudioParameterFloatAttributes().withStringFromValueFunction (txt)));
    };
    auto lin = [] (float a, float b, float step = 0.0f) { return NormalisableRange<float> (a, b, step); };
    auto bo  = [&p] (const String& id, const String& name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };
    auto ch  = [&p] (const String& id, const String& name, const StringArray& c, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def));
    };

    // ---------------- MASTER ----------------
    fl ("in_gain",  "Input Gain",  lin (-36.0f, 12.0f, 0.1f), 0.0f, dbText);
    fl ("out_gain", "Output Gain", lin (-36.0f, 12.0f, 0.1f), 0.0f, dbText);
    bo ("phase_inv", "Phase Invert", false);
    bo ("lim_on",  "Limiter On", false);
    fl ("lim_thr", "Limiter Threshold", lin (-30.0f, 0.0f, 0.1f), 0.0f, dbText);
    ch ("vu_src",  "VU Source", vuSources(), 1);
    fl ("vu_trim", "VU Trim", lin (4.0f, 26.0f, 0.5f), 18.0f, [] (float v, int) { return "-" + String (v, 1) + " dBFS"; });
    ch ("monitor", "Monitor", monitorModes(), 0);
    bo ("insert_on", "Insert On", true);

    // ---------------- PRE ----------------
    bo ("pre_on",   "Pre On", false);
    bo ("sat_on",   "Saturation On", true);
    fl ("sat_amt",  "Saturation", lin (0.0f, 100.0f, 0.1f), 30.0f, pctText);
    ch ("sat_type", "Saturation Type", satTypes(), 0);
    bo ("hp_on",    "HP On", false);
    fl ("hp_freq",  "HP Freq", logRange (20.0f, 20000.0f), 80.0f, freqText);
    ch ("hp_slope", "HP Slope", slopes(), 1);
    bo ("lp_on",    "LP On", false);
    fl ("lp_freq",  "LP Freq", logRange (20.0f, 20000.0f), 16000.0f, freqText);
    ch ("lp_slope", "LP Slope", slopes(), 1);
    fl ("flt_res",  "Filter Res", logRange (0.1f, 4.0f), 0.707f, [] (float v, int) { return String (v, 2); });
    bo ("thump_on", "Thump On", false);
    ch ("thump_amt","Thump Amount", thumpAmts(), 0);

    // ---------------- EQ ----------------
    bo ("eq_on", "EQ On", false);
    struct BandDef { const char* key; const char* name; float freq; int type; };
    const BandDef bands[] = { { "hi", "High", 8000.0f, 1 }, { "mid", "Mid", 2500.0f, 0 },
                              { "tone", "Tone", 400.0f, 0 }, { "lo", "Low", 90.0f, 1 } };
    for (auto& b : bands)
    {
        const String k = String ("eq_") + b.key;
        const bool shelf = (String (b.key) == "hi" || String (b.key) == "lo");
        bo (k + "_on",   String (b.name) + " On", true);
        fl (k + "_gain", String (b.name) + " Gain", lin (-18.0f, 18.0f, 0.1f), 0.0f, dbText);
        fl (k + "_freq", String (b.name) + " Freq", logRange (20.0f, 20000.0f), b.freq, freqText);
        ch (k + "_type", String (b.name) + " Type", shelf ? shelfTypes() : bellTypes(), b.type);
        fl (k + "_q",    String (b.name) + " Q", logRange (0.2f, 10.0f), 1.0f, qText);
    }

    // ---------------- DS2 ----------------
    bo ("ds_on",   "DS2 On", false);
    ch ("ds_mode", "DS2 Stereo Mode", stereoModes(), 0);
    bo ("ds_ext",  "DS2 Ext SC", false);
    fl ("ds_out",  "DS2 Output", lin (-12.0f, 12.0f, 0.1f), 0.0f, dbText);
    for (int i = 1; i <= 2; ++i)
    {
        const String k = "ds" + String (i);
        bo (k + "_on",     "DS" + String (i) + " On", i == 1);
        ch (k + "_type",   "DS" + String (i) + " Type", dsTypes(), i == 1 ? 3 : 1);
        fl (k + "_freq",   "DS" + String (i) + " Freq", logRange (20.0f, 20000.0f), i == 1 ? 6000.0f : 300.0f, freqText);
        fl (k + "_thr",    "DS" + String (i) + " Threshold", lin (-48.0f, 0.0f, 0.1f), -18.0f, dbText);
        bo (k + "_listen", "DS" + String (i) + " Listen", false);
    }

    // ---------------- COMP ----------------
    bo ("cmp_on",    "Comp On", false);
    ch ("cmp_mode",  "Comp Stereo Mode", stereoModes(), 0);
    bo ("cmp_ext",   "Comp Ext SC", false);
    ch ("cmp_type",  "Comp Type", compTypes(), 0);
    fl ("cmp_thr",   "Comp Threshold", lin (-50.0f, 0.0f, 0.1f), -12.0f, dbText);
    { auto r = NormalisableRange<float> (1.0f, 100.0f); r.setSkewForCentre (4.0f);
      fl ("cmp_ratio", "Comp Ratio", r, 4.0f, ratioText); }
    fl ("cmp_att",   "Comp Attack", logRange (0.4f, 150.0f), 10.0f, msText);
    fl ("cmp_rel",   "Comp Release", logRange (50.0f, 5000.0f), 200.0f, msText);
    fl ("cmp_mix",   "Comp Mix", lin (0.0f, 100.0f, 0.1f), 100.0f, pctText);
    fl ("cmp_out",   "Comp Output", lin (-12.0f, 12.0f, 0.1f), 0.0f, dbText);
    bo ("cmp_scf",   "Comp SC Filters", false);
    bo ("cmp_listen","Comp SC Listen", false);
    bo ("cmp_ls_on",   "Comp SC LowShelf On", true);
    fl ("cmp_ls_gain", "Comp SC LowShelf Gain", lin (-18.0f, 18.0f, 0.1f), 0.0f, dbText);
    fl ("cmp_ls_freq", "Comp SC LowShelf Freq", logRange (20.0f, 20000.0f), 120.0f, freqText);
    bo ("cmp_bell_on",   "Comp SC Bell On", true);
    fl ("cmp_bell_gain", "Comp SC Bell Gain", lin (-18.0f, 18.0f, 0.1f), 0.0f, dbText);
    fl ("cmp_bell_freq", "Comp SC Bell Freq", logRange (20.0f, 20000.0f), 1500.0f, freqText);
    bo ("cmp_hp_on",   "Comp SC HPF On", true);
    fl ("cmp_hp_freq", "Comp SC HPF Freq", logRange (20.0f, 20000.0f), 60.0f, freqText);

    // ---------------- GATE ----------------
    bo ("gt_on",    "Gate On", false);
    ch ("gt_mode",  "Gate Stereo Mode", stereoModes(), 0);
    bo ("gt_ext",   "Gate Ext SC", false);
    ch ("gt_type",  "Gate Type", gateTypes(), 0);
    fl ("gt_thr",   "Gate Threshold", lin (-80.0f, 0.0f, 0.1f), -40.0f, gateThrText);
    fl ("gt_floor", "Gate Floor", lin (-60.0f, 0.0f, 0.1f), -60.0f, dbText);
    fl ("gt_close", "Gate Close", lin (-80.0f, 0.0f, 0.1f), 0.0f, closeText);
    fl ("gt_att",   "Gate Attack", logRange (0.1f, 10000.0f), 1.0f, msText);
    fl ("gt_rel",   "Gate Release", logRange (0.1f, 10000.0f), 120.0f, msText);
    bo ("gt_scf",   "Gate SC Filters", false);
    fl ("gt_hp",    "Gate SC HP", logRange (20.0f, 15000.0f), 20.0f, freqText);
    fl ("gt_lp",    "Gate SC LP", logRange (20.0f, 15000.0f), 15000.0f, freqText);
    bo ("gt_link",  "Gate SC HP/LP Link", false);
    bo ("gt_listen","Gate SC Listen", false);

    return { p.begin(), p.end() };
}
} // namespace sc
