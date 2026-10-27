#include "PluginEditor.h"
#include "UI.h"

using namespace juce;
using namespace orymy;
using namespace orymy::ui;

namespace
{
constexpr float kW = 1536.0f, kH = 1024.0f;

struct KnobSpec { const char* id; float cx, cy, r; const char* mn; const char* mx; const char* name; float yMin; float yName; bool above; };
const KnobSpec kKnobs[] = {
    { "gateThr",   70, 650, 30, "-60 dB", "0 dB",   "THRESHOLD", 686, 704, false },
    { "gateRange", 190, 651, 24, "-80 dB", "0 dB",  "RANGE",     686, 704, false },
    { "compThr",   75, 807, 30, "-40 dB", "0 dB",   "THRESHOLD", 842, 861, false },
    { "compRatio", 167, 811, 22, "1:1",   "20:1",   "RATIO",     842, 861, false },
    { "compMakeup",257, 811, 22, "0 dB",  "20 dB",  "MAKEUP",    842, 861, false },
    { "tone",      401, 821, 30, "0",     "10",     "TONE",      857, 872, false },
    { "splitFreq", 561, 821, 28, "20 Hz", "20 kHz", "FREQ",      857, 872, false },
    { "subLevel",  719, 821, 28, "0",     "10",     "LEVEL",     857, 872, false },
    { "gain",      1207, 500, 25, "0",    "10",     "GAIN",      529, 461, true  },
    { "master",    1300, 499, 25, "0",    "10",     "MASTER",    529, 461, true  },
    { "cabMix",    1437, 500, 25, "0",    "100",    "MIX",       529, 462, true  },
    { "drive",     1004, 662, 28, "0",    "10",     "",          698, 0,   false },
    { "modAmt",    1137, 661, 28, "0",    "10",     "",          697, 0,   false },
    { "delayMs",   1287, 661, 28, "0 ms", "1000 ms","",          697, 0,   false },
    { "reverbMix", 1438, 661, 28, "0",    "100",    "",          698, 0,   false },
};

// ------------------------------------------------------------------ fondo estatico
class Backdrop : public Component
{
public:
    Image eye = ImageCache::getFromMemory (BinaryData::logo_eye_png, BinaryData::logo_eye_pngSize);
    Image ymy = ImageCache::getFromMemory (BinaryData::logo_ymy_png, BinaryData::logo_ymy_pngSize);
    Image head = ImageCache::getFromMemory (BinaryData::headstock_png, BinaryData::headstock_pngSize);

    Backdrop() { setOpaque (true); setBufferedToImage (true); setInterceptsMouseClicks (false, false); }

    static void panel (Graphics& g, float x, float y, float w, float h, float rad = 10.0f)
    {
        const Rectangle<float> r (x, y, w, h);
        g.setColour (Colours::black.withAlpha (0.07f)); g.fillRoundedRectangle (r.translated (0, 2.5f), rad);
        g.setGradientFill (ColourGradient (Colour (0xffffffff), 0, y, Colour (0xfff6f6f4), 0, y + h, false)); g.fillRoundedRectangle (r, rad);
        g.setColour (cBorder); g.drawRoundedRectangle (r.reduced (0.5f), rad, 1.0f);
    }
    static void title (Graphics& g, const String& t, float x, float cy) { g.setColour (cInk); drawSpaced (g, t, x, cy, 1.6f, 0, F (12.5f, true)); }
    static void lbl (Graphics& g, const String& t, float cx, float cy, bool bold = false, float sz = 10.5f, Colour c = cSub)
    { g.setColour (c); g.setFont (F (sz, bold)); g.drawText (t, Rectangle<float> (cx - 60, cy - 9, 120, 18), Justification::centred, false); }

