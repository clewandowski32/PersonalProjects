/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//inherits from LookAndFeel_V4 to allow drawing of custom rotary slider
struct LookAndFeel : juce::LookAndFeel_V4{
    void drawRotarySlider (juce::Graphics& g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                           juce::Slider& slider) override;
};

//custom rotary slider class inheriting from the juce::Slider class
struct RotarySliderWithLabels : juce::Slider{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                      juce::Slider::TextEntryBoxPosition::NoTextBox), param(&rap), suffix(unitSuffix){
      setLookAndFeel(&lnf);
  }
    
    ~RotarySliderWithLabels(){
        setLookAndFeel(nullptr);
    }
    
    //label associated with min and max values of each slider
    struct LabelPos{
        float pos;
        juce::String label;
    };
    
    //data structure for storing labels for each slider
    juce::Array<LabelPos> labels;
    
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    //return the height of the display text which will display current parameter values
    int getTextHeight() const {return 14;}
    //returns the display text itself
    juce::String getDisplayString() const;
private:
    //param value associated with each slider that will allow each slider to display param values along with modifying them
    juce::RangedAudioParameter* param;
    //suffix associated with each parameter i.e. Hz for filter freq, dB/Oct for cut filter slopes, dB for peak gain
    juce::String suffix;
    
    //in charge of drawing the custom rotary sliders
    LookAndFeel lnf;
};

//==============================================================================
/**
*/
//struct in charging of drawing the response curve
//ResponseCurveComponent only controls its own bounds so it will only draw within its own bounds
struct ResponseCurveComponent : juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();
    
    /** Receives a callback when a parameter has been changed.

        IMPORTANT NOTE: This will be called synchronously when a parameter changes, and
        many audio processors will change their parameter during their audio callback.
        This means that not only has your handler code got to be completely thread-safe,
        but it's also got to be VERY fast, and avoid blocking. If you need to handle
        this event on your message thread, use this callback to trigger an AsyncUpdater
        or ChangeBroadcaster which you can respond to on the message thread.
    */
    void parameterValueChanged (int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override{}
    
    /** The user-defined callback routine that actually gets called periodically.

        It's perfectly ok to call startTimer() or stopTimer() from within this
        callback to change the subsequent intervals.
    */
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    //returns the area in which the background and response curve will be drawn
    

private:
    SimpleEQAudioProcessor& audioProcessor;
    
    //will be set to true by the parameterValueChanged function to trigger a repaint of the responseCurve
    juce::Atomic<bool> parametersChanged {false};
    
    //instance of monochain needed to draw the response curve as copies of parameter values are needed
    MonoChain monoChain;
    
    //updates the monoChain's values, called in timerCallback and constructor
    void updateChain();
    
    //prerendered Image for response curve background plot
    juce::Image background;
    
    //draws region for rendering the component i.e the background region
    juce::Rectangle<int> getRenderArea();
    //draws region for rendering just the response curve which much be slightly smalelr than render area as vertical and
    //horizontal bounds of plot do not lie directly on edges of render area
    juce::Rectangle<int> getAnalysisArea();
    
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChanelFifo;
};

class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
   

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
    RotarySliderWithLabels peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    //alias for attaching the parameter value each slider represents
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment peakFreqSliderAttachment,
    peakGainSliderAttachment,
    peakQualitySliderAttachment,
    lowCutFreqSliderAttachment,
    highCutFreqSliderAttachment,
    lowCutSlopeSliderAttachment,
    highCutSlopeSliderAttachment;
    
    //returns a vector of sliders
    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
