#pragma once
#include "Common.h"
#include "Params.h"
#include "Analyzer.h"
#include <BinaryData.h>

// ============================================================================
//  ORYMY Bass  -  widgets de la interfaz (diseno base 1536 x 1024)
// ============================================================================
namespace orymy::ui
{
using namespace juce;

inline const Colour cBg (0xffeeeeec), cPanel (0xfffbfbfa), cBorder (0xffd3d3d1), cInk (0xff1a1a1a), cSub (0xff5c5c5c),
                    cAmber (0xffe8b21c), cGreen (0xff35d07f), cRed (0xffe5484d), cDark (0xff0e0e10);

inline Font F (float h, bool bold = false) { return Font (FontOptions (h, bold ? (int) Font::bold : (int) Font::plain)); }

inline float textWidth (const Font& f, const String& t)
{
    GlyphArrangement ga; ga.addLineOfText (f, t, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

// just: 0 izquierda, 1 centro, 2 derecha
inline void drawSpaced (Graphics& g, const String& s, float x, float cy, float sp, int just, const Font& f)
{
    g.setFont (f);
    Array<float> ws; float w = 0;
    for (auto c : s) { const float cw = textWidth (f, String::charToString (c)); ws.add (cw); w += cw + sp; }
    w -= sp;
    if (just == 1) x -= w * 0.5f; else if (just == 2) x -= w;
    const float h = f.getHeight() + 4.0f; int i = 0;
    for (auto c : s)
    {
        g.drawText (String::charToString (c), Rectangle<float> (x, cy - h * 0.5f, ws[i] + 4.0f, h), Justification::centredLeft, false);
        x += ws[i++] + sp;
    }
}

// ------------------------------------------------------------ estado compartido
enum class Verdict { Idle, Good, OutOfScale, OutOfTune };

struct UIState
{
    Analyzer analyzer;
    float inLevel = 0, outLevel = 0, gr = 0;
    bool gateOpen = true;
    float artAlpha = 0.14f; bool artLock = false;
    Verdict verdict = Verdict::Idle;

    Colour verdictColour() const
    {
        switch (verdict) { case Verdict::Good: return cGreen; case Verdict::OutOfScale: return cRed;
                           case Verdict::OutOfTune: return cAmber; default: return Colour (0xff9a9a9a); }
    }
    String verdictText() const
    {
        const auto& a = analyzer;
        String note = a.voiced && a.midi >= 0 ? String (kNoteNames[((a.midi % 12) + 12) % 12]) + String (a.midi / 12 - 1) : String();
        switch (verdict)
        {
            case Verdict::Good:       return note + String::fromUTF8 ("  ·  TOCANDO BIEN");
            case Verdict::OutOfScale: return note + String::fromUTF8 ("  ·  FUERA DE ESCALA");
            case Verdict::OutOfTune:  return note + String::fromUTF8 ("  ·  DESAFINADO ") + (a.cents > 0 ? "+" : "") + String ((int) std::lround (a.cents)) + String::fromUTF8 ("¢");
            default:                  return String::fromUTF8 ("ESPERANDO NOTA…");
        }
    }
};

// ------------------------------------------------------------ LookAndFeel
class OrymyLF : public LookAndFeel_V4
{
public:
    OrymyLF()
    {
        setColour (ComboBox::textColourId, cInk);
        setColour (PopupMenu::backgroundColourId, Colour (0xfff8f8f6));
        setColour (PopupMenu::textColourId, cInk);
        setColour (PopupMenu::highlightedBackgroundColourId, cAmber);
        setColour (PopupMenu::highlightedTextColourId, Colours::black);
        setColour (TooltipWindow::backgroundColourId, cDark);
        setColour (TooltipWindow::textColourId, Colours::white);
    }

    Font getComboBoxFont (ComboBox&) override { return F (13.5f); }
    Font getPopupMenuFont() override { return F (14.0f); }
    int getSliderThumbRadius (Slider&) override { return 7; }
    void positionComboBoxText (ComboBox& box, Label& label) override
    {
        label.setBounds (9, 0, box.getWidth() - 28, box.getHeight());
        label.setFont (getComboBoxFont (box));
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s) override
    {
        const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float cx = b.getCentreX(), cy = b.getCentreY();
        const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f, body = R * 0.80f;
        const bool on = s.isEnabled();
        const float ang = a0 + pos * (a1 - a0);

        // marcas
        g.setColour (Colour (0xff6d6d6d).withAlpha (on ? 0.9f : 0.4f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.0f, sn = std::sin (a), cs = -std::cos (a);
            g.drawLine (cx + sn * R * 0.90f, cy + cs * R * 0.90f, cx + sn * R, cy + cs * R, i % 5 == 0 ? 1.6f : 1.0f);
        }
        // arco de valor (luz)
        if (on)
        {
            Path arc; arc.addCentredArc (cx, cy, R * 0.86f, R * 0.86f, 0.0f, a0, ang, true);
            g.setColour (cAmber.withAlpha (0.28f)); g.strokePath (arc, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (cAmber); g.strokePath (arc, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        // sombra
        g.setColour (Colours::black.withAlpha (0.10f)); g.fillEllipse (cx - body, cy - body + 3.0f, body * 2, body * 2);
        g.setColour (Colours::black.withAlpha (0.10f)); g.fillEllipse (cx - body - 1, cy - body + 2.0f, body * 2 + 2, body * 2 + 2);
        // cuerpo metalico
        g.setGradientFill (ColourGradient (Colour (0xfffdfdfd), cx - body * 0.5f, cy - body * 0.6f, Colour (0xff9d9d9d), cx + body * 0.8f, cy + body, true));
        g.fillEllipse (cx - body, cy - body, body * 2, body * 2);
        g.setColour (Colour (0xff4a4a4a)); g.drawEllipse (cx - body, cy - body, body * 2, body * 2, 1.6f);
        const float fr = body * 0.78f;
        g.setGradientFill (ColourGradient (Colour (0xfff3f3f3), cx - fr * 0.4f, cy - fr * 0.6f, Colour (0xffc4c4c4), cx + fr * 0.6f, cy + fr * 0.8f, true));
        g.fillEllipse (cx - fr, cy - fr, fr * 2, fr * 2);
        g.setColour (Colours::white.withAlpha (0.55f)); g.drawEllipse (cx - fr, cy - fr, fr * 2, fr * 2, 1.0f);
        // aguja
        const float sn = std::sin (ang), cs = -std::cos (ang);
        g.setColour (on ? Colour (0xff1c1c1c) : Colour (0xff777777));
        g.drawLine (cx + sn * fr * 0.30f, cy + cs * fr * 0.30f, cx + sn * body * 0.92f, cy + cs * body * 0.92f, 2.8f);
    }

    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider& s) override
    {
        const float cx = (float) x + (float) w * 0.5f;
        g.setColour (Colour (0xff2c2c2e)); g.fillRoundedRectangle (cx - 1.2f, (float) y, 2.4f, (float) h, 1.2f);
        const float zeroY = (float) y + 7.0f + ((float) h - 14.0f) * 0.5f;
        g.setColour (cAmber.withAlpha (0.85f)); g.fillRect (cx - 1.2f, jmin (pos, zeroY), 2.4f, std::abs (pos - zeroY));
        g.setColour (Colours::black.withAlpha (0.18f)); g.fillEllipse (cx - 7.5f, pos - 6.0f, 15.0f, 15.0f);
        g.setGradientFill (ColourGradient (Colour (0xff4a4a4e), cx - 3, pos - 5, Colour (0xff0f0f11), cx + 4, pos + 7, true));
        g.fillEllipse (cx - 7.0f, pos - 7.0f, 14.0f, 14.0f);
        g.setColour (Colours::white.withAlpha (0.35f)); g.fillEllipse (cx - 3.5f, pos - 5.0f, 5.0f, 3.0f);
        if (s.isMouseOverOrDragging()) { g.setColour (cAmber.withAlpha (0.8f)); g.drawEllipse (cx - 7.0f, pos - 7.0f, 14.0f, 14.0f, 1.3f); }
    }

    void drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool) override
    {
        const auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        if (b.getComponentID() == "vswitch")
        {
            const float cx = r.getCentreX(), slotH = r.getHeight() - 18.0f, top = r.getY() + 14.0f;
            g.setColour (Colour (0xff8a8a8a)); // icono
            g.drawEllipse (cx - 4.0f, r.getY() + 1.0f, 8.0f, 8.0f, 1.2f);
            g.setColour (on ? cAmber : Colour (0xffb0b0b0)); g.fillRect (cx - 0.7f, r.getY() - 1.0f, 1.4f, 5.0f);
            g.setColour (Colour (0xff1c1c1e)); g.fillRoundedRectangle (cx - 4.5f, top, 9.0f, slotH, 4.5f);
            const float ty = on ? top + 5.0f : top + slotH - 5.0f;
            if (on) { g.setColour (cAmber.withAlpha (0.35f)); g.fillEllipse (cx - 10.0f, ty - 10.0f, 20.0f, 20.0f); }
            g.setGradientFill (ColourGradient (on ? Colour (0xfff7d56a) : Colour (0xfff1f1f1), cx - 2, ty - 3, on ? cAmber : Colour (0xff9a9a9a), cx + 3, ty + 4, true));
            g.fillEllipse (cx - 5.5f, ty - 5.5f, 11.0f, 11.0f);
            return;
        }
        const float c = jmin (r.getWidth(), r.getHeight()) * 0.5f - 2.0f, cx = r.getCentreX(), cy = r.getCentreY();
        const Colour col = on ? cAmber : Colour (0xff8c8c8c);
        if (on) { g.setColour (cAmber.withAlpha (0.25f)); g.fillEllipse (cx - c - 3, cy - c - 3, 2 * c + 6, 2 * c + 6); }
        g.setColour (over ? Colour (0xffffffff) : Colour (0xfff2f2f0)); g.fillEllipse (cx - c, cy - c, 2 * c, 2 * c);
        g.setColour (Colour (0xffb7b7b5)); g.drawEllipse (cx - c, cy - c, 2 * c, 2 * c, 1.2f);
        Path p; p.addCentredArc (cx, cy + 0.5f, c * 0.5f, c * 0.5f, 0.0f, 0.7f, MathConstants<float>::twoPi - 0.7f, true);
        p.startNewSubPath (cx, cy - c * 0.62f); p.lineTo (cx, cy - 1.0f);
        g.setColour (col); g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box) override
    {
        const auto r = Rectangle<float> (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f);
        g.setGradientFill (ColourGradient (Colour (0xffffffff), 0, 0, Colour (0xffeeeeec), 0, (float) h, false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (box.hasKeyboardFocus (true) ? cAmber : Colour (0xffbdbdbb)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
        const float ax = (float) w - 14.0f, ay = (float) h * 0.5f;
        if (box.getComponentID() == "modcombo")
        {
            const bool on = box.getProperties().getWithDefault ("lit", false);
            if (on) { g.setColour (cAmber.withAlpha (0.35f)); g.fillEllipse (ax - 7, ay - 7, 14, 14); }
            g.setColour (on ? cAmber : Colour (0xffa8a8a8)); g.fillEllipse (ax - 3.5f, ay - 3.5f, 7, 7);
        }
        else
        {
            Path p; p.startNewSubPath (ax - 4, ay - 2); p.lineTo (ax, ay + 2); p.lineTo (ax + 4, ay - 2);
            g.setColour (cInk); g.strokePath (p, PathStrokeType (1.5f));
        }
    }
};

// ------------------------------------------------------------ controles
struct Knob : Slider
{
    Knob()
    {
        setSliderStyle (RotaryHorizontalVerticalDrag);
        setTextBoxStyle (NoTextBox, true, 0, 0);
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        setPopupDisplayEnabled (true, true, nullptr);
        setMouseDragSensitivity (220);
    }
};
struct EqSlider : Slider
{
    EqSlider()
    {
        setSliderStyle (LinearVertical); setTextBoxStyle (NoTextBox, true, 0, 0);
        setPopupDisplayEnabled (true, true, nullptr);
    }
};
struct PowerBtn : ToggleButton { PowerBtn (const char* id = "power") { setComponentID (id); setClickingTogglesState (true); } };

// Boton plano (modos del EQ, preset, guardar)
struct FlatButton : Button
{
    String text; bool selected = false; int icon = 0;   // 0 texto, 1 flecha izq, 2 flecha der, 3 guardar
    explicit FlatButton (const String& t, int ic = 0) : Button (t), text (t), icon (ic) {}
    void paintButton (Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool dark = selected;
        Colour fill = dark ? Colour (0xff17171a) : (down ? Colour (0xffe2e2e0) : over ? Colour (0xfff7f7f5) : Colour (0xffeeeeec));
        g.setColour (fill); g.fillRoundedRectangle (r, 5.0f);
        g.setColour (dark ? Colour (0xff000000) : Colour (0xffc4c4c2)); g.drawRoundedRectangle (r, 5.0f, 1.0f);
        g.setColour (dark ? cAmber : cInk);
        if (icon == 1 || icon == 2)
        {
            const float cx = r.getCentreX(), cy = r.getCentreY(), d = icon == 1 ? -1.0f : 1.0f;
            Path p; p.startNewSubPath (cx - 3 * d, cy - 5); p.lineTo (cx + 3 * d, cy); p.lineTo (cx - 3 * d, cy + 5);
            g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        else
        {
            g.setFont (F (icon == 3 ? 12.5f : 13.0f, ! (icon == 3)));
            g.drawText (text, r.withTrimmedLeft (icon == 3 ? 18.0f : 0.0f), Justification::centred, false);
            if (icon == 3)
            {
                const float ix = r.getX() + 14.0f, iy = r.getCentreY();
                g.drawRoundedRectangle (ix - 6, iy - 6, 12, 12, 2.0f, 1.3f);
                g.drawRect (ix - 3.5f, iy - 6.0f, 7.0f, 4.0f, 1.0f);
            }
        }
    }
};

// ------------------------------------------------------------ vistas dinamicas
struct DynView : Component
{
    const UIState& s;
    explicit DynView (const UIState& st) : s (st) { setInterceptsMouseClicks (false, false); setOpaque (false); }
};

struct ArtView : DynView
{
    Image img = ImageCache::getFromMemory (BinaryData::art_png, BinaryData::art_pngSize);
    std::function<void()> onClick;
    explicit ArtView (const UIState& st) : DynView (st) { setInterceptsMouseClicks (true, false); setMouseCursor (MouseCursor::PointingHandCursor); }
    void mouseDown (const MouseEvent&) override { if (onClick) onClick(); }
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        Path clip; clip.addRoundedRectangle (b, 9.0f);
        g.reduceClipRegion (clip, {});
        g.setColour (Colour (240, 240, 240)); g.fillAll();
        const float a = jlimit (0.0f, 1.0f, s.artAlpha);
        g.setOpacity (a);
        g.drawImage (img, b);
        // bocina: resplandor que pulsa con el nivel
        const float L = jlimit (0.0f, 1.0f, s.outLevel * 2.2f);
        if (L > 0.01f)
        {
            const float sx = b.getWidth() / 612.0f, sy = b.getHeight() / 650.0f;
            const float cx = 430.0f * sx, cy = 400.0f * sy, rad = (120.0f + 40.0f * L) * sx;
            g.setGradientFill (ColourGradient (Colour (0xffffb347).withAlpha (0.30f * L * a), cx, cy, Colour (0x00ffb347), cx + rad, cy, true));
            g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
            for (int k = 0; k < 3; ++k)   // ondas de presion
            {
                const float rr = (40.0f + 36.0f * k + 30.0f * L) * sx;
                g.setColour (Colour (0xffffc266).withAlpha (jlimit (0.0f, 0.5f, 0.45f * L - 0.12f * k) * a));
                g.drawEllipse (cx - rr, cy - rr * 1.15f, rr * 2, rr * 2.3f, 1.4f);
            }
        }
        if (s.artAlpha < 0.5f)
        {
            g.setOpacity (1.0f - s.artAlpha * 2.0f);
            g.setColour (Colour (0xff6e6e6e).withAlpha (1.0f - s.artAlpha * 2.0f));
            drawSpaced (g, String::fromUTF8 ("TOCA EL BAJO PARA REVELAR LA ILUSTRACIÓN"), b.getCentreX(), b.getHeight() - 18.0f, 2.0f, 1, F (11.0f, true));
        }
    }
};

struct SpectrumView : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float x0 = 22.0f, x1 = x0 + 365.0f, top = 8.0f, bot = b.getHeight() - 6.0f;
        Path line, fill;
        const int N = Analyzer::kBands;
        fill.startNewSubPath (x0, bot);
        for (int i = 0; i < N; ++i)
        {
            const float x = x0 + (x1 - x0) * (float) i / (float) (N - 1), y = bot - s.analyzer.spec[i] * (bot - top);
            if (i == 0) line.startNewSubPath (x, y); else line.lineTo (x, y);
            fill.lineTo (x, y);
        }
        fill.lineTo (x1, bot); fill.closeSubPath();
        g.setGradientFill (ColourGradient (cAmber.withAlpha (0.55f), 0, top, cAmber.withAlpha (0.04f), 0, bot, false));
        g.fillPath (fill);
        g.setColour (Colour (0xffd4950a).withAlpha (0.9f));
        g.strokePath (line, PathStrokeType (1.4f, PathStrokeType::curved));
    }
};

struct ScopeView : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xffbdbdbb)); g.drawLine (0, b.getCentreY(), b.getWidth(), b.getCentreY(), 1.0f);
        Path p; const int N = Analyzer::kScope;
        for (int i = 0; i < N; ++i)
        {
            const float x = b.getWidth() * (float) i / (float) (N - 1), y = b.getCentreY() - s.analyzer.scope[i] * b.getHeight() * 0.45f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (Colour (0xff2b2b2e)); g.strokePath (p, PathStrokeType (1.5f, PathStrokeType::curved));
    }
};

struct HeaderBars : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const int idx[7] = { 6, 18, 32, 46, 60, 74, 88 };
        const float h = (float) getHeight();
        for (int i = 0; i < 7; ++i)
        {
            const float v = jlimit (0.08f, 1.0f, 0.12f + s.analyzer.spec[idx[i]] * 1.1f);
            g.setColour (Colour (0xff222224));
            g.fillRoundedRectangle (3.0f + (float) i * 4.2f, h * 0.5f - v * h * 0.5f, 2.2f, v * h, 1.0f);
        }
    }
};

struct LedView : DynView
{
    float v = 0; Colour col = cAmber;
    LedView (const UIState& st, Colour c) : DynView (st), col (c) {}
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat(); const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f - 3.0f;
        const float a = jlimit (0.0f, 1.0f, v);
        if (a > 0.02f) { g.setColour (col.withAlpha (0.35f * a)); g.fillEllipse (b.getCentreX() - r - 3, b.getCentreY() - r - 3, 2 * r + 6, 2 * r + 6); }
        g.setGradientFill (ColourGradient (col.interpolatedWith (Colours::white, 0.6f * a).withAlpha (0.35f + 0.65f * a), b.getCentreX() - 1, b.getCentreY() - 1,
                                           col.darker (0.5f).withAlpha (0.35f + 0.65f * a), b.getCentreX() + r, b.getCentreY() + r, true));
        g.fillEllipse (b.getCentreX() - r, b.getCentreY() - r, 2 * r, 2 * r);
        g.setColour (Colour (0x55000000)); g.drawEllipse (b.getCentreX() - r, b.getCentreY() - r, 2 * r, 2 * r, 0.8f);
    }
};

