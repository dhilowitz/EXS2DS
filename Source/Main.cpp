/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "DSEXS24.h"
#include "DSPresetConverter.h"
#include <tclap/CmdLine.h>

//==============================================================================
int main (int argc, char* argv[])
{
    // Wrap everything in a try block.  Do this every time,
    // because exceptions will be thrown for problems.
    try {

        TCLAP::CmdLine cmd("A command-line utility that converts Logic Sampler (EXS) files to DecentSampler format. At this point, it handles only the most basic mappings, but it's a start.", ' ', "1.1.0");
        TCLAP::UnlabeledValueArg<std::string>  inputFileArg( "<exs-file>", "The EXS file to convert.", true, "", "exs-file"  );
        cmd.add( inputFileArg );
            
        TCLAP::UnlabeledValueArg<std::string>  outputFileArg( "<ds-preset-file>", "The Decent Sampler files to write out. WARNING: If the file already exists it will be overwritten.", true, "", "ds-preset-file"  );
        cmd.add( outputFileArg );
        
        TCLAP::UnlabeledValueArg<std::string>  sampleDirectoryArg( "[sample-directory]", "If this optional value is specified, then the output file will look for sample files in this directory.", false, "", "sample-directory"  );
        cmd.add( sampleDirectoryArg );
        
//        TCLAP::UnlabeledValueArg<std::string>  sampleDirectoryArg( "[sample-directory]", "The Decent Sampler files to write out. WARNING: If the file already exists it will be overwritten.", true, "", "ds-preset-file"  );
//        cmd.add( outputFileArg );
            
        // Parse the argv array.
        cmd.parse( argc, argv );

        juce::File inputFile = juce::File(inputFileArg.getValue());
        if(!inputFile.existsAsFile()) {
            std::cerr << "\"" << inputFileArg.getValue() << "\" is not a file." << std::endl;
            return 2;
        }
        
        juce::File outputFile = juce::File::getCurrentWorkingDirectory().getChildFile(outputFileArg.getValue());
//        juce::String sampleDir;
//        if(argc == 4) {
//            sampleDir = argv[3];
//        }
        
        DSEXS24 exs;
        exs.loadExs(inputFile);
        
        DSPresetConverter presetMaker;
        presetMaker.parseDSEXS24(exs);
        
        
        juce::String possibleSampleDirectory = sampleDirectoryArg.getValue();
        if(possibleSampleDirectory.isEmpty()) {
            possibleSampleDirectory = inputFile.getFileNameWithoutExtension();
        }
        presetMaker.huntForSamples(inputFile.getParentDirectory(), possibleSampleDirectory);
        
        presetMaker.convertEXSLoopCrossfadePoints();
        
        juce::String desiredSamplePath = sampleDirectoryArg.getValue();
        if(desiredSamplePath.isNotEmpty())
            presetMaker.convertPathsToDesiredDirectory(inputFile.getParentDirectory(), possibleSampleDirectory);
        else
            presetMaker.convertPathsToRelative(inputFile.getParentDirectory());
        
        DBG(presetMaker.getXML());

        
        if(outputFile.existsAsFile()) {
            outputFile.deleteFile();
    //        std::cerr << "File \"" << argv[1] << "\" exists already." << std::endl;
    ////        return 3;
        }
    //
        outputFile.create();
        outputFile.appendText(presetMaker.getXML());
        
    } catch (TCLAP::ArgException &e)  // catch exceptions
    { std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }

    return 0;
}
