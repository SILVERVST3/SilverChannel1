#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sc;

SilverChannelProcessor::SilverChannelProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "SilverChannel01", createLayout())
{
    pre.bind (apvts); eq.bind (apvts); ds2.bind (apvts); comp.bind (apvts); gate.bind (apvts);

    preOn = rawParam (apvts, "pre_on"); eqOn = rawParam (apvts, "eq_on"); dsOn = rawParam (apvts, "ds_on");
    cmpOn = rawParam (apvts, "cmp_on"); gtOn = rawParam (apvts, "gt_on"); insOn = rawParam (apvts, "insert_on");
    inGain = rawParam (apvts, "in_gain"); outGain = rawParam (apvts, "out_gain"); phaseInv = rawParam (apvts, "phase_inv");
    limOn = rawParam (apvts, "lim_on"); limThr = rawParam (apvts, "lim_thr");
    vuSrc = rawParam (apvts, "vu_src"); vuTrim = rawParam (apvts, "vu_trim"); monitor = rawParam (apvts, "monitor");

    apvts.state.setProperty ("order", defaultOrderString(), nullptr);
    apvts.state.addListener (this);
    refreshOrder();
    insert.addChangeListener (this);

   #if ! SC_TEST_BUILD
    presets.loadDefaultIfExists();
   #endif
}

SilverChannelProcessor::~SilverChannelProcessor()
{
    insert.removeChangeListener (this);
    apvts.state.removeListener (this);
}

//==============================================================================
bool SilverChannelProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void SilverChannelProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (16, samplesPerBlock);
    work.setSize (2, maxBlock, false, true, true);
    scWork.setSize (2, maxBlock, false, true, true);

    pre.prepare (sr, maxBlock);
    eq.prepare (sr, maxBlock);
    ds2.prepare (sr, maxBlock);
    comp.prepare (sr, maxBlock);
    gate.prepare (sr, maxBlock);
    limiter.prepare (sr);
    insert.prepare (sr, maxBlock);

    inS.reset (sr, 0.03);  inS.setCurrentAndTargetValue (dbToGain (pf (inGain)));
    outS.reset (sr, 0.03); outS.setCurrentAndTargetValue (dbToGain (pf (outGain)));
    vuCoef = timeCoef (300.0f / 2.2f, sr); // integracao estilo VU (~300 ms ate 99%)
    vuState[0] = vuState[1] = 0.0f;
    updateLatency();
}

void SilverChannelProcessor::releaseResources()
{
    insert.release();
}

void SilverChannelProcessor::updateLatency()
{
    const int l = pre.getLatency() + insert.getLatency();
    setLatencySamples (l);
}

//==============================================================================
std::array<int, numModules> SilverChannelProcessor::getOrder() const
{
    std::array<int, numModules> o {};
    for (int i = 0; i < numModules; ++i) o[(size_t) i] = order[(size_t) i].load();
    return o;
}

void SilverChannelProcessor::refreshOrder()
{
    auto tokens = juce::StringArray::fromTokens (apvts.state.getProperty ("order", defaultOrderString()).toString(), ",", "");
    std::array<int, numModules> o {};
    std::array<bool, numModules> used {};
    int k = 0;
    for (auto& t : tokens)
    {
        const int m = t.trim().getIntValue();
        if (k < numModules && m >= 0 && m < numModules && ! used[(size_t) m]) { o[(size_t) k++] = m; used[(size_t) m] = true; }
    }
    for (int m = 0; m < numModules; ++m) if (! used[(size_t) m] && k < numModules) o[(size_t) k++] = m;
    for (int i = 0; i < numModules; ++i) order[(size_t) i].store (o[(size_t) i]);
}

void SilverChannelProcessor::moveModule (int moduleId, int direction)
{
    auto o = getOrder();
    int pos = -1;
    for (int i = 0; i < numModules; ++i) if (o[(size_t) i] == moduleId) pos = i;
    const int np = pos + direction;
    if (pos < 0 || np < 0 || np >= numModules) return;
    std::swap (o[(size_t) pos], o[(size_t) np]);
    juce::StringArray s;
    for (auto m : o) s.add (juce::String (m));
    undo.beginNewTransaction ("Move module");
    apvts.state.setProperty ("order", s.joinIntoString (","), &undo);
}

void SilverChannelProcessor::valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier& id)
{
    if (t == apvts.state && id.toString() == "order") refreshOrder();
}

