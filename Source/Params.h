#pragma once
#include "Common.h"

// ============================================================================
//  ORYMY Bass  -  parametros y presets
// ============================================================================
namespace orymy
{
static constexpr int kNumEq = 11;
inline const char* const kEqIds[kNumEq] = { "eq0","eq1","eq2","eq3","eq4","eq5","eq6","eq7","eq8","eq9","eq10" };
inline const char* const kEqLabels[kNumEq] = { "31","62","125","250","500","1K","2K","4K","8K","16K","20K" };
inline const double kEqFreqs[kNumEq] = { 31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000, 20000 };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def,
                   const char* unit, int decimals, float centre = -1.0f)
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (centre > 0.0f) r.setSkewForCentre (centre);
        p.push_back (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, r, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                [unit, decimals] (float v, int) { return juce::String (v, decimals) + unit; })));
    };
    auto bl = [&] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<APB> (juce::ParameterID { id, 1 }, name, def)); };
    auto ch = [&] (const char* id, const char* name, juce::StringArray items, int def)
    { p.push_back (std::make_unique<APC> (juce::ParameterID { id, 1 }, name, items, def)); };

    bl ("gateOn", "Gate On", true);
    fl ("gateThr", "Gate Threshold", -60.f, 0.f, -50.f, " dB", 1);
    fl ("gateRange", "Gate Range", -80.f, 0.f, -40.f, " dB", 1);

    bl ("compOn", "Comp On", true);
    fl ("compThr", "Comp Threshold", -40.f, 0.f, -18.f, " dB", 1);
    fl ("compRatio", "Comp Ratio", 1.f, 20.f, 3.f, ":1", 1);
    fl ("compMakeup", "Comp Makeup", 0.f, 20.f, 3.f, " dB", 1);

    bl ("toneOn", "Tone On", true);
    fl ("tone", "Bass Tone", 0.f, 10.f, 5.f, "", 1);
    bl ("splitOn", "Split On", false);
    fl ("splitFreq", "Split Freq", 20.f, 20000.f, 180.f, " Hz", 0, 800.f);
    bl ("subOn", "Sub On", false);
    fl ("subLevel", "Sub Level", 0.f, 10.f, 3.f, "", 1);

    for (int i = 0; i < kNumEq; ++i)
        fl (kEqIds[i], (juce::String ("EQ ") + kEqLabels[i]).toRawUTF8(), -12.f, 12.f, 0.f, " dB", 1);
    ch ("eqMode", "EQ Mode", { "Auto", "Bachata", "Merengue", "Custom" }, 0);

    ch ("ampModel", "Amp Model", { "Bass Amp", "Vintage Tube", "Modern Clean", "Grit" }, 0);
    fl ("gain", "Gain", 0.f, 10.f, 4.f, "", 1);
    fl ("master", "Master", 0.f, 10.f, 7.f, "", 1);
    fl ("drive", "Drive", 0.f, 10.f, 0.f, "", 1);

    ch ("cabModel", "Cabinet", { "4x10 Bass", "1x15 Vintage", "2x10 Tight", "8x10 Stack", "Direct (DI)" }, 0);
    fl ("cabMix", "Cabinet Mix", 0.f, 100.f, 100.f, " %", 0);

    bl ("modOn", "Mod On", false);
    ch ("modType", "Mod Type", { "Chorus", "Flanger", "Phaser", "Tremolo" }, 0);
    fl ("modAmt", "Mod Amount", 0.f, 10.f, 4.f, "", 1);

    bl ("delayOn", "Delay On", false);
    fl ("delayMs", "Delay Time", 0.f, 1000.f, 300.f, " ms", 0);
    ch ("delaySync", "Delay Sync", { "Libre", "1/4", "1/8", "1/8 Dotted", "1/16" }, 3);

    bl ("reverbOn", "Reverb On", false);
    ch ("reverbType", "Reverb Type", { "Room", "Hall", "Plate", "Spring" }, 0);
    fl ("reverbMix", "Reverb Mix", 0.f, 100.f, 15.f, " %", 0);

    return { p.begin(), p.end() };
}

// Lista de todos los ids (para presets / estado)
inline std::vector<std::string> allParamIds()
{
    std::vector<std::string> v = { "gateOn","gateThr","gateRange","compOn","compThr","compRatio","compMakeup",
        "toneOn","tone","splitOn","splitFreq","subOn","subLevel","eqMode","ampModel","gain","master","drive",
        "cabModel","cabMix","modOn","modType","modAmt","delayOn","delayMs","delaySync",
        "reverbOn","reverbType","reverbMix" };
    for (auto* e : kEqIds) v.push_back (e);
    return v;
}

// ----------------------------------------------------------------------------
//  Curvas del ecualizador grafico (31 Hz ... 20 kHz)
// ----------------------------------------------------------------------------
using EqCurve = std::array<float, kNumEq>;
inline const EqCurve kEqBachata  = { 2.f, 3.f, 2.f, 0.f, -1.f, 1.f, 2.f, 1.f, 0.f, -1.f, -3.f };
inline const EqCurve kEqMerengue = { 0.f, 1.f, 2.f, 0.f, -2.f, 0.f, 3.f, 4.f, 2.f, 1.f, 0.f };

// ----------------------------------------------------------------------------
//  Presets
// ----------------------------------------------------------------------------
struct PresetDef
{
    const char* name;                         // UTF-8
    std::map<std::string, float> v;           // sobrescribe los valores por defecto
    EqCurve eq;
};

