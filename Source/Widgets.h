#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "SilverLookAndFeel.h"

namespace ui
{
using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
// Knob rotativo com nome em cima e valor embaixo
//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (APVTS& s, const juce::String& id, const juce::String& title)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 14);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (slider);
        label.setText (title, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (scFont (10.5f));
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
        att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
        if (auto* p = s.getParameter (id))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }
    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (13));
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, r.getWidth(), 14);
        slider.setBounds (r);
    }
    juce::Slider slider;
    juce::Label label;
private:
    std::unique_ptr<APVTS::SliderAttachment> att;
};

//==============================================================================
// Botao liga/desliga ligado a parametro bool
//==============================================================================
class Toggle : public juce::TextButton
{
public:
    Toggle (APVTS& s, const juce::String& id, const juce::String& text, bool led = false)
        : juce::TextButton (text)
    {
        setClickingTogglesState (true);
        getProperties().set ("led", led);
        att = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
    }
private:
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

//==============================================================================
// Grupo de botoes de radio ligado a parametro de escolha
//==============================================================================
class Radio : public juce::Component
{
public:
    Radio (APVTS& s, const juce::String& id, int columns = 0) : cols (columns)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id));
        jassert (p != nullptr);
        for (int i = 0; i < p->choices.size(); ++i)
        {
            auto* b = buttons.add (new juce::TextButton (p->choices[i]));
            b->onClick = [this, i] { if (attachment) attachment->setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (b);
        }
        attachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { update ((int) std::lround (v)); }, s.undoManager);
        attachment->sendInitialUpdate();
    }
    void update (int idx) { for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == idx, juce::dontSendNotification); }
    void resized() override
    {
        const int n = buttons.size();
        const int c = cols > 0 ? cols : n;
        const int rows = (n + c - 1) / c;
        const float bw = getWidth() / (float) c, bh = getHeight() / (float) rows;
        for (int i = 0; i < n; ++i)
        {
            const int cx = i % c, ry = i / c;
            buttons[i]->setBounds (juce::Rectangle<float> (cx * bw, ry * bh, bw, bh).reduced (1.0f).toNearestInt());
        }
    }
    juce::OwnedArray<juce::TextButton> buttons;
private:
    int cols;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// Botao que alterna ciclicamente entre as opcoes de um parametro de escolha
//==============================================================================
class Cycle : public juce::TextButton
{
public:
    Cycle (APVTS& s, const juce::String& id, const juce::String& prefix = {}) : pre (prefix)
    {
        param = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id));
        jassert (param != nullptr);
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            current = (int) std::lround (v);
            setButtonText (pre + param->choices[current]);
        }, s.undoManager);
        attachment->sendInitialUpdate();
        onClick = [this]
        {
            if (juce::ModifierKeys::currentModifiers.isPopupMenu()) return;
            attachment->setValueAsCompleteGesture ((float) ((current + 1) % param->choices.size()));
        };
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu m;
            for (int i = 0; i < param->choices.size(); ++i) m.addItem (i + 1, param->choices[i], true, i == current);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r)
            { if (r > 0) attachment->setValueAsCompleteGesture ((float) (r - 1)); });
            return;
        }
        juce::TextButton::mouseDown (e);
    }
    int current = 0;
private:
    juce::String pre;
    juce::AudioParameterChoice* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

//==============================================================================
// Medidor de reducao de ganho (vertical, de cima para baixo)
//==============================================================================
class GRMeter : public juce::Component, private juce::Timer
{
public:
    GRMeter (std::function<float()> fetch, float maxDb = 15.0f, bool horizontalLeds = false)
        : get (std::move (fetch)), range (maxDb), leds (horizontalLeds) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (leds)
        {
            // 4 LEDs: -3 -6 -9 -12 (como o DS2 do Omni)
            const float w = r.getWidth() / 4.0f;
            const float steps[4] = { 3.0f, 6.0f, 9.0f, 12.0f };
            for (int i = 0; i < 4; ++i)
            {
                auto led = juce::Rectangle<float> (r.getX() + i * w, r.getY(), w, r.getHeight()).reduced (1.5f, 1.0f);
                g.setColour (value >= steps[i] ? pal::gr : pal::meterBg.withAlpha (0.85f));
                g.fillRoundedRectangle (led, 2.0f);
            }
            return;
        }
        g.setColour (pal::meterBg);
        g.fillRoundedRectangle (r, 2.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, value / range);
        auto bar = r.reduced (2.0f);
        g.setColour (pal::gr);
        g.fillRect (bar.withHeight (bar.getHeight() * frac));
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        for (float db = 3.0f; db < range; db += 3.0f)
        {
            const float yy = bar.getY() + bar.getHeight() * db / range;
            g.drawHorizontalLine ((int) yy, bar.getX(), bar.getRight());
        }
    }
