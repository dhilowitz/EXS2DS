/*
  ==============================================================================

    DSPresetMaker.h
    Created: 16 Aug 2021 9:55:21pm
    Author:  David Hilowitz

  ==============================================================================
*/

#pragma once

#include "DSEXS24.h"

class DSPresetMaker {
public:
    void parseDSEXS24(DSEXS24 exs24, juce::String samplePath, juce::File outputDir);
    void parseSFZValueTree(juce::ValueTree valueTree);
    juce::String getXML() {
        return valueTree.toXmlString();
    }
    
    enum HeaderLevel {
        headerLevelGlobal,
        headerLevelGroup,
        headerLevelRegion
    };
private:
    juce::ValueTree valueTree;
    void translateSFZRegionProperties(juce::ValueTree sfzRegion, juce::ValueTree &dsSample, HeaderLevel level);
};
