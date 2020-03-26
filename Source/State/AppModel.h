/*
  ==============================================================================

    AppModel.h
    Created: 12 Feb 2020 10:57:57pm
    Author:  Felix Faire

  ==============================================================================
*/

#pragma once
#include "vec3.hpp"
#include "../Audio/SpatialSampler.h"

struct SoundFileData
{
    SoundFileData(const String& name, const AudioBuffer<float>* data)
        : mName(name),
          mAudioData(data)
    {
        // TODO: make a proper address
        mOSCAddress = mName.replace(" ", "_") + "/";
    }
    
    String                              mName;
    String                              mOSCAddress;
    const AudioBuffer<float>*           mAudioData;
};

struct VoiceData
{
    double mStartTime;
    String mFileName;
    int    mID;
};

struct AppModel
{
public:
    AppModel()
    {
    }

    ~AppModel()
    {
    }
    
    void addSpeaker(const glm::vec3& pos)
    {
        mSpeakerPositions.push_back(pos);
        mSpeakerPositionsChanges.sendChangeMessage();
    }
    
    void removeSpeaker()
    {
        mSpeakerPositions.pop_back();
        mSpeakerPositionsChanges.sendChangeMessage();
    }
    
    void setSpeakerPosition(int index, const glm::vec3& pos)
    {
        jassert(index < mSpeakerPositions.size());
        mSpeakerPositions[index] = pos;
        mSpeakerPositionsChanges.sendChangeMessage();
    }
    
    const std::vector<glm::vec3>& getSpeakerPositions() const { return mSpeakerPositions; }
    const glm::vec3& getSpeakerPosition(int i) const          { return mSpeakerPositions[i]; }
    
    void setSoundbedAmplitude(int index, float newAmplitude)
    {
        jassert(mSoundAtmosphereAmplitudes.size() == mSoundAtmosphereData.size());
        jassert(index < mSoundAtmosphereAmplitudes.size());
        
        if (index >= mSoundAtmosphereAmplitudes.size())
            return;
        
        mSoundAtmosphereAmplitudes[index] = newAmplitude;
        mSoundBedAmplitudesChanges.sendChangeMessage();
    }
    
    const std::vector<float>&   getSoundBedAmpitudes() const { return mSoundAtmosphereAmplitudes; }
    float                       getSoundBedAmpitude(int i) const                 { return mSoundAtmosphereAmplitudes[i]; }
    
    void addSoundBedData(const String& name, const AudioBuffer<float>* data)
    {
        mSoundAtmosphereData.emplace_back(name, data);
        mSoundAtmosphereAmplitudes.push_back(1.0f);
    }
    
    void addSoundClipData(const String& name, const AudioBuffer<float>* data)
    {
        mSoundClipData.emplace_back(name, data);
    }
    
    void setAudioLevels(const std::vector<float>& newLevels)
    {
        if (newLevels.size() != mAudioLevels.size())
            mAudioLevels.resize(newLevels.size());
            
        for (int i = 0; i < newLevels.size(); ++i)
        {
            mAudioLevels[i] *= 0.9f;
            
            if (newLevels[i] > mAudioLevels[i])
                mAudioLevels[i] = newLevels[i];
        }
        
        mAudioLevelChanges.sendChangeMessage();
    }
    
    
    std::unique_ptr<PropertiesFile>     mSettingsFile;
        
        
    ChangeBroadcaster                   mSoundBedAmplitudesChanges;
    ChangeBroadcaster                   mSpeakerPositionsChanges;
    ChangeBroadcaster                   mAudioLevelChanges;

    
    File                                mCurrentSoundBedFolder;
    std::vector<SoundFileData>          mSoundAtmosphereData;
    
    File                                mCurrentSoundClipFolder;
    std::vector<SoundFileData>          mSoundClipData;
    
    std::vector<VoiceData>              mPlayingVoiceData;
    
    int                                 mCurrentNoteID = 0;
    
    std::vector<float>                  mAudioLevels;
    
    // IO Devices
    OSCReceiver                         mOSCReciever;
    AudioDeviceManager                  mDeviceManager;

private:

    // These must be changed via get / set to ensure all change messages are propogated correctly
    std::vector<float>                  mSoundAtmosphereAmplitudes;
    std::vector<glm::vec3>              mSpeakerPositions;
    
    friend class AppModelLoader;
    
};