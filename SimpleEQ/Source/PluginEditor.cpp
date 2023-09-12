/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider){
    using namespace juce;
    
    //rectangular bounds of rotary slider
    auto bounds = Rectangle<float>(x, y, width, height);
    
    //draws the circle representing each slider
    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);
    
    //draws a border around each slider
    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);
    
    //the juce::Slider object must be cast to a RotarySliderWithLabels to use the getTextHeight and getDisplayString methods
    if(auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider)){
        auto center = bounds.getCentre();
        
        Path p;
        
        //sets up a rectangle which will represent where the slider's level is at
        Rectangle<float> r;
        r.setLeft(center.getX()-2);
        r.setRight(center.getX()+2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY()-rswl->getTextHeight() * 1.5);
        
        p.addRoundedRectangle(r, 2.f);
        
        //rotating the rectangle to the correct angle
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        //mapping normalized rotary angle to be between rotary start and end angles
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        //rotate Path object around the center based on current position of the slider
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        
        g.fillPath(p);
        
        //draws the value of each parameter
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);
        
        g.setColour(Colours::black);
        g.fillRect(r);
        
        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
    
    
    
}


//paints each custom rotary slider
void RotarySliderWithLabels::paint(juce::Graphics &g){
    using namespace juce;
    
    //specifies the start and end angle of each slider in radians
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) +  MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);

    //call to drawRotarySlider override in LookAndFeel struct
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      //maps sliders current value from getValue() from its range to a normalized range
                                      //between 0 and 1
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng, endAng, *this);
    
    //drawing start and end labels for each parameter
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i){
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        //mapping the normalized position value to a range between start and end angle
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        //positioning the center of each text box slightly outside of the bounds of each slider
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
        
    }
    
}

//sets the bounds of each slider to be a square positioned at the top of each component's bounds
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);
    
    return r;
    
}

//returns the parameter value to displayed by each rotary slider in drawRotarySlider
juce::String RotarySliderWithLabels::getDisplayString() const{
    //if param successfully casted to a choice then its a slope param
    //the current choice's name at its given index should just be returned i.e it will return 12 dB/Oct, 24 dB/Oct, etc
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)){
        return choiceParam->getCurrentChoiceName();
    }
    juce::String str;
    //checking to see if param value exceeded 1000 in which /= 1000 and set addK to true
    bool addK = false;
    
    //all other params should be of this type but its good practice to check
    if(auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)){
        float val = getValue();
        
        if(val > 999.f){
            val /= 1000.f;
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 : 0));
    }
    else{
        jassertfalse;
    }
    //appending suffix if not empty with k if addK
    if(suffix.isNotEmpty()){
        str << " ";
        if (addK){
            str << "k";
        }
        str << suffix;
    }
    
    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor &p) :
audioProcessor(p),
leftChanelFifo(&audioProcessor.leftChannelFifo)
{
    //getting parameters and making the editor a listener to allow it to update the responseCurve on param changes
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->addListener(this);
    }
    
    //assures response curve monoChain values are set correctly upon loading the GUIs
    updateChain();
    
    //starting timer with 60 Hz refresh rate
    //parameterValueChanged function from Listener class will set an atomic flag which will be checked by timerCallback
    //to trigger a repaint
    startTimerHz(60);

}

ResponseCurveComponent::~ResponseCurveComponent(){
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->removeListener(this);
    }
}

