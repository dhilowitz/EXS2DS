/*
  ==============================================================================

    DSPresetConverter.h
    Created: 16 Aug 2021 9:55:21pm
    Author:  David Hilowitz

  ==============================================================================
*/

#pragma once

#include "DSEXS24.h"

class DSPresetConverter {
public:
    void parseDSEXS24(DSEXS24 exs24, juce::String samplePath, juce::File outputDir);
    void parseSFZValueTree(juce::ValueTree valueTree);
    juce::String getXML() {
        juce::XmlElement::TextFormat format;
        format.lineWrapLength = 20000;
//        format.newLineChars = "";
        return valueTree.toXmlString(format);
    }
    
    juce::ValueTree getValueTree() { return valueTree; }
    std::unique_ptr<juce::XmlElement> getXMLObject() { return valueTree.createXml(); }
    
    enum HeaderLevel {
        headerLevelGlobal,
        headerLevelGroup,
        headerLevelRegion
    };
private:
    juce::ValueTree valueTree;
    void translateSFZRegionProperties(juce::ValueTree sfzRegion, juce::ValueTree &dsSample, HeaderLevel level);
};
