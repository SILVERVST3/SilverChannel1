#include "Panels.h"

namespace ui
{
using juce::Rectangle;

static void placeKnob (juce::Component& k, Rectangle<int> r, int w = 70, int h = 80)
{
    k.setBounds (r.withSizeKeepingCentre (juce::jmin (w, r.getWidth()), juce::jmin (h, r.getHeight())));
}

//==============================================================================
ModulePanel::ModulePanel (SilverChannelProcessor& p, int id, const juce::String& onParam, bool canExpand, bool canMove)
    : moduleId (id), proc (p), s (p.apvts), expandable (canExpand), movable (canMove)
{
    nameBtn = std::make_unique<Toggle> (s, onParam, sc::moduleName (id), true);
    nameBtn->setTooltip ("Liga/desliga o modulo");
    nameBtn->onStateChange = [this] { repaint(); };
    addAndMakeVisible (*nameBtn);
    if (movable)
    {
        for (auto* b : { &leftBtn, &rightBtn }) { b->getProperties().set ("flat", true); addAndMakeVisible (b); }
        leftBtn.setTooltip ("Mover modulo para a esquerda");
        rightBtn.setTooltip ("Mover modulo para a direita");
        leftBtn.onClick  = [this] { proc.moveModule (moduleId, -1); };
        rightBtn.onClick = [this] { proc.moveModule (moduleId, +1); };
    }
    if (expandable)
    {
        expBtn.setTooltip ("Vista expandida: sidechain e modo estereo");
        expBtn.onClick = [this] { if (onExpand) onExpand (moduleId); };
        addAndMakeVisible (expBtn);
    }
}

void ModulePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f);
    juce::ColourGradient bg (pal::panelTop, r.getX(), r.getY(), pal::panelBot, r.getX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);
    auto h = r.removeFromTop (30.0f);
    juce::ColourGradient hg (pal::headerTop, h.getX(), h.getY(), pal::headerBot, h.getX(), h.getBottom(), false);
    g.setGradientFill (hg);
    juce::Path hp; hp.addRoundedRectangle (h.getX(), h.getY(), h.getWidth(), h.getHeight(), 6.0f, 6.0f, true, true, false, false);
    g.fillPath (hp);
    g.setColour (pal::panelEdge.withAlpha (0.7f));
    g.drawHorizontalLine ((int) h.getBottom(), h.getX(), h.getRight());
    g.setColour (pal::panelEdge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 1.0f);
    paintContent (g);
}

void ModulePanel::resized()
{
    auto r = getLocalBounds();
    auto h = r.removeFromTop (30).reduced (4, 4);
    if (movable) { leftBtn.setBounds (h.removeFromLeft (16)); rightBtn.setBounds (h.removeFromRight (16)); }
    if (expandable) { expBtn.setBounds (h.removeFromRight (26)); h.removeFromRight (2); }
    nameBtn->setBounds (h.reduced (2, 0));
    content = r.reduced (6, 6);
    layoutContent (content);
}

//==============================================================================
PrePanel::PrePanel (SilverChannelProcessor& p)
    : ModulePanel (p, sc::modPre, "pre_on", false),
      satOn (s, "sat_on", "SAT", true), hpOn (s, "hp_on", "HP", true), lpOn (s, "lp_on", "LP", true), thumpOn (s, "thump_on", "THUMP", true),
      satAmt (s, "sat_amt", "SATURATION"), hpFreq (s, "hp_freq", "HP FREQ"), lpFreq (s, "lp_freq", "LP FREQ"), res (s, "flt_res", "RES"),
      satType (s, "sat_type", 2), hpSlope (s, "hp_slope", "dB "), lpSlope (s, "lp_slope", "dB "), thumpAmt (s, "thump_amt")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &satOn, &hpOn, &lpOn, &thumpOn, &satAmt, &hpFreq, &lpFreq, &res, &satType, &hpSlope, &lpSlope, &thumpAmt })
        addAndMakeVisible (c);
    hpSlope.setTooltip ("Inclinacao do filtro (clique: proxima / botao direito: lista)");
    lpSlope.setTooltip (hpSlope.getTooltip());
}

