/*
  ==============================================================================

    SpaceViewerComponent.h
    Created: 13 Feb 2020 8:19:31pm
    Author:  Felix Faire

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "vec2.hpp"
#include "gtx/matrix_transform_2d.hpp"


class SpeakerComponent : public Component
{
public:
    SpeakerComponent(int id)
        : mID(id)
    {
        setRepaintsOnMouseActivity(true);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(2.0f).toFloat();
        b += Point<float>(mSubPixelDiff.x, mSubPixelDiff.y);

        if (isMouseOverOrDragging())
        {
            g.setColour(Colours::white.withAlpha(0.2f));
            g.fillEllipse(b);
        }

        g.setColour(Colours::white.withAlpha(0.8f));
        g.drawEllipse(b, 2.0f);
    }
    
    void setPosition(float x, float y)
    {
        glm::vec2 p(x, y);
        glm::vec2 pr(roundToInt(x), roundToInt(y));
        setCentrePosition((int)pr.x, (int)pr.y);
        mSubPixelDiff = p - pr;
    }

    
private:
    
    int mID = 0;
    glm::vec2 mPosition;
    glm::vec2 mSubPixelDiff;
};

//==============================================================================
/*
*/
class SpaceViewerComponent    : public Component
{
public:
    SpaceViewerComponent(AppModel& model)
        : mModel(model)
    {
        addAndMakeVisible(mDemoFileBox);

        for (int i = 0; i < mModel.mSpeakerPositions.size(); ++i)
        {
            SpeakerComponent* c = new SpeakerComponent(i);
            c->setSize(30, 30);
            addAndMakeVisible(*c);
            mSpeakers.add(c);
        }
        
        updateSpeakerBounds();
    }

    ~SpaceViewerComponent()
    {
    }

    void paint (Graphics& g) override
    {
        auto b = getViewerRect();
        
        g.setColour(Colour::greyLevel(0.1f));
        g.fillRoundedRectangle(b, 4.0f);
        
        const float minSize = jmin(b.getWidth(), b.getHeight());
        const float meter = minSize / mWindowDiameter;
        const int middleX = (int)b.getCentreX();
        const int middleY = (int)b.getCentreY();
        
        // Draw Grid
        g.setColour(Colour::greyLevel(0.2f));
        g.drawVerticalLine(middleX,   b.getY(), b.getBottom());
        g.drawHorizontalLine(middleY, b.getX(), b.getRight());
        
        for (auto x = meter; x < b.getWidth() * 0.5f; x += meter)
        {
            g.drawVerticalLine(middleX + x, b.getY(), b.getBottom());
            g.drawVerticalLine(middleX - x, b.getY(), b.getBottom());
        }

        for (auto y = meter; y < b.getHeight() * 0.5f; y += meter)
        {
            g.drawHorizontalLine(middleY + y, b.getX(), b.getRight());
            g.drawHorizontalLine(middleY - y, b.getX(), b.getRight());
        }
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(10);
        mDemoFileBox.setBounds(b.removeFromTop(mBoxHeight));
        //updateSpeakerBounds();
        updateMatrices();

        // Position speaker components
        for (int i = 0; i < mSpeakers.size(); ++i)
        {
            const auto& s = mModel.mSpeakerPositions[i];
            const auto p = getWorldToRect(glm::vec2(s.x, s.z));
            mSpeakers[i]->setPosition(p.x, p.y);
        }
    }
    
    void mouseDown(const MouseEvent& event) override
    {
        const int index = (int)mDemoFileBox.getSelectedId() - 1;
        const auto pos = getWorldPositionFromMouse(event);
        
        jassert(index < mModel.mSoundFiles.size());
        
        if (onTrigger != nullptr)
            onTrigger(index, pos);
    }
    
    void mouseDrag(const MouseEvent& event) override
    {
        const int index = (int)mDemoFileBox.getSelectedId() - 1;
        const auto pos = getWorldPositionFromMouse(event);
        
        jassert(index < mModel.mSoundFiles.size());
        
        if (onUpdate != nullptr)
            onUpdate(index, pos);
    }
    
    void updateFileList()
    {
        mDemoFileBox.clear();
        
        for (int i = 0; i < mModel.mSoundFiles.size(); ++i)
            mDemoFileBox.addItem(mModel.mSoundFiles[i].getFileNameWithoutExtension(), i + 1);
        
        if (mModel.mSoundFiles.size() > 0)
            mDemoFileBox.setSelectedId(1);
    }
    
    void updateSpeakerBounds()
    {
        mWindowDiameter = 1.0f;
        
        for (const auto& p : mModel.mSpeakerPositions)
        {
            const float l = glm::length(glm::vec2(p.x, p.z));
            
            if (l > mWindowDiameter)
                mWindowDiameter = l;
        }
        
        mWindowDiameter *= 2.5f;
        mMinWindowDiameter = mWindowDiameter;
    }
    
    void mouseMagnify (const MouseEvent& event, float scaleFactor) override
    {
        mWindowDiameter = jmin(mMaxWindowDiameter, jmax(mMinWindowDiameter, mWindowDiameter / scaleFactor));
        resized();
        repaint();
    }
    
    std::function<void(int, glm::vec3)> onTrigger;
    std::function<void(int, glm::vec3)> onUpdate;

private:

    Rectangle<float> getViewerRect()
    {
        auto b = getLocalBounds().reduced(10).toFloat();
        b.removeFromTop(mBoxHeight);
        b.removeFromTop(10.0f);
        return b;
    }

    const glm::vec3 getWorldPositionFromMouse(const MouseEvent& event)
    {
        const auto mp = event.getPosition();
        const auto p = getRectToWorld(glm::vec2(mp.x, mp.y));
        return glm::vec3(p.x, 0.0f, p.y);
    }
    
    void drawCircle(Graphics& g, float x, float y, float r, float thickness)
    {
        g.drawEllipse(x - r, y - r, r * 2.0f, r * 2.0f, thickness);
    }
    
    glm::vec2 getRectToWorld(const glm::vec2& p)
    {
        const auto pt = mRectToWorld * glm::vec3(p.x, p.y, 1.0f);
        return glm::vec2(pt.x, pt.y);
    }
    
    glm::vec2 getWorldToRect(const glm::vec2& p)
    {
        const auto pt = mWorldToRect * glm::vec3(p.x, p.y, 1.0f);
        return glm::vec2(pt.x, pt.y);
    }
    
    void updateMatrices()
    {
        auto b = getViewerRect();
        const float minDim = jmin(b.getWidth(), b.getHeight());
        
        glm::mat3 t = glm::mat3(1.0f);
        t = glm::scale(t, glm::vec2(1.0f) * mWindowDiameter);
        t = glm::scale(t, glm::vec2(1.0f / minDim));
        t = glm::translate(t, glm::vec2(-b.getCentreX(), -b.getCentreY()));
        
        mRectToWorld = t;
        mWorldToRect = glm::inverse(t);
    }

    glm::mat3 mRectToWorld;
    glm::mat3 mWorldToRect;
    ComboBox  mDemoFileBox;
    int       mBoxHeight = 40;
    AppModel& mModel;
    
    float     mWindowDiameter = 4.0f;
    float     mMinWindowDiameter = 4.0f;
    float     mMaxWindowDiameter = 100.0f;

    OwnedArray<SpeakerComponent> mSpeakers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpaceViewerComponent)
};
