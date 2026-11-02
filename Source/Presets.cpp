#include "Presets.h"
#include "Params.h"

namespace sc
{
const std::vector<PresetManager::Factory>& PresetManager::factory()
{
    // Valores em unidades reais (Hz, dB, ms, %). Escolhas = indice.
    // sat_type: 0 ODD 1 EVEN 2 HEAVY 3 CRUSH | slope: 0=6 1=12 2=18 3=24R
    // eq *_type: shelves 0 RES 1 SHELF 2 PARA ; mid/tone 0 WIDE 1 NARROW 2 PARA
    // ds*_type: 0 LO SHF 1 BELL 2 NOTCH 3 HI SHF | cmp_type 0 VCA 1 FET 2 OPT 3 SOFT
    static const std::vector<Factory> list = {
        { "Default", {}, {} },

        { "Vocal - Pop Presence", {
            { "pre_on", 1 }, { "sat_type", 0 }, { "sat_amt", 25 }, { "hp_on", 1 }, { "hp_freq", 90 }, { "hp_slope", 1 },
            { "ds_on", 1 }, { "ds1_type", 3 }, { "ds1_freq", 6500 }, { "ds1_thr", -26 },
            { "eq_on", 1 }, { "eq_hi_type", 0 }, { "eq_hi_freq", 10000 }, { "eq_hi_gain", 3 },
            { "eq_mid_freq", 3000 }, { "eq_mid_gain", 2 }, { "eq_tone_freq", 350 }, { "eq_tone_gain", -2 },
            { "cmp_on", 1 }, { "cmp_type", 2 }, { "cmp_thr", -20 }, { "cmp_ratio", 3 }, { "cmp_att", 10 }, { "cmp_rel", 300 } }, {} },

        { "Vocal - Rock Grit", {
            { "pre_on", 1 }, { "sat_type", 2 }, { "sat_amt", 45 }, { "hp_on", 1 }, { "hp_freq", 120 }, { "hp_slope", 1 },
            { "ds_on", 1 }, { "ds1_type", 3 }, { "ds1_freq", 6000 }, { "ds1_thr", -24 },
            { "eq_on", 1 }, { "eq_mid_type", 1 }, { "eq_mid_freq", 2000 }, { "eq_mid_gain", 3 },
            { "eq_tone_freq", 800 }, { "eq_tone_gain", 2 },
            { "cmp_on", 1 }, { "cmp_type", 1 }, { "cmp_thr", -24 }, { "cmp_ratio", 6 }, { "cmp_att", 3 }, { "cmp_rel", 120 } }, {} },

        { "Kick - Thump & Click", {
            { "pre_on", 1 }, { "sat_type", 1 }, { "sat_amt", 30 }, { "thump_on", 1 }, { "thump_amt", 1 },
            { "gt_on", 1 }, { "gt_thr", -35 }, { "gt_floor", -20 }, { "gt_att", 0.5f }, { "gt_rel", 150 },
            { "eq_on", 1 }, { "eq_lo_type", 2 }, { "eq_lo_freq", 60 }, { "eq_lo_gain", 3 }, { "eq_lo_q", 1.5f },
            { "eq_tone_type", 1 }, { "eq_tone_freq", 350 }, { "eq_tone_gain", -4 },
            { "eq_hi_type", 0 }, { "eq_hi_freq", 5000 }, { "eq_hi_gain", 3 },
            { "cmp_on", 1 }, { "cmp_type", 0 }, { "cmp_thr", -14 }, { "cmp_ratio", 4 }, { "cmp_att", 15 }, { "cmp_rel", 100 } }, {} },

        { "Snare - Crack", {
            { "pre_on", 1 }, { "sat_type", 0 }, { "sat_amt", 20 }, { "hp_on", 1 }, { "hp_freq", 90 }, { "hp_slope", 2 },
            { "gt_on", 1 }, { "gt_thr", -30 }, { "gt_floor", -18 }, { "gt_rel", 200 }, { "gt_scf", 1 }, { "gt_hp", 150 },
            { "eq_on", 1 }, { "eq_tone_freq", 220 }, { "eq_tone_gain", 3 },
            { "eq_mid_type", 1 }, { "eq_mid_freq", 4500 }, { "eq_mid_gain", 4 }, { "eq_lo_freq", 80 }, { "eq_lo_gain", -2 },
            { "cmp_on", 1 }, { "cmp_type", 1 }, { "cmp_thr", -18 }, { "cmp_ratio", 4 }, { "cmp_att", 8 }, { "cmp_rel", 120 } }, {} },

        { "Drum Bus - Crush Parallel", {
            { "pre_on", 1 }, { "sat_type", 3 }, { "sat_amt", 40 }, { "thump_on", 1 },
            { "eq_on", 1 }, { "eq_hi_type", 0 }, { "eq_hi_freq", 12000 }, { "eq_hi_gain", 2 },
            { "eq_lo_type", 0 }, { "eq_lo_freq", 70 }, { "eq_lo_gain", 2 },
            { "cmp_on", 1 }, { "cmp_type", 1 }, { "cmp_thr", -30 }, { "cmp_ratio", 20 }, { "cmp_att", 5 }, { "cmp_rel", 80 }, { "cmp_mix", 35 } }, {} },

        { "Bass - Warm DI", {
            { "pre_on", 1 }, { "sat_type", 1 }, { "sat_amt", 35 }, { "hp_on", 1 }, { "hp_freq", 35 }, { "hp_slope", 3 }, { "flt_res", 1.2f },
            { "lp_on", 1 }, { "lp_freq", 8000 }, { "lp_slope", 1 },
            { "eq_on", 1 }, { "eq_lo_freq", 80 }, { "eq_lo_gain", 2 }, { "eq_tone_freq", 250 }, { "eq_tone_gain", -3 },
            { "eq_mid_freq", 900 }, { "eq_mid_gain", 2 },
            { "cmp_on", 1 }, { "cmp_type", 2 }, { "cmp_thr", -22 }, { "cmp_ratio", 4 }, { "cmp_att", 20 }, { "cmp_rel", 400 } }, {} },

        { "Acoustic Guitar - Air", {
            { "pre_on", 1 }, { "sat_on", 0 }, { "hp_on", 1 }, { "hp_freq", 100 }, { "hp_slope", 1 },
            { "ds_on", 1 }, { "ds1_on", 0 }, { "ds2_on", 1 }, { "ds2_type", 1 }, { "ds2_freq", 220 }, { "ds2_thr", -24 },
            { "eq_on", 1 }, { "eq_hi_type", 0 }, { "eq_hi_freq", 9000 }, { "eq_hi_gain", 3 },
            { "eq_tone_freq", 250 }, { "eq_tone_gain", -3 }, { "eq_mid_freq", 3000 }, { "eq_mid_gain", 1.5f },
            { "cmp_on", 1 }, { "cmp_type", 3 }, { "cmp_thr", -20 }, { "cmp_ratio", 3 }, { "cmp_att", 15 }, { "cmp_rel", 250 } }, {} },

        { "Electric Guitar - Weight", {
            { "pre_on", 1 }, { "sat_type", 2 }, { "sat_amt", 20 }, { "hp_on", 1 }, { "hp_freq", 70 }, { "hp_slope", 3 }, { "flt_res", 1.4f },
            { "lp_on", 1 }, { "lp_freq", 9000 }, { "lp_slope", 1 },
            { "eq_on", 1 }, { "eq_tone_freq", 150 }, { "eq_tone_gain", 2 },
            { "eq_mid_type", 1 }, { "eq_mid_freq", 2500 }, { "eq_mid_gain", -3 },
            { "cmp_on", 1 }, { "cmp_type", 2 }, { "cmp_thr", -16 }, { "cmp_ratio", 2.5f } }, {} },

        { "Piano - Clean Glue", {
            { "eq_on", 1 }, { "eq_hi_freq", 10000 }, { "eq_hi_gain", 2 }, { "eq_tone_freq", 300 }, { "eq_tone_gain", -1.5f },
            { "cmp_on", 1 }, { "cmp_type", 3 }, { "cmp_thr", -18 }, { "cmp_ratio", 2 }, { "cmp_att", 20 }, { "cmp_rel", 300 } }, {} },

        { "Mix Bus - Opto Glue", {
            { "pre_on", 1 }, { "sat_type", 0 }, { "sat_amt", 10 },
            { "eq_on", 1 }, { "eq_hi_freq", 12000 }, { "eq_hi_gain", 1 }, { "eq_lo_freq", 60 }, { "eq_lo_gain", 1 },
            { "cmp_on", 1 }, { "cmp_type", 2 }, { "cmp_thr", -16 }, { "cmp_ratio", 2 }, { "cmp_att", 30 }, { "cmp_rel", 600 },
            { "lim_on", 1 }, { "lim_thr", -2 } }, {} },

        { "Room Mics - Smash", {
            { "pre_on", 1 }, { "sat_type", 2 }, { "sat_amt", 50 },
            { "gt_on", 1 }, { "gt_type", 1 }, { "gt_thr", -40 }, { "gt_floor", -15 },
            { "eq_on", 1 }, { "eq_hi_type", 0 }, { "eq_hi_freq", 8000 }, { "eq_hi_gain", -3 },
            { "cmp_on", 1 }, { "cmp_type", 1 }, { "cmp_thr", -35 }, { "cmp_ratio", 100 }, { "cmp_att", 20 }, { "cmp_rel", 60 } }, {} },

        { "Synth - Resonant Sweep", {
            { "pre_on", 1 }, { "sat_type", 0 }, { "sat_amt", 40 }, { "lp_on", 1 }, { "lp_freq", 1200 }, { "lp_slope", 3 }, { "flt_res", 3.0f } }, {} },

        { "Master - M/S De-Ess", {
            { "ds_on", 1 }, { "ds_mode", 2 }, { "ds1_type", 3 }, { "ds1_freq", 7000 }, { "ds1_thr", -30 },
            { "cmp_on", 1 }, { "cmp_type", 3 }, { "cmp_thr", -14 }, { "cmp_ratio", 1.8f }, { "cmp_att", 30 }, { "cmp_rel", 400 },
            { "lim_on", 1 }, { "lim_thr", -1.5f } }, "0,2,3,4,1,5" },
    };
    return list;
}

juce::File PresetManager::presetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                 .getChildFile ("SilverChannel 01").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::File PresetManager::defaultFile()
{
    return presetDir().getParentDirectory().getChildFile ("Default.scpreset");
}

void PresetManager::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void PresetManager::resetToDefaults()
{
    undo.beginNewTransaction ("Reset");
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    apvts.state.setProperty ("order", defaultOrderString(), &undo);
    currentName = "Default";
}

void PresetManager::loadFactory (int index)
{
    const auto& list = factory();
    if (index < 0 || index >= (int) list.size()) return;
    resetToDefaults();
    for (auto& [id, v] : list[(size_t) index].values) setParam (id, v);
    if (list[(size_t) index].order.isNotEmpty())
        apvts.state.setProperty ("order", list[(size_t) index].order, &undo);
    currentName = list[(size_t) index].name;
}

std::unique_ptr<juce::XmlElement> PresetManager::makeXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("SilverChannelPreset");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("order", apvts.state.getProperty ("order", defaultOrderString()).toString());
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* e = xml->createNewChildElement ("PARAM");
            e->setAttribute ("id", rp->paramID);
            e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }
    return xml;
}