void PrePanel::layoutContent (Rectangle<int> r)
{
    tSat = r.removeFromTop (14);
    satOn.setBounds (r.removeFromTop (20).withSizeKeepingCentre (64, 20));
    placeKnob (satAmt, r.removeFromTop (92), 92, 92);
    satType.setBounds (r.removeFromTop (44).reduced (6, 0));
    r.removeFromTop (8);
    tFlt = r.removeFromTop (14);
    for (int i = 0; i < 2; ++i)
    {
        auto row = r.removeFromTop (84);
        placeKnob (i == 0 ? (juce::Component&) hpFreq : (juce::Component&) lpFreq, row.removeFromLeft (82), 76, 80);
        auto col = row.reduced (2, 14);
        (i == 0 ? (juce::Component&) hpOn : (juce::Component&) lpOn).setBounds (col.removeFromTop (22));
        col.removeFromTop (6);
        (i == 0 ? (juce::Component&) hpSlope : (juce::Component&) lpSlope).setBounds (col.removeFromTop (22));
    }
    placeKnob (res, r.removeFromTop (80), 76, 78);
    r.removeFromTop (6);
    tThump = r.removeFromTop (14);
    auto th = r.removeFromTop (24);
    thumpOn.setBounds (th.removeFromLeft (th.getWidth() / 2 + 10).reduced (3, 0));
    thumpAmt.setBounds (th.reduced (3, 0));
}

void PrePanel::paintContent (juce::Graphics& g)
{
    drawSectionTitle (g, tSat, "SATURATION");
    drawSectionTitle (g, tFlt, "FILTERS");
    drawSectionTitle (g, tThump, "LOW END");
}

//==============================================================================
GatePanel::GatePanel (SilverChannelProcessor& p)
    : ModulePanel (p, sc::modGate, "gt_on", true),
      type (s, "gt_type"),
      thr (s, "gt_thr", "THRESHOLD"), floorK (s, "gt_floor", "FLOOR"), closeK (s, "gt_close", "CLOSE"),
      att (s, "gt_att", "ATTACK"), rel (s, "gt_rel", "RELEASE"),
      meter ([&p] { return p.gate.gr.exchange (0.0f); }, 60.0f),
      scf (s, "gt_scf", "SC FILT"), ext (s, "gt_ext", "EXT")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &type, &thr, &floorK, &closeK, &att, &rel, &meter, &scf, &ext })
        addAndMakeVisible (c);
}

void GatePanel::layoutContent (Rectangle<int> r)
{
    type.setBounds (r.removeFromTop (22));
    r.removeFromTop (6);
    auto bottom = r.removeFromBottom (22);
    scf.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 + 6).reduced (1, 0));
    ext.setBounds (bottom.reduced (1, 0));
    auto strip = r.removeFromRight (14);
    meter.setBounds (strip.withHeight (160).withY (r.getY() + 14));
    for (auto* k : { &thr, &floorK, &closeK, &att, &rel })
        placeKnob (*k, r.removeFromTop (82), 92, 80);
}

//==============================================================================
Ds2Panel::Ds2Panel (SilverChannelProcessor& p)
    : ModulePanel (p, sc::modDs2, "ds_on", true),
      on1 (s, "ds1_on", "DS 1", true), on2 (s, "ds2_on", "DS 2", true),
      lis1 (s, "ds1_listen", "LISTEN"), lis2 (s, "ds2_listen", "LISTEN"), ext (s, "ds_ext", "EXT SC"),
      type1 (s, "ds1_type"), type2 (s, "ds2_type"),
      f1 (s, "ds1_freq", "FREQ"), t1 (s, "ds1_thr", "THRESH"), f2 (s, "ds2_freq", "FREQ"), t2 (s, "ds2_thr", "THRESH"),
      out (s, "ds_out", "OUTPUT"),
      gr1 ([&p] { return p.ds2.gr[0].exchange (0.0f); }, 12.0f, true),
      gr2 ([&p] { return p.ds2.gr[1].exchange (0.0f); }, 12.0f, true)
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &on1, &on2, &lis1, &lis2, &ext, &type1, &type2, &f1, &t1, &f2, &t2, &out, &gr1, &gr2 })
        addAndMakeVisible (c);
}

