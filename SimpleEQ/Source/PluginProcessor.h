/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
//FIFO for GUI thread to retrieve blocks produced by single channel sample FIFO
template<typename T>
struct Fifo{
    void prepared(int numChannels, int numSamples){
        for(auto& buffer : buffers){
            buffer.setSize(numChannels,
                           numSamples,
                           false, //clear everything?
                           true, //including the extra space?
                           true); //avoid reallocating if possible
            buffer.clear();
        }
    }
    bool push(const T& t){
        auto write = fifo.write(1);
        if(write.blockSize1 > 0){
            buffers[write.startIndex1] = t;
            return true;
        }
        return false;
    }
    bool pull(const T& t){
        auto read = fifo.read(1);
        if(read.blockSize1 > 0){
            t = buffers[read.startIndex1];
            return true;
        }
        return false;
    }
    int getNumAvailableForReading() const{
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};

enum Channel{
    Right, //effectively 0
    Left //effectively 1
};

//FFT algorithm which will allow for generation of spectral analyzer requires a fix number of samples passed into it
//at any time. Because the audioBuffer can hold any number of samples at a given time, we must extract audio samples from
//each indivudal channel at a fixed rate to pass them to the FFT algorithm for processing
template<typename BlockType>
struct SingleChannelSampleFifo{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch){
        prepared.set(false);
    }
    
    void update(const BlockType& buffer){
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        for (int i = 0; i < buffer.getNumSamples(); ++i){
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }
    
    void prepare(int bufferSize){
        prepared.set(false);
        size.set(bufferSize);
        
        bufferToFill.setSize(1, //channel
                             bufferSize, //num samples
                             false, //keepExistingContent
                             true, //clear extra space
                             true); //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    
    int getNumCompleteBuffersAvailable() const {return audioBufferFifo.getNumAvailableForReading();}
    bool isPrepared() const {return prepared.get();}
    int getSize() const {return size.get();}
    bool getAudioBuffer(BlockType& buf){return audioBufferFifo.pull(buf);}
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;
    
    void pushNextSampleIntoFifo(float sample){
        if (fifoIndex == bufferToFill.getNumSamples()){
            auto ok = audioBufferFifo.push(bufferToFill);
            
            juce::ignoreUnused(ok);
            fifoIndex = 0;
        }
        
        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

enum Slope{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//struct for storing all paramters values from apvts
struct ChainSettings
{
    float peakFreq { 0 }, peakGainInDecibels { 0 }, peakQuality { 1.f };
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope { Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};

using Filter = juce::dsp::IIR::Filter<float>;

//each filter type in IIR filter class has response 12 dB/Oct when configured as Low Pass or High Pass
//for response of 48 dB/Oct need 4 filters chained together with ProcessorChain and then pass a ProcessingContext
//which will run through each Filter automatically
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

//defining a chain for mono signal path Low Cut -> Parametric -> High Cut
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
//need 2 monochains in order to do stereo processing

enum ChainPositions{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(const Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate);

template<int Index, typename ChainType, typename CoefficientsType>
void update(ChainType& chain, const CoefficientsType& coefficients){
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

//each time HC and LC mono chains need to be updateds this will be called
//this method taks a reference to the given mono chain, the new coefficients, and the new slope
//each link of a given chain is initially bypassed
//but is reactivated based on the filter slope and given its new coefficients
template<typename ChainType, typename CoefficientsType>
void updateCutFilter(ChainType& chain,
                     const CoefficientsType& coefficients,
                     const Slope slope){
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);
    
    //after getting array of coefficient filters
    //filters in chain can be set based on what slope value and cut coefficient values are selected and filters can be de-bypassed
    //can leverage case passthrough and a templated helper to eliminate code duplication
    switch(slope){
        case Slope_48:
        {
            update<3>(chain, coefficients);
        }
        case Slope_36:
        {
            update<2>(chain, coefficients);
        }
        case Slope_24:
        {
            update<1>(chain, coefficients);
        }
        case Slope_12:
        {
            update<0>(chain, coefficients);
        }
        
    }
}

inline auto makeLowCutFilter(const ChainSettings &chainSettings, double sampleRate){
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                       sampleRate,
                                                                                       (chainSettings.lowCutSlope + 1) * 2);
}

inline auto makeHighCutFilter(const ChainSettings &chainSettings, double sampleRate){
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                      sampleRate,
                                                                                      (chainSettings.highCutSlope + 1) * 2);
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    //every audio processor requires an apvts to connect to audio state values to our GUI knobs and sliders that will adjust these values
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo {Channel::Left};
    SingleChannelSampleFifo<BlockType> rightChannelFifo {Channel::Right};
private:
    MonoChain leftChain, rightChain;
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    
    
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    
    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
