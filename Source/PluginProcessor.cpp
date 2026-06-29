#include "PluginProcessor.h"
#include "PluginEditor.h"

CutOffAudioProcessor::CutOffAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "Parameters", createParameterLayout()),
       spectralAnalyser(*this)
{}

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

    // Initialize spectrum fifo
    const auto fifoSize = std::max(samplesPerBlock * 2, juce::roundToInt(sampleRate / 30.0));
    fifo.setTotalSize(fifoSize);
    fifoData.resize(fifoSize, 0.0f);
    fifo.reset();
    spectralAnalyser.prepare(sampleRate, fifoSize);
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

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto hpfFreq = apvts.getRawParameterValue("HPF")->load();
    auto lpfFreq = apvts.getRawParameterValue("LPF")->load();
    auto atk = apvts.getRawParameterValue("ATTACK")->load();
    auto dec = apvts.getRawParameterValue("DECAY")->load();
    auto thresh = apvts.getRawParameterValue("THRESHOLD")->load();
    auto gain = apvts.getRawParameterValue("GAIN")->load();

    hpf.setCutoffFrequency(hpfFreq);
    lpf.setCutoffFrequency(lpfFreq);
    
    compressor.setAttack(atk);
    compressor.setRelease(dec);
    compressor.setThreshold(thresh);
    
    outputGain.setGainDecibels(gain);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // ToDo: smoothing of cutoff values
    for (int ch = 0; ch < block.getNumChannels(); ++ch)
    {
        auto* data = block.getChannelPointer(ch);
        for (int i = 0; i < block.getNumSamples(); ++i)
        {
            data[i] = hpf.processSample(ch, data[i]);
            data[i] = lpf.processSample(ch, data[i]);
        }
    }

    compressor.process(context);
    outputGain.process(context);

    pushBufferToFifo(buffer);
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

    const auto hzAttributes = juce::AudioParameterFloatAttributes().withLabel("Hz");
    const auto dbAttributes = juce::AudioParameterFloatAttributes().withLabel("dB");
    const auto msAttributes = juce::AudioParameterFloatAttributes().withLabel("ms");

    params.push_back(std::make_unique<juce::AudioParameterFloat>("HPF", "HPF", juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f, hzAttributes));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LPF", "LPF", juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f, hzAttributes));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ATTACK", "Attack", juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 10.0f, msAttributes));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DECAY", "Decay", juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f, 0.5f), 100.0f, msAttributes));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("THRESHOLD", "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f, dbAttributes));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("GAIN", "Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f, dbAttributes));
    return { params.begin(), params.end() };
}

void CutOffAudioProcessor::pushBufferToFifo (const juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto sumFactor = 1.0f / float(numChannels);

    auto fifoWriter = fifo.write(numSamples);
    auto readIndex = 0;

    if(fifoWriter.blockSize1 > 0)
    {
        for(auto i = 0; i < fifoWriter.blockSize1; ++i)
        {
            const auto writeIdx = fifoWriter.startIndex1 + i;
            auto sample = 0.0f;
            for(auto c = 0; c < numChannels; ++c)
            {
                sample += buffer.getSample(c, readIndex) * sumFactor;
            }
            fifoData[writeIdx] = sample;
            readIndex++;
        }
    }

    if(fifoWriter.blockSize2 > 0)
    {
        for(auto i = 0; i < fifoWriter.blockSize2; ++i)
        {
            const auto writeIdx = fifoWriter.startIndex2 + i;
            auto sample = 0.0f;
            for(auto c = 0; c < numChannels; ++c)
            {
                sample += buffer.getSample(c, readIndex) * sumFactor;
            }
            fifoData[writeIdx] = sample;
            readIndex++;
        }
    }
}

int CutOffAudioProcessor::getDataFromFifo(std::vector<float>& output, const int indexOffset)
{
    auto fifoReader = fifo.read(output.size() - indexOffset);
    auto writeIndex = indexOffset;

    if(fifoReader.blockSize1 > 0)
    {
        for(auto i = 0; i < fifoReader.blockSize1; ++i)
        {
            const auto readIdx = fifoReader.startIndex1 + i;
            output[writeIndex] = fifoData[readIdx];
            writeIndex++;
        }
    }

    if(fifoReader.blockSize2 > 0)
    {
        for(auto i = 0; i < fifoReader.blockSize2; ++i)
        {
            const auto readIdx = fifoReader.startIndex2 + i;
            output[writeIndex] = fifoData[readIdx];
            writeIndex++;
        }
    }

    return fifoReader.blockSize1 + fifoReader.blockSize2;
}

bool CutOffAudioProcessor::getScopeData(std::vector<float>& output)
{
    auto newFrame = spectralAnalyser.isNewFrameAvailable();
    if(newFrame)
    {
        spectralAnalyser.getCurrentFrame(output);
    }
    return newFrame;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CutOffAudioProcessor(); }