void Ds2Panel::layoutContent (Rectangle<int> r)
{
    auto section = [&r] (Toggle& on, Cycle& ty, Knob& f, Knob& t, GRMeter& g, Toggle& lis)
    {
        auto h = r.removeFromTop (22);
        on.setBounds (h.removeFromLeft (56).reduced (1, 0));
        ty.setBounds (h.reduced (1, 0));
        auto k = r.removeFromTop (84);
        placeKnob (f, k.removeFromLeft (k.getWidth() / 2), 72, 80);
        placeKnob (t, k, 72, 80);
        auto l = r.removeFromTop (20);
        g.setBounds (l.removeFromLeft (78).reduced (0, 3));
        lis.setBounds (l.reduced (2, 1));
    };
    section (on1, type1, f1, t1, gr1, lis1);
    r.removeFromTop (6);
    sep = r.removeFromTop (6);
    section (on2, type2, f2, t2, gr2, lis2);
    r.removeFromTop (10);
    placeKnob (out, r.removeFromTop (82), 80, 80);
    ext.setBounds (r.removeFromBottom (22).reduced (14, 0));
}

void Ds2Panel::paintContent (juce::Graphics& g)
{
    g.setColour (pal::panelEdge.withAlpha (0.5f));
    g.drawHorizontalLine (sep.getCentreY(), (float) sep.getX(), (float) sep.getRight());
}

//==============================================================================
EqPanel::Band::Band (APVTS& s, const juce::String& key, const juce::String& title)
    : on (s, "eq_" + key + "_on", title, true), type (s, "eq_" + key + "_type"),
      gain (s, "eq_" + key + "_gain", "GAIN"), freq (s, "eq_" + key + "_freq", "FREQ"), q (s, "eq_" + key + "_q", "Q"),
      typeParam (s.getRawParameterValue ("eq_" + key + "_type"))
{}

EqPanel::EqPanel (SilverChannelProcessor& p) : ModulePanel (p, sc::modEq, "eq_on", false)
{
    const char* keys[4]   = { "hi", "mid", "tone", "lo" };
    const char* titles[4] = { "HIGH", "MID", "TONE", "LOW" };
    for (int i = 0; i < 4; ++i)
    {
        bands[i] = std::make_unique<Band> (s, keys[i], titles[i]);
        for (juce::Component* c : std::initializer_list<juce::Component*> { &bands[i]->on, &bands[i]->type, &bands[i]->gain, &bands[i]->freq, &bands[i]->q })
            addAndMakeVisible (c);
    }
    timerCallback();
    startTimerHz (8);
}

void EqPanel::layoutContent (Rectangle<int> r)
{
    const int rowH = r.getHeight() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto row = r.removeFromTop (rowH);
        rows[i] = row;
        auto h = row.removeFromTop (22).reduced (4, 0);
        bands[i]->on.setBounds (h.removeFromLeft (h.getWidth() / 2).reduced (1, 0));
        bands[i]->type.setBounds (h.reduced (1, 0));
        row.removeFromTop (2);
        const int w = row.getWidth() / 3;
        placeKnob (bands[i]->gain, row.removeFromLeft (w), 70, 82);
        placeKnob (bands[i]->freq, row.removeFromLeft (w), 70, 82);
        placeKnob (bands[i]->q, row, 70, 82);
    }
}

void EqPanel::paintContent (juce::Graphics& g)
{
    g.setColour (pal::panelEdge.withAlpha (0.45f));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine (rows[i].getY() - 1, (float) rows[i].getX() + 4.0f, (float) rows[i].getRight() - 4.0f);
}

void EqPanel::timerCallback()
{
    for (auto& b : bands)
    {
        const bool para = (int) std::lround (b->typeParam->load()) == 2;
        b->q.setEnabled (para);
        b->q.setAlpha (para ? 1.0f : 0.35f);
    }
}

//==============================================================================
CompPanel::CompPanel (SilverChannelProcessor& p)
    : ModulePanel (p, sc::modComp, "cmp_on", true),
      type (s, "cmp_type", 2),
      thr (s, "cmp_thr", "THRESHOLD"), ratio (s, "cmp_ratio", "RATIO"), att (s, "cmp_att", "ATTACK"),
      rel (s, "cmp_rel", "RELEASE"), mix (s, "cmp_mix", "MIX"), out (s, "cmp_out", "OUTPUT"),
      meter ([&p] { return p.comp.gr.exchange (0.0f); }, 15.0f),
      scf (s, "cmp_scf", "SC FILT"), ext (s, "cmp_ext", "EXT")
{
    for (juce::Component* c : std::initializer_list<juce::Component*> { &type, &thr, &ratio, &att, &rel, &mix, &out, &meter, &scf, &ext })
        addAndMakeVisible (c);
}

