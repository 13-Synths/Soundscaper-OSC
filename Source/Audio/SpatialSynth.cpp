
#include "SpatialSynth.h"


//==============================================================================
SpatialSynth::SpatialSynth()
{
    
}

SpatialSynth::~SpatialSynth()
{
}

//==============================================================================
SpatialSynthVoice* SpatialSynth::getVoice (const int index) const
{
    const ScopedLock sl (lock);
    return voices [index];
}

void SpatialSynth::clearVoices()
{
    const ScopedLock sl (lock);
    voices.clear();
}

SpatialSynthVoice* SpatialSynth::addVoice (SpatialSynthVoice* const newVoice)
{
    const ScopedLock sl (lock);
    newVoice->setCurrentPlaybackSampleRate (sampleRate);
    newVoice->setNumSpeakerOutputs((int)speakerPositions.size());
    return voices.add (newVoice);
}

void SpatialSynth::removeVoice (const int index)
{
    const ScopedLock sl (lock);
    voices.remove (index);
}

void SpatialSynth::clearSounds()
{
    const ScopedLock sl (lock);
    sounds.clear();
}

SpatialSynthSound* SpatialSynth::addSound (const SpatialSynthSound::Ptr& newSound)
{
    const ScopedLock sl (lock);
    return sounds.add (newSound);
}

void SpatialSynth::removeSound (const int index)
{
    const ScopedLock sl (lock);
    sounds.remove (index);
}

void SpatialSynth::setNoteStealingEnabled (const bool shouldSteal)
{
    shouldStealNotes = shouldSteal;
}

void SpatialSynth::setMinimumRenderingSubdivisionSize (int numSamples, bool shouldBeStrict) noexcept
{
    jassert (numSamples > 0); // it wouldn't make much sense for this to be less than 1
    minimumSubBlockSize = numSamples;
    subBlockSubdivisionIsStrict = shouldBeStrict;
}

//==============================================================================
void SpatialSynth::setSampleRate(const double newRate)
{
    if (sampleRate != newRate)
    {
        const ScopedLock sl (lock);
        allNotesOff (false);
        sampleRate = newRate;

        for (auto* voice : voices)
            voice->setCurrentPlaybackSampleRate(newRate);
    }
}

void SpatialSynth::updateSpeakerPositions(std::vector<glm::vec3> &positions)
{
    const ScopedLock sl (lock);
    
    speakerPositions = positions;
    
    for (auto* voice : voices)
        voice->setNumSpeakerOutputs((int)positions.size());
}

template <typename floatType>
void SpatialSynth::processNextBlock (AudioBuffer<floatType>& outputAudio,
                                    int startSample,
                                    int numSamples)
{
    // must set the sample rate before using this!
    jassert (sampleRate != 0);
    const int targetChannels = outputAudio.getNumChannels();
    
    const ScopedLock sl (lock);
    
    for (auto* voice : voices)
        if (voice->getNeedsDBAPUpdate())
            voice->updateDBAPAmplitudes(speakerPositions);
    
    if (targetChannels > 0)
        renderVoices (outputAudio, startSample, numSamples);
}

// explicit template instantiation
template void SpatialSynth::processNextBlock<float>  (AudioBuffer<float>&,  int, int);
template void SpatialSynth::processNextBlock<double> (AudioBuffer<double>&, int, int);

void SpatialSynth::renderNextBlock (AudioBuffer<float>& outputAudio,
                                   int startSample, int numSamples)
{
    processNextBlock (outputAudio, startSample, numSamples);
}

void SpatialSynth::renderNextBlock (AudioBuffer<double>& outputAudio,
                                   int startSample, int numSamples)
{
    processNextBlock (outputAudio, startSample, numSamples);
}

void SpatialSynth::renderVoices (AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (buffer, startSample, numSamples);
}

void SpatialSynth::renderVoices (AudioBuffer<double>& buffer, int startSample, int numSamples)
{
    for (auto* voice : voices)
        voice->renderNextBlock (buffer, startSample, numSamples);
}

