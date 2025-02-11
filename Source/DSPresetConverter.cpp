/*
  ==============================================================================

    DSPresetConverter.cpp
    Created: 16 Aug 2021 9:55:21pm
    Author:  David Hilowitz

  ==============================================================================
*/

#include "DSPresetConverter.h"

 DSPresetConverter::DSPresetConverter() {
     audioFormatManager.registerBasicFormats();
}

void DSPresetConverter::parseDSEXS24(DSEXS24 exs24) {

    // Setup the EXS24 data
    juce::Array<DSEXS24Zone> &zones = exs24.getZones();
    juce::Array<DSEXS24Group> &groups = exs24.getGroups();
    juce::Array<DSEXS24Sample> &samples = exs24.getSamples();
    valueTree = juce::ValueTree("DecentSampler");
    
    // Add a generic UI
    addGenericUI();
    
    juce::ValueTree groupsVT = juce::ValueTree("groups");
    valueTree.appendChild(groupsVT, nullptr);
    
    for (DSEXS24Zone zone : zones) {
        if(zone.groupIndex < 0) {
            continue;
        } else if(zone.groupIndex > 100) {
            printf("This zone's group index is greater than 100. This converter may not support this file.");
            continue;
        }
        while (zone.groupIndex >= groups.size()) {
            DSEXS24Group newGroup;
            newGroup.name = "Couldn't find group index " + juce::String(zone.groupIndex);
            groups.add(newGroup);
        }
    }
    
    int highestSequenceNumber = 0;
    
    // Iterate through the groups and add their respective samples
    for(int groupIndex = -1; groupIndex < groups.size(); groupIndex++) {
        bool hasSamples = false;
        DSEXS24Group group = (groupIndex >= 0) ? groups[groupIndex] : DSEXS24Group();
        juce::ValueTree dsGroup ("group");
        dsGroup.setProperty("attack", "0.001", nullptr);
        if(group.name != "") {
            dsGroup.setProperty("name", group.name, nullptr);
        }
//        dsGroup.setProperty("_exsGroupIndex", groupIndex, nullptr);
        if(group.pan != 0) {
            dsGroup.setProperty("pan", group.pan, nullptr);
        }
        if(group.volume != 0) {
            dsGroup.setProperty("volume", juce::String(group.volume) + "dB", nullptr);
        }
        
        if(group.seqNumber != 0) {
            dsGroup.setProperty("seqPosition", group.seqNumber, nullptr);
            if(group.seqNumber > highestSequenceNumber) {
                highestSequenceNumber = group.seqNumber;
            }
        }
//        dsGroup.setProperty("_exsSequence", group.exsSequence, nullptr);
        
        for (DSEXS24Zone zone : zones) {
            if (zone.groupIndex == groupIndex) {
                juce::ValueTree dsSample("sample");
                
                int sampleIndex = zone.sampleIndex;
                if(sampleIndex < 0 || sampleIndex >= samples.size()) {
                    continue;
                }
                
                DSEXS24Sample &sample = samples.getReference(sampleIndex);
                dsSample.setProperty("path", sample.fileName, nullptr);
                    
                dsSample.setProperty("name", zone.name, nullptr);
                if(zone.pitch == false) {
                    dsSample.setProperty("pitchKeyTrack", 0, nullptr);
                }
                dsSample.setProperty("rootNote", zone.key, nullptr);
                dsSample.setProperty("loNote", zone.keyLow, nullptr);
                dsSample.setProperty("hiNote", zone.keyHigh, nullptr);
                dsSample.setProperty("loVel", zone.velocityRangeOn ? zone.loVel : 0, nullptr);
                dsSample.setProperty("hiVel", zone.velocityRangeOn ? zone.hiVel : 127, nullptr);
                
                float tuning = (float) zone.coarseTuning + (((float)zone.fineTuning)/100.0f);
                if(tuning != 0) {
                    dsSample.setProperty("tuning", tuning, nullptr);
                }
                if(zone.pan != 0) {
                    dsSample.setProperty("pan", zone.pan, nullptr);
                }
                if(zone.volume != 0) {
                    dsSample.setProperty("volume", juce::String(zone.volume) + "dB", nullptr);
                }
                
                if(zone.sampleStart != 0) {
                    dsSample.setProperty("start", zone.sampleStart, nullptr);
                }
                
                if(zone.sampleEnd != 0) {
                    dsSample.setProperty("end", zone.sampleEnd - 1, nullptr);
                }
                
                if(zone.loopEnabled) {
                    dsSample.setProperty("loopEnabled", zone.loopEnabled, nullptr);
                    dsSample.setProperty("loopStart", zone.loopStart, nullptr);
                    dsSample.setProperty("loopEnd", ((zone.loopEnd > 0 ? zone.loopEnd - 1 : 0)), nullptr);
                
                    if(zone.loopCrossfadeMilliseconds != 0) {
                        dsSample.setProperty("loopCrossfadeMilliseconds", zone.loopCrossfadeMilliseconds, nullptr);
                        dsSample.setProperty("loopCrossfade", 48 * zone.loopCrossfadeMilliseconds, nullptr);
                    }
                    
                    dsSample.setProperty("loopCrossfadeMode", zone.loopEqualPower ? "equal_power" : "linear", nullptr);
                }
                
                
                hasSamples = true;
                dsGroup.appendChild(dsSample, nullptr);
            }
        }
        if(hasSamples) {
            groupsVT.appendChild(dsGroup, nullptr);
        }
    }
    
    // Go back through and set seqLength as needed
    for(juce::ValueTree groupVT : groupsVT) {
        if(groupVT.hasProperty("seqPosition")) {
            groupVT.setProperty("seqLength", highestSequenceNumber, nullptr);
            groupVT.setProperty("seqMode", "round_robin", nullptr);
        }
        for(juce::ValueTree sampleVT : groupVT) {
            if(sampleVT.hasProperty("seqPosition")) {
                sampleVT.setProperty("seqLength", highestSequenceNumber, nullptr);
                sampleVT.setProperty("seqMode", "round_robin", nullptr);
            }
        }
    }
    
}

