#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace sc
{
//==============================================================================
// INSERT: hospeda um plugin VST3 externo dentro da cadeia do SilverChannel
//==============================================================================
class InsertHost : public juce::ChangeBroadcaster
{
public:
    InsertHost();
    ~InsertHost() override;

    // thread de mensagens
    bool loadFromFile (const juce::File& file, juce::String& error);
    void unload();
    bool isLoaded() const { return instance != nullptr; }
    juce::String getName() const;
    juce::String getPath() const { return pluginPath; }
    juce::AudioPluginInstance* getInstance() { return instance.get(); }

    void prepare (double sampleRate, int maxBlock);
    void release();

    // thread de audio
    void process (float* L, float* R, int n);

    // estado
    std::unique_ptr<juce::XmlElement> saveState() const;
    void restoreState (const juce::XmlElement& xml);

    int getLatency() const;

private:
    void configure (juce::AudioPluginInstance& inst);

    juce::AudioPluginFormatManager formats;
    std::unique_ptr<juce::AudioPluginInstance> instance;
    juce::String pluginPath;
    juce::CriticalSection lock;
    juce::AudioBuffer<float> work;
    juce::MidiBuffer midi;
    double sr = 44100.0;
    int block = 512;
    bool prepared = false;
};
} // namespace sc