//==============================================================================
void SpatialSynth::noteOn (const int noteID,
                           const int soundID,
                           const float velocity,
                           const glm::vec3& pos)
{
    const ScopedLock sl (lock);

    auto* sound = sounds[soundID].get();
    
    // If hitting a note that's still ringing, stop it first (it could be
    // still playing because of the sustain or sostenuto pedal).
    for (auto* voice : voices)
        if (voice->getCurrentNoteID() == noteID)
            stopVoice (voice, 1.0f, true);

    // TODO: remove midi references from these stealing functions
    startVoice (findFreeVoice (sound, soundID, shouldStealNotes),
                sound, noteID, velocity, pos);

}

void SpatialSynth::startVoice (SpatialSynthVoice* const voice,
                              SpatialSynthSound* const sound,
                              const int noteID,
                              const float velocity,
                              const glm::vec3& pos)
{
    if (voice != nullptr && sound != nullptr)
    {
        if (voice->currentlyPlayingSound != nullptr)
            voice->stopNote (0.0f, false);

        voice->noteOnTime = ++lastNoteOnCounter;
        voice->currentlyPlayingSound = sound;

        voice->startNote (noteID, velocity, pos, sound);
    }
}

void SpatialSynth::stopVoice (SpatialSynthVoice* voice, float velocity, const bool allowTailOff)
{
    jassert (voice != nullptr);

    voice->stopNote (velocity, allowTailOff);

    // the subclass MUST call clearCurrentNote() if it's not tailing off! RTFM for stopNote()!
    jassert (allowTailOff || (voice->getCurrentNoteID() < 0 && voice->getCurrentlyPlayingSound() == nullptr));
}

void SpatialSynth::noteOff (const int noteID,
                            const float velocity,
                            const bool allowTailOff)
{
    const ScopedLock sl (lock);

    for (auto* voice : voices)
    {
        if (voice->getCurrentNoteID() == noteID)
        {
            if (auto sound = voice->getCurrentlyPlayingSound())
            {
                stopVoice (voice, velocity, allowTailOff);    
            }
        }
    }
}

void SpatialSynth::allNotesOff (const bool allowTailOff)
{
    const ScopedLock sl (lock);

    for (auto* voice : voices)
        voice->stopNote (1.0f, allowTailOff);
}

void SpatialSynth::handlePositionChange (int noteID, glm::vec3 newPosition)
{
    const ScopedLock sl (lock);

    for (auto* voice : voices)
        if (voice->getCurrentNoteID() == noteID)
            voice->positionChanged (newPosition);
}


//==============================================================================
SpatialSynthVoice* SpatialSynth::findFreeVoice (SpatialSynthSound* soundToPlay,
                                                int midiNoteNumber,
                                                const bool stealIfNoneAvailable) const
{
    const ScopedLock sl (lock);

    for (auto* voice : voices)
        if ((! voice->isVoiceActive()) && voice->canPlaySound (soundToPlay))
            return voice;

    if (stealIfNoneAvailable)
        return findVoiceToSteal (soundToPlay, midiNoteNumber);

    return nullptr;
}

SpatialSynthVoice* SpatialSynth::findVoiceToSteal (SpatialSynthSound* soundToPlay,
                                                   int midiNoteNumber) const
{
    // This voice-stealing algorithm applies the following heuristics:
    // - Re-use the oldest notes first
    // - Protect the lowest & topmost notes, even if sustained, but not if they've been released.

    // apparently you are trying to render audio without having any voices...
    jassert (! voices.isEmpty());

    // this is a list of voices we can steal, sorted by how long they've been running
    Array<SpatialSynthVoice*> usableVoices;
    usableVoices.ensureStorageAllocated (voices.size());

    for (auto* voice : voices)
    {
        if (voice->canPlaySound (soundToPlay))
        {
            jassert (voice->isVoiceActive()); // We wouldn't be here otherwise

            usableVoices.add (voice);

            // NB: Using a functor rather than a lambda here due to scare-stories about
            // compilers generating code containing heap allocations..
            struct Sorter
            {
                bool operator() (const SpatialSynthVoice* a, const SpatialSynthVoice* b) const noexcept { return a->wasStartedBefore (*b); }
            };

            std::sort (usableVoices.begin(), usableVoices.end(), Sorter());
        }
    }

    // The oldest note that's playing with the target sound is ideal..
    for (auto* voice : usableVoices)
        if (voice->getCurrentlyPlayingSound() == soundToPlay)
            return voice;

    // TODO: add heuristic to preserve sounds with most playtime left?..

    jassert(usableVoices.size() >= 0);

    return usableVoices[0];
}
