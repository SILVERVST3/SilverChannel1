#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace sc
{
class PresetManager
{
public:
    struct Factory
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
        juce::String order; // vazio = ordem padrao
    };

    PresetManager (juce::AudioProcessorValueTreeState& s, juce::UndoManager& u) : apvts (s), undo (u) {}

    static const std::vector<Factory>& factory();
    static juce::File presetDir();
    static juce::File defaultFile();

    void resetToDefaults();
    void loadFactory (int index);
    bool saveUser (const juce::String& name);
    juce::Array<juce::File> userPresets() const;
    bool loadFile (const juce::File& f);
    bool deleteFile (const juce::File& f);
    bool setAsDefault();
    bool loadDefaultIfExists();

    juce::String currentName { "Default" };

private:
    void setParam (const juce::String& id, float realValue);
    void applyXml (const juce::XmlElement& xml);
    std::unique_ptr<juce::XmlElement> makeXml() const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::UndoManager& undo;
};
} // namespace sc