struct SpeakerView : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float L = jlimit (0.0f, 1.0f, s.outLevel * 2.5f), cx = b.getCentreX(), cy = b.getCentreY();
        g.setColour (Colour (0xfff4f4f2)); g.fillRoundedRectangle (b.reduced (1), 7.0f);
        g.setColour (Colour (0xffc2c2c0)); g.drawRoundedRectangle (b.reduced (1), 7.0f, 1.2f);
        const float R = b.getWidth() * 0.40f;
        g.setColour (Colour (0xff2a2a2d)); g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
        const float c = R * (0.74f + 0.14f * L);
        g.setGradientFill (ColourGradient (Colour (0xff5a5a60), cx - c * 0.5f, cy - c * 0.5f, Colour (0xff131315), cx + c, cy + c, true));
        g.fillEllipse (cx - c, cy - c, c * 2, c * 2);
        g.setColour (Colour (0xff0a0a0b).withAlpha (0.7f)); g.drawEllipse (cx - c * 0.66f, cy - c * 0.66f, c * 1.32f, c * 1.32f, 1.0f);
        const float d = R * (0.30f + 0.10f * L);
        g.setGradientFill (ColourGradient (Colour (0xffe9e9e9), cx - d * 0.4f, cy - d * 0.4f, Colour (0xff7a7a7a), cx + d, cy + d, true));
        g.fillEllipse (cx - d, cy - d, d * 2, d * 2);
        if (L > 0.05f) { g.setColour (cAmber.withAlpha (0.8f * L)); g.drawEllipse (cx - R - 1, cy - R - 1, R * 2 + 2, R * 2 + 2, 1.6f); }
    }
};