    void paint (Graphics& g) override
    {
        // placa exterior plateada
        g.setGradientFill (ColourGradient (Colour (0xffcfcfcd), 0, 0, Colour (0xffe9e9e7), 0, kH, false)); g.fillAll();
        g.setColour (Colours::white.withAlpha (0.6f)); g.drawRect (0.0f, 0.0f, kW, kH, 2.0f);
        const Rectangle<float> win (12, 30, 1512, 965);
        g.setColour (Colours::black.withAlpha (0.12f)); g.fillRoundedRectangle (win.translated (0, 3), 14.0f);
        g.setGradientFill (ColourGradient (Colour (0xfff4f4f2), 0, 30, Colour (0xffe7e7e5), 0, 995, false)); g.fillRoundedRectangle (win, 14.0f);
        g.setColour (Colour (0xffbfbfbd)); g.drawRoundedRectangle (win, 14.0f, 1.4f);

        // --- cabecera
        g.setColour (Colours::black); g.setOpacity (1.0f); g.drawImage (eye, Rectangle<float> (28, 40, 56, 44));
        g.setColour (cInk); drawSpaced (g, "O R Y M Y", 102, 58, 7.0f, 0, F (19.0f, true));
        g.setColour (Colour (0xffe1e1df)); g.fillRoundedRectangle (250, 50, 42, 17, 3.0f);
        g.setColour (Colour (0xffb8b8b6)); g.drawRoundedRectangle (250, 50, 42, 17, 3.0f, 1.0f);
        g.setColour (cInk); drawSpaced (g, "BASS", 271, 58.5f, 1.5f, 1, F (10.0f, true));
        g.setColour (Colour (0xffbdbdbb)); g.drawHorizontalLine (62, 338, 545); g.drawHorizontalLine (62, 990, 1125);
        g.setColour (cInk); drawSpaced (g, "YMY  SOUNDS   VST", 770, 59, 5.0f, 1, F (12.0f, true));
        g.setColour (Colours::black); g.drawImage (ymy, Rectangle<float> (1152, 36, 118, 67));
        g.setColour (cSub); const char* t3[] = { "EL SONIDO", "DE LA BACHATA", "Y EL MERENGUE" };
        for (int i = 0; i < 3; ++i) drawSpaced (g, t3[i], 1307, 51.0f + 13.0f * (float) i, 1.4f, 0, F (9.0f, true));

        // --- presets
        panel (g, 24, 100, 285, 482); title (g, "PRESETS", 34, 119);
        // --- noise gate / compresor
        panel (g, 24, 590, 282, 146); title (g, "NOISE GATE", 34, 603);
        panel (g, 24, 745, 282, 145); title (g, "COMPRESOR", 34, 757);
        // --- ilustracion
        panel (g, 316, 94, 614, 652, 11);
        // --- fila inferior
        panel (g, 335, 757, 473, 131); g.setColour (cBorder); g.drawVerticalLine (505, 764, 882); g.drawVerticalLine (667, 764, 882);
        title (g, "BASS TONE", 348, 771); title (g, "SPLIT", 516, 772); title (g, "SUB", 677, 772);
        panel (g, 818, 757, 399, 131); g.setColour (cInk); drawSpaced (g, "PLAYING", 844, 772, 1.6f, 0, F (12.5f, true));
        g.setColour (cAmber); g.fillEllipse (827, 767, 9, 9); g.setColour (cInk);
        g.setColour (cSub); drawSpaced (g, "ESCALA ACTUAL", 828, 849, 1.0f, 0, F (9.0f, true));
        panel (g, 1229, 757, 283, 131); title (g, "NOTAS", 1250, 770);
        // --- EQ
        panel (g, 941, 111, 571, 265); title (g, "GRAPHIC EQ", 955, 131);
        g.setColour (Colour (0xfff2f2f0)); g.fillRoundedRectangle (985, 148, 415, 212, 8.0f);
        g.setColour (Colour (0xffd0d0ce)); g.drawRoundedRectangle (985.5f, 148.5f, 414, 211, 8.0f, 1.0f);
        const float gy[5] = { 173, 212, 251, 289, 327 }; const char* gl[5] = { "+12", "+6", "0", "-6", "-12" };
        for (int i = 0; i < 5; ++i)
        {
            g.setColour (Colour (0xffd9d9d7)); g.drawHorizontalLine ((int) gy[i], 992, 1394);
            lbl (g, gl[i], 966, gy[i], false, 10.0f);
        }
        for (int i = 0; i < kNumEq; ++i) lbl (g, kEqLabels[i], 1007.0f + 36.5f * (float) i, 349, false, 10.0f);
        // --- ampli / cabina
        panel (g, 941, 388, 411, 162); title (g, "AMP", 955, 399);
        panel (g, 1360, 388, 152, 162); title (g, "CABINET", 1370, 400);
        // --- efectos
        panel (g, 941, 562, 571, 183); title (g, "EFFECTS", 955, 577);
        g.setColour (cBorder); g.drawVerticalLine (1062, 590, 735); g.drawVerticalLine (1217, 590, 735); g.drawVerticalLine (1365, 590, 735);
        g.setColour (cInk); drawSpaced (g, "DRIVE", 1003, 612, 1.4f, 1, F (11.5f, true));
        drawSpaced (g, "MODULATION", 1124, 611, 1.4f, 1, F (11.5f, true));
        drawSpaced (g, "DELAY", 1275, 611, 1.4f, 1, F (11.5f, true));
        drawSpaced (g, "REVERB", 1425, 611, 1.4f, 1, F (11.5f, true));
        // --- perillas: escalas y nombres
        for (auto& k : kKnobs)
        {
            lbl (g, k.mn, k.cx - k.r * 0.75f, k.yMin, false, 9.5f);
            lbl (g, k.mx, k.cx + k.r * 0.85f, k.yMin, false, 9.5f);
            if (k.name[0]) lbl (g, k.name, k.cx, k.yName, ! k.above, k.above ? 10.5f : 10.5f, k.above ? cInk : cSub);
        }
        // --- cuello del bajo
        g.setColour (Colours::black); g.drawImage (head, Rectangle<float> (50, 896, 78, 76));
        g.setColour (cSub); drawSpaced (g, "ORYMY BASS", 1415, 926, 3.0f, 1, F (11.5f, true));
        drawSpaced (g, "YMY SOUNDS VST", 1420, 951, 3.0f, 1, F (10.5f, true));
        g.setColour (cInk); Path crown; crown.startNewSubPath (1478, 930); crown.lineTo (1481, 918); crown.lineTo (1486, 925); crown.lineTo (1491, 918); crown.lineTo (1494, 930);
        g.strokePath (crown, PathStrokeType (1.6f)); g.drawLine (1478, 931, 1494, 931, 1.6f);
    }
};
}