private:
    void timerCallback() override
    {
        const float v = get ? get() : 0.0f;
        value = v > value ? v : value * 0.86f;
        if (value < 0.05f) value = 0.0f;
        repaint();
    }
    std::function<float()> get;
    float range, value = 0.0f;
    bool leds;
};

//==============================================================================
// Medidor de pico estereo com indicador de clip e retencao
//==============================================================================
class PeakMeter : public juce::Component, private juce::Timer
{
public:
    PeakMeter (std::function<float(int)> fetch) : get (std::move (fetch)) { startTimerHz (30); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto clipArea = r.removeFromTop (6.0f);
        const float w = r.getWidth() / 2.0f;
        for (int c = 0; c < 2; ++c)
        {
            auto col = juce::Rectangle<float> (r.getX() + c * w, r.getY(), w, r.getHeight()).reduced (1.0f, 0.0f);
            g.setColour (pal::meterBg); g.fillRect (col);
            const float frac = dbToFrac (lvl[c]);
            auto bar = col.withTop (col.getBottom() - col.getHeight() * frac);
            juce::ColourGradient cg (pal::meterLo, col.getX(), col.getBottom(), pal::meterHi, col.getX(), col.getY(), false);
            cg.addColour (0.75, pal::meterMid);
            g.setGradientFill (cg); g.fillRect (bar);
            const float hy = col.getBottom() - col.getHeight() * dbToFrac (hold[c]);
            g.setColour (juce::Colours::white); g.drawHorizontalLine ((int) hy, col.getX(), col.getRight());
            auto ci = juce::Rectangle<float> (clipArea.getX() + c * w, clipArea.getY(), w, clipArea.getHeight()).reduced (1.0f, 0.5f);
            g.setColour (clipped[c] ? pal::clip : pal::meterBg.withAlpha (0.6f)); g.fillRect (ci);
        }
    }
    void mouseDown (const juce::MouseEvent&) override { clipped[0] = clipped[1] = false; hold[0] = hold[1] = -100.0f; repaint(); }
private:
    static float dbToFrac (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f); }
    void timerCallback() override
    {
        for (int c = 0; c < 2; ++c)
        {
            const float pk = get ? get (c) : 0.0f;
            const float db = juce::Decibels::gainToDecibels (pk, -100.0f);
            lvl[c] = db > lvl[c] ? db : lvl[c] - 1.2f;
            if (db > hold[c]) { hold[c] = db; holdCount[c] = 45; }
            else if (--holdCount[c] <= 0) hold[c] = juce::jmax (lvl[c], hold[c] - 0.8f);
            if (pk >= 0.999f) clipped[c] = true;
        }
        repaint();
    }
    std::function<float(int)> get;
    float lvl[2] { -100.0f, -100.0f }, hold[2] { -100.0f, -100.0f };
    int holdCount[2] { 0, 0 };
    bool clipped[2] { false, false };
};