void CompPanel::layoutContent (Rectangle<int> r)
{
    type.setBounds (r.removeFromTop (46).reduced (4, 0));
    r.removeFromTop (6);
    auto bottom = r.removeFromBottom (22);
    scf.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 + 6).reduced (1, 0));
    ext.setBounds (bottom.reduced (1, 0));

    auto row1 = r.removeFromTop (104);
    meter.setBounds (row1.removeFromRight (20).reduced (0, 6));
    placeKnob (thr, row1, 104, 104);
    auto row2 = r.removeFromTop (84);
    placeKnob (ratio, row2.removeFromLeft (row2.getWidth() / 2), 76, 82);
    placeKnob (att, row2, 76, 82);
    auto row3 = r.removeFromTop (84);
    placeKnob (rel, row3.removeFromLeft (row3.getWidth() / 2), 76, 82);
    placeKnob (mix, row3, 76, 82);
    placeKnob (out, r.removeFromTop (84), 80, 82);
}

//==============================================================================
namespace
{
struct InsertWindow : public juce::DocumentWindow
{
    InsertWindow (const juce::String& name)
        : juce::DocumentWindow (name, pal::panelBot, juce::DocumentWindow::closeButton) {}
    void closeButtonPressed() override { if (onClose) onClose(); }
    std::function<void()> onClose;
};

void collectVst3 (const juce::File& dir, juce::Array<juce::File>& out, int depth)
{
    if (depth > 4 || ! dir.isDirectory()) return;
    for (const auto& f : dir.findChildFiles (juce::File::findFilesAndDirectories, false))
    {
        if (f.hasFileExtension ("vst3")) { if (! f.getFileName().containsIgnoreCase ("SilverChannel")) out.add (f); }
        else if (f.isDirectory()) collectVst3 (f, out, depth + 1);
    }
}
}

InsertPanel::InsertPanel (SilverChannelProcessor& p) : ModulePanel (p, sc::modInsert, "insert_on", false)
{
    plus.setTooltip ("Carregar plugin VST3 no ponto de insert");
    plus.onClick = [this] { showMenu(); };
    edit.onClick = [this] { openEditor(); };
    addAndMakeVisible (plus);
    addAndMakeVisible (edit);
    proc.insert.addChangeListener (this);
}

InsertPanel::~InsertPanel()
{
    proc.insert.removeChangeListener (this);
    closeEditor();
}

void InsertPanel::layoutContent (Rectangle<int> r)
{
    plus.setBounds (r.removeFromTop (44).withSizeKeepingCentre (44, 44));
    edit.setBounds (r.removeFromBottom (24).reduced (4, 0));
    r.removeFromBottom (6);
    nameArea = r.reduced (2, 8);
}

void InsertPanel::paintContent (juce::Graphics& g)
{
    auto a = nameArea.toFloat();
    g.setColour (pal::meterBg.withAlpha (0.08f));
    g.fillRoundedRectangle (a, 4.0f);
    g.setColour (pal::panelEdge.withAlpha (0.6f));
    g.drawRoundedRectangle (a, 4.0f, 1.0f);
    const juce::String txt = proc.insert.isLoaded() ? proc.insert.getName() : juce::String ("EMPTY");
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, a.getCentreX(), a.getCentreY()));
    g.setColour (proc.insert.isLoaded() ? pal::navy : pal::textDim);
    g.setFont (scFont (13.0f));
    auto rot = juce::Rectangle<float> (0, 0, a.getHeight() - 10.0f, a.getWidth()).withCentre (a.getCentre());
    g.drawFittedText (txt, rot.toNearestInt(), juce::Justification::centred, 1);
    g.restoreState();
}

void InsertPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    closeEditor();
    repaint();
}