struct AmpHeadView : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        const float W = b.getWidth(), H = b.getHeight();
        const float glow = jlimit (0.0f, 1.0f, s.outLevel * 3.0f);
        // cuerpo
        g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (b.translated (0, 2).reduced (1), 8.0f);
        g.setGradientFill (ColourGradient (Colour (0xff2b2b30), 0, 0, Colour (0xff0d0d10), 0, H, false)); g.fillRoundedRectangle (b.reduced (1), 8.0f);
        g.setGradientFill (ColourGradient (Colour (0xfff2f2f2), 0, 0, Colour (0xff8a8a8a), 0, 9, false)); g.fillRoundedRectangle (4, 3, W - 8, 8, 3.0f);
        // rejilla + valvulas incandescentes
        const Rectangle<float> gr (12, 16, W - 24, 46);
        g.setColour (Colour (0xff070708)); g.fillRoundedRectangle (gr, 4.0f);
        for (int i = 0; i < 4; ++i)
        {
            const float tx = gr.getX() + gr.getWidth() * (0.14f + 0.24f * (float) i), ty = gr.getBottom() - 8.0f;
            g.setGradientFill (ColourGradient (Colour (0xffff8a2a).withAlpha (0.18f + 0.55f * glow), tx, ty, Colour (0x00ff8a2a), tx, gr.getY(), false));
            g.fillRect (tx - 16.0f, gr.getY(), 32.0f, gr.getHeight());
        }
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (gr.toNearestInt());
            g.setColour (Colour (0xff3a3a40).withAlpha (0.85f));
            for (float x = gr.getX(); x < gr.getRight(); x += 3.0f) g.drawVerticalLine ((int) x, gr.getY(), gr.getBottom());
            for (float y = gr.getY(); y < gr.getBottom(); y += 3.0f) g.drawHorizontalLine ((int) y, gr.getX(), gr.getRight());
        }
        // placa
        const Rectangle<float> pl (W * 0.5f - 42, 28, 84, 22);
        g.setGradientFill (ColourGradient (Colour (0xff3b3b40), 0, pl.getY(), Colour (0xff17171a), 0, pl.getBottom(), false)); g.fillRoundedRectangle (pl, 4.0f);
        g.setColour (Colour (0xffb5b5b5)); g.drawRoundedRectangle (pl, 4.0f, 1.0f);
        g.setColour (Colours::white); drawSpaced (g, "ORYMY", pl.getCentreX(), pl.getCentreY(), 2.0f, 1, F (13.0f, true));
        // panel de controles
        const Rectangle<float> cp (8, 68, W - 16, H - 76);
        g.setGradientFill (ColourGradient (Colour (0xff3d3d42), 0, cp.getY(), Colour (0xff1c1c20), 0, cp.getBottom(), false)); g.fillRoundedRectangle (cp, 4.0f);
        for (int i = 0; i < 9; ++i)
        {
            const float kx = cp.getX() + 40.0f + (float) i * ((cp.getWidth() - 64.0f) / 8.0f), ky = cp.getCentreY();
            g.setGradientFill (ColourGradient (Colour (0xfff6f6f6), kx - 3, ky - 4, Colour (0xff7f7f7f), kx + 5, ky + 6, true)); g.fillEllipse (kx - 6.5f, ky - 6.5f, 13, 13);
            g.setColour (Colour (0xff1b1b1b)); const float a = -2.2f + 0.55f * (float) i;
            g.drawLine (kx, ky, kx + std::sin (a) * 5.0f, ky - std::cos (a) * 5.0f, 1.6f);
        }
        auto led = [&] (float lx, Colour c, float v)
        {
            const float ly = cp.getCentreY();
            if (v > 0.05f) { g.setColour (c.withAlpha (0.35f * v)); g.fillEllipse (lx - 8, ly - 8, 16, 16); }
            g.setColour (c.withAlpha (0.25f + 0.75f * v)); g.fillEllipse (lx - 4, ly - 4, 8, 8);
        };
        led (cp.getX() + 10, Colour (0xffff3b30), 1.0f);
        led (cp.getX() + 24, cAmber, glow);
        led (cp.getRight() - 10, cGreen, s.gateOpen ? 1.0f : 0.0f);
    }
};

