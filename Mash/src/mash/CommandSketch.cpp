// Copyright © 2015, Battelle National Biodefense Institute (BNBI);
// all rights reserved. Authored by: Brian Ondov, Todd Treangen,
// Sergey Koren, and Adam Phillippy
//
// See the LICENSE.txt file included with this software for license information.

#include "CommandSketch.h"
#include "Sketch.h"
#include "sketchParameterSetup.h"
#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace mash {

CommandSketch::CommandSketch()
: Command()
{
    name = "sketch";
    summary = "Create sketches (reduced representations for fast operations).";
    description = "Create a sketch file, which is a reduced representation of a sequence or set of sequences (based on min-hashes) that can be used for fast distance estimations. Inputs can be fasta or fastq files (gzipped or not), and \"-\" can be given to read from standard input. Input files can also be files of file names (see -l). For output, one sketch file will be generated, but it can have multiple sketches within it, divided by sequences or files (see -i). By default, the output file name will be the first input file with a '.msh' extension, or 'stdin.msh' if standard input is used (see -o).";
    argumentString = "<input> [<input>] ...";
    
    useOption("help");
    addOption("list", Option(Option::Boolean, "l", "Input", "List input. Lines in each <input> specify paths to sequence files, one per line.", ""));
    addOption("prefix", Option(Option::File, "o", "Output", "Output prefix (first input file used if unspecified). The suffix '.msh' will be appended.", ""));
    addOption("id", Option(Option::File, "I", "Sketch", "ID field for sketch of reads (instead of first sequence ID).", ""));
    addOption("comment", Option(Option::File, "C", "Sketch", "Comment for a sketch of reads (instead of first sequence comment).", ""));
    useSketchOptions();
}

int CommandSketch::run() const
{
    if ( arguments.size() == 0 || options.at("help").active )
    {
        print();
        return 0;
    }
    
    int verbosity = 1;//options.at("silent").active ? 0 : options.at("verbose").active ? 2 : 1;
    bool list = options.at("list").active;
    
    Sketch::Parameters parameters;
    
    if ( sketchParameterSetup(parameters, *(Command *)this) )
    {
    	return 1;
    }
    
    for ( int i = 0; i < arguments.size(); i++ )
    {
        if ( false && hasSuffix(arguments[i], suffixSketch) )
        {
            cerr << "ERROR: " << arguments[i] << " looks like it is already sketched." << endl;
            exit(1);
        }
    }
    
    Sketch sketch;
    
    uint64_t lengthMax;
    double randomChance;
    int kMin;
    string lengthMaxName;
    int warningCount = 0;
    
    vector<string> files;
    
    for ( int i = 0; i < arguments.size(); i++ )
    {
        if ( list )
        {
            splitFile(arguments[i], files);
        }
        else
        {
            files.push_back(arguments[i]);
        }
    }
    
    if ( getOption("id").active || getOption("comment").active )
    {
    	if ( files.size() > 1 && ! parameters.reads )
    	{
    		cerr << "WARNING: -I and -C will only apply to first sketch" << endl;
    	}
    }
    
    if ( parameters.reads )
    {
    	sketch.initFromReads(files, parameters);
    }
    else
    {
	    sketch.initFromFiles(files, parameters, verbosity);
	}
	
	if ( getOption("id").active )
	{
		sketch.setReferenceName(0, getOption("id").argument);
	}
    
	if ( getOption("comment").active )
	{
		sketch.setReferenceComment(0, getOption("comment").argument);
	}
    
    double lengthThreshold = (parameters.warning * sketch.getKmerSpace()) / (1. - parameters.warning);
    
	for ( int i = 0; i < sketch.getReferenceCount(); i++ )
	{
		uint64_t length = sketch.getReference(i).length;
		
		if ( length > lengthThreshold )
		{
			if ( warningCount == 0 || length > lengthMax )
			{
				lengthMax = length;
				lengthMaxName = sketch.getReference(i).name;
				randomChance = sketch.getRandomKmerChance(i);
				kMin = sketch.getMinKmerSize(i);
			}
			
			warningCount++;
		}
	}
	
    string prefix;
    
    if ( options.at("prefix").argument.length() > 0 )
    {
        prefix = options.at("prefix").argument;
    }
    else
    {
        if ( arguments[0] == "-" )
        {
            prefix = "stdin";
        }
        else
        {
            prefix = arguments[0];
        }
    }
    
    string suffix = parameters.windowed ? suffixSketchWindowed : suffixSketch;
    
    if ( ! hasSuffix(prefix, suffix) )
    {
        prefix += suffix;
    }
    
//    cerr << "Writing to " << prefix << "..." << endl;
    
    sketch.writeToCapnp(prefix.c_str());
    
    if ( warningCount > 0 && ! parameters.reads )
    {
    	warnKmerSize(parameters, *this, lengthMax, lengthMaxName, randomChance, kMin, warningCount);
    }
    
    return 0;
}

int CommandSketch::run(std::string file_name) const
{
    
    int verbosity = 1;//options.at("silent").active ? 0 : options.at("verbose").active ? 2 : 1;
    bool list = options.at("list").active;
    
    Sketch::Parameters parameters;
    
    if ( sketchParameterSetup(parameters, *(Command *)this) )
    {
        return 1;
    }
    
    if ( false && hasSuffix(file_name.c_str(), suffixSketch) )
    {
        cerr << "ERROR: " << file_name << " looks like it is already sketched." << endl;
        exit(1);
    }
    
       
    Sketch sketch;
    uint64_t lengthMax;
    double randomChance;
    int kMin;
    string lengthMaxName;
    int warningCount = 0;
    
    vector<string> files;
    files.push_back(file_name);

    
    if ( getOption("id").active || getOption("comment").active )
    {
        if ( files.size() > 1 && ! parameters.reads )
        {
            cerr << "WARNING: -I and -C will only apply to first sketch" << endl;
        }
    }
    
    if ( parameters.reads )
    {
        sketch.initFromReads(files, parameters);
    }
    else
    {
        sketch.initFromFiles(files, parameters, verbosity);
    }
    
    if ( getOption("id").active )
    {
        sketch.setReferenceName(0, getOption("id").argument);
    }
    
    if ( getOption("comment").active )
    {
        sketch.setReferenceComment(0, getOption("comment").argument);
    }
    
    double lengthThreshold = (parameters.warning * sketch.getKmerSpace()) / (1. - parameters.warning);
    
    for ( int i = 0; i < sketch.getReferenceCount(); i++ )
    {
        uint64_t length = sketch.getReference(i).length;
        
        if ( length > lengthThreshold )
        {
            if ( warningCount == 0 || length > lengthMax )
            {
                lengthMax = length;
                lengthMaxName = sketch.getReference(i).name;
                randomChance = sketch.getRandomKmerChance(i);
                kMin = sketch.getMinKmerSize(i);
            }
            
            warningCount++;
        }
    }
    
    string prefix;
    
    prefix = "stdin";
    string suffix = parameters.windowed ? suffixSketchWindowed : suffixSketch;
    
    if ( ! hasSuffix(prefix, suffix) )
    {
        prefix += suffix;
    }
    
    string output_name = file_name + suffix;
//    cerr << "Writing to " << prefix << "..." << endl;
    
    sketch.writeToCapnp(output_name.c_str());
    
    if ( warningCount > 0 && ! parameters.reads )
    {
        warnKmerSize(parameters, *this, lengthMax, lengthMaxName, randomChance, kMin, warningCount);
    }
    
    return 0;
}

} // namespace mash