void DSPresetConverter::parseSFZValueTree(juce::ValueTree sfz) {
    valueTree = juce::ValueTree("DecentSampler");
    juce::ValueTree groupsVT = juce::ValueTree("groups");
    translateSFZRegionProperties(sfz, groupsVT, headerLevelGlobal);
    valueTree.appendChild(groupsVT, nullptr);
    
    for (auto sfzSection : sfz) {
        if(sfzSection.hasType("group")) {
            juce::ValueTree dsGroup ("group");
            translateSFZRegionProperties(sfzSection, dsGroup, headerLevelGroup);
            for (auto sfzRegion : sfzSection) {
                juce::ValueTree dsSample("sample");
                translateSFZRegionProperties(sfzRegion, dsSample, headerLevelRegion);
                dsGroup.appendChild(dsSample, nullptr);
            }
            groupsVT.appendChild(dsGroup, nullptr);
        }
    }
}

void DSPresetConverter::translateSFZRegionProperties(juce::ValueTree sfzRegion, juce::ValueTree &dsEntity, HeaderLevel level) {
    for (int i = 0; i < sfzRegion.getNumProperties(); i++) {
        juce::String key = sfzRegion.getPropertyName(i).toString();
        juce::var value =      sfzRegion.getProperty(key);
        
        if(level == headerLevelGroup && key == "group_label") {
            dsEntity.setProperty("name", value, nullptr);
        } else if(key == "amp_veltrack") {
            dsEntity.setProperty("ampVelTrack", ((float)value)/100.0f, nullptr);
        } else if(key == "ampeg_attack") {
            dsEntity.setProperty("attack", value, nullptr);
        } else if(key == "ampeg_release") {
            dsEntity.setProperty("release", value, nullptr);
        } else if(key == "ampeg_sustain") {
            dsEntity.setProperty("sustain", value, nullptr);
        } else if(key == "ampeg_decay") {
            dsEntity.setProperty("decay", value, nullptr);
        } else if(key == "group") {
            dsEntity.setProperty("tags", "voice-group-" + value.toString(), nullptr);
        } else if(key == "end") {
            dsEntity.setProperty("end", value, nullptr);
        } else if(key == "hikey") {
            dsEntity.setProperty("hiNote", value, nullptr);
        } else if(key == "hivel") {
            dsEntity.setProperty("hiVel", value, nullptr);
        } else if(key == "key") {
            dsEntity.setProperty("rootNote", value, nullptr);
            dsEntity.setProperty("loNote", value, nullptr);
            dsEntity.setProperty("hiNote", value, nullptr);
        } else if(key == "lokey") {
            dsEntity.setProperty("loNote", value, nullptr);
        } else if(key == "loop_end") {
            dsEntity.setProperty("loopEnd", value, nullptr);
        } else if(key == "loop_mode") {
            if(value == "loop_continuous") {
                dsEntity.setProperty("loopEnabled", (value == "loop_continuous") ? "true" : "false", nullptr);
            }
        } else if(key == "loop_start") {
            dsEntity.setProperty("loopStart", value, nullptr);
        } else if(key == "lovel") {
            dsEntity.setProperty("loVel", value, nullptr);
        } else if(key == "off_by") {
            dsEntity.setProperty("silencedByTags", "voice-group-" + value.toString(), nullptr);
        } else if(key == "off_mode") {
            dsEntity.setProperty("silencingMode", value, nullptr);
        } else if(key == "offset") {
            dsEntity.setProperty("start", value, nullptr);
        } else if(key == "pitch_keycenter") {
            dsEntity.setProperty("rootNote", value, nullptr);
        } else if(key == "sample") {
            dsEntity.setProperty("path", value, nullptr);
        } else if(key == "seq_position") {
            dsEntity.setProperty("seqPosition", value, nullptr);
            dsEntity.setProperty("seqMode", "round_robin", nullptr);
        } else if(key == "seq_length") {
            dsEntity.setProperty("seqLength", value, nullptr);
            dsEntity.setProperty("seqMode", "round_robin", nullptr);
        } else if(key == "sw_previous") {
            dsEntity.setProperty("previousNote", value, nullptr);
        } else if(key == "trigger") {
            dsEntity.setProperty("trigger", value, nullptr);
        } else if(key == "tune") {
            dsEntity.setProperty("tuning", ((int)value)/100.0, nullptr);
        } else if(key == "volume") {
            dsEntity.setProperty("volume", value.toString() + "dB", nullptr);
        } else {
            switch (level) {
                case headerLevelGlobal:
                    DBG("<global> opcode " + key + " not supported.");
                    break;
                case headerLevelGroup:
                    DBG("<group> opcode " + key + " not supported.");
                    break;
                case headerLevelRegion:
                default:
                    DBG("<region> opcode " + key + " not supported.");
                    break;
            }
        }
    }
}