void InsertPanel::showMenu()
{
    juce::PopupMenu m, found;
    juce::Array<juce::File> files;
   #if JUCE_PLUGINHOST_VST3
    auto paths = juce::VST3PluginFormat().getDefaultLocationsToSearch();
    for (int i = 0; i < paths.getNumPaths(); ++i) collectVst3 (paths[i], files, 0);
   #endif
    for (int i = 0; i < files.size(); ++i) found.addItem (100 + i, files[i].getFileNameWithoutExtension());
    m.addSectionHeader ("INSERT (VST3)");
    m.addSubMenu ("Plugins instalados", found, files.size() > 0);
    m.addItem (1, "Procurar arquivo .vst3...");
    m.addSeparator();
    m.addItem (2, "Abrir editor", proc.insert.isLoaded());
    m.addItem (3, "Remover plugin", proc.insert.isLoaded());

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&plus),
        [this, files] (int r)
        {
            auto load = [this] (const juce::File& f)
            {
                juce::String err;
                if (! proc.insert.loadFromFile (f, err))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Insert", err);
                repaint();
            };
            if (r >= 100 && r - 100 < files.size()) load (files[r - 100]);
            else if (r == 1)
            {
                chooser = std::make_unique<juce::FileChooser> ("Escolha um plugin VST3",
                    juce::File ("C:\\Program Files\\Common Files\\VST3"), "*.vst3");
                chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [load] (const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) load (fc.getResult()); });
            }
            else if (r == 2) openEditor();
            else if (r == 3) { proc.insert.unload(); repaint(); }
        });
}

void InsertPanel::openEditor()
{
    if (window != nullptr) { window->toFront (true); return; }
    auto* inst = proc.insert.getInstance();
    if (inst == nullptr || ! inst->hasEditor()) return;
    auto w = std::make_unique<InsertWindow> ("SilverChannel 01 - INSERT: " + inst->getName());
    w->setUsingNativeTitleBar (true);
    w->setContentOwned (inst->createEditorIfNeeded(), true);
    w->setResizable (false, false);
    w->setAlwaysOnTop (true);
    juce::Component::SafePointer<InsertPanel> safe (this);
    w->onClose = [safe] { juce::MessageManager::callAsync ([safe] { if (safe) safe->closeEditor(); }); };
    w->centreAroundComponent (this, w->getWidth(), w->getHeight());
    w->setVisible (true);
    window = std::move (w);
}

void InsertPanel::closeEditor()
{
    window.reset();
}

//==============================================================================
MasterPanel::MasterPanel (SilverChannelProcessor& p)
    : proc (p),
      vuL ([&p] { return p.vuLevel[0].load(); }, "L"), vuR ([&p] { return p.vuLevel[1].load(); }, "R"),
      vuSrc (p.apvts, "vu_src"), monitor (p.apvts, "monitor", 3),
      trim (p.apvts, "vu_trim", "TRIM"), limThr (p.apvts, "lim_thr", "LIMITER"),
      limOn (p.apvts, "lim_on", "LIMIT", true), phase (p.apvts, "phase_inv", "PHASE"),
      inMeter ([&p] (int c) { return p.inPeak[c].exchange (0.0f); }),
      outMeter ([&p] (int c) { return p.outPeak[c].exchange (0.0f); }),
      limGr ([&p] { return p.limiter.gr.exchange (0.0f); }, 12.0f)
{
    for (auto* f : { &inFader, &outFader })
    {
        f->setSliderStyle (juce::Slider::LinearVertical);
        f->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 14);
        addAndMakeVisible (f);
    }
    inAtt  = std::make_unique<APVTS::SliderAttachment> (p.apvts, "in_gain", inFader);
    outAtt = std::make_unique<APVTS::SliderAttachment> (p.apvts, "out_gain", outFader);
    inFader.setDoubleClickReturnValue (true, 0.0);
    outFader.setDoubleClickReturnValue (true, 0.0);
    phase.setButtonText (juce::String::fromUTF8 ("\xc3\x98 PHASE"));
    for (juce::Component* c : std::initializer_list<juce::Component*> { &vuL, &vuR, &vuSrc, &monitor, &trim, &limThr, &limOn, &phase, &inMeter, &outMeter, &limGr })
        addAndMakeVisible (c);
}

void MasterPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f);
    juce::ColourGradient bg (pal::panelTop, r.getX(), r.getY(), pal::panelBot, r.getX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 6.0f);
    auto h = r.removeFromTop (30.0f);
    juce::ColourGradient hg (pal::navyMid, h.getX(), h.getY(), pal::navy, h.getX(), h.getBottom(), false);
    g.setGradientFill (hg);
    juce::Path hp; hp.addRoundedRectangle (h.getX(), h.getY(), h.getWidth(), h.getHeight(), 6.0f, 6.0f, true, true, false, false);
    g.fillPath (hp);
    g.setColour (juce::Colours::white);
    g.setFont (scFont (13.0f));
    g.drawText ("MASTER", h, juce::Justification::centred);
    g.setColour (pal::panelEdge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 6.0f, 1.0f);

    drawSectionTitle (g, tVu, "METERS");
    drawSectionTitle (g, tIo, "INPUT / OUTPUT");
    drawSectionTitle (g, tMon, "MONITOR");
    g.setColour (pal::navy); g.setFont (scFont (10.5f));
    g.drawText ("IN", inLbl, juce::Justification::centred);
    g.drawText ("OUT", outLbl, juce::Justification::centred);
    g.setFont (scFont (8.5f)); g.setColour (pal::textDim);
    g.drawText ("GR", limGr.getBounds().translated (0, -12).withHeight (12).expanded (6, 0), juce::Justification::centred);
}

void MasterPanel::resized()
{
    auto r = getLocalBounds().reduced (8, 0);
    r.removeFromTop (36);
    tVu = r.removeFromTop (14);
    auto vu = r.removeFromTop (96);
    vuL.setBounds (vu.removeFromLeft (vu.getWidth() / 2).reduced (2, 0));
    vuR.setBounds (vu.reduced (2, 0));
    r.removeFromTop (4);
    auto vrow = r.removeFromTop (72);
    trim.setBounds (vrow.removeFromRight (78).reduced (2, 0));
    vuSrc.setBounds (vrow.withSizeKeepingCentre (vrow.getWidth() - 10, 24));
    r.removeFromTop (4);
    tIo = r.removeFromTop (14);
    auto io = r.removeFromTop (262);
    auto lab = io.removeFromTop (14);
    auto colIn = io.removeFromLeft (84);
    auto colOut = io.removeFromLeft (84);
    inLbl = lab.withX (colIn.getX()).withWidth (colIn.getWidth());
    outLbl = lab.withX (colOut.getX()).withWidth (colOut.getWidth());
    inMeter.setBounds (colIn.removeFromRight (22).withTrimmedBottom (20).reduced (0, 4));
    inFader.setBounds (colIn);
    outMeter.setBounds (colOut.removeFromRight (22).withTrimmedBottom (20).reduced (0, 4));
    outFader.setBounds (colOut);
    auto right = io.reduced (4, 0);
    limOn.setBounds (right.removeFromTop (24).reduced (6, 0));
    right.removeFromTop (2);
    auto lk = right.removeFromTop (90);
    limGr.setBounds (lk.removeFromRight (12).reduced (0, 14));
    limThr.setBounds (lk);
    right.removeFromTop (30);
    phase.setBounds (right.removeFromTop (24).reduced (6, 0));
    r.removeFromTop (4);
    tMon = r.removeFromTop (14);
    monitor.setBounds (r.removeFromTop (50).reduced (10, 0));
}

