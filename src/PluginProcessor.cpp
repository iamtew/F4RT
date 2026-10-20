#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "parameters/FactoryBank.h"

#include <cstring>

namespace
{
const juce::Identifier kPatchName { "PATCH_NAME" };
constexpr const char* kInitPatch = "Init Patch";
constexpr const char* kPatchExt = ".f4rtp";

juce::File libraryRoot()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Stupid Systems LLC")
        .getChildFile ("F4RT");
}

juce::File ensureDir (juce::File dir)
{
    dir.createDirectory();
    return dir;
}

juce::String legalStem (const juce::String& name)
{
    auto stem = juce::File::createLegalFileName (name.trim());
    if (stem.endsWithIgnoreCase (kPatchExt))
        stem = stem.dropLastCharacters ((int) std::strlen (kPatchExt));
    return stem;
}
} // namespace

F4RTAudioProcessor::F4RTAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    apvts.state.setProperty (kPatchName, kInitPatch, nullptr);
}

void F4RTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    analyser.prepare (sampleRate);
    const int ch = juce::jmax (2, getTotalNumOutputChannels());
    fx.prepare (sampleRate, samplesPerBlock, ch);
}

bool F4RTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

float F4RTAudioProcessor::raw (const char* id) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return 0.f;
}

int F4RTAudioProcessor::choice (const char* id) const
{
    return juce::roundToInt (raw (id));
}

float F4RTAudioProcessor::hostBpm() const
{
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 1.0)
                    return (float) *b;
    return 120.f;
}

void F4RTAudioProcessor::setF (const char* id, float v)
{
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void F4RTAudioProcessor::setChoice (const char* id, int v)
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
        p->endChangeGesture();
    }
}

void F4RTAudioProcessor::setB (const char* id, bool v)
{
    if (auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (id)))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (v ? 1.f : 0.f);
        p->endChangeGesture();
    }
}

