#pragma once
#include <JuceHeader.h>
#include "BlucherEngine.h"
class BlucherProcessor : public juce::AudioProcessor {
public:
    BlucherProcessor();
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "The Blucher"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::String loadSample(int,const juce::File&);
    bool exportWav(const juce::File&);
    void generate(bool mutate);
    void undo();
    void swapAB();
    void toggleStep(int,bool lock);
    void editStep(int,int action);
    void reslice();
    juce::String slotName(int) const;
    blucher::Pattern getPattern() const;
    juce::AudioProcessorValueTreeState parameters;
    mutable juce::CriticalSection mutex;
    blucher::Locks locks;
    std::atomic<bool> preview{false};
    std::atomic<int> currentStep{-1};
    std::atomic<double> currentBpm{120};
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void checkpoint();
    blucher::Engine engine;
    std::array<juce::String,blucher::slots> names;
    std::vector<blucher::Pattern> history;
    blucher::Pattern alternate;
    double rate=44100, previewBeat=0;
    std::array<bool,128> held{};
    int transpose=0;
    bool wasPreview=false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlucherProcessor)
};