void DSPresetConverter::addGenericUI() {
    juce::String effectsXML = "<effects> \
      <effect type=\"lowpass\" frequency=\"22000.0\"/>\
          <effect type=\"chorus\"  mix=\"0.0\" modDepth=\"0.2\" modRate=\"0.2\" />\
          <effect type=\"reverb\" wetLevel=\"0.5\"/>\
        </effects>";
    
    valueTree.appendChild(juce::ValueTree::fromXml(effectsXML), nullptr);
    
    juce::String uiXML = "<ui width=\"812\" height=\"375\" bgImage=\"Images/background.jpg\">\
        <tab name=\"main\">\
          <labeled-knob x=\"255\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Attack\" type=\"float\" minValue=\"0.0\" maxValue=\"5\" value=\"0.2\" >\
            <binding type=\"amp\" level=\"instrument\" position=\"0\" parameter=\"ENV_ATTACK\" />\
          </labeled-knob>\
          <labeled-knob x=\"330\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Decay\" type=\"float\" minValue=\"0.0\" maxValue=\"5\" value=\"1\" >\
            <binding type=\"amp\" level=\"instrument\" position=\"0\" parameter=\"ENV_DECAY\" />\
          </labeled-knob>\
          <labeled-knob x=\"405\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Sustain\" type=\"float\" minValue=\"0.0\" maxValue=\"1\" value=\"1\" >\
            <binding type=\"amp\" level=\"instrument\" position=\"0\" parameter=\"ENV_SUSTAIN\" />\
          </labeled-knob>\
          <labeled-knob x=\"480\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Release\" type=\"float\" minValue=\"0.01\" maxValue=\"4.0\" value=\"0.1\" >\
            <binding type=\"amp\" level=\"instrument\" position=\"0\" parameter=\"ENV_RELEASE\" />\
          </labeled-knob>\
          <labeled-knob x=\"562\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Chorus\" type=\"float\" minValue=\"0.0\" maxValue=\"1\" value=\"0\" >\
            <binding type=\"effect\" level=\"instrument\" position=\"1\" parameter=\"FX_MIX\" />\
          </labeled-knob>\
          <labeled-knob x=\"635\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\"\
                        trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Tone\" type=\"float\" minValue=\"0.5\" maxValue=\"1\" value=\"1\">\
            <binding type=\"effect\" level=\"instrument\" position=\"0\" parameter=\"FX_FILTER_FREQUENCY\" translation=\"table\" translationTable=\"0,33;0.3,150;0.4,450;0.5,1100;0.7,4100;0.9,11000;1.0001,22000\"/>\
          </labeled-knob>\
          <labeled-knob x=\"710\" y=\"80\" width=\"100\" textSize=\"16\" textColor=\"AA000000\" trackForegroundColor=\"CC000000\" trackBackgroundColor=\"66999999\"\
                        label=\"Reverb\" type=\"percent\" minValue=\"0\" maxValue=\"100\"\
                        textColor=\"FF000000\" value=\"50\">\
            <binding type=\"effect\" level=\"instrument\" position=\"2\"\
                     parameter=\"FX_REVERB_WET_LEVEL\" translation=\"linear\"\
                     translationOutputMin=\"0\" translationOutputMax=\"1\" />\
          </labeled-knob>\
        </tab>\
      </ui>";
    
    valueTree.appendChild(juce::ValueTree::fromXml(uiXML), nullptr);
}

