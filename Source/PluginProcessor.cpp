#include "PluginProcessor.h"
#include "PluginEditor.h"

CutOffAudioProcessor::CutOffAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "Parameters", createParameterLayout()) {}

CutOffAudioProcessor::~CutOffAudioProcessor() {}

const juce::String CutOffAudioProcessor::getName() const { return "Cut-Off"; }
bool CutOffAudioProcessor::acceptsMidi() const { return false; }
bool CutOffAudioProcessor::producesMidi() const { return false; }
bool CutOffAudioProcessor::isMidiEffect() const { return false; }
double CutOffAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int CutOffAudioProcessor::getNumPrograms() { return 1; }
int CutOffAudioProcessor::getCurrentProgram() { return 0; }
void CutOffAudioProcessor::setCurrentProgram (int index) {}
const juce::String CutOffAudioProcessor::getProgramName (int index) { return {}; }
void CutOffAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void CutOffAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) 
{
    // Tell the DSP modules about the host's sample rate and buffer size
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    // Initialize High-Pass Filter
    hpf.prepare(spec);
    hpf.setType(juce::dsp::StateVariableTPTFilterType::highpass);

    // Initialize Low-Pass Filter
    lpf.prepare(spec);
    lpf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    // Initialize Compressor
    compressor.prepare(spec);
    compressor.setRatio(4.0f); // Setting a standard 4:1 compression ratio

    // Initialize Gain
    outputGain.prepare(spec);
}
void CutOffAudioProcessor::releaseResources() {}

bool CutOffAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void CutOffAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) 
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any unused channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // 1. Grab the live numbers from your UI knobs
    auto hpfFreq = apvts.getRawParameterValue("HPF")->load();
    auto lpfFreq = apvts.getRawParameterValue("LPF")->load();
    auto atk = apvts.getRawParameterValue("ATTACK")->load();
    auto dec = apvts.getRawParameterValue("DECAY")->load();
    auto thresh = apvts.getRawParameterValue("THRESHOLD")->load();
    auto gain = apvts.getRawParameterValue("GAIN")->load();

    // 2. Feed those numbers into the DSP engines
    hpf.setCutoffFrequency(hpfFreq);
    lpf.setCutoffFrequency(lpfFreq);
    
    compressor.setAttack(atk);
    compressor.setRelease(dec); // Using Decay knob to control compressor release
    compressor.setThreshold(thresh);
    
    outputGain.setGainDecibels(gain);

    // 3. Process the Filters (Sample by Sample)
    juce::dsp::AudioBlock<float> block(buffer);
    for (int ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        for (int i = 0; i < block.getNumSamples(); ++i)
        {
            // Run the audio through the HPF, then the LPF
            data[i] = hpf.processSample(ch, data[i]);
            data[i] = lpf.processSample(ch, data[i]);
        }
    }

    // 4. Process the Dynamics (Compressor & Gain)
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);
    outputGain.process(context);

    // --- Send output to the Spectrum Visualizer ---
    if (block.getNumChannels() > 0)
    {
        auto* channelData = block.getChannelPointer(0); // Use Left channel
        for (int i = 0; i < block.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelData[i]);
        }
    }
}


bool CutOffAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CutOffAudioProcessor::createEditor() { return new CutOffAudioProcessorEditor (*this); }

void CutOffAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CutOffAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout CutOffAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("HPF", "HPF", juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LPF", "LPF", juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f, 0.5f), 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESHOLD", "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    return { params.begin(), params.end() };
}

void CutOffAudioProcessor::pushNextSampleIntoFifo (float sample) noexcept
{
    // When the fifo contains enough data, copy it to the fftData array to be analyzed
    if (fifoIndex == fftSize)
    {
        if (! nextFFTBlockReady)
        {
            juce::zeromem (fftData, sizeof (fftData));
            memcpy (fftData, fifo, sizeof (fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CutOffAudioProcessor(); }