f4rt::Params F4RTAudioProcessor::currentParams() const
{
    f4rt::Params p = f4rt::defaultParams();
    for (int i = 0; i < 4; ++i)
    {
        p.osc[i].vol = raw (ParamID::oVol[i]);
        p.osc[i].pan = raw (ParamID::oPan[i]);
        p.osc[i].coarse = raw (ParamID::oCoarse[i]);
        p.osc[i].fine = raw (ParamID::oFine[i]);
        p.osc[i].phase = raw (ParamID::oPhase[i]);
        p.osc[i].width = raw (ParamID::oWidth[i]);
        p.osc[i].type = choice (ParamID::oType[i]);
        for (int j = 0; j < 4; ++j)
            p.fm[i][j] = raw (ParamID::fm[i][j]);
    }
    p.fmEnvDepth = raw (ParamID::fmEnvDepth);
    p.formA = choice (ParamID::formA);
    p.formB = choice (ParamID::formB);
    p.formMorph = raw (ParamID::formMorph);
    p.formTrack = raw (ParamID::formTrack);
    p.formMix = raw (ParamID::formMix);
    p.filtType = choice (ParamID::filtType);
    p.cutoff = raw (ParamID::cutoff);
    p.res = raw (ParamID::res);
    p.filtDrive = raw (ParamID::filtDrive);
    p.keyTrack = raw (ParamID::keyTrack);
    p.filtEnv = raw (ParamID::filtEnv);
    p.distType = choice (ParamID::distType);
    p.distDrive = raw (ParamID::distDrive);
    p.distTone = raw (ParamID::distTone);
    p.distMix = raw (ParamID::distMix);
    for (int i = 0; i < 6; ++i)
    {
        p.env[i].a = raw (ParamID::eAtk[i]);
        p.env[i].d = raw (ParamID::eDec[i]);
        p.env[i].s = raw (ParamID::eSus[i]);
        p.env[i].r = raw (ParamID::eRel[i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        p.lfo[i].shape = choice (ParamID::lShape[i]);
        p.lfo[i].rateHz = raw (ParamID::lRate[i]);
        p.lfo[i].sync = raw (ParamID::lSync[i]) > 0.5f;
        p.lfo[i].amt = raw (ParamID::lAmt[i]);
        if (auto* arr = apvts.state.getPropertyPointer ("LFO" + juce::String (i)))
        {
            auto str = arr->toString();
            juce::StringArray parts;
            parts.addTokens (str, ",", "");
            for (int n = 0; n < f4rt::kLfoDrawN && n < parts.size(); ++n)
                p.lfoDraw[i][n] = parts[n].getFloatValue();
        }
    }
    for (int i = 0; i < 7; ++i)
        p.macros[i] = raw (ParamID::macro[i]);
    p.polyMode = choice (ParamID::polyMode);
    p.voices = juce::roundToInt (raw (ParamID::voices));
    p.level = raw (ParamID::level);
    return p;
}

f4rt::Matrix F4RTAudioProcessor::currentMatrix() const
{
    f4rt::Matrix mx;
    for (int i = 0; i < f4rt::kMatrixSlots; ++i)
    {
        mx.row[i].src = choice (ParamID::mxSrc[i]);
        mx.row[i].dest = choice (ParamID::mxDst[i]);
        mx.row[i].amt = raw (ParamID::mxAmt[i]);
        mx.row[i].on = raw (ParamID::mxOn[i]) > 0.5f;
    }
    return mx;
}

void F4RTAudioProcessor::writeParams (const f4rt::Params& p)
{
    for (int i = 0; i < 4; ++i)
    {
        setF (ParamID::oVol[i], p.osc[i].vol);
        setF (ParamID::oPan[i], p.osc[i].pan);
        setF (ParamID::oCoarse[i], p.osc[i].coarse);
        setF (ParamID::oFine[i], p.osc[i].fine);
        setF (ParamID::oPhase[i], p.osc[i].phase);
        setF (ParamID::oWidth[i], p.osc[i].width);
        setChoice (ParamID::oType[i], p.osc[i].type);
        for (int j = 0; j < 4; ++j)
            setF (ParamID::fm[i][j], p.fm[i][j]);
    }
    setF (ParamID::fmEnvDepth, p.fmEnvDepth);
    setChoice (ParamID::formA, p.formA);
    setChoice (ParamID::formB, p.formB);
    setF (ParamID::formMorph, p.formMorph);
    setF (ParamID::formTrack, p.formTrack);
    setF (ParamID::formMix, p.formMix);
    setChoice (ParamID::filtType, p.filtType);
    setF (ParamID::cutoff, p.cutoff);
    setF (ParamID::res, p.res);
    setF (ParamID::filtDrive, p.filtDrive);
    setF (ParamID::keyTrack, p.keyTrack);
    setF (ParamID::filtEnv, p.filtEnv);
    setChoice (ParamID::distType, p.distType);
    setF (ParamID::distDrive, p.distDrive);
    setF (ParamID::distTone, p.distTone);
    setF (ParamID::distMix, p.distMix);
    for (int i = 0; i < 6; ++i)
    {
        setF (ParamID::eAtk[i], p.env[i].a);
        setF (ParamID::eDec[i], p.env[i].d);
        setF (ParamID::eSus[i], p.env[i].s);
        setF (ParamID::eRel[i], p.env[i].r);
    }
    for (int i = 0; i < 4; ++i)
    {
        setChoice (ParamID::lShape[i], p.lfo[i].shape);
        setF (ParamID::lRate[i], p.lfo[i].rateHz);
        setB (ParamID::lSync[i], p.lfo[i].sync);
        setF (ParamID::lAmt[i], p.lfo[i].amt);
    }
    for (int i = 0; i < 7; ++i)
        setF (ParamID::macro[i], p.macros[i]);
    setChoice (ParamID::polyMode, p.polyMode);
    setF (ParamID::voices, (float) p.voices);
    setF (ParamID::level, p.level);
}

void F4RTAudioProcessor::randomize (int kind)
{
    auto p = currentParams();
    const int mode = choice (ParamID::randMode);
    mutateSeed += 17u;
    if (kind == f4rt::randFxOnly)
    {
        setB (ParamID::dlyOn, true);
        setF (ParamID::dlyT, 0.1f + (mutateSeed % 40) / 80.f);
        setB (ParamID::revOn, (mutateSeed & 1) != 0);
        setF (ParamID::revSz, 0.2f + (mutateSeed % 50) / 80.f);
        return;
    }
    f4rt::fillParams (p, mutateSeed, mode, kind);
    writeParams (p);
}

void F4RTAudioProcessor::loadFactory (int index)
{
    f4rt::Params p;
    f4rt::loadFactory (p, index);
    writeParams (p);
    factoryIndex = index;
    char name[64];
    f4rt::factoryName (index, name, 64);
    apvts.state.setProperty (kPatchName, juce::String (name), nullptr);
}

bool F4RTAudioProcessor::importWavetable (const juce::File& file)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
    if (r == nullptr)
        return false;
    juce::AudioBuffer<float> buf ((int) r->numChannels, (int) r->lengthInSamples);
    r->read (&buf, 0, (int) r->lengthInSamples, 0, true, true);
    engine.loadUserWt (buf.getReadPointer (0), buf.getNumSamples());
    return true;
}

void F4RTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (pendingNote.exchange (false, std::memory_order_relaxed))
    {
        juce::MidiMessage on = juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100);
        midi.addEvent (on, 0);
        juce::MidiMessage off = juce::MidiMessage::noteOff (1, 48);
        midi.addEvent (off, juce::jmax (1, buffer.getNumSamples() - 1));
    }

    auto p = currentParams();
    auto mx = currentMatrix();
    engine.render (buffer, midi, p, mx, hostBpm());
    fx.process (buffer, apvts);
    analyser.push (buffer);
    for (int i = 0; i < 6; ++i)
        envMon[i] = engine.slot[0].voice.env[i].level;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            analyser.fmVis[i][j] = p.fm[i][j];
}