struct FretboardView : DynView
{
    using DynView::DynView;
    static float fretX (float x0, float x1, int n) { return x0 + (x1 - x0) * (1.0f - std::pow (2.0f, -(float) n / 12.0f)) / 0.75f; }
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff202023), 0, 0, Colour (0xff0b0b0d), 0, b.getHeight(), false));
        g.fillRoundedRectangle (b, 6.0f);
        const float x0 = 14.0f, x1 = b.getWidth() - 10.0f, y0 = 8.0f, y1 = b.getHeight() - 8.0f;
        g.setColour (Colour (0xffd8d8d8).withAlpha (0.9f)); g.fillRect (x0 - 3, y0 - 2, 3.5f, y1 - y0 + 4);          // cejuela
        g.setColour (Colour (0xffc9c9c9).withAlpha (0.32f));
        for (int n = 1; n <= 24; ++n) g.fillRect (fretX (x0, x1, n) - 0.8f, y0 - 2, 1.6f, y1 - y0 + 4);
        const int dots[] = { 3, 5, 7, 9, 12, 15, 17, 19, 21, 24 };
        g.setColour (Colours::white.withAlpha (0.30f));
        for (int d : dots)
        {
            const float mx = 0.5f * (fretX (x0, x1, d - 1) + fretX (x0, x1, d));
            if (d == 12 || d == 24) { g.fillEllipse (mx - 3, y0 + (y1 - y0) * 0.30f - 3, 6, 6); g.fillEllipse (mx - 3, y0 + (y1 - y0) * 0.70f - 3, 6, 6); }
            else g.fillEllipse (mx - 3, (y0 + y1) * 0.5f - 3, 6, 6);
        }
        // cuerdas: G D A E (arriba -> abajo)
        const int open[4] = { 43, 38, 33, 28 };
        const float th[4] = { 1.1f, 1.5f, 1.9f, 2.4f };
        int lit = -1, litFret = 0;
        const auto& a = s.analyzer;
        if (a.voiced && a.midi >= 28)
        {
            int bestFret = 99;
            for (int i = 3; i >= 0; --i) { const int f = a.midi - open[i]; if (f >= 0 && f <= 24 && f < bestFret) { bestFret = f; lit = i; } }
            litFret = bestFret;
        }
        const Colour vc = s.verdictColour();
        for (int i = 0; i < 4; ++i)
        {
            const float y = y0 + (y1 - y0) * ((float) i + 0.5f) / 4.0f;
            const bool on = (i == lit);
            if (on) { g.setColour (vc.withAlpha (0.35f)); g.fillRect (x0, y - th[i] - 2, x1 - x0, th[i] * 2 + 4); }
            g.setColour (on ? vc.brighter (0.4f) : Colour (0xffcfcfd2).withAlpha (0.75f));
            g.fillRect (x0, y - th[i] * 0.5f, x1 - x0, th[i]);
        }
        if (lit >= 0)
        {
            const float y = y0 + (y1 - y0) * ((float) lit + 0.5f) / 4.0f;
            const float x = litFret == 0 ? x0 + 8.0f : 0.5f * (fretX (x0, x1, litFret - 1) + fretX (x0, x1, litFret));
            g.setGradientFill (ColourGradient (vc.withAlpha (0.85f), x, y, vc.withAlpha (0.0f), x + 22, y, true)); g.fillEllipse (x - 22, y - 22, 44, 44);
            g.setColour (vc.brighter (0.5f)); g.fillEllipse (x - 7, y - 7, 14, 14);
            g.setColour (Colours::white.withAlpha (0.9f)); g.fillEllipse (x - 2.5f, y - 3.5f, 5, 4);
        }
    }
};