// Convert the current internal valueTree, which is in a basic DecentSampler format, into an SFZ file
juce::String DSPresetConverter::getSFZ() {
    // Initialize the SFZ file with a header
    juce::String sfz = "// SFZ file created with EXS2ALL by David Hilowitz\n\n";
    
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        sfz = sfz + "// Converted EXS file was empty. ";
        return sfz;
    }
    
    sfz = sfz + "\n<control>\n";

    auto parseSampleAndGroupProperties = [this](juce::String &sfzFile, juce::ValueTree &valueTree, HeaderLevel level) {
        if(level == headerLevelGroup) {
             if (valueTree.hasProperty("name")) {
                 sfzFile = sfzFile + "group_label=" + valueTree.getProperty("name").toString() + " ";
             }
        }
        if(valueTree.hasProperty("path")) {
            sfzFile = sfzFile + "sample=" + valueTree.getProperty("path").toString() + " ";
        }
        if(valueTree.hasProperty("rootNote")) {
            sfzFile = sfzFile + "pitch_keycenter=" + valueTree.getProperty("rootNote").toString() + " ";
        }
        if(valueTree.hasProperty("loNote") ) {
            sfzFile = sfzFile + "lokey=" + valueTree.getProperty("loNote").toString() + " ";
        }
        if(valueTree.hasProperty("hiNote")) {
            sfzFile = sfzFile + "hikey=" + valueTree.getProperty("hiNote").toString() + " ";
        }
        if(valueTree.hasProperty("loVel") && valueTree.getProperty("loVel").toString() != "0") {
            sfzFile = sfzFile + "lovel=" + valueTree.getProperty("loVel").toString() + " ";
        }
        if(valueTree.hasProperty("hiVel") && valueTree.getProperty("hiVel").toString() != "127") {
            sfzFile = sfzFile + "hivel=" + valueTree.getProperty("hiVel").toString() + " ";
        }
        if(valueTree.hasProperty("start") && valueTree.getProperty("start").toString() != "0") {
            sfzFile = sfzFile + "offset=" + valueTree.getProperty("start").toString() + " ";
        }
        if(valueTree.hasProperty("end")) {
            sfzFile = sfzFile + "end=" + valueTree.getProperty("end").toString() + " ";
        }
        if(valueTree.hasProperty("loopEnabled")) {
            sfzFile = sfzFile + "loop_mode=loop_continuous ";
        }
        if(valueTree.hasProperty("loopStart")) {
            sfzFile = sfzFile + "loop_start=" + valueTree.getProperty("loopStart").toString() + " ";
        }
        if(valueTree.hasProperty("loopEnd")) {
            sfzFile = sfzFile + "loop_end=" + valueTree.getProperty("loopEnd").toString() + " ";
        }
        
        if(valueTree.hasProperty("pan")) {
            sfzFile = sfzFile + "pan=" + valueTree.getProperty("pan").toString() + " ";
        }

        if(valueTree.hasProperty("ampVelTrack") && valueTree.getProperty("ampVelTrack") != "1.0") {
            float value = valueTree.getProperty("ampVelTrack");
            if(value >= 0 && value < 1) {
                sfzFile = sfzFile + "amp_veltrack=" + juce::String((int)(value*100)) + " ";
            }
        }
        if(valueTree.hasProperty("attack")) {
            sfzFile = sfzFile + "ampeg_attack=" + valueTree.getProperty("attack").toString() + " ";
        }
        if(valueTree.hasProperty("release")) {
            sfzFile = sfzFile + "ampeg_release=" + valueTree.getProperty("release").toString() + " ";
        }
        if(valueTree.hasProperty("sustain")) {
            sfzFile = sfzFile + "ampeg_sustain=" + valueTree.getProperty("sustain").toString() + " ";
        }
        if(valueTree.hasProperty("decay")) {
            sfzFile = sfzFile + "ampeg_decay=" + valueTree.getProperty("decay").toString() + " ";
        }
        if(valueTree.hasProperty("seqPosition")) {
            sfzFile = sfzFile + "seq_position=" + valueTree.getProperty("seqPosition").toString() + " ";
        }
        if(valueTree.hasProperty("seqLength")) {
            sfzFile = sfzFile + "seq_length=" + valueTree.getProperty("seqLength").toString() + " ";
        }
        if(valueTree.hasProperty("tags")) {
            if(valueTree.getProperty("tags").toString().contains("voice-group-")) {
                sfzFile = sfzFile + "group=" + valueTree.getProperty("tags").toString().replace("voice-group-", "") + " ";
            }
        }
        
        if(valueTree.hasProperty("silencedByTags")) {
            sfzFile = sfzFile + "off_by=" + valueTree.getProperty("silencedByTags").toString().replace("voice-group-", "") + " ";
        }
        if(valueTree.hasProperty("silencingMode")) {
            sfzFile = sfzFile + "off_mode=" + valueTree.getProperty("silencingMode").toString() + " ";
        }
        
        
        if(valueTree.hasProperty("previousNote")) {
            sfzFile = sfzFile + "sw_previous=" + valueTree.getProperty("previousNote").toString() + " ";
        }
        if(valueTree.hasProperty("trigger")) {
            sfzFile = sfzFile + "trigger=" + valueTree.getProperty("trigger").toString() + " ";
        }
        if(valueTree.hasProperty("tuning")) {
            sfzFile = sfzFile + "tune=" + juce::String((int)((float)valueTree.getProperty("tuning") * 100.0f)) + " ";
        }
        if(valueTree.hasProperty("volume")) {
            sfzFile = sfzFile + "volume=" + juce::String(linearOrDbStringToDb(valueTree.getProperty("volume").toString())) + " ";
        }
    };
    
    parseSampleAndGroupProperties(sfz, groupsValueTree, headerLevelGlobal);
    sfz << "\n";
    
    // Iterate through the groups and add their samples
     for (auto groupValueTree : groupsValueTree) {
         if(!groupValueTree.hasType("group")) {
             continue;
         }
         sfz << "<group>";
         parseSampleAndGroupProperties(sfz, groupValueTree, headerLevelGroup);
         sfz << "\n";
        
         for (auto sampleValueTree : groupValueTree) {
             if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                 continue;
             }
             
             sfz << "<region>";
             parseSampleAndGroupProperties(sfz, sampleValueTree, headerLevelRegion);
             sfz << "\n";
         }
     }
    return sfz;
}


double DSPresetConverter::linearOrDbStringToDb(const juce::String inputString) {
    if (inputString.contains("dB")) {
        return juce::jlimit(
             (double) -100,
             (double) 24,
             inputString.upToFirstOccurrenceOf("dB", false, true).getDoubleValue());
        
    } else {
        return juce::Decibels::gainToDecibels(inputString.getDoubleValue());
    }
}