juce::File F4RTAudioProcessor::patchesDir() const
{
    return ensureDir (libraryRoot().getChildFile ("Patches"));
}

juce::File F4RTAudioProcessor::favFile() const
{
    return ensureDir (libraryRoot()).getChildFile ("favorites.txt");
}

juce::StringArray F4RTAudioProcessor::patchNames() const
{
    juce::StringArray names;
    for (const auto& entry : juce::RangedDirectoryIterator (patchesDir(), false, "*.f4rtp"))
        names.add (entry.getFile().getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

juce::String F4RTAudioProcessor::patchName() const
{
    return apvts.state.getProperty (kPatchName, kInitPatch).toString();
}

void F4RTAudioProcessor::initPatch()
{
    for (auto* base : getParameters())
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (base);
        if (param == nullptr)
            continue;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->getDefaultValue());
        param->endChangeGesture();
    }
    apvts.state.setProperty (kPatchName, kInitPatch, nullptr);
}

bool F4RTAudioProcessor::loadPatchByName (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitPatch)
    {
        initPatch();
        return true;
    }
    const auto src = patchesDir().getChildFile (stem + kPatchExt);
    auto xml = juce::XmlDocument::parse (src);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    apvts.state.setProperty (kPatchName, stem, nullptr);
    return true;
}

bool F4RTAudioProcessor::savePatchAs (const juce::String& name)
{
    const auto stem = legalStem (name);
    if (stem.isEmpty() || stem == kInitPatch)
        return false;
    apvts.state.setProperty (kPatchName, stem, nullptr);
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr)
        return false;
    return patchesDir().getChildFile (stem + kPatchExt).replaceWithText (xml->toString());
}

bool F4RTAudioProcessor::savePatch()
{
    const auto name = patchName();
    if (name.isEmpty() || name == kInitPatch)
        return false;
    return savePatchAs (name);
}

bool F4RTAudioProcessor::exportPatch (const juce::File& dest)
{
    auto file = dest.hasFileExtension ("f4rtp") ? dest : dest.withFileExtension ("f4rtp");
    auto xml = apvts.copyState().createXml();
    if (xml == nullptr)
        return false;
    return file.replaceWithText (xml->toString());
}

bool F4RTAudioProcessor::importPatch (const juce::File& src)
{
    const auto stem = legalStem (src.getFileNameWithoutExtension());
    if (stem.isEmpty())
        return false;
    const auto dest = patchesDir().getChildFile (stem + kPatchExt);
    if (src.getFullPathName() != dest.getFullPathName())
    {
        dest.deleteFile();
        if (! src.copyFileTo (dest))
            return false;
    }
    return loadPatchByName (stem);
}

juce::StringArray F4RTAudioProcessor::favoriteNames() const
{
    juce::StringArray a;
    a.addLines (favFile().loadFileAsString());
    a.trim();
    a.removeEmptyStrings();
    return a;
}

void F4RTAudioProcessor::toggleFavorite (const juce::String& name)
{
    auto a = favoriteNames();
    if (a.contains (name))
        a.removeString (name);
    else
        a.add (name);
    favFile().replaceWithText (a.joinIntoString ("\n"));
}

bool F4RTAudioProcessor::isFavorite (const juce::String& name) const
{
    return favoriteNames().contains (name);
}

void F4RTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        xml->setAttribute ("uiScale", uiScale);
        copyXmlToBinary (*xml, destData);
    }
}

void F4RTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            uiScale = juce::jlimit (0.4f, 1.5f, (float) xml->getDoubleAttribute ("uiScale", uiScale));
            xml->removeAttribute ("uiScale");
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
        }
}

juce::AudioProcessorEditor* F4RTAudioProcessor::createEditor()
{
    return new F4RTAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new F4RTAudioProcessor();
}