// ============================================================================
class OrymyBassEditor::Content : public Component, private Timer
{
public:
    explicit Content (OrymyBassAudioProcessor& p) : proc (p), apvts (p.apvts)
    {
        setLookAndFeel (&lf);
        addAndMakeVisible (backdrop);

        // ---------- vistas dinamicas (debajo de los controles)
        art = std::make_unique<ArtView> (ui);
        art->onClick = [this] { ui.artLock = ! ui.artLock; };
        addAndMakeVisible (*art);
        add (spectrum, std::make_unique<SpectrumView> (ui));
        add (scope, std::make_unique<ScopeView> (ui));
        add (hdr, std::make_unique<HeaderBars> (ui));
        add (ampHead, std::make_unique<AmpHeadView> (ui));
        add (speaker, std::make_unique<SpeakerView> (ui));
        add (fret, std::make_unique<FretboardView> (ui));
        add (playing, std::make_unique<PlayingView> (ui));
        add (staff, std::make_unique<StaffView> (ui));
        add (ledGate, std::make_unique<LedView> (ui, cAmber));
        add (ledComp, std::make_unique<LedView> (ui, cRed));

        // ---------- perillas
        for (auto& k : kKnobs)
        {
            auto kn = std::make_unique<Knob>();
            const float side = (k.r + 7.0f) * 2.0f;
            kn->setBounds (Rectangle<float> (k.cx - side / 2, k.cy - side / 2, side, side).toNearestInt());
            addAndMakeVisible (*kn);
            sliderAtt.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, k.id, *kn));
            knobs[k.id] = std::move (kn);
        }

        // ---------- botones de encendido
        addPower ("gateOn", 287, 606, 22); addPower ("compOn", 286, 761, 22);
        addPower ("modOn", 1194, 610, 22); addPower ("delayOn", 1344, 610, 22); addPower ("reverbOn", 1491, 610, 22);
        addSwitch ("toneOn", 474); addSwitch ("splitOn", 634); addSwitch ("subOn", 779);

        // ---------- desplegables
        addCombo ("ampModel", 1166, 417, 182, 26, StringArray { "Bass Amp", "Vintage Tube", "Modern Clean", "Grit" });
        addCombo ("cabModel", 1415, 423, 82, 22, StringArray { "4x10 Bass", "1x15 Vintage", "2x10 Tight", "8x10 Stack", "Direct (DI)" });
        addCombo ("modType", 1073, 711, 129, 24, StringArray { "Chorus", "Flanger", "Phaser", "Tremolo" })->setComponentID ("modcombo");
        addCombo ("delaySync", 1229, 711, 121, 25, StringArray { "Libre", "1/4", "1/8", "1/8 Dotted", "1/16" });
        addCombo ("reverbType", 1380, 711, 117, 24, StringArray { "Room", "Hall", "Plate", "Spring" });
        modCombo = combos["modType"].get();

        // ---------- EQ grafico
        for (int i = 0; i < kNumEq; ++i)
        {
            auto s = std::make_unique<EqSlider>();
            s->setBounds (Rectangle<float> (1007.0f + 36.5f * (float) i - 14.0f, 166.0f, 28.0f, 168.0f).toNearestInt());
            addAndMakeVisible (*s);
            sliderAtt.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (apvts, kEqIds[i], *s));
            s->onDragStart = [this] { if (eqModeNow() != 3) setEqMode (3, false); };
            eq[(size_t) i] = std::move (s);
        }
        const char* names[4] = { "AUTO", "BACHATA", "MERENGUE", "CUSTOM" }; const int ys[4] = { 149, 191, 228, 265 };
        for (int i = 0; i < 4; ++i)
        {
            modeBtn[i] = std::make_unique<FlatButton> (names[i]);
            modeBtn[i]->setBounds (1411, ys[i], 90, 27);
            modeBtn[i]->onClick = [this, i] { setEqMode (i, true); };
            addAndMakeVisible (*modeBtn[i]);
        }
        addAndMakeVisible (*scope); scope->setBounds (1418, 314, 80, 42);

        // ---------- presets
        presetList.onSelect = [this] (int i) { proc.loadPreset (i); };
        presetList.setBounds (28, 134, 276, 396);
        addAndMakeVisible (presetList);
        prevBtn = std::make_unique<FlatButton> ("", 1); nextBtn = std::make_unique<FlatButton> ("", 2);
        saveBtn = std::make_unique<FlatButton> ("Save Preset", 3);
        prevBtn->setBounds (34, 535, 30, 24); nextBtn->setBounds (66, 535, 30, 24); saveBtn->setBounds (178, 535, 118, 24);
        const int n = (int) presets().size();
        prevBtn->onClick = [this, n] { proc.loadPreset ((proc.getCurrentPreset() + n - 1) % n); };
        nextBtn->onClick = [this, n] { proc.loadPreset ((proc.getCurrentPreset() + 1) % n); };
        saveBtn->onClick = [this] { saveFlash = proc.saveUserPreset() ? 45 : -45; };
        addAndMakeVisible (*prevBtn); addAndMakeVisible (*nextBtn); addAndMakeVisible (*saveBtn);

        // ---------- posicion de las vistas
        art->setBounds (317, 95, 612, 650);
        spectrum->setBounds (985, 148, 415, 212);
        hdr->setBounds (1408, 46, 34, 28);
        ampHead->setBounds (948, 420, 204, 112);
        speaker->setBounds (1368, 420, 40, 40);
        fret->setBounds (130, 905, 1185, 70);
        playing->setBounds (818, 757, 399, 131);
        staff->setBounds (1240, 785, 270, 100);
        ledGate->setBounds (110, 613, 26, 26); ledComp->setBounds (128, 749, 24, 24);

        // el orden z: backdrop, art, spectrum... ya anadidos antes que los controles
        setSize ((int) kW, (int) kH);
        startTimerHz (30);
    }

    ~Content() override { stopTimer(); setLookAndFeel (nullptr); }

    void resized() override { backdrop.setBounds (getLocalBounds()); }

