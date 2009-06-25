/*
Copyright 2007, 2008 Daniel Zerbino (zerbino@ebi.ac.uk)

    This file is part of Velvet.

    Velvet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Velvet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Velvet; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "run.h"

static void printUsage()
{
	puts("Usage:");
	puts("./velvetg directory [options]");
	puts("");
	puts("\tdirectory\t\t\t: working directory name");
	puts("");
	puts("Standard options:");
	puts("\t-cov_cutoff <floating-point>\t: removal of low coverage nodes AFTER tour bus (default: no removal)");
	puts("\t-ins_length <integer>\t\t: expected distance between two paired end reads (default: no read pairing)");
	puts("\t-read_trkg <yes|no>\t\t: tracking of short read positions in assembly (default: no tracking)");
	puts("\t-min_contig_lgth <integer>\t: minimum contig length exported to contigs.fa file (default: hash length * 2)");
	puts("\t-amos_file <yes|no>\t\t: export assembly to AMOS file (default: no export)");
	puts("\t-exp_cov <floating point>\t: expected coverage of unique regions (default: no long or paired-end read resolution)");
	puts("");
	puts("Advanced options:");
	puts("\t-ins_length2 <integer>\t\t: expected distance between two paired-end reads in the second short-read dataset (default: no read pairing)");
	puts("\t-ins_length_long <integer>\t: expected distance between two long paired-end reads (default: no read pairing)");
	puts("\t-ins_length*_sd <integer>\t: est. standard deviation of respective dataset (default: 10\% of corresponding length)");
	puts("\t\t[replace '*' by nothing, '2' or '_long' as necessary]");
	puts("\t-scaffolding <yes|no>\t\t: scaffolding of contigs used paired end information (default: on)");
	puts("\t-max_branch_length <integer>\t: maximum length in base pair of bubble (default: 100)");
	puts("\t-max_divergence <floating-point>: maximum divergence rate between two branches in a bubble (default: 0.2)");
	puts("\t-max_gap_count <integer>\t: maximum number of gaps allowed in the alignment of the two branches of a bubble (default: 3)");
	puts("\t-min_pair_count <integer>\t: minimum number of paired end connections to justify the scaffolding of two long contigs (default: 10)");
	puts("\t-max_coverage <floating point>\t: removal of high coverage nodes AFTER tour bus (default: no removal)");
	puts("\t-long_mult_cutoff <int>\t\t: minimum number of long reads required to merge contigs (default: 2)");
	puts("");
	puts("Output:");
	puts("\tdirectory/contigs.fa\t\t: fasta file of contigs longer than twice hash length");
	puts("\tdirectory/stats.txt\t\t: stats file (tab-spaced) useful for determining appropriate coverage cutoff");
	puts("\tdirectory/LastGraph\t\t: special formatted file with all the information on the final graph");
	puts("\tdirectory/velvet_asm.afg\t: (if requested) AMOS compatible assembly file");
}

int main(int argc, char **argv)
{
	ReadSet *sequences = NULL;
	RoadMapArray *rdmaps;
	PreGraph *preGraph;
	Graph *graph;
	char *directory, *graphFilename, *preGraphFilename, *seqFilename,
	    *roadmapFilename;
	double coverageCutoff = -1;
	double maxCoverageCutoff = -1;
	double expectedCoverage = -1;
	int longMultCutoff = -1;
	Coordinate minContigLength = -1;
	Coordinate minContigKmerLength;
	boolean *dubious = NULL;
	Coordinate insertLength[CATEGORIES];
	Coordinate insertLengthLong = -1;
	Coordinate std_dev[CATEGORIES];
	Coordinate std_dev_long = -1;
	short int accelerationBits = 24;
	boolean readTracking = false;
	boolean exportAssembly = false;
	FILE *file;
	int arg_index, arg_int;
	double arg_double;
	char *arg;
	Coordinate *sequenceLengths = NULL;
	Category cat;
	boolean scaffolding = true;
	int pebbleRounds = 1;

	setProgramName("velvetg");

	for (cat = 0; cat < CATEGORIES; cat++) {
		insertLength[cat] = -1;
		std_dev[cat] = -1;
	}

	// Error message
	if (argc == 1) {
		puts("velvetg - de Bruijn graph construction, error removal and repeat resolution");
		printf("Version %i.%i.%2.2i\n", VERSION_NUMBER,
		       RELEASE_NUMBER, UPDATE_NUMBER);
		puts("\nCopyright 2007, 2008 Daniel Zerbino (zerbino@ebi.ac.uk)");
		puts("This is free software; see the source for copying conditions.  There is NO");
		puts("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		printUsage();
		return 1;
	}

	if (strcmp(argv[1], "--help") == 0) {
		printUsage();
		return 0;
	}

	// Memory allocation 
	directory = argv[1];
	graphFilename = mallocOrExit(strlen(directory) + 100, char);
	preGraphFilename =
	    mallocOrExit(strlen(directory) + 100, char);
	roadmapFilename = mallocOrExit(strlen(directory) + 100, char);
	seqFilename = mallocOrExit(strlen(directory) + 100, char);
	// Argument parsing
	for (arg_index = 2; arg_index < argc; arg_index++) {
		arg = argv[arg_index++];
		if (arg_index >= argc) {
			puts("Unusual number of arguments!");
			printUsage();
			exit(1);
		}

		if (strcmp(arg, "-cov_cutoff") == 0) {
			sscanf(argv[arg_index], "%lf", &coverageCutoff);
		} else if (strcmp(arg, "-exp_cov") == 0) {
			sscanf(argv[arg_index], "%lf", &expectedCoverage);
			if (expectedCoverage > 0)
				readTracking = true;
		} else if (strcmp(arg, "-ins_length") == 0) {
			sscanf(argv[arg_index], "%li", &insertLength[0]);
			if (&insertLength[0] < 0) {
				printf("Invalid insert length: %li\n",
				       insertLength[0]);
				exit(1);
			}
		} else if (strcmp(arg, "-ins_length_sd") == 0) {
			sscanf(argv[arg_index], "%li", &std_dev[0]);
			if (&std_dev[0] < 0) {
				printf("Invalid std deviation: %li\n",
				       std_dev[0]);
				exit(1);
			}
		} else if (strncmp(arg, "-ins_length", 11) == 0
			   && strchr(arg, 'd') == NULL) {
			sscanf(arg, "-ins_length%hi", (short int *) &cat);
			if (cat < 1 || cat > CATEGORIES) {
				printf("Unknown option: %s\n", arg);
				exit(1);
			}
			sscanf(argv[arg_index], "%li",
			       &insertLength[cat - 1]);
			if (&insertLength[cat - 1] < 0) {
				printf("Invalid insert length: %li\n",
				       insertLength[cat - 1]);
				exit(1);
			}
		} else if (strncmp(arg, "-ins_length", 11) == 0) {
			sscanf(arg, "-ins_length%hi_sd",
			       (short int *) &cat);
			if (cat < 1 || cat > CATEGORIES) {
				printf("Unknown option: %s\n", arg);
				exit(1);
			}
			sscanf(argv[arg_index], "%li", &std_dev[cat - 1]);
			if (&std_dev[cat - 1] < 0) {
				printf("Invalid std deviation: %li\n",
				       std_dev[cat - 1]);
				exit(1);
			}
		} else if (strcmp(arg, "-ins_length_long") == 0) {
			sscanf(argv[arg_index], "%li", &insertLengthLong);
		} else if (strcmp(arg, "-ins_length_long_sd") == 0) {
			sscanf(argv[arg_index], "%li", &std_dev_long);
		} else if (strcmp(arg, "-read_trkg") == 0) {
			readTracking =
			    (strcmp(argv[arg_index], "yes") == 0);
		} else if (strcmp(arg, "-scaffolding") == 0) {
			scaffolding =
			    (strcmp(argv[arg_index], "yes") == 0);
		} else if (strcmp(arg, "-amos_file") == 0) {
			exportAssembly =
			    (strcmp(argv[arg_index], "yes") == 0);
		} else if (strcmp(arg, "-min_contig_lgth") == 0) {
			sscanf(argv[arg_index], "%li", &minContigLength);
		} else if (strcmp(arg, "-accel_bits") == 0) {
			sscanf(argv[arg_index], "%hi", &accelerationBits);
			if (accelerationBits < 0) {
				printf
				    ("Illegal acceleration parameter: %s\n",
				     argv[arg_index]);
				printUsage();
				return -1;
			}
		} else if (strcmp(arg, "-max_branch_length") == 0) {
			sscanf(argv[arg_index], "%i", &arg_int);
			setMaxReadLength(arg_int);
			setLocalMaxReadLength(arg_int);
		} else if (strcmp(arg, "-max_divergence") == 0) {
			sscanf(argv[arg_index], "%lf", &arg_double);
			setMaxDivergence(arg_double);
			setLocalMaxDivergence(arg_double);
		} else if (strcmp(arg, "-max_gap_count") == 0) {
			sscanf(argv[arg_index], "%i", &arg_int);
			setMaxGaps(arg_int);
			setLocalMaxGaps(arg_int);
		} else if (strcmp(arg, "-min_pair_count") == 0) {
			sscanf(argv[arg_index], "%i", &arg_int);
			setUnreliableConnectionCutoff(arg_int);
		} else if (strcmp(arg, "-max_coverage") == 0) {
			sscanf(argv[arg_index], "%lf", &maxCoverageCutoff);
		} else if (strcmp(arg, "-long_mult_cutoff") == 0) {
			sscanf(argv[arg_index], "%i", &longMultCutoff);
			setMultiplicityCutoff(longMultCutoff);
		} else if (strcmp(arg, "--help") == 0) {
			printUsage();
			return 0;	
		} else {
			printf("Unknown option: %s;\n", arg);
			printUsage();
			return 1;
		}
	}

	// Bookkeeping
	logInstructions(argc, argv, directory);

	strcpy(seqFilename, directory);
	strcat(seqFilename, "/Sequences");

	strcpy(roadmapFilename, directory);
	strcat(roadmapFilename, "/Roadmaps");

	strcpy(preGraphFilename, directory);
	strcat(preGraphFilename, "/PreGraph");

	if (!readTracking) {
		strcpy(graphFilename, directory);
		strcat(graphFilename, "/Graph");
	} else {
		strcpy(graphFilename, directory);
		strcat(graphFilename, "/Graph2");
	}

	// Graph uploading or creation
	if ((file = fopen(graphFilename, "r")) != NULL) {
		fclose(file);
		graph = importGraph(graphFilename);
	} else if ((file = fopen(preGraphFilename, "r")) != NULL) {
		fclose(file);
		sequences = importReadSet(seqFilename);
		convertSequences(sequences);
		graph =
		    importPreGraph(preGraphFilename, sequences,
				   readTracking, accelerationBits);
		sequenceLengths =
		    getSequenceLengths(sequences, getWordLength(graph));
		correctGraph(graph, sequenceLengths);
		exportGraph(graphFilename, graph, sequences->tSequences);
	} else if ((file = fopen(roadmapFilename, "r")) != NULL) {
		fclose(file);
		rdmaps = importRoadMapArray(roadmapFilename);
		preGraph = newPreGraph_pg(rdmaps, seqFilename);
		clipTips_pg(preGraph);
		exportPreGraph_pg(preGraphFilename, preGraph);
		destroyPreGraph_pg(preGraph);

		sequences = importReadSet(seqFilename);
		convertSequences(sequences);
		graph =
		    importPreGraph(preGraphFilename, sequences,
				   readTracking, accelerationBits);
		sequenceLengths =
		    getSequenceLengths(sequences, getWordLength(graph));
		correctGraph(graph, sequenceLengths);
		exportGraph(graphFilename, graph, sequences->tSequences);
	} else {
		puts("No Roadmap file to build upon! Please run velveth (see manual)");
		exit(1);
	}

	// Set insert lengths and their standard deviations
	for (cat = 0; cat < CATEGORIES; cat++) {
		if (insertLength[cat] > -1 && std_dev[cat] < 0)
			std_dev[cat] = insertLength[cat] / 10;
		setInsertLengths(graph, cat,
				 insertLength[cat], std_dev[cat]);
	}

	if (insertLengthLong > -1 && std_dev_long < 0)
		std_dev_long = insertLengthLong / 10;
	setInsertLengths(graph, CATEGORIES,
			 insertLengthLong, std_dev_long);

	// Coverage cutoff
	if (coverageCutoff < 0) {
		puts("WARNING: NO COVERAGE CUTOFF PROVIDED");
		puts("Velvet will probably leave behind many detectable errors");
		puts("See manual for instructions on how to set the coverage cutoff parameter");
	}

	dubious =
	    removeLowCoverageNodesAndDenounceDubiousReads(graph,
							  coverageCutoff);
	removeHighCoverageNodes(graph, maxCoverageCutoff);
	clipTipsHard(graph);

	if (expectedCoverage > 0) {
		if (sequences == NULL) {
			sequences = importReadSet(seqFilename);
			convertSequences(sequences);
		}
		// Mixed length sequencing
		readCoherentGraph(graph, isUniqueSolexa, expectedCoverage,
				  sequences);

		// Paired ends module
		for (cat = 0; cat < CATEGORIES; cat++) {
			if (insertLength[cat] > -1) {
				pairUpReads(sequences, 2 * cat + 1);
				pebbleRounds++;
			}
		}

		if (insertLengthLong > -1) {
			pairUpReads(sequences, 2 * CATEGORIES + 1);
			pebbleRounds++;
		}

		detachDubiousReads(sequences, dubious);
		activateGapMarkers(graph);
		for ( ;pebbleRounds > 0; pebbleRounds--)
			exploitShortReadPairs(graph, sequences, dubious, scaffolding);
	} else {
		puts("WARNING: NO EXPECTED COVERAGE PROVIDED");
		puts("Velvet will be unable to resolve any repeats");
		puts("See manual for instructions on how to set the expected coverage parameter");
	}

	free(dubious);

	concatenateGraph(graph);

	if (minContigLength < 2 * getWordLength(graph))
		minContigKmerLength = getWordLength(graph);
	else
		minContigKmerLength = minContigLength - getWordLength(graph) + 1;		

	strcpy(graphFilename, directory);
	strcat(graphFilename, "/contigs.fa");
	exportLongNodeSequences(graphFilename, graph, minContigKmerLength); 

	strcpy(graphFilename, directory);
	strcat(graphFilename, "/stats.txt");
	displayGeneralStatistics(graph, graphFilename);

	if (sequences == NULL) {
		sequences = importReadSet(seqFilename);
		convertSequences(sequences);
	}

	strcpy(graphFilename, directory);
	strcat(graphFilename, "/LastGraph");
	exportGraph(graphFilename, graph, sequences->tSequences);

	if (exportAssembly) {
		strcpy(graphFilename, directory);
		strcat(graphFilename, "/velvet_asm.afg");
		exportAMOSContigs(graphFilename, graph, minContigKmerLength, sequences);
	}

	logFinalStats(graph, minContigKmerLength, directory);

	destroyGraph(graph);
	free(graphFilename);
	free(preGraphFilename);
	free(seqFilename);
	free(roadmapFilename);
	destroyReadSet(sequences);
	return 0;
}
