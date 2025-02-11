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
    DSPresetConverter();
    void parseDSEXS24(DSEXS24 exs24);
    void parseSFZValueTree(juce::ValueTree valueTree);
    
    bool huntForSamples(juce::File inputDirectory, juce::String sampleSetName);
    bool convertPathsToDesiredDirectory(juce::File inputDirectory, juce::String desiredDirectoryName);
    bool convertPathsToRelative(juce::File inputDirectory);
    bool convertEXSLoopCrossfadePoints();
    bool copySamplesOverToNewDirectory(juce::File rootOutputDirectory, juce::String sampleSetName, bool skipAudioProcessing, int overrideBitrate);
    
    
    juce::String getXML() {
        juce::XmlElement::TextFormat format;
        format.lineWrapLength = 20000;
//        format.newLineChars = "";
        return valueTree.toXmlString(format);
    }
    juce::String getSFZ();
    
    juce::ValueTree getValueTree() { return valueTree; }
    std::unique_ptr<juce::XmlElement> getXMLObject() { return valueTree.createXml(); }
    
    enum HeaderLevel {
        headerLevelGlobal,
        headerLevelGroup,
        headerLevelRegion
    };
private:
    juce::AudioFormatManager audioFormatManager;
    juce::ValueTree valueTree;
    void translateSFZRegionProperties(juce::ValueTree sfzRegion, juce::ValueTree &dsSample, HeaderLevel level);
    void addGenericUI();

    double linearOrDbStringToDb(const juce::String inputString);
};
