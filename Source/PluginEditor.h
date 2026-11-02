#pragma once
#include "PluginProcessor.h"
#include "Panels.h"

//==============================================================================
// Editor principal: barra superior + faixa de modulos + MASTER
//==============================================================================
class SilverChannelEditor : public juce::AudioProcessorEditor,
                            private juce::ValueTree::Listener,
                            private juce::Timer
{
public:
    static constexpr int baseW = 1290, baseH = 668;

    explicit SilverChannelEditor (SilverChannelProcessor&);
    ~SilverChannelEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    class Main : public juce::Component
    {
    public:
        explicit Main (SilverChannelEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        SilverChannelEditor& ed;
    };

    void layoutModules();
    void showLoadMenu();
    void showSaveMenu();
    void showExpanded (int moduleId);
    void timerCallback() override;
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override { layoutModules(); }

    SilverChannelProcessor& proc;
    SilverLookAndFeel lnf;
    Main main { *this };
    juce::TooltipWindow tooltips { nullptr, 600 };

    std::array<std::unique_ptr<ui::ModulePanel>, sc::numModules> panels;
    std::unique_ptr<ui::MasterPanel> master;
    std::unique_ptr<ui::ExpandedView> expanded;

    juce::TextButton loadBtn { "LOAD" }, saveBtn { "SAVE" }, undoBtn { "UNDO" }, redoBtn { "REDO" }, presetLbl;
    juce::ComboBox sizeBox;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String shownPreset;
};
