#include "InsertHost.h"

namespace sc
{
InsertHost::InsertHost()
{
   #if JUCE_PLUGINHOST_VST3
    formats.addFormat (new juce::VST3PluginFormat());
   #endif
}

InsertHost::~InsertHost()
{
    const juce::ScopedLock sl (lock);
    instance.reset();
}

juce::String InsertHost::getName() const
{
    return instance != nullptr ? instance->getName() : juce::String();
}

void InsertHost::configure (juce::AudioPluginInstance& inst)
{
    // tenta estereo; se nao der, mantem o layout padrao do plugin
    juce::AudioProcessor::BusesLayout st;
    st.inputBuses.add (juce::AudioChannelSet::stereo());
    st.outputBuses.add (juce::AudioChannelSet::stereo());
    if (! inst.setBusesLayout (st))
        inst.enableAllBuses();
    inst.setRateAndBufferSizeDetails (sr, block);
    inst.prepareToPlay (sr, block);
}

bool InsertHost::loadFromFile (const juce::File& file, juce::String& error)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (formats.getNumFormats() == 0) { error = "Hospedagem VST3 indisponivel nesta compilacao."; return false; }

    juce::OwnedArray<juce::PluginDescription> types;
    for (int i = 0; i < formats.getNumFormats(); ++i)
        formats.getFormat (i)->findAllTypesForFile (types, file.getFullPathName());

    if (types.isEmpty()) { error = "Nenhum plugin VST3 encontrado em:\n" + file.getFullPathName(); return false; }

    auto inst = formats.createPluginInstance (*types[0], sr, block, error);
    if (inst == nullptr) { if (error.isEmpty()) error = "Falha ao abrir o plugin."; return false; }

    configure (*inst);

    sendSynchronousChangeMessage(); // fecha janelas de editor abertas do plugin anterior
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (lock);
        old = std::move (instance);
        instance = std::move (inst);
        pluginPath = file.getFullPathName();
        const int chans = juce::jmax (2, instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels());
        work.setSize (chans, block, false, true, true);
    }
    if (old) old->releaseResources();
    old.reset();
    sendChangeMessage();
    return true;
}

void InsertHost::unload()
{
    JUCE_ASSERT_MESSAGE_THREAD
    sendSynchronousChangeMessage();
    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (lock);
        old = std::move (instance);
        pluginPath.clear();
    }
    if (old) old->releaseResources();
    old.reset();
    sendChangeMessage();
}

void InsertHost::prepare (double sampleRate, int maxBlock)
{
    const juce::ScopedLock sl (lock);
    sr = sampleRate; block = maxBlock; prepared = true;
    if (instance)
    {
        instance->releaseResources();
        configure (*instance);
    }
    const int chans = instance ? juce::jmax (2, instance->getTotalNumInputChannels(), instance->getTotalNumOutputChannels()) : 2;
    work.setSize (chans, block, false, true, true);
}

void InsertHost::release()
{
    const juce::ScopedLock sl (lock);
    if (instance) instance->releaseResources();
}

void InsertHost::process (float* L, float* R, int n)
{
    const juce::ScopedTryLock sl (lock);
    if (! sl.isLocked() || instance == nullptr || n > work.getNumSamples()) return;

    const int ins  = instance->getTotalNumInputChannels();
    const int outs = instance->getTotalNumOutputChannels();
    const int chans = juce::jmax (ins, outs);
    if (chans > work.getNumChannels()) return;

    juce::AudioBuffer<float> buf (work.getArrayOfWritePointers(), chans, n);
    buf.clear();
    if (ins >= 1) buf.copyFrom (0, 0, L, n);
    if (ins >= 2) buf.copyFrom (1, 0, R, n);
    if (ins == 1) { buf.addFrom (0, 0, R, n); buf.applyGain (0, 0, n, 0.5f); }

    midi.clear();
    instance->processBlock (buf, midi);

    if (outs >= 2) { juce::FloatVectorOperations::copy (L, buf.getReadPointer (0), n); juce::FloatVectorOperations::copy (R, buf.getReadPointer (1), n); }
    else if (outs == 1) { juce::FloatVectorOperations::copy (L, buf.getReadPointer (0), n); juce::FloatVectorOperations::copy (R, buf.getReadPointer (0), n); }
}

int InsertHost::getLatency() const
{
    return instance ? instance->getLatencySamples() : 0;
}

std::unique_ptr<juce::XmlElement> InsertHost::saveState() const
{
    auto xml = std::make_unique<juce::XmlElement> ("INSERT");
    xml->setAttribute ("path", pluginPath);
    if (instance)
    {
        juce::MemoryBlock mb;
        instance->getStateInformation (mb);
        xml->setAttribute ("state", mb.toBase64Encoding());
    }
    return xml;
}

void InsertHost::restoreState (const juce::XmlElement& xml)
{
    const auto path = xml.getStringAttribute ("path");
    if (path.isEmpty()) { if (instance) unload(); return; }
    juce::String err;
    if (path != pluginPath || instance == nullptr)
        if (! loadFromFile (juce::File (path), err)) return;
    juce::MemoryBlock mb;
    if (mb.fromBase64Encoding (xml.getStringAttribute ("state")) && mb.getSize() > 0 && instance)
        instance->setStateInformation (mb.getData(), (int) mb.getSize());
}
} // namespace sc