struct PlayingView : DynView
{
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto& a = s.analyzer;
        const Rectangle<float> bar (10, 33, 377, 42);
        g.setGradientFill (ColourGradient (Colour (0xff18181b), 0, bar.getY(), Colour (0xff050506), 0, bar.getBottom(), false));
        g.fillRoundedRectangle (bar, 7.0f);
        const float xs[7] = { 40, 92, 143, 196, 250, 300, 352 };
        const int steps[7] = { 0, 2, 3, 5, 7, 8, 10 };
        const int minorRoot = (a.keyMajor + 9) % 12;
        const int cur = a.voiced ? ((a.midi % 12) + 12) % 12 : -1;
        for (int i = 0; i < 7; ++i)
        {
            const int pc = (minorRoot + steps[i]) % 12;
            const bool lit = (pc == cur);
            const float cx = xs[i], cy = bar.getCentreY();
            if (lit)
            {
                g.setColour (cAmber.withAlpha (0.28f)); g.fillEllipse (cx - 23, cy - 23, 46, 46);
                g.setColour (cAmber.withAlpha (0.20f)); g.fillEllipse (cx - 17, cy - 17, 34, 34);
                g.setColour (cAmber); g.drawEllipse (cx - 17, cy - 17, 34, 34, 1.8f);
            }
            g.setColour (lit ? cAmber : Colour (0xffe9e9e9));
            g.setFont (F (17.0f, true));
            g.drawText (kNoteNames[pc], Rectangle<float> (cx - 24, cy - 12, 48, 24), Justification::centred, false);
        }
        // escala actual
        g.setColour (cInk); g.setFont (F (14.5f));
        const int rel = a.keyMajor;
        g.drawText (String (kNoteNames[rel]) + " Mayor / " + kNoteNames[minorRoot] + " Menor", Rectangle<float> (16, 101, 200, 22), Justification::centredLeft, false);
        // veredicto
        const Colour vc = s.verdictColour();
        const float ly = 112.0f, lx = 232.0f;
        if (s.verdict != Verdict::Idle) { g.setColour (vc.withAlpha (0.35f)); g.fillEllipse (lx - 10, ly - 10, 20, 20); }
        g.setColour (vc); g.fillEllipse (lx - 5.5f, ly - 5.5f, 11, 11);
        g.setColour (s.verdict == Verdict::Idle ? cSub : cInk);
        g.setFont (F (12.5f, true));
        g.drawText (s.verdictText(), Rectangle<float> (lx + 12, ly - 11, 160, 22), Justification::centredLeft, false);
    }
};

