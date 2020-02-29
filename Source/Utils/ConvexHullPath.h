/*
  ==============================================================================

    ConvexHullPath.h
    Created: 26 Feb 2020 10:00:50pm
    Author:  Felix Faire

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "vec2.hpp"

class ConvexHullPath
{
public:
    ConvexHullPath()
    {
    }
    
    // Performs gift wrapping algorithm on points to create convex hull
    void updatePoints(const std::vector<glm::vec2>& points, const glm::vec2& centerPos)
    {
        int leftestIndex = 0;
        
        for (int i = 1; i < points.size(); i++)
            if (points[i].x < points[leftestIndex].x)
                leftestIndex = i;
                  
        mConvexHullPoints.clear();

        // Start from leftmost point, keep moving counterclockwise
        // until reach the start point again.  This loop runs O(h)
        // times where h is number of points in result or output.
        int p = leftestIndex;
        do
        {
            // Add current point to result
            mConvexHullPoints.push_back(points[p]);
      
            // Search for a point 'q' such that orientation(p, x,
            // q) is counterclockwise for all points 'x'. The idea
            // is to keep track of last visited most counterclock-
            // wise point in q. If any point 'i' is more counterclock-
            // wise than q, then update q.
            int q = (p + 1) % points.size();
            
            for (int i = 0; i < points.size(); i++)
            {
               // If i is more counterclockwise than current q, then
               // update q
               if (orientation(points[p], points[i], points[q]) == 1)
                   q = i;
            }
      
            // Now q is the most counterclockwise with respect to p
            // Set p as q for next iteration, so that q is added to
            // result 'hull'
            p = q;
      
        } while (p != leftestIndex);  // While we don't come to first point
      
        
        jassert(mConvexHullPoints.size() > 1);
        
        mPath.clear();
        
        const int sz = (int)mConvexHullPoints.size();

        for (int i = 0; i < sz; ++i)
        {
            auto p = mConvexHullPoints[i];
            const auto& prevp = mConvexHullPoints[(i - 1 + sz) % sz];
            const auto& nextp = mConvexHullPoints[(i + 1) % sz];
            auto prevT = p - prevp;
            auto nextT = nextp - p;
            prevT = glm::normalize(glm::vec2(-prevT.y, prevT.x));
            nextT = glm::normalize(glm::vec2(-nextT.y, nextT.x));
            p += (prevT + nextT) * mOffset * (1.0f - 0.5f * glm::dot(prevT, nextT));
            
            if (i == 0)
                mPath.startNewSubPath(p.x, p.y);
            else
                mPath.lineTo(p.x, p.y);
        }
        
        mPath.closeSubPath();
        mPath = mPath.createPathWithRoundedCorners(mOffset);
    }
    
    float                   mOffset = 15.0f;
    std::vector<glm::vec2>  mConvexHullPoints;
    Path                    mPath;
    
private:
    
    // To find orientation of ordered triplet (p, q, r).
    // The function returns following values
    // 0 --> p, q and r are colinear
    // 1 --> Clockwise
    // 2 --> Counterclockwise
    int orientation(const glm::vec2& p,const glm::vec2& q,const glm::vec2& r)
    {
        int val = (q.y - p.y) * (r.x - q.x) -
                  (q.x - p.x) * (r.y - q.y);
      
        if (val == 0) return 0;  // colinear
        return (val > 0)? 1: 2; // clock or counterclock wise
    }

};