inline const std::vector<PresetDef>& presets()
{
    static const std::vector<PresetDef> list = {
    { "Bachata Antony Santos",
      { {"ampModel",1},{"gain",3.5f},{"master",7},{"compThr",-20},{"compRatio",3.5f},{"compMakeup",4},{"tone",4.5f},
        {"splitOn",1},{"splitFreq",140},{"cabModel",0},{"reverbOn",1},{"reverbMix",8},{"gateThr",-52} },
      { 3.f, 3.f, 2.f, 0.f, -1.f, 1.5f, 2.f, .5f, -1.f, -2.f, -4.f } },
    { "Raulín Rodríguez",
      { {"ampModel",0},{"gain",4.5f},{"drive",1},{"compRatio",4},{"compMakeup",4},{"tone",5.5f},{"splitOn",1},{"splitFreq",180},{"cabModel",2} },
      { 1.f, 2.f, 3.f, 1.f, 0.f, 2.f, 3.f, 1.f, .5f, 0.f, -2.f } },
    { "Romeo Santos",
      { {"ampModel",2},{"gain",3},{"tone",6},{"subOn",1},{"subLevel",3.5f},{"compRatio",4},{"cabModel",0},
        {"modOn",1},{"modType",0},{"modAmt",2},{"reverbOn",1},{"reverbType",2},{"reverbMix",12} },
      { 3.f, 4.f, 1.f, -1.f, -2.f, 0.f, 2.f, 3.f, 2.f, 1.f, -1.f } },
    { "Max Santos Bass",
      { {"ampModel",1},{"gain",5},{"drive",2},{"subOn",1},{"subLevel",5},{"cabModel",3},{"compRatio",5},{"compMakeup",5},{"tone",4} },
      { 4.f, 4.f, 3.f, 1.f, -1.f, 0.f, 1.f, 0.f, -1.f, -2.f, -4.f } },
    { "Slap Bachata",
      { {"ampModel",2},{"gain",3.5f},{"drive",.5f},{"compThr",-22},{"compRatio",6},{"compMakeup",5},{"tone",7},
        {"splitOn",1},{"splitFreq",200},{"cabModel",2} },
      { 3.f, 4.f, 0.f, -3.f, -4.f, -1.f, 2.f, 4.f, 5.f, 3.f, 0.f } },
    { "Mambo Clarity",
      { {"ampModel",2},{"gain",2.5f},{"compRatio",2.5f},{"tone",6.5f},{"cabModel",2},{"reverbOn",1},{"reverbMix",6} },
      { -2.f, 0.f, 1.f, 0.f, -1.f, 2.f, 4.f, 4.f, 2.f, 1.f, 0.f } },
    { "Bajo Clásico",
      { {"ampModel",0},{"gain",3},{"tone",5},{"cabModel",1} },
      { 1.f, 2.f, 2.f, 1.f, 0.f, 0.f, 1.f, 0.f, -1.f, -2.f, -4.f } },
    { "Bajo Moderno",
      { {"ampModel",2},{"gain",3},{"tone",6},{"subOn",1},{"subLevel",2},{"cabModel",0} },
      { 2.f, 3.f, 1.f, -1.f, -1.f, 1.f, 3.f, 3.f, 2.f, 1.f, 0.f } },
    { "Bajo Profundo",
      { {"ampModel",1},{"gain",4},{"tone",3},{"subOn",1},{"subLevel",6},{"cabModel",3},{"compRatio",4} },
      { 5.f, 5.f, 3.f, 1.f, -2.f, -2.f, -1.f, -2.f, -3.f, -4.f, -6.f } },
    { "Bajo Suelto",
      { {"ampModel",0},{"gain",3.5f},{"compOn",0},{"tone",5.5f},{"cabModel",1},{"gateThr",-55} },
      { 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 2.f, 1.f, 0.f, 0.f, -2.f } },
    { "Bachata Romántica",
      { {"ampModel",1},{"gain",3},{"tone",4},{"cabModel",1},{"cabMix",90},{"modOn",1},{"modType",0},{"modAmt",3},
        {"reverbOn",1},{"reverbType",1},{"reverbMix",18},{"compRatio",3} },
      { 3.f, 3.f, 2.f, 1.f, 0.f, 1.f, 1.f, 0.f, -1.f, -2.f, -4.f } },
    { "Merengue Brillante",
      { {"ampModel",2},{"gain",3},{"drive",1},{"compRatio",4},{"tone",7},{"cabModel",2} },
      { 0.f, 1.f, 2.f, 0.f, -2.f, 1.f, 3.f, 4.f, 3.f, 2.f, 1.f } },
    { "Merengue Clásico",
      { {"ampModel",0},{"gain",4},{"tone",5.5f},{"cabModel",0},{"splitOn",1},{"splitFreq",170},{"compRatio",3.5f} },
      { 0.f, 1.f, 2.f, 1.f, -1.f, 0.f, 3.f, 3.f, 1.f, 0.f, -2.f } },
    { "Urbano Bass",
      { {"ampModel",2},{"gain",3},{"tone",4.5f},{"subOn",1},{"subLevel",7},{"cabModel",0},{"compRatio",5} },
      { 5.f, 4.f, 1.f, -2.f, -3.f, -2.f, 0.f, 2.f, 2.f, 1.f, 0.f } },
    { "Personalizado", {}, { 0,0,0,0,0,0,0,0,0,0,0 } }
    };
    return list;
}
inline constexpr int kUserPresetIndex = 14;   // "Personalizado"
}
