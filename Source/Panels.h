#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"

namespace ui
{
//==============================================================================
// Base de um modulo (coluna) — cabecalho com setas de reordenacao,
// nome (liga/desliga) e botao de expandir
//==============================================================================
class ModulePanel : public juce::Component
{
public:
    ModulePanel (SilverChannelProcessor& p, int moduleId, const juce::String& onParam, bool canExpand, bool canMove = true);
    void paint (juce::Graphics&) override;
    void resized() override;
    virtual void layoutContent (juce::Rectangle<int> area) = 0;
    virtual void paintContent (juce::Graphics&) {}
    virtual int preferredWidth() const = 0;

    std::function<void (int)> onExpand;
    const int moduleId;

protected:
    SilverChannelProcessor& proc;
    APVTS& s;
    std::unique_ptr<Toggle> nameBtn;
    juce::TextButton leftBtn { "<" }, rightBtn { ">" }, expBtn { "SC" };
    bool expandable, movable;
    juce::Rectangle<int> content;
};

//==============================================================================
class PrePanel : public ModulePanel
{
public:
    explicit PrePanel (SilverChannelProcessor&);
    void layoutContent (juce::Rectangle<int>) override;
    void paintContent (juce::Graphics&) override;
    int preferredWidth() const override { return 168; }
private:
    Toggle satOn, hpOn, lpOn, thumpOn;
    Knob satAmt, hpFreq, lpFreq, res;
    Radio satType;
    Cycle hpSlope, lpSlope, thumpAmt;
    juce::Rectangle<int> tSat, tFlt, tThump;
};

class GatePanel : public ModulePanel
{
public:
    explicit GatePanel (SilverChannelProcessor&);
    void layoutContent (juce::Rectangle<int>) override;
    int preferredWidth() const override { return 132; }
private:
    Radio type;
    Knob thr, floorK, closeK, att, rel;
    GRMeter meter;
    Toggle scf, ext;
};

class Ds2Panel : public ModulePanel
{
public:
    explicit Ds2Panel (SilverChannelProcessor&);
    void layoutContent (juce::Rectangle<int>) override;
    void paintContent (juce::Graphics&) override;
    int preferredWidth() const override { return 158; }
private:
    Toggle on1, on2, lis1, lis2, ext;
    Cycle type1, type2;
    Knob f1, t1, f2, t2, out;
    GRMeter gr1, gr2;
    juce::Rectangle<int> sep;
};

class EqPanel : public ModulePanel, private juce::Timer
{
public:
    explicit EqPanel (SilverChannelProcessor&);
    void layoutContent (juce::Rectangle<int>) override;
    void paintContent (juce::Graphics&) override;
    int preferredWidth() const override { return 214; }
private:
    void timerCallback() override;
    struct Band
    {
        Band (APVTS& s, const juce::String& key, const juce::String& title);
        Toggle on; Cycle type; Knob gain, freq, q;
        std::atomic<float>* typeParam;
    };
    std::unique_ptr<Band> bands[4];
    juce::Rectangle<int> rows[4];
};

class CompPanel : public ModulePanel
{
public:
    explicit CompPanel (SilverChannelProcessor&);
    void layoutContent (juce::Rectangle<int>) override;
    int preferredWidth() const override { return 168; }
private:
    Radio type;
    Knob thr, ratio, att, rel, mix, out;
    GRMeter meter;
    Toggle scf, ext;
};

class InsertPanel : public ModulePanel, private juce::ChangeListener
{
public:
    explicit InsertPanel (SilverChannelProcessor&);
    ~InsertPanel() override;
    void layoutContent (juce::Rectangle<int>) override;
    void paintContent (juce::Graphics&) override;
    int preferredWidth() const override { return 82; }
private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void showMenu();
    void openEditor();
    void closeEditor();
    juce::TextButton plus { "+" }, edit { "EDIT" };
    std::unique_ptr<juce::DocumentWindow> window;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> nameArea;
};

//==============================================================================
// MASTER: VUs, faders de entrada/saida com medidores, limiter e monitor
//==============================================================================
class MasterPanel : public juce::Component
{
public:
    explicit MasterPanel (SilverChannelProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    SilverChannelProcessor& proc;
    VUMeter vuL, vuR;
    Radio vuSrc, monitor;
    Knob trim, limThr;
    Toggle limOn, phase;
    juce::Slider inFader, outFader;
    std::unique_ptr<APVTS::SliderAttachment> inAtt, outAtt;
    PeakMeter inMeter, outMeter;
    GRMeter limGr;
    juce::Rectangle<int> tVu, tIo, tLim, tMon, inLbl, outLbl;
};

//==============================================================================
// Vista expandida (sidechain / modo estereo) para GATE, DS2 e COMP
//==============================================================================
class ExpandedView : public juce::Component
{
public:
    ExpandedView (SilverChannelProcessor&, int moduleId);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onClose;
private:
    SilverChannelProcessor& proc;
    int moduleId;
    juce::Rectangle<int> panel;
    juce::TextButton close { "COLLAPSE" };
    juce::OwnedArray<juce::Component> owned;
    std::unique_ptr<Toggle> onBtn, scfBtn, extBtn, listenBtn;
    std::unique_ptr<Radio> mode;
    // COMP
    std::unique_ptr<Toggle> lsOn, bOn, hOn;
    std::unique_ptr<Knob> lsG, lsF, bG, bF, hF;
    // GATE
    std::unique_ptr<Knob> gHp, gLp;
    std::unique_ptr<Toggle> gLink;
    float linkRatio = 1.0f; bool linking = false;
    // DS2
    std::unique_ptr<Radio> dsType1, dsType2;
    std::unique_ptr<Toggle> dsLis1, dsLis2;
    std::unique_ptr<Knob> dsF1, dsF2, dsT1, dsT2;
    juce::Rectangle<int> groupTitles[3];
    juce::StringArray groupNames;
};
} // namespace ui
