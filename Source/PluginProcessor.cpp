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
    return p;
}
BlucherProcessor::BlucherProcessor() : AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)), parameters(*this,nullptr,"BLUCHER",layout()) {}
bool BlucherProcessor::isBusesLayoutSupported(const BusesLayout& b) const { return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo() || b.getMainOutputChannelSet()==juce::AudioChannelSet::mono(); }
void BlucherProcessor::prepareToPlay(double r,int) { rate=r; previewBeat=0; held.fill(false); transpose=0; wasPreview=false; }
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
        if(lock.isLocked() && (hostPlaying || audition || midiPlaying)) {
            auto value=engine.atBeat(beat,bpm,division,midiPlaying ? transpose : 0,grain);
            if(audio.getNumChannels()==1) audio.setSample(0,i,(value[0]+value[1])*.5f*gain);
            else { audio.setSample(0,i,value[0]*gain); audio.setSample(1,i,value[1]*gain); }
        }
        if(hostPlaying || audition || midiPlaying) beat+=bpm/(60.0*rate);
    }
    previewBeat=beat;
    currentStep.store((hostPlaying || audition || std::any_of(held.begin(),held.end(),[](bool h){return h;})) ? int(std::fmod(std::floor(beat/blucher::Engine::stepBeats(division))+160000,16)) : -1);
    midi.clear();
}
void BlucherProcessor::checkpoint() { history.push_back(engine.pattern); if(history.size()>64) history.erase(history.begin()); }
void BlucherProcessor::generate(bool mutate) { const juce::ScopedLock l(mutex); checkpoint(); engine.generate(uint32_t(juce::Random::getSystemRandom().nextInt()),parameters.getRawParameterValue("chaos")->load(),mutate,locks); }
void BlucherProcessor::undo() { const juce::ScopedLock l(mutex); if(!history.empty()){ engine.pattern=history.back(); history.pop_back(); } }
void BlucherProcessor::swapAB() { const juce::ScopedLock l(mutex); checkpoint(); std::swap(engine.pattern,alternate); }
void BlucherProcessor::toggleStep(int i,bool lock) { const juce::ScopedLock l(mutex); if(i<0||i>=16)return; checkpoint(); auto& s=engine.pattern.notes[size_t(i)]; if(lock)s.locked=!s.locked; else s.enabled=!s.enabled; }
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
    for(int i=0;i<frames;++i){auto v=snapshot.atBeat(i*bpm/(60*exportRate),bpm,division,0,grain);for(int ch=0;ch<2;++ch)output.setSample(ch,i,v[size_t(ch)]*gain);}
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
        juce::MemoryBlock pcm; juce::MemoryOutputStream out(pcm,false);for(size_t j=0;j<s.left.size();++j){out.writeFloat(s.left[j]);out.writeFloat(s.right[j]);}
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
    const juce::ScopedLock l(mutex);engine=std::move(restored);names=std::move(restoredNames);alternate=alternateRestored;locks=restoredLocks;history.clear();
}
juce::AudioProcessorEditor* BlucherProcessor::createEditor(){return new BlucherEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BlucherProcessor();}