//==============================================================================
ExpandedView::ExpandedView (SilverChannelProcessor& p, int m) : proc (p), moduleId (m)
{
    auto& s = p.apvts;
    const juce::String pre = m == sc::modComp ? "cmp" : (m == sc::modGate ? "gt" : "ds");
    onBtn  = std::make_unique<Toggle> (s, pre + "_on", sc::moduleName (m), true);
    extBtn = std::make_unique<Toggle> (s, pre + "_ext", "EXT SC", true);
    mode   = std::make_unique<Radio> (s, pre + "_mode");
    addAndMakeVisible (*onBtn); addAndMakeVisible (*extBtn); addAndMakeVisible (*mode);
    if (m != sc::modDs2)
    {
        scfBtn    = std::make_unique<Toggle> (s, pre + "_scf", "SC FILTERS", true);
        listenBtn = std::make_unique<Toggle> (s, pre + "_listen", "SC LISTEN", true);
        addAndMakeVisible (*scfBtn); addAndMakeVisible (*listenBtn);
    }
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);

    if (m == sc::modComp)
    {
        lsOn = std::make_unique<Toggle> (s, "cmp_ls_on", "LOW SHELF", true);
        bOn  = std::make_unique<Toggle> (s, "cmp_bell_on", "BELL", true);
        hOn  = std::make_unique<Toggle> (s, "cmp_hp_on", "HPF", true);
        lsG = std::make_unique<Knob> (s, "cmp_ls_gain", "GAIN");  lsF = std::make_unique<Knob> (s, "cmp_ls_freq", "FREQ");
        bG  = std::make_unique<Knob> (s, "cmp_bell_gain", "GAIN"); bF = std::make_unique<Knob> (s, "cmp_bell_freq", "FREQ");
        hF  = std::make_unique<Knob> (s, "cmp_hp_freq", "FREQ");
        for (juce::Component* c : std::initializer_list<juce::Component*> { lsOn.get(), bOn.get(), hOn.get(), lsG.get(), lsF.get(), bG.get(), bF.get(), hF.get() })
            addAndMakeVisible (c);
        groupNames = { "SIDECHAIN LOW SHELF", "SIDECHAIN BELL", "SIDECHAIN HPF" };
    }
    else if (m == sc::modGate)
    {
        gHp = std::make_unique<Knob> (s, "gt_hp", "SC HP");
        gLp = std::make_unique<Knob> (s, "gt_lp", "SC LP");
        gLink = std::make_unique<Toggle> (s, "gt_link", "LINK HP/LP", true);
        for (juce::Component* c : std::initializer_list<juce::Component*> { gHp.get(), gLp.get(), gLink.get() }) addAndMakeVisible (c);
        auto hpV = [this] { return proc.apvts.getRawParameterValue ("gt_hp")->load(); };
        auto lpV = [this] { return proc.apvts.getRawParameterValue ("gt_lp")->load(); };
        linkRatio = lpV() / juce::jmax (1.0f, hpV());
        gLink->onClick = [this, hpV, lpV] { if (gLink->getToggleState()) linkRatio = lpV() / juce::jmax (1.0f, hpV()); };
        auto setP = [this] (const char* id, float v)
        {
            auto* prm = proc.apvts.getParameter (id);
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture();
        };
        gHp->slider.onValueChange = [this, setP, hpV]
        {
            if (linking || ! gLink->getToggleState()) return;
            linking = true; setP ("gt_lp", juce::jlimit (20.0f, 15000.0f, hpV() * linkRatio)); linking = false;
        };
        gLp->slider.onValueChange = [this, setP, lpV]
        {
            if (linking || ! gLink->getToggleState()) return;
            linking = true; setP ("gt_hp", juce::jlimit (20.0f, 15000.0f, lpV() / linkRatio)); linking = false;
        };
        groupNames = { "SIDECHAIN FILTER (BAND-PASS)" };
    }
    else
    {
        dsType1 = std::make_unique<Radio> (s, "ds1_type"); dsType2 = std::make_unique<Radio> (s, "ds2_type");
        dsLis1 = std::make_unique<Toggle> (s, "ds1_listen", "LISTEN", true); dsLis2 = std::make_unique<Toggle> (s, "ds2_listen", "LISTEN", true);
        dsF1 = std::make_unique<Knob> (s, "ds1_freq", "FREQ"); dsT1 = std::make_unique<Knob> (s, "ds1_thr", "THRESHOLD");
        dsF2 = std::make_unique<Knob> (s, "ds2_freq", "FREQ"); dsT2 = std::make_unique<Knob> (s, "ds2_thr", "THRESHOLD");
        for (juce::Component* c : std::initializer_list<juce::Component*> { dsType1.get(), dsType2.get(), dsLis1.get(), dsLis2.get(), dsF1.get(), dsT1.get(), dsF2.get(), dsT2.get() })
            addAndMakeVisible (c);
        groupNames = { "DS 1 - SIDECHAIN SHAPE", "DS 2 - SIDECHAIN SHAPE" };
    }
}

void ExpandedView::mouseDown (const juce::MouseEvent& e)
{
    if (! panel.contains (e.getPosition()) && onClose) onClose();
}