struct StaffView : DynView
{
    Image clef = ImageCache::getFromMemory (BinaryData::clef_png, BinaryData::clef_pngSize);
    using DynView::DynView;
    void paint (Graphics& g) override
    {
        const auto& a = s.analyzer;
        g.setColour (Colour (0xff9a9a98));
        for (int i = 0; i < 5; ++i) g.drawHorizontalLine (15 + 9 * i, 42.0f, 252.0f);
        g.setColour (Colours::black); g.setOpacity (0.9f); g.drawImage (clef, Rectangle<float> (8, 6, 22, 56));
        const float xs[7] = { 59, 89, 117, 147, 176, 206, 235 };
        const int steps[7] = { 0, 2, 4, 5, 7, 9, 11 };
        const int cur = a.voiced ? ((a.midi % 12) + 12) % 12 : -1;
        const Colour vc = s.verdictColour();
        for (int i = 0; i < 7; ++i)
        {
            const int pc = (a.keyMajor + steps[i]) % 12;
            const float cx = xs[i], cy = 48.0f - 5.0f * (float) i;
            const bool lit = (pc == cur);
            if (lit) { g.setColour (vc.withAlpha (0.3f)); g.fillEllipse (cx - 11, cy - 11, 22, 22); }
            g.setColour (lit ? vc : Colour (0xff3d3d3f)); const float r = lit ? 5.5f : 3.6f; g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
            g.setColour (lit ? cInk : cSub); g.setFont (F (12.0f, lit));
            g.drawText (kNoteNames[pc], Rectangle<float> (cx - 16, 78, 32, 18), Justification::centred, false);
        }
    }
};

