/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DSEXS24.h"
#include "DSPresetMaker.h"

//==============================================================================
int main (int argc, char* argv[])
{

    // Check the number of parameters
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <exs-file> <ds-file> [sample-directory]" << std::endl;
        return 1;
    }

    juce::File inputFile = juce::File(argv[1]);
    if(!inputFile.existsAsFile()) {
        std::cerr << "\"" << argv[1] << "\" is not a file." << std::endl;
        return 2;
    }
    
    juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
    juce::String sampleDir;
    if(argc == 4) {
        sampleDir = argv[3];
    }
    
    DSEXS24 exs;
    exs.loadExs(inputFile);
    
    DSPresetMaker presetMaker;
    presetMaker.parseDSEXS24(exs, sampleDir, outputFile.getParentDirectory());
//    presetMaker.parseEXSValueTree(exs.getValueTree());
    DBG(presetMaker.getXML());

    
    if(outputFile.existsAsFile()) {
        outputFile.deleteFile();
//        std::cerr << "File \"" << argv[1] << "\" exists already." << std::endl;
////        return 3;
    }
//
    outputFile.create();
    outputFile.appendText(presetMaker.getXML());

    return 0;
}