//==============================================================================
void SilverChannelProcessor::processChunk (float* L, float* R, float* sL, float* sR, int n, bool ext)
{
    // ---- entrada: ganho + inversao de fase ----
    inS.setTargetValue (dbToGain (pf (inGain)));
    const float ph = pb (phaseInv) ? -1.0f : 1.0f;
    float pk0 = 0, pk1 = 0;
    for (int i = 0; i < n; ++i)
    {
        const float g = inS.getNextValue() * ph;
        L[i] *= g; R[i] *= g;
        pk0 = std::max (pk0, std::abs (L[i])); pk1 = std::max (pk1, std::abs (R[i]));
    }
    atomicMax (inPeak[0], pk0); atomicMax (inPeak[1], pk1);

    // VU de entrada (calculado antes dos modulos)
    const int vs = pi (vuSrc);
    auto runVu = [this, n] (const float* a, const float* b)
    {
        for (int i = 0; i < n; ++i)
        {
            vuState[0] = a[i] * a[i] + vuCoef * (vuState[0] - a[i] * a[i]);
            vuState[1] = b[i] * b[i] + vuCoef * (vuState[1] - b[i] * b[i]);
        }
    };
    if (vs == 0) runVu (L, R);

    // ---- modulos na ordem escolhida ----
    gate.blockGr = ds2.blockGr = comp.blockGr = 0.0f;
    for (int i = 0; i < numModules; ++i)
    {
        switch (order[(size_t) i].load (std::memory_order_relaxed))
        {
            case modPre:    if (pb (preOn)) pre.process (L, R, n); else pre.processDelayOnly (L, R, n); break;
            case modGate:   if (pb (gtOn))  gate.process (L, R, sL, sR, n, ext); break;
            case modDs2:    if (pb (dsOn))  ds2.process (L, R, sL, sR, n, ext); break;
            case modEq:     if (pb (eqOn))  eq.process (L, R, n); break;
            case modComp:   if (pb (cmpOn)) comp.process (L, R, sL, sR, n, ext); break;
            case modInsert: if (pb (insOn)) insert.process (L, R, n); break;
            default: break;
        }
    }

    // ---- saida: fader + limiter + monitor ----
    outS.setTargetValue (dbToGain (pf (outGain)));
    for (int i = 0; i < n; ++i) { const float g = outS.getNextValue(); L[i] *= g; R[i] *= g; }
    limiter.blockGr = 0.0f;
    if (pb (limOn)) limiter.process (L, R, n, pf (limThr));

    const int mon = pi (monitor);
    if (mon != 0)
    {
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i], v;
            switch (mon)
            {
                case 1: v = 0.5f * (l + r); L[i] = R[i] = v; break;   // MONO
                case 2: R[i] = l; break;                               // L
                case 3: L[i] = r; break;                               // R
                case 4: v = 0.5f * (l + r); L[i] = R[i] = v; break;   // M
                case 5: v = 0.5f * (l - r); L[i] = R[i] = v; break;   // S
                default: break;
            }
        }
    }

    pk0 = pk1 = 0;
    for (int i = 0; i < n; ++i) { pk0 = std::max (pk0, std::abs (L[i])); pk1 = std::max (pk1, std::abs (R[i])); }
    atomicMax (outPeak[0], pk0); atomicMax (outPeak[1], pk1);

    // ---- VU ----
    const float trim = pf (vuTrim);
    if (vs == 1) runVu (L, R);
    if (vs == 2)
    {
        const float tot = gate.blockGr + ds2.blockGr + comp.blockGr + limiter.blockGr;
        vuLevel[0].store (-tot); vuLevel[1].store (-tot);
    }
    else
    {
        vuLevel[0].store (gainToDb (std::sqrt (vuState[0])) + trim);
        vuLevel[1].store (gainToDb (std::sqrt (vuState[1])) + trim);
    }
}

void SilverChannelProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    auto mainIn = getBusBuffer (buffer, true, 0);
    const int mainCh = mainIn.getNumChannels();
    if (mainCh == 0 || total == 0) return;
    isStereo.store (mainCh > 1);

    juce::AudioBuffer<float> scBus;
    bool ext = false;
    if (getBusCount (true) > 1)
    {
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled())
        {
            scBus = getBusBuffer (buffer, true, 1);
            ext = scBus.getNumChannels() > 0;
        }
    }
    hasSidechain.store (ext);

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        float* L = work.getWritePointer (0);
        float* R = work.getWritePointer (1);
        juce::FloatVectorOperations::copy (L, mainIn.getReadPointer (0, start), n);
        juce::FloatVectorOperations::copy (R, mainIn.getReadPointer (mainCh > 1 ? 1 : 0, start), n);

        float* sL = scWork.getWritePointer (0);
        float* sR = scWork.getWritePointer (1);
        if (ext)
        {
            juce::FloatVectorOperations::copy (sL, scBus.getReadPointer (0, start), n);
            juce::FloatVectorOperations::copy (sR, scBus.getReadPointer (scBus.getNumChannels() > 1 ? 1 : 0, start), n);
        }
        else { juce::FloatVectorOperations::clear (sL, n); juce::FloatVectorOperations::clear (sR, n); }

        processChunk (L, R, sL, sR, n, ext);

        if (mainCh > 1)
        {
            juce::FloatVectorOperations::copy (buffer.getWritePointer (0, start), L, n);
            juce::FloatVectorOperations::copy (buffer.getWritePointer (1, start), R, n);
        }
        else
        {
            auto* o = buffer.getWritePointer (0, start);
            for (int i = 0; i < n; ++i) o[i] = 0.5f * (L[i] + R[i]);
        }
    }
    for (int c = mainCh; c < getTotalNumOutputChannels(); ++c)
        if (c < buffer.getNumChannels()) buffer.clear (c, 0, total);
}

//==============================================================================
void SilverChannelProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    xml->setAttribute ("uiScale", (double) uiScale);
    xml->setAttribute ("presetName", presets.currentName);
    xml->addChildElement (insert.saveState().release());
    copyXmlToBinary (*xml, destData);
}

void SilverChannelProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    std::unique_ptr<juce::XmlElement> ins;
    if (auto* e = xml->getChildByName ("INSERT"))
    {
        ins = std::make_unique<juce::XmlElement> (*e);
        xml->removeChildElement (e, true);
    }
    uiScale = (float) xml->getDoubleAttribute ("uiScale", 1.0);
    presets.currentName = xml->getStringAttribute ("presetName", "Default");
    xml->removeAttribute ("uiScale");
    xml->removeAttribute ("presetName");

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.hasProperty ("order")) tree.setProperty ("order", defaultOrderString(), nullptr);
    apvts.replaceState (tree);
    refreshOrder();

    if (ins != nullptr)
    {
        auto restore = [this, x = std::shared_ptr<juce::XmlElement> (std::move (ins))]
        {
            insert.restoreState (*x);
            updateLatency();
        };
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) restore();
        else juce::MessageManager::callAsync (restore);
    }
}

juce::AudioProcessorEditor* SilverChannelProcessor::createEditor()
{
    return new SilverChannelEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SilverChannelProcessor();
}