bool DSPresetConverter::huntForSamples(juce::File inputDirectory, juce::String sampleSetName) {
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        return true;
    }
    auto parseSampleAndGroupProperties = [this](juce::ValueTree &valueTree, juce::File inputDirectory, juce::String sampleSetName) {
        if(valueTree.hasProperty("path")) {
            juce::String path = valueTree.getProperty("path").toString();
            
            juce::File sampleFile = juce::File::getCurrentWorkingDirectory().getChildFile(path);
            if(sampleFile.existsAsFile()) {
                std::cout << "Sample file \"" << path << "\" found." << std::endl;
                valueTree.setProperty("path", sampleFile.getFullPathName(), nullptr);
                return true;
            }
            
            sampleFile = inputDirectory.getChildFile(sampleSetName).getChildFile(path);
            if(sampleFile.existsAsFile()) {
//                valueTree.setProperty("path", sampleFile.getRelativePathFrom(inputDirectory), nullptr);
                valueTree.setProperty("path", sampleFile.getFullPathName(), nullptr);
                std::cout << "Sample file path changed to \"" << valueTree.getProperty("path").toString() << "\"." << std::endl;
                return true;
            }
            
            sampleFile = inputDirectory.getChildFile("Samples").getChildFile(path);
            if(sampleFile.existsAsFile()) {
//                valueTree.setProperty("path", sampleFile.getRelativePathFrom(inputDirectory), nullptr);
                valueTree.setProperty("path", sampleFile.getFullPathName(), nullptr);
                std::cout << "Sample file path changed to \"" << valueTree.getProperty("path").toString() << "\"." << std::endl;
                return true;
            }
    
            std::cerr << "Sample file \"" << path << "\" not found." << std::endl;
            return false;
        }
        return true;
    };

    parseSampleAndGroupProperties(groupsValueTree, inputDirectory, sampleSetName);
    // Iterate through the groups and add their samples
        for (auto groupValueTree : groupsValueTree) {
            if(!groupValueTree.hasType("group")) {
                continue;
            }
            parseSampleAndGroupProperties(groupValueTree, inputDirectory, sampleSetName);
            
            for (auto sampleValueTree : groupValueTree) {
                if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                    continue;
                }
                if(!parseSampleAndGroupProperties(sampleValueTree, inputDirectory, sampleSetName)) {
                    std::cerr << "Sample file \"" << sampleValueTree.getProperty("path").toString() << "\" not found." << std::endl;
                    return false;
                }
            }
        }
    return true;
}

