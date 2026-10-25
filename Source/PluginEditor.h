#pragma once
#include "PluginProcessor.h"
class BlucherEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer {
public:
    explicit BlucherEditor(BlucherProcessor&);
    ~BlucherEditor() override { stopTimer(); }
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray&,int,int) override;
private:
    void timerCallback() override;
    void chooseSample(int);
    void load(int,const juce::File&);
    void exportAudio(bool drag);
    BlucherProcessor& processor;
    std::array<juce::TextButton,8> sampleButtons;
    std::array<juce::TextButton,16> stepButtons;
    std::array<juce::ToggleButton,4> lockButtons;
    std::array<juce::Slider,4> sliders;
    std::array<juce::Label,4> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    juce::TextButton generate{"GENERATE"},mutate{"MUTATE"},undo{"UNDO"},ab{"SWAP A/B"},play{"PREVIEW"},slice{"RE-SLICE"},save{"EXPORT WAV"},drag{"DRAG WAV"};
    juce::ComboBox division;
    juce::ToggleButton grains{"Grain mode"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> grainAttachment;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File dragFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlucherEditor)
};
