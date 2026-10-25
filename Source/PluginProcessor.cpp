#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessorValueTreeState::ParameterLayout BlucherProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"chaos",1},"Chaos",0.f,1.f,.45f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"sensitivity",1},"Slice sensitivity",0.f,1.f,.45f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gain",1},"Output",0.f,1.f,.75f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bpm",1},"Preview BPM",40.f,240.f,120.f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"division",1},"Step length",juce::StringArray{"1/2","1/4","1/8","1/16","1/32","1/8 triplet","1/16 triplet","1/16 dotted"},3));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"grain",1},"Grain mode",false));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"formant",1},"Formant semitones",-12.f,12.f,0.f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"spectral",1},"Spectral blend",0.f,1.f,0.f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"vocoder",1},"Vocoder mix",0.f,1.f,0.f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"clarity",1},"Vocal clarity",0.f,1.f,.2f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"layerMix",1},"Layer mix",0.f,1.f,.5f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"layers",1},"Layer count",juce::StringArray{"1 voice","2 voices","3 voices","4 voices"},0));
    juce::StringArray slots;for(int i=1;i<=8;++i)slots.add("Slot "+juce::String(i));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"blendSlot",1},"Spectral source",slots,1));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"vocalSlot",1},"Vocal source",slots,0));
    slots.insert(0,"Internal synth");
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"carrier",1},"Vocoder carrier",slots,0));
    return p;
}
BlucherProcessor::BlucherProcessor() : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)), parameters(*this,nullptr,"BLUCHER",layout()) { setLatencySamples(blucher::VoiceRenderer::latency); }
bool BlucherProcessor::isBusesLayoutSupported(const BusesLayout& b) const { return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() || b.getMainOutputChannelSet()==juce::AudioChannelSet::mono(); }
void BlucherProcessor::prepareToPlay(double r,int) { rate=r; previewBeat=0; held.fill(false); transpose=0; wasPreview=false; rendererActive=false; renderer.prepare(r); }
void BlucherProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    double bpm=parameters.getRawParameterValue("bpm")->load();
    double beat=previewBeat;
    bool hostPlaying=false, hostPosition=false;
    if (auto* playhead=getPlayHead()) if (auto pos=playhead->getPosition()) {
        hostPlaying=pos->getIsPlaying();
        if (auto tempo=pos->getBpm()) if (*tempo>0 && std::isfinite(*tempo)) bpm=*tempo;
        if (auto ppq=pos->getPpqPosition()) { if(std::isfinite(*ppq)) { beat=*ppq; hostPosition=true; } }
    }
    currentBpm.store(bpm);
    const bool audition=preview.load();
    if (audition && !wasPreview && !hostPlaying) previewBeat=0;
    wasPreview=audition;
    if (!hostPlaying || !hostPosition) beat=previewBeat;
    const int division=int(parameters.getRawParameterValue("division")->load());
    const bool grain=parameters.getRawParameterValue("grain")->load()>.5f;
    const float gain=parameters.getRawParameterValue("gain")->load();
    const auto sound=soundSettings();
    const juce::ScopedTryLock lock(mutex);
    auto event=midi.cbegin();
    for(int i=0;i<audio.getNumSamples();++i) {
        while(event!=midi.cend() && (*event).samplePosition<=i) {
            auto message=(*event).getMessage();
            if(message.isNoteOn()) { held[size_t(message.getNoteNumber())]=true; transpose=message.getNoteNumber()-60; }
            if(message.isNoteOff()) held[size_t(message.getNoteNumber())]=false;
            if(message.isAllNotesOff() || message.isAllSoundOff()) held.fill(false);
            ++event;
        }
        bool midiPlaying=std::any_of(held.begin(),held.end(),[](bool h){return h;});
        if(lock.isLocked()) {
            // Restart the spectral buffers on the next activation to avoid stale audio.
            const bool active=hostPlaying || audition || midiPlaying;
            std::array<float,2> value{};
            if(active) { if(!rendererActive)renderer.prepare(rate);rendererActive=true;value=renderer.process(engine,beat,bpm,division,midiPlaying ? transpose : 0,grain,sound); }
            else rendererActive=false;
            for(auto& v:value)v=std::isfinite(v)?juce::jlimit(-1.f,1.f,v*gain):0.f;
            if(audio.getNumChannels()==1) audio.setSample(0,i,(value[0]+value[1])*.5f);
            else { audio.setSample(0,i,value[0]); audio.setSample(1,i,value[1]); }
        }
        if(hostPlaying || audition || midiPlaying) beat+=bpm/(60.0*rate);
    }
    previewBeat=beat;
    currentStep.store((hostPlaying || audition || std::any_of(held.begin(),held.end(),[](bool h){return h;})) ? int(std::fmod(std::floor(beat/blucher::Engine::stepBeats(division))+160000,16)) : -1);
    midi.clear();
}
blucher::SoundSettings BlucherProcessor::soundSettings() const {
    blucher::SoundSettings s;
    auto value=[this](const char* id){return parameters.getRawParameterValue(id)->load();};
    s.spectral={value("formant"),value("spectral"),value("vocoder"),value("clarity")};
    s.layers=1+int(value("layers"));s.layerMix=value("layerMix");s.blendSlot=int(value("blendSlot"));s.vocalSlot=int(value("vocalSlot"));s.carrier=int(value("carrier"));return s;
}
void BlucherProcessor::randomizeSound() {
    // Called by the editor; category locks constrain sound randomization too.
    blucher::Locks current;std::vector<int> available;
    {const juce::ScopedLock l(mutex);checkpoint();current=locks;for(int i=0;i<8;++i)if(engine.samples[size_t(i)].loaded())available.push_back(i);}
    auto& random=juce::Random::getSystemRandom();float chaos=parameters.getRawParameterValue("chaos")->load();
    auto set=[this](const char* id,float value){auto* p=parameters.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();};
    if(!current.pitch)set("formant",(random.nextFloat()*2-1)*12*chaos);
    if(!current.effects){set("spectral",random.nextFloat()*chaos);set("vocoder",random.nextFloat()*chaos);set("clarity",.15f+random.nextFloat()*.65f);set("layers",float(random.nextInt(4)));set("layerMix",random.nextFloat()*.7f);}
    if(!current.source && !available.empty()){set("blendSlot",float(available[size_t(random.nextInt(int(available.size())))]));set("vocalSlot",float(available[size_t(random.nextInt(int(available.size())))]));set("carrier",random.nextBool()?0.f:float(1+available[size_t(random.nextInt(int(available.size())))]));}
}
void BlucherProcessor::applySound(const blucher::SoundSettings& s) {
    auto set=[this](const char* id,float value){auto* p=parameters.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();};
    set("formant",s.spectral.formant);set("spectral",s.spectral.blend);set("vocoder",s.spectral.vocoder);set("clarity",s.spectral.clarity);
    set("layers",float(s.layers-1));set("layerMix",s.layerMix);set("blendSlot",float(s.blendSlot));set("vocalSlot",float(s.vocalSlot));set("carrier",float(s.carrier));
}
void BlucherProcessor::checkpoint() { history.push_back(engine.pattern);soundHistory.push_back(soundSettings()); if(history.size()>64){history.erase(history.begin());soundHistory.erase(soundHistory.begin());} }
void BlucherProcessor::generate(bool mutate) { const juce::ScopedLock l(mutex); checkpoint(); engine.generate(uint32_t(juce::Random::getSystemRandom().nextInt()),parameters.getRawParameterValue("chaos")->load(),mutate,locks); }
void BlucherProcessor::undo() { blucher::SoundSettings sound;{const juce::ScopedLock l(mutex);if(history.empty())return;engine.pattern=history.back();history.pop_back();sound=soundHistory.back();soundHistory.pop_back();}applySound(sound); }
void BlucherProcessor::swapAB() { const juce::ScopedLock l(mutex); checkpoint(); std::swap(engine.pattern,alternate); }
void BlucherProcessor::toggleStep(int i,bool lock) { const juce::ScopedLock l(mutex); if(i<0||i>=16)return; checkpoint(); auto& s=engine.pattern.notes[size_t(i)]; if(lock)s.locked=!s.locked; else s.enabled=!s.enabled; }
void BlucherProcessor::editStep(int i,int action) {
    const juce::ScopedLock l(mutex); if(i<0||i>=16||action==0)return;checkpoint();auto& s=engine.pattern.notes[size_t(i)];
    if(action>=100&&action<108){s.slot=action-100;s.slice=0;}
    if(action==1)s.reverse=!s.reverse;
    if(action==2)s.slice=std::max(0,s.slice-1);
    if(action==3){const auto& sample=engine.samples[size_t(s.slot)];s.slice=std::min(s.slice+1,std::max(0,int(sample.cuts.size())-2));}
    if(action==4)s.semitones=std::max(-24,s.semitones-1);
    if(action==5)s.semitones=std::min(24,s.semitones+1);
    if(action==6)s.semitones=0;
    if(action>=11&&action<=18)s.repeats=action-10;
}
blucher::Pattern BlucherProcessor::getPattern() const { const juce::ScopedLock l(mutex); return engine.pattern; }
juce::String BlucherProcessor::slotName(int i) const { const juce::ScopedLock l(mutex); const auto& s=engine.samples[size_t(i)]; return names[size_t(i)].isEmpty() ? "Drop sample / click to load" : names[size_t(i)]+"  /  "+juce::String(int(s.cuts.size())-1)+" slices"; }
void BlucherProcessor::reslice() { const juce::ScopedLock l(mutex); for(auto& s:engine.samples) if(s.loaded())s.slice(parameters.getRawParameterValue("sensitivity")->load()); }
juce::String BlucherProcessor::loadSample(int slot,const juce::File& file) {
    if(slot<0||slot>=8)return "Invalid slot";
    juce::AudioFormatManager formats; formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if(!reader)return "Unsupported or unreadable audio file";
    if(reader->lengthInSamples<2 || reader->lengthInSamples>reader->sampleRate*30 || reader->sampleRate<8000 || reader->sampleRate>192000)return "Use a sample between 2 frames and 30 seconds, at 8–192 kHz";
    juce::AudioBuffer<float> buffer(2,int(reader->lengthInSamples));
    if(!reader->read(&buffer,0,buffer.getNumSamples(),0,true,true))return "Could not decode audio";
    blucher::Sample sample; sample.rate=reader->sampleRate;
    sample.left.assign(buffer.getReadPointer(0),buffer.getReadPointer(0)+buffer.getNumSamples());
    sample.right.assign(buffer.getReadPointer(1),buffer.getReadPointer(1)+buffer.getNumSamples());
    for(auto* channel:{&sample.left,&sample.right})for(auto& value:*channel)value=std::isfinite(value)?juce::jlimit(-1.f,1.f,value):0.f;
    sample.slice(parameters.getRawParameterValue("sensitivity")->load());
    {const juce::ScopedLock l(mutex); engine.samples[size_t(slot)]=std::move(sample); names[size_t(slot)]=file.getFileName();}
    return {};
}
bool BlucherProcessor::exportWav(const juce::File& file) {
    blucher::Engine snapshot; {const juce::ScopedLock l(mutex);snapshot=engine;}
    double bpm=currentBpm.load(); if(bpm<=0)bpm=120;
    const int division=int(parameters.getRawParameterValue("division")->load());
    const bool grain=parameters.getRawParameterValue("grain")->load()>.5f;
    const float gain=parameters.getRawParameterValue("gain")->load();
    constexpr double exportRate=44100;
    const int frames=int(std::ceil(16*blucher::Engine::stepBeats(division)*60/bpm*exportRate));
    juce::AudioBuffer<float> output(2,frames);
    const auto sound=soundSettings();blucher::VoiceRenderer offline;offline.prepare(exportRate);
    for(int i=0;i<frames+blucher::VoiceRenderer::latency;++i){auto v=offline.process(snapshot,i*bpm/(60*exportRate),bpm,division,0,grain,sound);if(i>=blucher::VoiceRenderer::latency)for(int ch=0;ch<2;++ch){float value=v[size_t(ch)]*gain;output.setSample(ch,i-blucher::VoiceRenderer::latency,std::isfinite(value)?juce::jlimit(-1.f,1.f,value):0.f);}}
    juce::TemporaryFile temp(file);
    auto stream=temp.getFile().createOutputStream(); if(!stream)return false;
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(format.createWriterFor(stream.get(),exportRate,2,24,{},0));
    if(!writer)return false;
    stream.release(); bool success=writer->writeFromAudioSampleBuffer(output,0,frames); writer.reset();
    return success && temp.overwriteTargetFileWithTemporary();
}
namespace {
juce::ValueTree patternTree(const blucher::Pattern& p,const char* type) {
    juce::ValueTree tree(type);tree.setProperty("seed",juce::int64(p.seed),nullptr);
    for(const auto& s:p.notes){juce::ValueTree n("STEP");n.setProperty("slot",s.slot,nullptr);n.setProperty("slice",s.slice,nullptr);n.setProperty("pitch",s.semitones,nullptr);n.setProperty("repeats",s.repeats,nullptr);n.setProperty("gain",s.gain,nullptr);n.setProperty("gate",s.gate,nullptr);n.setProperty("on",s.enabled,nullptr);n.setProperty("reverse",s.reverse,nullptr);n.setProperty("lock",s.locked,nullptr);tree.addChild(n,-1,nullptr);}return tree;
}
blucher::Pattern readPattern(juce::ValueTree t) {
    blucher::Pattern p;p.seed=uint32_t(juce::int64(t.getProperty("seed",1)));
    for(int i=0;i<juce::jmin(16,t.getNumChildren());++i){auto n=t.getChild(i);auto& s=p.notes[size_t(i)];s.slot=juce::jlimit(0,7,int(n["slot"]));s.slice=juce::jlimit(0,127,int(n["slice"]));s.semitones=juce::jlimit(-24,24,int(n["pitch"]));s.repeats=juce::jlimit(1,8,int(n["repeats"]));s.gain=juce::jlimit(0.f,1.f,float(n["gain"]));s.gate=juce::jlimit(.05f,1.f,float(n["gate"]));s.enabled=bool(n["on"]);s.reverse=bool(n["reverse"]);s.locked=bool(n["lock"]);}return p;
}
}
void BlucherProcessor::getStateInformation(juce::MemoryBlock& block) {
    auto tree=parameters.copyState();const juce::ScopedLock l(mutex);
    tree.addChild(patternTree(engine.pattern,"PATTERN"),-1,nullptr);tree.addChild(patternTree(alternate,"ALTERNATE"),-1,nullptr);
    tree.setProperty("rhythmLock",locks.rhythm,nullptr);tree.setProperty("sourceLock",locks.source,nullptr);tree.setProperty("pitchLock",locks.pitch,nullptr);tree.setProperty("fxLock",locks.effects,nullptr);
    for(int i=0;i<8;++i){const auto& s=engine.samples[size_t(i)];if(!s.loaded())continue;juce::ValueTree t("SAMPLE");t.setProperty("slot",i,nullptr);t.setProperty("name",names[size_t(i)],nullptr);t.setProperty("rate",s.rate,nullptr);
        juce::MemoryBlock pcm; {juce::MemoryOutputStream out(pcm,false);for(size_t j=0;j<s.left.size();++j){out.writeFloat(s.left[j]);out.writeFloat(s.right[j]);}}
        t.setProperty("pcm",juce::var(pcm),nullptr);juce::StringArray cuts;for(int cut:s.cuts)cuts.add(juce::String(cut));t.setProperty("cuts",cuts.joinIntoString(","),nullptr);tree.addChild(t,-1,nullptr);}
    juce::MemoryOutputStream output(block,false); tree.writeToStream(output);
}
void BlucherProcessor::setStateInformation(const void* data,int size) {
    auto tree=juce::ValueTree::readFromData(data,size);if(!tree.isValid() || !tree.hasType("BLUCHER"))return;
    blucher::Engine restored;std::array<juce::String,8> restoredNames;
    restored.pattern=readPattern(tree.getChildWithName("PATTERN"));
    for(int i=0;i<tree.getNumChildren();++i){auto t=tree.getChild(i);if(!t.hasType("SAMPLE"))continue;int slot=int(t["slot"]);if(slot<0||slot>=8)continue;auto pcm=t["pcm"];auto* bytes=pcm.getBinaryData();double sr=double(t["rate"]);if(!bytes || sr<8000 || sr>192000 || bytes->getSize()%8!=0 || bytes->getSize()>size_t(sr*30)*8)continue;
        auto& s=restored.samples[size_t(slot)];s.rate=sr;juce::MemoryInputStream input(*bytes,false);size_t count=bytes->getSize()/8;s.left.resize(count);s.right.resize(count);for(size_t j=0;j<count;++j){float a=input.readFloat(),b=input.readFloat();s.left[j]=std::isfinite(a)?juce::jlimit(-1.f,1.f,a):0;s.right[j]=std::isfinite(b)?juce::jlimit(-1.f,1.f,b):0;}
        auto cuts=juce::StringArray::fromTokens(t["cuts"].toString(),",","");for(const auto& c:cuts){int v=c.getIntValue();if(v>=0&&v<=int(count)&&(s.cuts.empty()||v>s.cuts.back()))s.cuts.push_back(v);}if(s.cuts.size()<2||s.cuts.front()!=0||s.cuts.back()!=int(count))s.slice();restoredNames[size_t(slot)]=t["name"].toString();}
    auto alternateRestored=readPattern(tree.getChildWithName("ALTERNATE"));
    blucher::Locks restoredLocks{bool(tree["rhythmLock"]),bool(tree["sourceLock"]),bool(tree["pitchLock"]),bool(tree["fxLock"])};
    // Keep APVTS free of embedded audio; this prevents duplicate sample data on each save.
    for(int i=tree.getNumChildren()-1;i>=0;--i)if(tree.getChild(i).hasType("SAMPLE")||tree.getChild(i).hasType("PATTERN")||tree.getChild(i).hasType("ALTERNATE"))tree.removeChild(i,nullptr);
    parameters.replaceState(tree);
    const juce::ScopedLock l(mutex);engine=std::move(restored);names=std::move(restoredNames);alternate=alternateRestored;locks=restoredLocks;history.clear();soundHistory.clear();renderer.prepare(rate);rendererActive=false;
}
juce::AudioProcessorEditor* BlucherProcessor::createEditor(){return new BlucherEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BlucherProcessor();}
