#include "PluginProcessor.h"
#include <iostream>
#include <cstdlib>
#define CHECK(x) do { if(!(x)){std::cerr << "Failed at line " << __LINE__ << ": " #x "\n";std::exit(1);} }while(false)
int main(){
    juce::ScopedJuceInitialiser_GUI gui;
    const auto directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("blucher-test","",false);
    CHECK(directory.createDirectory().wasOk());
    const auto source=directory.getChildFile("source.wav");
    juce::AudioBuffer<float> sample(2,44100);
    for(int i=0;i<44100;++i)for(int ch=0;ch<2;++ch)sample.setSample(ch,i,float(std::sin(i*.05))* .6f);
    juce::WavAudioFormat wav;
    auto stream=source.createOutputStream();CHECK(stream!=nullptr);
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),44100,2,24,{},0));CHECK(writer!=nullptr);stream.release();CHECK(writer->writeFromAudioSampleBuffer(sample,0,44100));writer.reset();
    BlucherProcessor original;CHECK(original.loadSample(0,source).isEmpty());CHECK(original.loadSample(1,source).isEmpty());original.generate(false);original.toggleStep(3,true);original.swapAB();original.generate(false);original.toggleStep(2,true);
    {const juce::ScopedLock l(original.mutex);original.locks.pitch=true;}
    juce::MemoryBlock state;original.getStateInformation(state);CHECK(state.getSize()>44100*8);
    CHECK(source.deleteFile()); // Session restore must not rely on original files.
    BlucherProcessor restored;restored.setStateInformation(state.getData(),int(state.getSize()));CHECK(restored.slotName(0)==original.slotName(0));CHECK(restored.getPattern().seed==original.getPattern().seed);CHECK(restored.getPattern().notes[2].locked);CHECK(restored.locks.pitch);
    original.prepareToPlay(44100,512);restored.prepareToPlay(44100,512);original.preview=true;restored.preview=true;
    double energy=0;
    for(int block=0;block<32;++block){juce::AudioBuffer<float> a(2,512),b(2,512);juce::MidiBuffer ma,mb;original.processBlock(a,ma);restored.processBlock(b,mb);for(int ch=0;ch<2;++ch)for(int i=0;i<512;++i){CHECK(a.getSample(ch,i)==b.getSample(ch,i));energy+=std::abs(a.getSample(ch,i));}}
    CHECK(energy>1);
    CHECK(original.exportWav(directory.getChildFile("a.wav")));CHECK(restored.exportWav(directory.getChildFile("b.wav")));
    juce::MemoryBlock aFile,bFile;CHECK(directory.getChildFile("a.wav").loadFileAsData(aFile));CHECK(directory.getChildFile("b.wav").loadFileAsData(bFile));CHECK(aFile==bFile);
    std::unique_ptr<juce::AudioFormatReader> reader(wav.createReaderFor(directory.getChildFile("b.wav").createInputStream().release(),true));CHECK(reader!=nullptr);CHECK(reader->lengthInSamples==88200);reader.reset();
    original.swapAB();restored.swapAB();CHECK(original.getPattern().seed==restored.getPattern().seed);CHECK(original.getPattern().notes[3].locked==restored.getPattern().notes[3].locked);
    juce::MemoryBlock again;restored.getStateInformation(again);CHECK(again.getSize()<state.getSize()+4096);
    CHECK(!restored.loadSample(0,source).isEmpty()); // Failed loads preserve the bank.
    CHECK(restored.slotName(0).contains("source.wav"));
    CHECK(directory.deleteRecursively());
    std::cout<<"State tests passed: embedded samples, source-free restore, A/B, locks, exact playback and WAV roundtrip.\n";
}
