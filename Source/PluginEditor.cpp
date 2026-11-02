#include "PluginEditor.h"

SilverChannelEditor::SilverChannelEditor (SilverChannelProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (main);

    panels[sc::modPre]    = std::make_unique<ui::PrePanel> (p);
    panels[sc::modGate]   = std::make_unique<ui::GatePanel> (p);
    panels[sc::modDs2]    = std::make_unique<ui::Ds2Panel> (p);
    panels[sc::modEq]     = std::make_unique<ui::EqPanel> (p);
    panels[sc::modComp]   = std::make_unique<ui::CompPanel> (p);
    panels[sc::modInsert] = std::make_unique<ui::InsertPanel> (p);
    for (auto& pn : panels)
    {
        pn->onExpand = [this] (int m) { showExpanded (m); };
        main.addAndMakeVisible (*pn);
    }
    master = std::make_unique<ui::MasterPanel> (p);
    main.addAndMakeVisible (*master);

    for (auto* b : { &loadBtn, &saveBtn, &undoBtn, &redoBtn, &presetLbl }) main.addAndMakeVisible (b);
    loadBtn.onClick = [this] { showLoadMenu(); };
    presetLbl.onClick = [this] { showLoadMenu(); };
    saveBtn.onClick = [this] { showSaveMenu(); };
    undoBtn.onClick = [this] { proc.undo.undo(); };
    redoBtn.onClick = [this] { proc.undo.redo(); };
    undoBtn.setTooltip ("Desfazer (ate 32 niveis)");
    redoBtn.setTooltip ("Refazer");
    presetLbl.getProperties().set ("flat", false);

    const int sizes[] = { 70, 80, 90, 100, 115, 130, 150 };
    for (int i = 0; i < 7; ++i) sizeBox.addItem (juce::String (sizes[i]) + "%", sizes[i]);
    sizeBox.onChange = [this]
    {
        const float sc = sizeBox.getSelectedId() / 100.0f;
        setSize ((int) std::round (baseW * sc), (int) std::round (baseH * sc));
    };
    main.addAndMakeVisible (sizeBox);

    proc.apvts.state.addListener (this);
    addMouseListener (this, true);

    setResizable (true, true);
    setResizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / baseH);
    const float s0 = juce::jlimit (0.6f, 2.0f, proc.uiScale);
    setSize ((int) std::round (baseW * s0), (int) std::round (baseH * s0));
    layoutModules();
    startTimerHz (10);
}

SilverChannelEditor::~SilverChannelEditor()
{
    removeMouseListener (this);
    proc.apvts.state.removeListener (this);
    expanded.reset();
    setLookAndFeel (nullptr);
}

void SilverChannelEditor::resized()
{
    const float sc = getWidth() / (float) baseW;
    proc.uiScale = sc;
    main.setTransform (juce::AffineTransform::scale (sc));
    main.setBounds (0, 0, baseW, baseH);
    const int pct = (int) std::round (sc * 100.0f);
    sizeBox.setText (juce::String (pct) + "%", juce::dontSendNotification);
}

void SilverChannelEditor::mouseDown (const juce::MouseEvent&)
{
    proc.undo.beginNewTransaction(); // cada gesto do mouse vira um passo de undo
}

void SilverChannelEditor::layoutModules()
{
    // barra superior
    auto top = juce::Rectangle<int> (0, 0, baseW, 46).reduced (12, 9);
    top.removeFromLeft (250); // logo
    loadBtn.setBounds (top.removeFromLeft (64)); top.removeFromLeft (6);
    saveBtn.setBounds (top.removeFromLeft (64)); top.removeFromLeft (10);
    presetLbl.setBounds (top.removeFromLeft (300)); top.removeFromLeft (10);
    undoBtn.setBounds (top.removeFromLeft (60)); top.removeFromLeft (6);
    redoBtn.setBounds (top.removeFromLeft (60));
    sizeBox.setBounds (top.removeFromRight (84));

    // faixa de modulos na ordem atual
    const auto order = proc.getOrder();
    int x = 10;
    const int y = 54, h = baseH - y - 10;
    for (int i = 0; i < sc::numModules; ++i)
    {
        auto& pn = panels[(size_t) order[(size_t) i]];
        pn->setBounds (x, y, pn->preferredWidth(), h);
        x += pn->preferredWidth() + 6;
    }
    master->setBounds (x + 6, y, baseW - x - 16, h);
    if (expanded) expanded->setBounds (0, 0, baseW, baseH);
    main.repaint();
}

