/*
  ==============================================================================

    SpaceConfigComponent.h
    Created: 6 Mar 2020 3:28:28pm
    Author:  Felix Faire

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../UIElements/SpaceViewerComponent.h"
#include "../UIElements/ChannelInfoComponent.h"
#include "../UIElements/SpeakerInfoListComponent.h"
#include "../UIElements/ComponentContainer.h"



class SpaceConfigComponent : public Component,
                             public ChangeListener
{
public:
    SpaceConfigComponent(AppModel& model)
        : mModel(model)
    {
        mModel.mSpeakerPositionsChanges.addChangeListener(this);
        mModel.mAudioLevelChanges.addChangeListener(this);
        mModel.mDeviceManager.addChangeListener(this);


        if (!mSender.connect ("127.0.0.1", 9001))
            showConnectionErrorMessage ("Error: could not connect to UDP port 9001.");

        // Buttons
        mAddButton.reset(new TextButton("Add"));
        mRemoveButton.reset(new TextButton("Remove"));
        mListViewToggleButton.reset(new ToggleButton("Show List View"));

        addAndMakeVisible(*mAddButton);
        addAndMakeVisible(*mRemoveButton);
        addAndMakeVisible(*mListViewToggleButton);

        mAddButton->onClick = [this] () {
            auto& r = Random::getSystemRandom();
            mModel.addSpeaker(glm::vec3(r.nextFloat(), r.nextFloat(), r.nextFloat()) * 2.0f - 1.0f);
        };

        mRemoveButton->onClick = [this] () {
            mModel.removeSpeaker();
        };

        mListViewToggleButton->onClick = [this]() {

            if (mListViewToggleButton->getToggleState())
                mViewContainer->setContent(mSpeakerListView.get());
            else
                mViewContainer->setContent(mSpace.get());

            this->resized();
        };

        // Views
        mSpace.reset(new SpaceViewerComponent(mModel));
        mSpeakerListView.reset(new SpeakerInfoListComponent(mModel));
        mViewContainer.reset(new ComponentContainer());
        mViewContainer->setContent(mSpace.get());
        mChannelBar.reset(new ChannelInfoComponentBar());
        
        mSpace->onTrigger = [this](glm::vec3 p) {
            
            const int noteID = ++mModel.mCurrentNoteID;
            const int soundID = 0;
            mSender.send("/start", noteID,
                                   soundID,
                                   p.x, p.y, p.z);
        };
        
        mSpace->onUpdate = [this](glm::vec3 p) {
            mSender.send("/update", mModel.mCurrentNoteID,
                                    p.x, p.y, p.z);
        };
        
        addAndMakeVisible(*mViewContainer);
        addAndMakeVisible(*mChannelBar);
        
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        
        auto topBar = b.removeFromTop(40);

        mAddButton->setBounds(topBar.removeFromLeft(mAddButton->getBestWidthForHeight(topBar.getHeight())).reduced(0, 5));
        mRemoveButton->setBounds(topBar.removeFromLeft(mRemoveButton->getBestWidthForHeight(topBar.getHeight())).reduced(10, 5));
        mListViewToggleButton->setBounds(topBar);

        b.removeFromTop(5);
        auto cb = b.removeFromBottom(20);
        b.removeFromBottom(5);
        
        mViewContainer->setBounds(b);
        mChannelBar->setBounds(cb);
    }
    

private:

    //==============================================================================
    void showConnectionErrorMessage (const String& messageText)
    {
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
            "Connection error",
            messageText,
            "OK");
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &mModel.mSpeakerPositionsChanges)
        {
            mSpace->updateComponentPositions();
            mChannelBar->updateNumOutputChannels(mModel);
        }
        else if (source == &mModel.mAudioLevelChanges)
        {
            mChannelBar->updateAudioLevels(mModel);
        }
        else if (source == &mModel.mDeviceManager)
        {
            const auto& setup = mModel.mDeviceManager.getAudioDeviceSetup();
            mChannelBar->updateActivatedChannels(setup);
        }
    }
    
    

    AppModel& mModel;
    OSCSender mSender;

    // Buttons
    std::unique_ptr<ToggleButton>   mListViewToggleButton;
    std::unique_ptr<TextButton>     mAddButton;
    std::unique_ptr<TextButton>     mRemoveButton;
    
    // View
    std::unique_ptr<ComponentContainer>        mViewContainer;
    std::unique_ptr<SpaceViewerComponent>      mSpace;
    std::unique_ptr<SpeakerInfoListComponent>  mSpeakerListView;
    
    // Bottom Bar
    std::unique_ptr<ChannelInfoComponentBar>   mChannelBar;
    
};
