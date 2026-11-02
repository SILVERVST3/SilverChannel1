#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Params.h"
#include "Modules.h"
#include "InsertHost.h"
#include "Presets.h"

class SilverChannelProcessor : public juce::AudioProcessor,
                               private juce::ValueTree::Listener,
                               private juce::ChangeListener
{
public:
    SilverChannelProcessor();
    ~SilverChannelProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SilverChannel 01"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- ordem dos modulos ----
    std::array<int, sc::numModules> getOrder() const;
    void moveModule (int moduleId, int direction);

    // ---- estado publico ----
    juce::UndoManager undo { 30000, 32 };
    juce::AudioProcessorValueTreeState apvts;
    sc::InsertHost insert;
    sc::PresetManager presets { apvts, undo };

    // medidores (lidos pela GUI com exchange(0))
    std::atomic<float> inPeak[2] { 0.0f, 0.0f }, outPeak[2] { 0.0f, 0.0f };
    std::atomic<float> vuLevel[2] { 0.0f, 0.0f };
    std::atomic<float> grPre { 0.0f };
    std::atomic<bool>  isStereo { true }, hasSidechain { false };
    sc::GateModule gate;
    sc::Ds2Module  ds2;
    sc::CompModule comp;
    sc::Limiter    limiter;

    float uiScale = 1.0f;

private:
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override { refreshOrder(); }
    void refreshOrder();
    void changeListenerCallback (juce::ChangeBroadcaster*) override { updateLatency(); }
    void updateLatency();
    void processChunk (float* L, float* R, float* sL, float* sR, int n, bool ext);

    sc::PreModule pre;
    sc::EqModule  eq;
    std::array<std::atomic<int>, sc::numModules> order;

    std::atomic<float> *preOn{}, *eqOn{}, *dsOn{}, *cmpOn{}, *gtOn{}, *insOn{};
    std::atomic<float> *inGain{}, *outGain{}, *phaseInv{}, *limOn{}, *limThr{}, *vuSrc{}, *vuTrim{}, *monitor{};

    juce::AudioBuffer<float> work, scWork;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inS, outS;
    double sr = 44100.0;
    int maxBlock = 512;
    float vuState[2] { 0.0f, 0.0f }, vuCoef = 0.0f;
    int lastInsertLatency = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SilverChannelProcessor)
};
