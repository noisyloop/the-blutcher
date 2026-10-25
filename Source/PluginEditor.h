#pragma once
#include "PluginProcessor.h"
class BlucherStepButton : public juce::TextButton {
public:
    void paintButton(juce::Graphics& g,bool over,bool down) override {
        auto colour=findColour(juce::TextButton::buttonColourId);
        g.setColour(colour.brighter(down?.2f:(over?.1f:0.f)));
        g.fillRoundedRectangle(getLocalBounds().toFloat(),5.f);
        g.setColour(findColour(juce::TextButton::textColourOffId));
        g.setFont(12.f);
        g.drawFittedText(getButtonText(),getLocalBounds().reduced(3),juce::Justification::centred,3);
    }
};
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
    void showSound(bool);
    BlucherProcessor& processor;
    std::array<juce::TextButton,8> sampleButtons;
    std::array<BlucherStepButton,16> stepButtons;
    std::array<juce::ToggleButton,4> lockButtons;
    std::array<juce::Slider,4> sliders;
    std::array<juce::Label,4> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    juce::TextButton generate{"GENERATE"},mutate{"MUTATE"},undo{"UNDO"},ab{"SWAP A/B"},play{"PREVIEW"},slice{"RE-SLICE"},save{"EXPORT WAV"},drag{"DRAG WAV"};
    juce::TextButton soundTab{"SOUND LAB"}, soundRandom{"RANDOMIZE SOUND"};
    bool soundVisible=false;
    std::array<juce::Slider,5> soundSliders;
    std::array<juce::Label,5> soundLabels;
    std::array<juce::ComboBox,4> soundChoices;
    std::array<juce::Label,4> choiceLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> soundChoiceAttachments;
    juce::ComboBox division;
    juce::ToggleButton grains{"Grain mode"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> grainAttachment;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File dragFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlucherEditor)
};