void ExpandedView::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.35f));
    auto r = panel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.fillRoundedRectangle (r.translated (0, 4), 8.0f);
    juce::ColourGradient bg (pal::panelTop, r.getX(), r.getY(), pal::panelBot, r.getX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (pal::navy);
    g.drawRoundedRectangle (r, 8.0f, 1.5f);
    auto title = r.removeFromTop (28.0f);
    g.setFont (scFont (12.0f));
    g.drawText (juce::String (sc::moduleName (moduleId)) + "  -  EXPANDED VIEW", title.reduced (12, 0), juce::Justification::centredLeft);
    g.setColour (pal::textDim);
    g.setFont (scFont (9.5f));
    g.drawText ("STEREO MODE", mode->getBounds().translated (0, -13).withHeight (12), juce::Justification::centred);
    for (int i = 0; i < groupNames.size() && i < 3; ++i)
        drawSectionTitle (g, groupTitles[i], groupNames[i]);
}

void ExpandedView::resized()
{
    panel = getLocalBounds().withSizeKeepingCentre (620, 330);
    auto r = panel.reduced (12, 0);
    auto title = r.removeFromTop (28);
    close.setBounds (title.removeFromRight (90).reduced (0, 4));
    r.removeFromTop (16);
    auto top = r.removeFromTop (26);
    onBtn->setBounds (top.removeFromLeft (80).reduced (2, 0));
    top.removeFromLeft (10);
    mode->setBounds (top.removeFromLeft (150));
    top.removeFromLeft (10);
    if (scfBtn) { scfBtn->setBounds (top.removeFromLeft (110).reduced (2, 0)); top.removeFromLeft (4); }
    extBtn->setBounds (top.removeFromLeft (90).reduced (2, 0));
    if (listenBtn) { top.removeFromLeft (4); listenBtn->setBounds (top.removeFromLeft (110).reduced (2, 0)); }
    r.removeFromTop (14);

    if (moduleId == sc::modComp)
    {
        const int w = r.getWidth() / 3;
        Toggle* ons[3] = { lsOn.get(), bOn.get(), hOn.get() };
        Knob* gains[3] = { lsG.get(), bG.get(), nullptr };
        Knob* freqs[3] = { lsF.get(), bF.get(), hF.get() };
        for (int i = 0; i < 3; ++i)
        {
            auto g = r.removeFromLeft (w).reduced (6, 0);
            groupTitles[i] = g.removeFromTop (14);
            ons[i]->setBounds (g.removeFromTop (24).withSizeKeepingCentre (110, 22));
            g.removeFromTop (6);
            auto k = g.removeFromTop (100);
            if (gains[i]) { placeKnob (*gains[i], k.removeFromLeft (k.getWidth() / 2), 84, 96); placeKnob (*freqs[i], k, 84, 96); }
            else placeKnob (*freqs[i], k, 84, 96);
        }
    }
    else if (moduleId == sc::modGate)
    {
        auto g = r.withSizeKeepingCentre (320, r.getHeight());
        groupTitles[0] = g.removeFromTop (14);
        g.removeFromTop (6);
        auto k = g.removeFromTop (110);
        placeKnob (*gHp, k.removeFromLeft (110), 96, 106);
        placeKnob (*gLp, k.removeFromRight (110), 96, 106);
        gLink->setBounds (k.withSizeKeepingCentre (96, 24));
    }
    else
    {
        const int w = r.getWidth() / 2;
        Radio* ty[2] = { dsType1.get(), dsType2.get() };
        Toggle* li[2] = { dsLis1.get(), dsLis2.get() };
        Knob* fq[2] = { dsF1.get(), dsF2.get() };
        Knob* th[2] = { dsT1.get(), dsT2.get() };
        for (int i = 0; i < 2; ++i)
        {
            auto g = r.removeFromLeft (w).reduced (10, 0);
            groupTitles[i] = g.removeFromTop (14);
            g.removeFromTop (4);
            ty[i]->setBounds (g.removeFromTop (24));
            g.removeFromTop (6);
            auto k = g.removeFromTop (100);
            placeKnob (*fq[i], k.removeFromLeft (k.getWidth() / 2), 90, 96);
            placeKnob (*th[i], k, 90, 96);
            li[i]->setBounds (g.removeFromTop (24).withSizeKeepingCentre (100, 22));
        }
    }
}
} // namespace ui