// This function needs to do more than just copy samples to the new directory, it needs to
// also make sure that the paths in the XML are updated to reflect the new location.
// It also needs to burn the loop crossfades into the wave files.
bool DSPresetConverter::copySamplesOverToNewDirectory(juce::File rootOutputDirectory, juce::String sampleSetName, bool skipAudioProcessing, int overrideBitrate) {
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        return true;
    }
    
    int             globalStart = groupsValueTree.getProperty("start", 0);
    int             globalEnd = groupsValueTree.getProperty("end", -1);
    bool            globalLoopEnabled = groupsValueTree.getProperty("loopEnabled", false);
    int             globalLoopStart = groupsValueTree.getProperty("loopStart", 0);
    int             globalLoopEnd = groupsValueTree.getProperty("loopEnd", -1);
    int             globalLoopCrossfade = groupsValueTree.getProperty("loopCrossfade", 0);
    juce::String    globalLoopCrossfadeMode = groupsValueTree.getProperty("loopCrossfadeMode", "linear");
    
    auto processAudioFiles = [this](juce::ValueTree &valueTree, juce::File outputDirectory, juce::String sampleSetName, int start, int end,  bool loopEnabled, int loopStart, int loopEnd, int loopCrossfade, juce::String loopCrossfadeMode, bool simpleCopy, int overrideBitrate) {
        if(!valueTree.hasProperty("path")) {
            return true;
        }
        juce::String path = valueTree.getProperty("path").toString();
        
        juce::File sampleFile = juce::File(path);
        if(!sampleFile.existsAsFile()) {
            std::cerr << "Sample file \"" << path << "\" not found." << std::endl;
            return false;
        }
        

        if(!outputDirectory.getChildFile("Samples").exists()) {
            juce::Result result = outputDirectory.getChildFile("Samples").createDirectory();
        }
        
        if(!outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).exists()) {
            juce::Result result = outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).createDirectory();
        }
        
        if(simpleCopy) {
            // Copy the file
            bool result = sampleFile.copyFileTo(outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).getChildFile(sampleFile.getFileName()));
            if(!result) {
                std::cerr << "Sample file \"" << path << "\" could not be copied." << std::endl;
                return false;
            }
            
            // Update the path
            juce::String simpleFilePath = "Samples/" + sampleSetName + "/" + sampleFile.getFileName();
            valueTree.setProperty("path", simpleFilePath, nullptr);
            std::cout << "Sample file path changed to \"" << simpleFilePath << "\"." << std::endl;
            return true;
        }

        std::unique_ptr<juce::AudioFormatReader> reader (audioFormatManager.createReaderFor (sampleFile));
    
        std::unique_ptr<juce::AudioFormat> audioFormat;
        if(sampleFile.hasFileExtension("wav")) {
            audioFormat = std::unique_ptr<juce::AudioFormat>(new juce::WavAudioFormat());
        } else if(sampleFile.hasFileExtension("aif") || sampleFile.hasFileExtension("aiff")) {
            audioFormat = std::unique_ptr<juce::AudioFormat>(new juce::AiffAudioFormat());
        } else if(sampleFile.hasFileExtension("flac")) {
            audioFormat = std::unique_ptr<juce::AudioFormat>(new juce::FlacAudioFormat());
        } else {
            std::cerr << "Sample file \"" << path << "\" is in an unrecognizable format." << std::endl;
            return false;
        }
        
        // Get the loop points from the input file
        int numChannels = reader->numChannels;
        int sourceSampleRate = reader->sampleRate;
        
        int bitsPerSample = reader->bitsPerSample;
        if(overrideBitrate == 16 || overrideBitrate == 24 || overrideBitrate == 32)
            bitsPerSample = overrideBitrate;
        int fileLength = (int) reader->lengthInSamples;
        int fileMetadataLoopStart = reader->metadataValues.getValue("Loop0Start", "0").getIntValue();
        int fileMetadataLoopEnd = reader->metadataValues.getValue("Loop0End", "0").getIntValue();

        if(fileMetadataLoopStart > 0 && loopStart == 0 && fileMetadataLoopEnd > 0 && loopEnd == -1) {
            loopEnabled = true;
            loopStart = fileMetadataLoopStart;
            loopEnd = fileMetadataLoopEnd;
        }
        
        if(loopEnabled && loopEnd == -1) {
            loopEnd = fileLength - 1;
        }

        if(start < 0 || start >= fileLength) {
            std::cerr << "Sample file \"" << path << "\" has a start point that is out of range." << std::endl;
            return false;
        }

        if(end >= fileLength) {
            std::cerr << "Sample file \"" << path << "\" has an end point that is out of range." << std::endl;
            return false;
        }

        if(loopEnabled && end < loopStart) {
            std::cerr << "Sample file \"" << path << "\" has an end point that is before the loop start point." << std::endl;
            return false;
        }

        if(loopEnabled && end < loopEnd) {
            std::cerr << "Sample file \"" << path << "\" has an end point that is before the loop end point." << std::endl;
            return false;
        }
        
        if(loopEnabled && (start > (loopStart - loopCrossfade))) {
            std::cerr << "Sample file \"" << path << "\" has a start point that is after the loop start point - crossfade." << std::endl;
            return false;
        }
        
        if(start > end) {
            std::cerr << "Sample file \"" << path << "\" has a start point that is after the end point." << std::endl;
            return false;
        }

        

        // Create buffer to read the original file
        juce::AudioBuffer<float> buffer (numChannels, (int) reader->lengthInSamples);
        reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);

        // Create output buffer with the correct size
        int outputLength = loopEnabled ? (1 + loopEnd - start) : (end + 1 - start);
        if(outputLength < 0) {
            std::cerr << "Sample file \"" << path << "\" has an end point that is before the start point." << std::endl;
            return false;
        }
        juce::AudioBuffer<float> outputBuffer (numChannels, outputLength);

        // If the loop is enabled, crossfade it
        if(loopEnabled) {
            if(loopEnd < 0) {
                loopEnd = fileLength - 1;
            }
            loopStart = juce::jlimit(0, fileLength - 1, loopStart);
            loopEnd = juce::jlimit(0, fileLength - 1, loopEnd);
            
            for (int channel = 0; channel < numChannels; ++channel)
            {
                int index = 0;
                int chunk1Length = loopStart - start;
                // Copy the beginning of the file up to loopStart
                outputBuffer.copyFrom (channel, index, buffer, channel, start, chunk1Length);
                index += chunk1Length;
                
                // Copy the portion of the loop before the crossfade starts
                int chunk2Length = loopEnd - loopStart - loopCrossfade;
                outputBuffer.copyFrom (channel, index, buffer, channel, loopStart, chunk2Length);
                index += chunk2Length;
                
               // Perform the crossfade
               for (int i = 0; i <= loopCrossfade; ++i)
               {
                   float fadeOut = std::cos ((float)i / loopCrossfade * juce::MathConstants<float>::halfPi);
                   float fadeIn = std::sin ((float)i / loopCrossfade * juce::MathConstants<float>::halfPi);
                   
                   float sampleToFadeOut = buffer.getSample (channel, (loopEnd - loopCrossfade) + i);
                   float sampleToFadeIn = buffer.getSample (channel, (loopStart - loopCrossfade) + i);
                   
                   outputBuffer.setSample (channel, index + i,
                                           sampleToFadeOut * fadeOut +
                                           sampleToFadeIn * fadeIn);
               }
            }
        } else {
            int newFileLength = (end - start) + 1;
            for (int channel = 0; channel < numChannels; ++channel) {
                // Copy the beginning of the file up to loopStart
                outputBuffer.copyFrom (channel, 0, buffer, channel, start, newFileLength);
                
            }
        }
        
        // Now that we've adjust 
        end -= start;
        if(loopEnabled) {
            loopStart -= start;
            loopEnd -= start;
        }
        end = juce::jmin(end, outputLength);
        start = 0;

        // Create the output directory
        if(!outputDirectory.getChildFile("Samples").exists())
            outputDirectory.getChildFile("Samples").createDirectory();
        
        if(!outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).exists())
            outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).createDirectory();
        
        
        if(!outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).exists()) {
            std::cerr << "Unable to create output directory." << std::endl;
            return false;
        }
        
        // Write the new buffer to the output file
        juce::File outputFile = outputDirectory.getChildFile("Samples").getChildFile(sampleSetName).getNonexistentChildFile(sampleFile.getFileNameWithoutExtension(), audioFormat->getFileExtensions()[0]);
        
        juce::StringPairArray metadata;

        if(loopEnabled) {
            // Set Loop0Start and Loop0End in metadata
            metadata.set("Loop0Start", juce::String(loopStart));
            metadata.set("Loop0End", juce::String(loopEnd));
            metadata.set("Loop0Identifier",juce::String(0));
            metadata.set("Loop0Type", juce::String(0));
            metadata.set("NumSampleLoops", "1");
        }
        
        std::unique_ptr<juce::AudioFormatWriter> writer (audioFormat->createWriterFor (new juce::FileOutputStream (outputFile), sourceSampleRate, numChannels, bitsPerSample, metadata, 0));
        if (writer.get() != nullptr) {
            writer->writeFromAudioSampleBuffer (outputBuffer, 0, outputBuffer.getNumSamples());
            writer->flush();
        }

        // Update the properties in the value tree
        juce::String simpleFilePath = "Samples/" + sampleSetName + "/" + outputFile.getFileName();
        valueTree.setProperty("path", simpleFilePath, nullptr);
        std::cout << "Sample file path changed to \"" << simpleFilePath << "\"." << std::endl;

        valueTree.setProperty("start", start, nullptr);
        valueTree.setProperty("end", end, nullptr);

        if(loopEnabled) {
            valueTree.setProperty("loopEnabled", loopEnabled, nullptr);
            valueTree.setProperty("loopStart", loopStart , nullptr);
            valueTree.setProperty("loopEnd", loopEnd, nullptr);
            valueTree.setProperty("loopCrossfade", 0, nullptr);
            valueTree.removeProperty("loopCrossfadeMode", nullptr);
        }
        
        return true;        
    };

    if(!processAudioFiles(groupsValueTree, rootOutputDirectory, sampleSetName, globalStart, globalEnd, globalLoopEnabled, globalLoopStart, globalLoopEnd, globalLoopCrossfade, globalLoopCrossfadeMode,skipAudioProcessing, overrideBitrate))
        return false;

    // Iterate through the groups and add their samples
        for (auto groupValueTree : groupsValueTree) {
            if(!groupValueTree.hasType("group")) {
                continue;
            }

            int             groupStart = groupValueTree.getProperty("start", globalStart);
            int             groupEnd = groupValueTree.getProperty("end", globalEnd);
            bool            groupLoopEnabled = groupValueTree.getProperty("loopEnabled", globalLoopEnabled);
            int             groupLoopStart = groupValueTree.getProperty("loopStart", globalLoopStart);
            int             groupLoopEnd = groupValueTree.getProperty("loopEnd", globalLoopEnd);
            int             groupLoopCrossfade = groupValueTree.getProperty("loopCrossfade", globalLoopCrossfade);
            juce::String    groupLoopCrossfadeMode = groupValueTree.getProperty("loopCrossfadeMode", globalLoopCrossfadeMode);
            
            if(!processAudioFiles(groupValueTree, rootOutputDirectory, sampleSetName, groupStart, groupEnd, groupLoopEnabled, groupLoopStart, groupLoopEnd, groupLoopCrossfade, groupLoopCrossfadeMode, skipAudioProcessing, overrideBitrate))
                return false;
            
            for (auto sampleValueTree : groupValueTree) {
                if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                    continue;
                }

                int             sampleStart = sampleValueTree.getProperty("start", groupStart);
                int             sampleEnd = sampleValueTree.getProperty("end", groupEnd);
                bool            sampleLoopEnabled = sampleValueTree.getProperty("loopEnabled", groupLoopEnabled);
                int             sampleLoopStart = sampleValueTree.getProperty("loopStart", groupLoopStart);
                int             sampleLoopEnd = sampleValueTree.getProperty("loopEnd", groupLoopEnd);
                int             sampleLoopCrossfade = sampleValueTree.getProperty("loopCrossfade", groupLoopCrossfade);
                juce::String    sampleLoopCrossfadeMode = sampleValueTree.getProperty("loopCrossfadeMode", groupLoopCrossfadeMode);

                if(!processAudioFiles(sampleValueTree, rootOutputDirectory, sampleSetName, sampleStart, sampleEnd, sampleLoopEnabled, sampleLoopStart, sampleLoopEnd, sampleLoopCrossfade, sampleLoopCrossfadeMode, skipAudioProcessing, overrideBitrate)) {
                    std::cerr << "A problem was encountered when processing file \"" << sampleValueTree.getProperty("path").toString() << "\". Halting conversion process." << std::endl;
                    return false;
                }
            }
        }
    return true;
}