void SilverChannelEditor::Main::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (pal::bgTop, 0, 0, pal::bgBottom, 0, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();
    // escovado sutil
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int yy = 0; yy < getHeight(); yy += 3) g.drawHorizontalLine (yy, 0.0f, (float) getWidth());

    // barra superior
    auto bar = juce::Rectangle<float> (0, 0, (float) getWidth(), 46.0f);
    juce::ColourGradient bg2 (juce::Colour (0xfff3f5f7), 0, 0, juce::Colour (0xffc6ccd3), 0, 46.0f, false);
    g.setGradientFill (bg2);
    g.fillRect (bar);
    g.setColour (pal::panelEdge);
    g.drawHorizontalLine (46, 0.0f, (float) getWidth());

    g.setColour (pal::navy);
    g.setFont (scFont (21.0f));
    g.drawText ("SILVER", 16, 6, 100, 34, juce::Justification::centredLeft);
    g.setColour (pal::accent);
    g.setFont (scFont (21.0f, false));
    g.drawText ("CHANNEL", 98, 6, 110, 34, juce::Justification::centredLeft);
    g.setColour (pal::navy);
    auto badge = juce::Rectangle<float> (207, 13, 34, 20);
    g.fillRoundedRectangle (badge, 3.0f);
    g.setColour (juce::Colours::white);
    g.setFont (scFont (12.0f));
    g.drawText ("01", badge, juce::Justification::centred);

    // indicadores
    g.setFont (scFont (10.0f));
    auto ind = juce::Rectangle<int> (getWidth() - 230, 14, 120, 18);
    g.setColour (ed.proc.hasSidechain.load() ? pal::accent : pal::textDim.withAlpha (0.6f));
    g.drawText (ed.proc.hasSidechain.load() ? "EXT SC: CONECTADO" : "EXT SC: --", ind.removeFromLeft (120), juce::Justification::centredRight);
}

void SilverChannelEditor::timerCallback()
{
    const auto name = proc.presets.currentName;
    if (name != shownPreset) { shownPreset = name; presetLbl.setButtonText (name); }
    undoBtn.setEnabled (proc.undo.canUndo());
    redoBtn.setEnabled (proc.undo.canRedo());
    main.repaint (getWidth() - 260, 0, 140, 46);
}

void SilverChannelEditor::valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier& id)
{
    if (t == proc.apvts.state && id.toString() == "order")
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SilverChannelEditor> (this)]
                                         { if (safe) safe->layoutModules(); });
}

void SilverChannelEditor::showExpanded (int moduleId)
{
    expanded = std::make_unique<ui::ExpandedView> (proc, moduleId);
    expanded->onClose = [this]
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<SilverChannelEditor> (this)]
                                         { if (safe) safe->expanded.reset(); });
    };
    main.addAndMakeVisible (*expanded);
    expanded->setBounds (0, 0, baseW, baseH);
}

void SilverChannelEditor::showLoadMenu()
{
    juce::PopupMenu m, fac, usr;
    const auto& f = sc::PresetManager::factory();
    for (int i = 0; i < (int) f.size(); ++i) fac.addItem (1000 + i, f[(size_t) i].name);
    const auto files = proc.presets.userPresets();
    for (int i = 0; i < files.size(); ++i) usr.addItem (2000 + i, files[i].getFileNameWithoutExtension());
    m.addSubMenu ("Factory", fac);
    m.addSubMenu ("User", usr, files.size() > 0);
    m.addItem (3, "Abrir arquivo de preset...");
    m.addSeparator();
    m.addItem (1, "Full Reset (padrao de fabrica)");
    juce::PopupMenu del;
    for (int i = 0; i < files.size(); ++i) del.addItem (3000 + i, files[i].getFileNameWithoutExtension());
    m.addSubMenu ("Apagar preset de usuario", del, files.size() > 0);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&loadBtn), [this, files] (int r)
    {
        if (r == 1) proc.presets.resetToDefaults();
        else if (r >= 1000 && r < 2000) proc.presets.loadFactory (r - 1000);
        else if (r >= 2000 && r < 3000) proc.presets.loadFile (files[r - 2000]);
        else if (r >= 3000) proc.presets.deleteFile (files[r - 3000]);
        else if (r == 3)
        {
            chooser = std::make_unique<juce::FileChooser> ("Abrir preset", sc::PresetManager::presetDir(), "*.scpreset");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.presets.loadFile (fc.getResult()); });
        }
    });
}

void SilverChannelEditor::showSaveMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Salvar como...");
    m.addItem (2, "Definir como preset padrao (Set as Default)");
    m.addItem (3, "Abrir pasta de presets");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&saveBtn), [this] (int r)
    {
        if (r == 1)
        {
            auto* aw = new juce::AlertWindow ("Salvar preset", "Nome do preset:", juce::MessageBoxIconType::NoIcon);
            aw->addTextEditor ("name", proc.presets.currentName);
            aw->addButton ("Salvar", 1, juce::KeyPress (juce::KeyPress::returnKey));
            aw->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
            aw->setLookAndFeel (&lnf);
            aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int res)
            {
                if (res == 1) proc.presets.saveUser (aw->getTextEditorContents ("name"));
            }), true);
        }
        else if (r == 2) proc.presets.setAsDefault();
        else if (r == 3) sc::PresetManager::presetDir().startAsProcess();
    });
}
