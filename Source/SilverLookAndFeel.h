#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
// Paleta SILVER (mesma familia do RefPro SILVER / SILVER-MB / VX_Leveler):
// prata / cinza claro com textos e acentos em azul-marinho
//==============================================================================
namespace pal
{
const juce::Colour bgTop      { 0xffcfd4da };
const juce::Colour bgBottom   { 0xffaab1b9 };
const juce::Colour panelTop   { 0xffeceef1 };
const juce::Colour panelBot   { 0xffd5d9de };
const juce::Colour panelEdge  { 0xff8a919a };
const juce::Colour headerTop  { 0xffdde1e6 };
const juce::Colour headerBot  { 0xffc3c9d0 };
const juce::Colour navy       { 0xff13294b };
const juce::Colour navyMid    { 0xff24467c };
const juce::Colour accent     { 0xff3a78d8 };
const juce::Colour accentLite { 0xff7fb0f5 };
const juce::Colour textDim    { 0xff5b6676 };
const juce::Colour groove     { 0xff9aa1aa };
const juce::Colour meterBg    { 0xff18202e };
const juce::Colour meterLo    { 0xff3a78d8 };
const juce::Colour meterMid   { 0xff7fb0f5 };
const juce::Colour meterHi    { 0xffe7b143 };
const juce::Colour clip       { 0xffd8443a };
const juce::Colour gr         { 0xffe39b2d };
const juce::Colour vuFace     { 0xfff3eedf };
}

inline juce::Font scFont (float h, bool bold = true)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

class SilverLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SilverLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, pal::navy);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, pal::accentLite);
        setColour (juce::Label::textColourId, pal::navy);
        setColour (juce::Label::textWhenEditingColourId, pal::navy);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
        setColour (juce::TextEditor::textColourId, pal::navy);
        setColour (juce::TextEditor::highlightColourId, pal::accentLite);
        setColour (juce::TextEditor::outlineColourId, pal::panelEdge);
        setColour (juce::PopupMenu::backgroundColourId, pal::panelTop);
        setColour (juce::PopupMenu::textColourId, pal::navy);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::navyMid);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId, pal::textDim);
        setColour (juce::ComboBox::backgroundColourId, pal::panelTop);
        setColour (juce::ComboBox::textColourId, pal::navy);
        setColour (juce::ComboBox::outlineColourId, pal::panelEdge);
        setColour (juce::ComboBox::arrowColourId, pal::navy);
        setColour (juce::AlertWindow::backgroundColourId, pal::panelTop);
        setColour (juce::AlertWindow::textColourId, pal::navy);
        setColour (juce::AlertWindow::outlineColourId, pal::panelEdge);
        setColour (juce::TextButton::textColourOffId, pal::navy);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::ResizableWindow::backgroundColourId, pal::panelBot);
        setColour (juce::DocumentWindow::textColourId, pal::navy);
    }

    juce::Font getLabelFont (juce::Label&) override { return scFont (11.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return scFont (juce::jmin (11.5f, h * 0.62f)); }
    juce::Font getPopupMenuFont() override { return scFont (13.0f, false); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return scFont (12.0f); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        using namespace juce;
        const float r  = jmin (w, h) * 0.5f - 3.0f;
        const float cx = x + w * 0.5f, cy = y + h * 0.5f;
        const bool  en = s.isEnabled();

        // trilho
        Path track; track.addCentredArc (cx, cy, r, r, 0.0f, startA, endA, true);
        g.setColour (pal::groove.withAlpha (0.55f));
        g.strokePath (track, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // arco de valor (bipolar quando a faixa cruza zero)
        const double mn = s.getMinimum(), mx = s.getMaximum();
        float from = startA;
        if (mn < 0.0 && mx > 0.0) from = startA + (endA - startA) * (float) s.valueToProportionOfLength (0.0);
        const float to = startA + pos * (endA - startA);
        Path val; val.addCentredArc (cx, cy, r, r, 0.0f, jmin (from, to), jmax (from, to), true);
        g.setColour ((en ? pal::accent : pal::groove));
        g.strokePath (val, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // corpo metalico
        const float kr = r - 5.0f;
        g.setColour (Colours::black.withAlpha (0.22f));
        g.fillEllipse (cx - kr + 1.0f, cy - kr + 2.0f, kr * 2.0f, kr * 2.0f);
        ColourGradient body (Colour (0xfffbfcfd), cx - kr * 0.6f, cy - kr * 0.8f,
                             Colour (0xff98a0a9), cx + kr * 0.6f, cy + kr, true);
        g.setGradientFill (body);
        g.fillEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (Colour (0xff6c747e));
        g.drawEllipse (cx - kr, cy - kr, kr * 2.0f, kr * 2.0f, 1.0f);
        const float ir = kr * 0.72f;
        ColourGradient cap (Colour (0xffe9ecef), cx, cy - ir, Colour (0xffb9c0c7), cx, cy + ir, false);
        g.setGradientFill (cap);
        g.fillEllipse (cx - ir, cy - ir, ir * 2.0f, ir * 2.0f);

        // ponteiro
        const float ang = startA + pos * (endA - startA);
        Line<float> l (cx + std::sin (ang) * kr * 0.25f, cy - std::cos (ang) * kr * 0.25f,
                       cx + std::sin (ang) * kr * 0.92f, cy - std::cos (ang) * kr * 0.92f);
        g.setColour (en ? pal::navy : pal::textDim);
        g.drawLine (l, 2.6f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace juce;
        if (style != Slider::LinearVertical) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
        const float cx = x + w * 0.5f;
        Rectangle<float> groove (cx - 3.0f, (float) y + 4.0f, 6.0f, (float) h - 8.0f);
        g.setColour (pal::meterBg.withAlpha (0.85f));
        g.fillRoundedRectangle (groove, 3.0f);
        // marcacoes
        g.setColour (pal::textDim.withAlpha (0.6f));
        for (int i = 0; i <= 8; ++i)
        {
            const float yy = (float) y + 6.0f + (h - 12.0f) * i / 8.0f;
            g.drawHorizontalLine ((int) yy, cx - 12.0f, cx - 6.0f);
            g.drawHorizontalLine ((int) yy, cx + 6.0f, cx + 12.0f);
        }
        // capa do fader
        Rectangle<float> cap (cx - 13.0f, pos - 9.0f, 26.0f, 18.0f);
        g.setColour (Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (cap.translated (0, 2), 3.0f);
        ColourGradient cg (Colour (0xfff6f7f9), cap.getX(), cap.getY(), Colour (0xff9da5ae), cap.getX(), cap.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (Colour (0xff6c747e)); g.drawRoundedRectangle (cap, 3.0f, 1.0f);
        g.setColour (pal::navy); g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 6.0f, 2.0f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        using namespace juce;
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        const bool flat = b.getProperties().getWithDefault ("flat", false);
        if (flat && ! on && ! over) return;
        if (on)
        {
            ColourGradient cg (pal::navyMid, r.getX(), r.getY(), pal::navy, r.getX(), r.getBottom(), false);
            g.setGradientFill (cg);
        }
        else
        {
            ColourGradient cg (Colour (0xfff7f8fa), r.getX(), r.getY(), Colour (0xffc9ced5), r.getX(), r.getBottom(), false);
            g.setGradientFill (cg);
        }
        g.fillRoundedRectangle (r, 3.0f);
        if (over || down) { g.setColour (pal::accent.withAlpha (down ? 0.35f : 0.18f)); g.fillRoundedRectangle (r, 3.0f); }
        g.setColour (on ? pal::navy : pal::panelEdge);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        if (on && (bool) b.getProperties().getWithDefault ("led", false))
        {
            g.setColour (pal::accentLite);
            g.fillEllipse (r.getX() + 4.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.setColour (b.getToggleState() ? juce::Colours::white : pal::navy);
        if (! b.isEnabled()) g.setColour (pal::textDim.withAlpha (0.6f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (2, 0), juce::Justification::centred, 1);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (pal::panelTop);
        g.setColour (pal::panelEdge);
        g.drawRect (0, 0, w, h);
    }
};