// Go through the value tree and convert the EXS loop crossfade value which are in milliseconds to the DecentSampler sample-based format
bool DSPresetConverter::convertEXSLoopCrossfadePoints() {
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        return true;
    }
    
    int globalLoopCrossfadeMilliseconds = groupsValueTree.getProperty("loopCrossfadeMilliseconds", -1);
    groupsValueTree.removeProperty("loopCrossfadeMilliseconds", nullptr);
    
    auto updateLoopCrossfade = [this](juce::ValueTree &valueTree, int parentLoopCrossfadeMilliseconds) {
        int loopCrossfadeMilliseconds = valueTree.getProperty("loopCrossfadeMilliseconds", parentLoopCrossfadeMilliseconds);
        valueTree.removeProperty("loopCrossfadeMilliseconds", nullptr);
        if(loopCrossfadeMilliseconds == -1) {
            return true;
        }
        
        if(!valueTree.hasProperty("path")) {
            return true;
        }
        juce::String path = valueTree.getProperty("path").toString();
        
        juce::File sampleFile = juce::File(path);
        if(!sampleFile.existsAsFile()) {
            std::cerr << "Sample file \"" << path << "\" not found." << std::endl;
            return false;
        }
        
        std::unique_ptr<juce::AudioFormatReader> reader (audioFormatManager.createReaderFor (sampleFile));
        int sourceSampleRate = reader->sampleRate;
        
        valueTree.setProperty("loopCrossfade", (int) ((float) loopCrossfadeMilliseconds * 0.001 * sourceSampleRate), nullptr);
        
        return true;
    };
    
    if(!updateLoopCrossfade(groupsValueTree, 0))
        return false;
    
    // Iterate through the groups and add their samples
    for (auto groupValueTree : groupsValueTree) {
        if(!groupValueTree.hasType("group")) {
            continue;
        }
        
        int groupLoopCrossfadeMilliseconds = groupValueTree.getProperty("loopCrossfadeMilliseconds", globalLoopCrossfadeMilliseconds);
        groupValueTree.removeProperty("loopCrossfadeMilliseconds", nullptr);
        if(!updateLoopCrossfade(groupValueTree, groupLoopCrossfadeMilliseconds))
            return false;
        
        for (auto sampleValueTree : groupValueTree) {
            if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                continue;
            }
            
            int sampleLoopCrossfadeMilliseconds = sampleValueTree.getProperty("loopCrossfadeMilliseconds", groupLoopCrossfadeMilliseconds);
            groupsValueTree.removeProperty("loopCrossfadeMilliseconds", nullptr);
            
            if(!updateLoopCrossfade(sampleValueTree, sampleLoopCrossfadeMilliseconds)) {
                std::cerr << "Sample file \"" << sampleValueTree.getProperty("path").toString() << "\" not found." << std::endl;
                return false;
            }
        }
    }
        
    return true;
}