void PresetManager::applyXml (const juce::XmlElement& xml)
{
    undo.beginNewTransaction ("Load preset");
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto* e : xml.getChildWithTagNameIterator ("PARAM"))
        setParam (e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("value"));
    apvts.state.setProperty ("order", xml.getStringAttribute ("order", defaultOrderString()), &undo);
}

bool PresetManager::saveUser (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto xml = makeXml();
    xml->setAttribute ("name", name.trim());
    if (! xml->writeTo (presetDir().getChildFile (clean + ".scpreset"))) return false;
    currentName = name.trim();
    return true;
}

juce::Array<juce::File> PresetManager::userPresets() const
{
    auto files = presetDir().findChildFiles (juce::File::findFiles, false, "*.scpreset");
    files.sort();
    return files;
}

bool PresetManager::loadFile (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("SilverChannelPreset")) return false;
    applyXml (*xml);
    currentName = xml->getStringAttribute ("name", f.getFileNameWithoutExtension());
    return true;
}

bool PresetManager::deleteFile (const juce::File& f) { return f.deleteFile(); }

bool PresetManager::setAsDefault()
{
    auto xml = makeXml();
    xml->setAttribute ("name", "Default");
    return xml->writeTo (defaultFile());
}

bool PresetManager::loadDefaultIfExists()
{
    const auto f = defaultFile();
    if (! f.existsAsFile()) return false;
    const bool ok = loadFile (f);
    undo.clearUndoHistory();
    currentName = "Default";
    return ok;
}
} // namespace sc
