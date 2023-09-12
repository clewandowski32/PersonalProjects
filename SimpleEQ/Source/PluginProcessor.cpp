/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    //pass a process spec object to each Chain which will then be passed to each link in the chain
    juce::dsp::ProcessSpec spec;
    //max number of samples processed at 1 time
    spec.maximumBlockSize = samplesPerBlock;
    //number of channels: 1 because mono can only handle 1 channel of audio
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();
    
    //preparing FIFOs data structures for processing by FFT algorithm
    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);
    
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    
    //always start with updating chain settings before we allow the left and right chain to actually process the audio in the buffer
    
    updateFilters();

    
    
    
    //need to extract L and R channel from the buffer to the processing context
    //which will then be passed to the processor chain and ran through each link
    
    juce::dsp::AudioBlock<float> block(buffer);
    
    //L and R channel live at 0 and 1 index in the audio block
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    //process contexts to wrap the L and R channels
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    //pass contexts to L and R mono chains to process audio
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    //update right and left channel FIFOs with buffer
    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);    
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
    return new SimpleEQAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if(tree.isValid()){
        apvts.replaceState(tree);
        updateFilters();
    }
}

//gets current paramter values from the apvts which stores them
//must use getRawParamterValue() method to return a non-normalized value for each
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts){
    ChainSettings settings;
    
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    
    return settings;
}

Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate){
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                chainSettings.peakFreq,
                                                                chainSettings.peakQuality,
                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

//uses makePeakFilter method to get new peak coefficients
//updatees peak coefficient values of left and right mono chains
void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings){
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());
    
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

//helper method to be used for updating coefficient values on initialization and changes to filter parameters
//this function needs to be free as it will be called in pluginProcessorEditor to allow for repaints
void /*SimpleEQAudioProcessor::*/updateCoefficients(const Coefficients &old, const Coefficients &replacements){
    *old = *replacements;
}

//uses HPHO Butterworth method to get new coefficients based on current LC freq and slope
//individual links of each mono chain are then updated based on new coefficients
void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings){
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

//uses LPHO Butterworth method to get new coefficients based on current HC freq and slope
//individual links of each mono chain are then updated based on new coefficients
void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings){
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

//initializes a chainSettings object which holds all current parameters values
//calls functions to update LC, HC, and Peak filters with chainSetting object
//this method will be called by prepareToPlay and by processBlock each time an audio buffer needs to be processed
//setStateInformation also calls this method to restore apvts settings from data stored in memory between loads of the plugin
void SimpleEQAudioProcessor::updateFilters(){
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

//method to create parameter layout variable to be passed to apvts
//paramters will be HC, LC and Parametric Band
//paramteric band will allow to control freq, gain, and quality
//HC and LC bands will allow to control freq and band slope
juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    //paramID: LowCut Freq, paramName: LowCut Freq
    //normalisable range: 20 Hz to 20000 Hz, step size: 1 (deals with how much each turn of the knob changes the value),
    //skew factor: 1 (percentage of knob reserved for a given range), default value: 20 Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("LowCut Freq", 1),
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("HighCut Freq", 1),
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Peak Freq", 1),
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           750.f));
    //gain measured in dB from -24 to 24 dB
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Peak Gain", 1),
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Peak Quality", 1),
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));
    //setting up string array for adjusting slope on LC, HC band
    juce::StringArray stringArray;
    for(int i = 0; i < 4; i++){
        juce::String str;
        str << (12 + i*12);
        str << " db/Oct";
        stringArray.add(str);
    }
    //LC and HC slope choice paramters
    //param id: LowCut Slope, param name: LowCut Slope, choice stringArray: stringArray, default index: 0
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("LowCut Slope", 1), "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID("HighCut Slope", 1), "HighCut Slope", stringArray, 0));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