bool DSPresetConverter::convertPathsToDesiredDirectory(juce::File inputDirectory, juce::String desiredDirectoryName) {
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        return true;
    }

    desiredDirectoryName = desiredDirectoryName.replace("\\", "/");

    if(desiredDirectoryName.startsWith("/")) {
        desiredDirectoryName = desiredDirectoryName.substring(1);
    }

    if(desiredDirectoryName.endsWith("/")) {
        desiredDirectoryName = desiredDirectoryName.dropLastCharacters(1);
    }
    
    auto convertPath = [this](juce::ValueTree &valueTree, juce::String desiredDirectoryName) {
        if(!valueTree.hasProperty("path")) {
            return true;
        }
        juce::String path = valueTree.getProperty("path").toString();
        
        juce::File sampleFile = juce::File(path);
        if(!sampleFile.existsAsFile()) {
            std::cerr << "Sample file \"" << path << "\" not found." << std::endl;
            return false;
        }
        
        juce::String simpleFilePath = desiredDirectoryName + "/" + sampleFile.getFileName();
        valueTree.setProperty("path", simpleFilePath, nullptr);
        std::cout << "Sample file path changed to \"" << simpleFilePath << "\"." << std::endl;
        return true;
    };
    
    if(!convertPath(groupsValueTree, desiredDirectoryName))
        return false;
    
    // Iterate through the groups and add their samples
    for (auto groupValueTree : groupsValueTree) {
        if(!groupValueTree.hasType("group")) {
            continue;
        }
        
        if(!convertPath(groupValueTree, desiredDirectoryName))
            return false;
        
        for (auto sampleValueTree : groupValueTree) {
            if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                continue;
            }
            
            if(!convertPath(sampleValueTree, desiredDirectoryName)) {
                std::cerr << "Sample file \"" << sampleValueTree.getProperty("path").toString() << "\" not found." << std::endl;
                return false;
            }
        }
    }
        
    return true;
}

bool DSPresetConverter::convertPathsToRelative(juce::File inputDirectory) {
    juce::ValueTree groupsValueTree = valueTree.getChildWithName("groups");
    if(groupsValueTree == juce::ValueTree()) {
        return true;
    }
    
    auto convertPath = [this](juce::ValueTree &valueTree, juce::File inputDirectory) {
        if(!valueTree.hasProperty("path")) {
            return true;
        }
        juce::String path = valueTree.getProperty("path").toString();
        
        juce::File sampleFile = juce::File(path);
        if(!sampleFile.existsAsFile()) {
            std::cerr << "Sample file \"" << path << "\" not found." << std::endl;
            return false;
        }
        
        juce::String simpleFilePath = sampleFile.getRelativePathFrom(inputDirectory);
        valueTree.setProperty("path", simpleFilePath, nullptr);
        std::cout << "Sample file path changed to \"" << simpleFilePath << "\"." << std::endl;
        return true;
    };
    
    if(!convertPath(groupsValueTree, inputDirectory))
        return false;
    
    // Iterate through the groups and add their samples
    for (auto groupValueTree : groupsValueTree) {
        if(!groupValueTree.hasType("group")) {
            continue;
        }
        
        if(!convertPath(groupValueTree, inputDirectory))
            return false;
        
        for (auto sampleValueTree : groupValueTree) {
            if(!sampleValueTree.hasType(juce::Identifier("sample"))) {
                continue;
            }
            
            if(!convertPath(sampleValueTree, inputDirectory)) {
                std::cerr << "Sample file \"" << sampleValueTree.getProperty("path").toString() << "\" not found." << std::endl;
                return false;
            }
        }
    }
        
    return true;
}