//draws response curve component including response curve background plot
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    
    //draws static response curve background image
    g.drawImage(background, getLocalBounds().toFloat());

    //returns resized bounds for rendering response curve component within bounds of plot graph
    auto responseArea = getAnalysisArea();
    
    auto w = responseArea.getWidth();
    
    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> magnitudes;
    
    magnitudes.resize(w);
    
    for(int i = 0; i < w; ++i){
        //expressed in gain units which are multiplicative rather than additive
        double mag = 1.f;
        
        //mapping normalized magnitude to its frequency within human hearing range 20 Hz to 20000 Hz
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        //checking if a chain is bypassed and then multiplying the curr magnitude by the magnitude at the given frequency
        //in the given chain
        if(! monoChain.isBypassed<ChainPositions::Peak>()){
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if(! lowCut.isBypassed<0>()){
            mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! lowCut.isBypassed<1>()){
            mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! lowCut.isBypassed<2>()){
            mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! lowCut.isBypassed<3>()){
            mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        if(! highCut.isBypassed<0>()){
            mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! highCut.isBypassed<1>()){
            mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! highCut.isBypassed<2>()){
            mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if(! highCut.isBypassed<3>()){
            mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        //converting gain into decibels for mapping the response curve within a dB range
        magnitudes[i] = Decibels::gainToDecibels(mag);
    }
    
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    //maps each dB unit to within the range we specified
    auto map = [outputMin, outputMax](double input){
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));
    for(size_t i = 1; i < magnitudes.size(); ++i){
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }
    
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

//makes new background image based on width and height of component
//draws plot graph of the response curve component
void ResponseCurveComponent::resized(){
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    
    Graphics g(background);
    
    //array of frequencies for drawing plot lines in range 20 Hz - 20 kHz
    Array<float> freqs{
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    //stores x position for each frequency label within nornalized range to be within bounds of component
    Array<float> xPos;
    for(auto f : freqs){
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xPos.add(left + width * normX);
    }
    
    //plots the normalized values
    g.setColour(Colours::dimgrey);
    for(auto xs : xPos){
        //normalizing x value to allow each line to be drawn within the bounds of the response curve component
        
        g.drawVerticalLine(xs, top, bottom);
    }
    
    //array of gain units for drawing horizontal plot lines
    Array<float> gain{
        -24, -12, 0, 12, 24
    };
    
    //mapping each gain line from gain units to a normalized range between the top and bottom of the component
    for(auto gDb : gain){
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        
        g.drawHorizontalLine(y, left, right);
    }
    
//    g.drawRect(getAnalysisArea());
    
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    //drawing labels for each frequency value
    for(int i = 0; i < freqs.size(); ++i){
        auto f = freqs[i];
        auto xs = xPos[i];
        
        bool addK = false;
        String str;
        if(f > 999.f){
            addK = true;
            f /= 1000.f;
        }
        
        str << f;
        if(addK){
            str << "k";
        }
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(xs, 0);
        r.setY(1);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    //drawing labels for each gain value
    for(auto gDb : gain){
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if(gDb > 0){
            str << "+";
        }
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        //setting bounding box for each label
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
        //setting values for spectrum analyzer
        str.clear();
        str << (gDb - 24.f);
        
        //drawing label within fitted, positioned rectangle
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

//responsible for reszing the response curve component bounds to avoid graphical collisions
juce::Rectangle<int> ResponseCurveComponent::getRenderArea(){
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    
    return bounds;
}

//sets up bounds of response curve to fit within bounds of plot graph
juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea(){
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}


//sets the atomic flag to true which will cause the timerCallBack function to execute
void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue){
    parametersChanged.set(true);
}

//checks atomic flag, if true, sets to false and calls updateChain
//and then signals a repaint
void ResponseCurveComponent::timerCallback(){
    //storing data incoming channel fifos
    juce::AudioBuffer<float> tempIncomingBuffer;
    
    //while there are buffers to be pulled from left channel FIFO, can be converted into FFT data
    while(leftChanelFifo->getNumCompleteBuffersAvailable() > 0){
        if(leftChanelFifo->getAudioBuffer(tempIncomingBuffer)){
            
        }
    }
    
    if(parametersChanged.compareAndSetBool(false, true)){
        //update the monoChain
        updateChain();
        //signal a repaint
        repaint();
    }
}
//updates peak, LC, and HC filters in PluginEditor monoChain object
void ResponseCurveComponent::updateChain(){
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}



//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
//attaching sliders to the parameters they represent

peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
responseCurveComponent(audioProcessor),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    
    //initialize the start and end labels for each slider which will be drawn in in each sliders paint function
    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20kHz"});
    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});
    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10.0"});
    
    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20kHz"});
    lowCutSlopeSlider.labels.add({0.f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});
    
    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20kHz"});
    highCutSlopeSlider.labels.add({0.f, "12"});
    highCutSlopeSlider.labels.add({1.f, "20kHz"});
    
    
    //adds slider components and makes them visible
    for(auto* comp : getComps()){
        addAndMakeVisible(comp);
    }
    
    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

 
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    //region of window reserved for filter response curve
    float heightRatio = 25.f / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * heightRatio);
    
    responseCurveComponent.setBounds(responseArea);
    
    bounds.removeFromTop(5);
    
    //region reserved for LC and HC filters
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    //remaining third reserved for peak filter sliders
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    
    
}



std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps(){
    return{
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &highCutSlopeSlider,
        &lowCutSlopeSlider,
        &responseCurveComponent
    };
}