struct PresetListView : Component
{
    int selected = 0; int hover = -1; std::function<void (int)> onSelect;
    static constexpr float rowH = 25.86f;
    void paint (Graphics& g) override
    {
        const auto& list = presets();
        for (size_t i = 0; i < list.size(); ++i)
        {
            const float cy = 13.0f + (float) i * rowH;
            const Rectangle<float> row (4, cy - 12.0f, getWidth() - 8.0f, 24.0f);
            const bool sel = (int) i == selected;
            if (sel) { g.setColour (Colour (0xff131316)); g.fillRoundedRectangle (row, 5.0f); }
            else if ((int) i == hover) { g.setColour (Colour (0xffededeb)); g.fillRoundedRectangle (row, 5.0f); }
            if (sel) { g.setColour (cAmber.withAlpha (0.35f)); g.fillEllipse (10, cy - 6, 12, 12); g.setColour (cAmber); g.fillEllipse (12.5f, cy - 3.5f, 7, 7); }
            else { g.setColour (Colour (0xff8b8b8b)); g.drawEllipse (12.5f, cy - 3.0f, 6, 6, 1.1f); }
            g.setColour (sel ? Colours::white : cInk); g.setFont (F (14.5f, false));
            g.drawText (String::fromUTF8 (list[i].name), Rectangle<float> (30, cy - 11, getWidth() - 36.0f, 22), Justification::centredLeft, false);
        }
    }
    int rowAt (Point<int> p) const { const int i = (int) std::floor (((float) p.y - 13.0f + rowH * 0.5f) / rowH); return (i >= 0 && i < (int) presets().size()) ? i : -1; }
    void mouseDown (const MouseEvent& e) override { const int i = rowAt (e.getPosition()); if (i >= 0 && onSelect) onSelect (i); }
    void mouseMove (const MouseEvent& e) override { const int h = rowAt (e.getPosition()); if (h != hover) { hover = h; repaint(); } }
    void mouseExit (const MouseEvent&) override { hover = -1; repaint(); }
};
}