//==============================================================================
// VU analogico (mostrador creme, ponteiro azul-marinho)
//==============================================================================
class VUMeter : public juce::Component, private juce::Timer
{
public:
    VUMeter (std::function<float()> fetch, juce::String name) : get (std::move (fetch)), title (std::move (name)) { startTimerHz (40); }
    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        ColourGradient bezel (Colour (0xfff2f4f6), r.getX(), r.getY(), Colour (0xff8e959e), r.getX(), r.getBottom(), false);
        g.setGradientFill (bezel); g.fillRoundedRectangle (r, 5.0f);
        auto face = r.reduced (4.0f);
        ColourGradient fg (pal::vuFace, face.getCentreX(), face.getY(), Colour (0xffd9d2bd), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (fg); g.fillRoundedRectangle (face, 3.0f);

        const float cx = face.getCentreX(), cy = face.getBottom() + face.getHeight() * 0.35f;
        const float rad = face.getHeight() * 1.05f;
        const float a0 = -0.82f, a1 = 0.82f;
        auto angFor = [&] (float vu) { // -20..+3 VU, escala nao linear como um VU real
            const float lin = std::pow (10.0f, vu / 20.0f);
            const float lmin = std::pow (10.0f, -20.0f / 20.0f), lmax = std::pow (10.0f, 3.0f / 20.0f);
            return a0 + (a1 - a0) * (lin - lmin) / (lmax - lmin);
        };
        // arco vermelho acima de 0
        Path red; red.addCentredArc (cx, cy, rad * 0.86f, rad * 0.86f, 0.0f, angFor (0.0f), angFor (3.0f), true);
        g.setColour (pal::clip.withAlpha (0.85f)); g.strokePath (red, PathStrokeType (3.5f));
        Path blk; blk.addCentredArc (cx, cy, rad * 0.86f, rad * 0.86f, 0.0f, angFor (-20.0f), angFor (0.0f), true);
        g.setColour (pal::navy.withAlpha (0.8f)); g.strokePath (blk, PathStrokeType (1.2f));

        g.setFont (scFont (8.5f));
        const float marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 2, 3 };
        for (float m : marks)
        {
            const float a = angFor (m);
            const float x1 = cx + std::sin (a) * rad * 0.86f, y1 = cy - std::cos (a) * rad * 0.86f;
            const float x2 = cx + std::sin (a) * rad * 0.93f, y2 = cy - std::cos (a) * rad * 0.93f;
            g.setColour (m > 0 ? pal::clip : pal::navy);
            g.drawLine (x1, y1, x2, y2, 1.2f);
            if (m == -20 || m == -10 || m == -5 || m == -3 || m == 0 || m == 3)
            {
                const float tx = cx + std::sin (a) * rad * 1.01f, ty = cy - std::cos (a) * rad * 1.01f;
                g.drawText (String ((int) std::abs (m)), Rectangle<float> (tx - 10, ty - 6, 20, 12), Justification::centred);
            }
        }
        g.setColour (pal::navy);
        g.setFont (scFont (10.0f));
        g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.52f).withHeight (14.0f), Justification::centred);
        g.setFont (scFont (8.5f));
        g.drawText (title, face.reduced (5.0f, 3.0f), Justification::bottomLeft);

        // ponteiro
        const float a = angFor (jlimit (-22.0f, 4.0f, needle));
        g.setColour (pal::navy);
        g.drawLine (cx, cy, cx + std::sin (a) * rad * 0.95f, cy - std::cos (a) * rad * 0.95f, 1.6f);
        g.saveState();
        g.reduceClipRegion (face.toNearestInt());
        g.setColour (Colour (0xff2b2f36));
        g.fillEllipse (cx - 14.0f, face.getBottom() - 8.0f, 28.0f, 28.0f);
        g.restoreState();
    }
private:
    void timerCallback() override
    {
        const float target = get ? get() : -40.0f;
        needle += (target - needle) * 0.25f; // inercia do ponteiro
        repaint();
    }
    std::function<float()> get;
    juce::String title;
    float needle = -25.0f;
};

//==============================================================================
// Rotulo de secao simples
//==============================================================================
inline void drawSectionTitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t)
{
    g.setColour (pal::textDim);
    g.setFont (scFont (9.5f));
    g.drawText (t, r, juce::Justification::centred);
    const int tw = (int) juce::GlyphArrangement::getStringWidth (scFont (9.5f), t) + 10;
    const int cy = r.getCentreY();
    g.setColour (pal::panelEdge.withAlpha (0.5f));
    g.drawHorizontalLine (cy, (float) r.getX() + 4.0f, (float) r.getCentreX() - tw / 2.0f);
    g.drawHorizontalLine (cy, (float) r.getCentreX() + tw / 2.0f, (float) r.getRight() - 4.0f);
}
} // namespace ui
