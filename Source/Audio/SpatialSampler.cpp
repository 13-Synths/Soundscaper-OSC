#include "SpatialSampler.h"


SpatialSamplerSound::SpatialSamplerSound (const String& soundName,
                            AudioFormatReader& source,
                            int note,
                            double attackTimeSecs,
                            double releaseTimeSecs,
                            double maxSampleLengthSeconds)
    : name (soundName),
      sourceSampleRate (source.sampleRate),
      noteID (note)
{
    if (sourceSampleRate > 0 && source.lengthInSamples > 0)
    {
        length = jmin ((int) source.lengthInSamples,
                       (int) (maxSampleLengthSeconds * sourceSampleRate));

        data.reset (new AudioBuffer<float> (jmin (2, (int) source.numChannels), length + 4));

        source.read (data.get(), 0, length + 4, 0, true, true);

        adsrParams.attack  = static_cast<float> (attackTimeSecs);
        adsrParams.release = static_cast<float> (releaseTimeSecs);
    }
}

SpatialSamplerSound::~SpatialSamplerSound()
{
}


//==============================================================================
SpatialSamplerVoice::SpatialSamplerVoice() {}
SpatialSamplerVoice::~SpatialSamplerVoice() {}

bool SpatialSamplerVoice::canPlaySound (SpatialSynthSound* sound)
{
    return dynamic_cast<const SpatialSamplerSound*> (sound) != nullptr;
}

void SpatialSamplerVoice::startNote (int noteID, float velocity, const glm::vec3& pos, SpatialSynthSound* s)
{
    if (auto* sound = dynamic_cast<const SpatialSamplerSound*> (s))
    {
        /*pitchRatio = std::pow (2.0, (midiNoteNumber - sound->midiRootNote) / 12.0)
                        * sound->sourceSampleRate / getSampleRate();*/
        pitchRatio = 1.0;
        mPosition = pos;
        sourceSamplePosition = 0.0;
        mNeedsDBAPUpdate = true;
        mCurrentNoteID = noteID;

        adsr.setSampleRate (sound->sourceSampleRate);
        adsr.setParameters (sound->adsrParams);

        adsr.noteOn();
    }
    else
    {
        jassertfalse; // this object can only play SpatialSamplerSounds!
    }
}

void SpatialSamplerVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        clearCurrentNote();
        adsr.reset();
    }
}


//==============================================================================
void SpatialSamplerVoice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (auto* playingSound = static_cast<SpatialSamplerSound*> (getCurrentlyPlayingSound().get()))
    {
        // Get sample data
        auto& data = *playingSound->data;
        const float* const inL = data.getReadPointer (0);
        const float* const inR = data.getNumChannels() > 1 ? data.getReadPointer (1) : nullptr;
            
        const int numChannels = outputBuffer.getNumChannels();
        const float invSamples = 1.0f / (float)numSamples;
        
        // Calculate increments to smoothly interpolate channel amplitudes
        for (int i = 0; i < mChannelAmplitudes.size(); ++i)
            mChannelAmplitudeIncrements[i] = (mChannelAmplitudeTargets[i] - mChannelAmplitudes[i]) * invSamples;
        
        int i = 0;
        
        while (--numSamples >= 0)
        {
            auto pos = (int)sourceSamplePosition;
            auto alpha = (float)(sourceSamplePosition - pos);
            auto invAlpha = 1.0f - alpha;
            
            for (int i = 0; i < mChannelAmplitudes.size(); ++i)
                mChannelAmplitudes[i] += mChannelAmplitudeIncrements[i];

            // just using a very simple linear interpolation here..
            float l = (inL[pos] * invAlpha + inL[pos + 1] * alpha);
            float r = (inR != nullptr) ? (inR[pos] * invAlpha + inR[pos + 1] * alpha) : l;

            auto envelopeValue = adsr.getNextSample();
            
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* out = outputBuffer.getWritePointer (ch, startSample);
            
                out[i] += (l + r) * 0.5f * envelopeValue * mChannelAmplitudes[ch];
            }
        
            sourceSamplePosition += pitchRatio;
            i++;
            
            if (sourceSamplePosition > playingSound->length)
            {
                stopNote (0.0f, false);
                break;
            }
        }
    }
}