private:
    OrymyBassAudioProcessor& proc;
    AudioProcessorValueTreeState& apvts;
    OrymyLF lf;
    UIState ui;
    Backdrop backdrop;

    std::unique_ptr<ArtView> art;
    std::unique_ptr<SpectrumView> spectrum; std::unique_ptr<ScopeView> scope; std::unique_ptr<HeaderBars> hdr;
    std::unique_ptr<AmpHeadView> ampHead; std::unique_ptr<SpeakerView> speaker; std::unique_ptr<FretboardView> fret;
    std::unique_ptr<PlayingView> playing; std::unique_ptr<StaffView> staff;
    std::unique_ptr<LedView> ledGate, ledComp;
    std::map<std::string, std::unique_ptr<Knob>> knobs;
    std::map<std::string, std::unique_ptr<ComboBox>> combos;
    std::vector<std::unique_ptr<PowerBtn>> powers;
    std::array<std::unique_ptr<EqSlider>, (size_t) kNumEq> eq;
    std::array<std::unique_ptr<FlatButton>, 4> modeBtn;
    PresetListView presetList;
    std::unique_ptr<FlatButton> prevBtn, nextBtn, saveBtn;
    ComboBox* modCombo = nullptr;
    int saveFlash = 0, holdFrames = 0;

    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> sliderAtt;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>> buttonAtt;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtt;

    template <typename T> void add (std::unique_ptr<T>& dst, std::unique_ptr<T> v) { dst = std::move (v); addAndMakeVisible (*dst); }

    void addPower (const char* id, float cx, float cy, float size)
    {
        auto b = std::make_unique<PowerBtn>();
        b->setBounds (Rectangle<float> (cx - size / 2, cy - size / 2, size, size).toNearestInt());
        addAndMakeVisible (*b);
        buttonAtt.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *b));
        powers.push_back (std::move (b));
    }
    void addSwitch (const char* id, float cx)
    {
        auto b = std::make_unique<PowerBtn> ("vswitch");
        b->setBounds ((int) cx - 12, 799, 24, 48);
        addAndMakeVisible (*b);
        buttonAtt.push_back (std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *b));
        powers.push_back (std::move (b));
    }
    ComboBox* addCombo (const char* id, int x, int y, int w, int h, const StringArray& items)
    {
        auto c = std::make_unique<ComboBox>();
        c->addItemList (items, 1); c->setBounds (x, y, w, h); c->setJustificationType (Justification::centredLeft);
        addAndMakeVisible (*c);
        comboAtt.push_back (std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, *c));
        auto* raw = c.get(); combos[id] = std::move (c);
        return raw;
    }

    int eqModeNow() const { return (int) apvts.getRawParameterValue ("eqMode")->load(); }
    void setParamReal (const char* id, float v)
    {
        if (auto* p = apvts.getParameter (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture(); }
    }
    void setEqMode (int m, bool applyCurve)
    {
        setParamReal ("eqMode", (float) m);
        if (! applyCurve) return;
        if (m == 1 || m == 2)
        {
            const auto& c = (m == 1) ? kEqBachata : kEqMerengue;
            for (int i = 0; i < kNumEq; ++i) setParamReal (kEqIds[i], c[(size_t) i]);
        }
    }

    void timerCallback() override
    {
        ui.inLevel = proc.inLevel.load(); ui.outLevel = proc.outLevel.load(); ui.gr = proc.compGr.load(); ui.gateOpen = proc.gateOpen.load();
        proc.outLevel.store (proc.outLevel.load() * 0.7f);
        ui.analyzer.update (proc.inRing, proc.outRing, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0);

        const auto& a = ui.analyzer;
        if (! a.voiced) ui.verdict = Verdict::Idle;
        else if (std::abs (a.cents) > 30.0f) ui.verdict = Verdict::OutOfTune;
        else if (a.keyConfident && ! pcInMajorScale (((a.midi % 12) + 12) % 12, a.keyMajor)) ui.verdict = Verdict::OutOfScale;
        else ui.verdict = Verdict::Good;

        const bool active = ui.inLevel > 0.004f || ui.outLevel > 0.006f;
        if (active) holdFrames = 45; else if (holdFrames > 0) --holdFrames;
        const float target = (active || holdFrames > 0 || ui.artLock) ? 1.0f : 0.14f;
        ui.artAlpha += (target - ui.artAlpha) * (target > ui.artAlpha ? 0.20f : 0.04f);

        // sincronizar controles con el estado del procesador
        presetList.selected = proc.getCurrentPreset(); presetList.repaint();
        const int em = eqModeNow();
        for (int i = 0; i < 4; ++i) { if (modeBtn[(size_t) i]->selected != (i == em)) { modeBtn[(size_t) i]->selected = (i == em); modeBtn[(size_t) i]->repaint(); } }
        const bool synced = (int) apvts.getRawParameterValue ("delaySync")->load() != 0;
        if (knobs["delayMs"]->isEnabled() == synced) knobs["delayMs"]->setEnabled (! synced);
        const bool modOn = apvts.getRawParameterValue ("modOn")->load() > 0.5f;
        if ((bool) modCombo->getProperties().getWithDefault ("lit", false) != modOn) { modCombo->getProperties().set ("lit", modOn); modCombo->repaint(); }
        if (saveFlash != 0)
        {
            saveBtn->text = saveFlash > 0 ? String::fromUTF8 ("¡Guardado!") : String ("Error"); saveBtn->repaint();
            saveFlash += saveFlash > 0 ? -1 : 1;
            if (saveFlash == 0) { saveBtn->text = "Save Preset"; saveBtn->repaint(); }
        }
        ledGate->v = ui.gateOpen && ui.inLevel > 0.002f ? 1.0f : 0.0f;
        ledComp->v = jlimit (0.0f, 1.0f, ui.gr / 10.0f);

        for (Component* c : { (Component*) art.get(), (Component*) spectrum.get(), (Component*) scope.get(), (Component*) hdr.get(),
                              (Component*) ampHead.get(), (Component*) speaker.get(), (Component*) fret.get(), (Component*) playing.get(),
                              (Component*) staff.get(), (Component*) ledGate.get(), (Component*) ledComp.get() })
            c->repaint();
    }
};

// ============================================================================
OrymyBassEditor::OrymyBassEditor (OrymyBassAudioProcessor& p) : AudioProcessorEditor (&p)
{
    content = std::make_unique<Content> (p);
    addAndMakeVisible (*content);
    constrainer.setFixedAspectRatio ((double) kW / (double) kH);
    constrainer.setSizeLimits (768, 512, 2304, 1536);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (1152, 768);
}

OrymyBassEditor::~OrymyBassEditor() { content.reset(); }

void OrymyBassEditor::resized()
{
    if (content == nullptr) return;
    content->setTransform (AffineTransform::scale ((float) getWidth() / kW));
    content->setBounds (0, 0, (int) kW, (int) kH);
}
