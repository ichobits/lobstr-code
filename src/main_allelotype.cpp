/*
Copyright (C) 2011-2014 Melissa Gymrek <mgymrek@mit.edu>

This file is part of lobSTR.

lobSTR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

lobSTR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with lobSTR.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <err.h>
#include <getopt.h>
#include <stdlib.h>

#include <boost/algorithm/string.hpp>
#include <ctime>
#include <iostream>
#include <string>

#include "src/common.h"
#include "src/FastaFileReader.h"
#include "src/Genotyper.h"
#include "src/MSReadRecord.h"
#include "src/NoiseModel.h"
#include "src/ReadContainer.h"
#include "src/ReferenceSTR.h"
#include "src/STRIntervalTree.h"
#include "src/runtime_parameters.h"

using namespace std;

// Number of N's used to pad each reference
const int PAD = 50;
// Keep track of reference nucleotide for each STR
map<pair<string,int>, string> ref_nucleotides;
// Keep track of reference repseq for each STR
map<pair<string,int>, string> ref_repseq;
// Keep track of extended region around each STR
map<pair<string,int>, string> ref_ext_nucleotides;
// Keep track of reference STRs
vector<ReferenceSTR> reference_strs;

void LoadReference();

void show_help() {
  std::stringstream help_msg;
  help_msg << "\n" << "To train the genotyping noise model from a set of aligned reads:\n"
	   << "allelotype --command train [OPTIONS] --bam <input.bam> "	
	   << "--noise_model <noisemodelprefix> --strinfo <strinfo.tab> "
	   << "--index-prefix $PATH_TO_INDEX/lobSTR_ "
	   << " --haploid chrY\n\n"
	   << "Training outputs model files: \n"
	   << "   <noisemodelprefix>.stepmodel\n"
	   << "   <noisemodelprefix>.stuttermodel\n\n"
	   << "To run str profiling on a set of aligned reads:\n"
	   << "allelotype --command classify [OPTIONS] --bam <input.bam> "
	   << "--noise_model <noisemodelprefix> [--no-rmdup] --strinfo <strinfo.tab> "
	   << "--out <output_prefix> --index-prefix $PATH_TO_INDEX/lobSTR_\n\n"
	   << "Classifying outputs the files: \n"
	   << "   <output_prefix>.vcf \n"
	   << "   <output_prefix>.allelotype.stats \n\n"
	   << "Parameter description:\n"
	   << "--command [train|classify]:     (REQUIRED) specify which of the tasks\n"
	   << "                                described above to perform\n"
	   << "--bam <f1.bam,[f2.bam, ...]>:   (REQUIRED) comma-separated list\n"
	   << "                                of bam files to analyze. Each sample should have\n"
	   << "                                a unique read group.\n"
	   << "--out <STRING>:                 Prefix to name output files.\n"
	   << "--strinfo <strinfo.tab>:        (REQUIRED)\n"
	   << "                                File containing statistics for each STR,\n"
	   << "                                available in the data/ directory of the\n"
	   << "                                lobSTR download.\n"
	   << "--noise_model <STRING>:         (REQUIRED)\n"
	   << "                                prefix of files to write (--command train)\n"
	   << "                                or read (--command classify) noise model\n"
	   << "                                parameters to.\n"
	   << "                                An example is $PATH_TO_LOBSTR/models/illumina2\n"
	   << "--index-prefix <STRING>:        (REQUIRED) prefix for lobSTR's bwa reference\n"
	   << "                                (must be same as for lobSTR alignment)\n"
	   << "--no-rmdup:                     don't remove pcr duplicates before allelotyping.\n"
     << "--realign:                      Redo local realignment. Useful if using alignments generated\n"
     << "                                by other tools.\n"
     << "--min-het-freq <FLOAT>:         minimum frequency to make a heterozygous call. (Default: " << min_het_freq << ")\n"
	   << "--haploid <chrX,[chrY,...]>:    comma-separated list of chromosomes\n"
	   << "                                that should be forced to have homozygous\n"
	   << "                                calls. Specify --haploid all if the organism\n"
	   << "                                is haploid. Will be applied to all samples.\n"
     << "--regions <STRING>:             Restrict allelotyping to STRs falling in regions\n"
     << "                                specified by this bed file.\n"
	   << "-h,--help:                      display this message\n"
	   << "-v,--verbose:                   print out helpful progress messages\n"
	   << "--quiet                         don't print anything to stderr or stdout\n"
	   << "--version:                      print out allelotype program version number\n\n"
	   << "Options for calculating and reporting allelotypes:\n"
     << "--gridk                         Search genotype grid including all observed alleles +/- kbp (Default: " << gridk <<")\n"
	   << "--include-gl                    Include the GL field in the VCF file (default = false)\n"
     << "--dont-include-pl               Do not print the PL field in the VCF file (default = false). Makes VCFs smaller.\n\n"
	   << "Default options for filtering reads:\n"
	   << "--min-border <INT>:             Filter reads that do not extend past both ends of the STR region\n"
	   << "                                by at least <INT> bp. Default: " << min_border << "\n"
	   << "                                To include partially spanning reads, specify a large negative number.\n"
	   << "--mapq <INT>:                   Filter reads with mapq scores of more than\n"
	   << "                                <INT>. Default: " << max_mapq << "\n"
	   << "--max-matedist <INT>:           Filter reads with a mate distance larger than <INT> bp. Default: " << max_matedist << "\n"
     << "--max-diff-ref <INT>:           Only use reads differing by at most this number of bp from the \n"
     << "                                reference allele. Default: " << max_diff_ref << "\n"
	   << "--min-bp-before-indel <INT>:    Filter reads with an indel occurring less than <INT> bases\n"
	   << "                                from either end of the read.  Default: " << min_bp_before_indel << "\n"  
	   << "--min-read-end-match <INT>:     Filter reads whose alignments don't exactly match the reference for at least\n"
	   << "                                <INT> bp at both ends. Default: " << min_read_end_match << "\n"
	   << "--maximal-end-match <INT>:      Filter reads whose prefix/suffix matches to reference are <= those \n"
	   << "                                obtained when shifting the read ends by distances within <INT> bp. Default: " << maximal_end_match_window << "\n\n"
	   << "Optional arguments for filtering reads:\n"
	   << "--chrom <STRING>:               only look at reads from this chromosome\n"
	   << "--unit:                         filter reads differing by a non-integer\n"
	   << "                                number of repeat copies from reference\n"
	   << "--max-repeats-in-ends <INT>:    Filter reads with more than <INT> occurrences of the repeat motif in \n"
	   << "                                the 4*period bp on either end of the read. -1 means no filter is applied.\n"
     << "                                Default: " << max_repeats_in_ends << "\n"
     << "--filter-mapq0:                 Filter reads with map quality 0. Only use for alignments not \n"
	   << "                                generated by lobSTR\n"
	   << "--filter-clipped:               Filter reads with hard or soft clipped bases at the ends\n"
	   << "--filter-reads-with-n:          Filter reads that have one or more N bases\n\n"
	   << "Additional options\n"
     << "--report-nocalls:               Report all records that were covered by at least 1 read, even if no calls\n"
     << "--output-bams:                  Output BAM files:\n"
     << "                                   <out>.reads.bam: contains all reads used for analysis (before collapsing duplicates)\n"
     << "                                   <out>.filtered.bam: contains all reads removed by filters\n"
     << "                                where <out> is the argument to --out\n"
     << "--chunksize <INT>:              Number of loci to read into memory at a time (default: 1000)\n";
  cerr << help_msg.str();
  exit(1);
}

/*
 * parse the command line options
 */
void parse_commandline_options(int argc, char* argv[]) {
  enum LONG_OPTIONS {
    OPT_ANNOTATION,
    OPT_BAM,
    OPT_CHROM,
    OPT_CHUNKSIZE,
    OPT_COMMAND,
    OPT_DEBUG,
    OPT_DONT_INCLUDE_PL,
    OPT_FILTER_CLIPPED,
    OPT_FILTER_MAPQ0,
    OPT_FILTER_READS_WITH_N,
    OPT_GRIDK,
    OPT_HAPLOID,
    OPT_HELP,
    OPT_INCLUDE_GL,
    OPT_INDEX,
    OPT_MAX_DIFF_REF,
    OPT_MAX_REPEATS_IN_ENDS,
    OPT_MAXMAPQ,
    OPT_MAXMATEDIST,
    OPT_MAXIMAL_END_MATCH_WINDOW,
    OPT_MIN_BORDER,
    OPT_MIN_BP_BEFORE_INDEL,
    OPT_MIN_HET_FREQ,
    OPT_MIN_READ_END_MATCH,
    OPT_NOISEMODEL,
    OPT_NORMDUP,
    OPT_NOWEB,
    OPT_OUTPUT,
    OPT_OUTPUT_BAMS,
    OPT_PRINT_READS,
    OPT_QUIET,
    OPT_REALIGN,
    OPT_REGIONS,
    OPT_REPORT_NOCALLS,
    OPT_STRINFO,
    OPT_UNIT,
    OPT_VERBOSE,
    OPT_VERSION
  };

  int ch;
  int option_index = 0;

  static struct option long_options[] = {
    {"annotation", 1, 0, OPT_ANNOTATION},
    {"bam", 1, 0, OPT_BAM},
    {"chrom", 1, 0, OPT_CHROM},
    {"chunksize", 1, 0, OPT_CHUNKSIZE},
    {"command", 1, 0, OPT_COMMAND},
    {"debug", 0, 0, OPT_DEBUG},
    {"dont-include-pl", 0, 0, OPT_DONT_INCLUDE_PL},
    {"filter-clipped", 0, 0, OPT_FILTER_CLIPPED},
    {"filter-mapq0", 0, 0, OPT_FILTER_MAPQ0},
    {"filter-reads-with-n", 0, 0, OPT_FILTER_READS_WITH_N},
    {"gridk", 1, 0, OPT_GRIDK},
    {"haploid", 1, 0, OPT_HAPLOID},
    {"help", 0, 0, OPT_HELP},
    {"index-prefix", 1, 0, OPT_INDEX},
    {"max-diff-ref", 1, 0, OPT_MAX_DIFF_REF},
    {"mapq", 1, 0, OPT_MAXMAPQ},
    {"max-matedist", 1, 0, OPT_MAXMATEDIST},
    {"max-repeats-in-ends", 1, 0, OPT_MAX_REPEATS_IN_ENDS},
    {"maximal-end-match", 1, 0, OPT_MAXIMAL_END_MATCH_WINDOW},
    {"min-border", 1, 0, OPT_MIN_BORDER},
    {"min-bp-before-indel", 1, 0, OPT_MIN_BP_BEFORE_INDEL},
    {"min-het-freq", 1, 0, OPT_MIN_HET_FREQ},
    {"min-read-end-match", 1, 0, OPT_MIN_READ_END_MATCH},
    {"noise_model", 1, 0, OPT_NOISEMODEL},
    {"no-rmdup", 0, 0, OPT_NORMDUP},
    {"noweb", 0, 0, OPT_NOWEB},
    {"out", 1, 0, OPT_OUTPUT},
    {"output-bams", 0, 0, OPT_OUTPUT_BAMS},
    {"quiet", 0, 0, OPT_QUIET},
    {"reads", 0, 0, OPT_PRINT_READS},
    {"realign", 0, 0, OPT_REALIGN},
    {"regions", 1, 0, OPT_REGIONS},
    {"report-nocalls", 0, 0, OPT_REPORT_NOCALLS},
    {"strinfo", 1, 0, OPT_STRINFO},
    {"unit", 0, 0, OPT_UNIT},
    {"verbose", 0, 0, OPT_VERBOSE},
    {"version", 0, 0, OPT_VERSION},
    {"include-gl", 0, 0, OPT_INCLUDE_GL},
    {NULL, no_argument, NULL, 0},
  };
  program = ALLELOTYPE;
  ch = getopt_long(argc, argv, "hv?",
                   long_options, &option_index);
  while (ch != -1) {
    switch (ch) {
    case OPT_ANNOTATION:
      annotation_files_string = string(optarg);
      AddOption("annotation", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_BAM:
      bam_files_string = string(optarg);
      AddOption("bam", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_CHROM:
      use_chrom = string(optarg);
      AddOption("chrom", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_CHUNKSIZE:
      CHUNKSIZE = atoi(optarg);
      AddOption("chunksize", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_COMMAND:
      command = string(optarg);
      AddOption("command", string(optarg), true, &user_defined_arguments_allelotyper);
      if ((command != "train") & (command != "classify")) {
        PrintMessageDieOnError("Command " + command + " is invalid. Must be train or classify", ERROR);
        exit(1);
      }
      break;
    case OPT_DEBUG:
      debug = true;
      break;
    case OPT_DONT_INCLUDE_PL:
      dont_include_pl = true;
      AddOption("dont-includ-pl", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_FILTER_CLIPPED:
      filter_clipped = true;
      AddOption("filter-clipped", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_FILTER_MAPQ0:
      filter_mapq0 = true;
      AddOption("filter-mapq0", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_FILTER_READS_WITH_N:
      filter_reads_with_n = true;
      AddOption("filter-reads-with-n", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_GRIDK:
      gridk = atoi(optarg);
      AddOption("gridk", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_HAPLOID:
      haploid_chroms_string = string(optarg);
      AddOption("haploid", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case 'h':
    case OPT_HELP:
      show_help();
      exit(1);
      break;
    case OPT_INCLUDE_GL:
      include_gl = true;
      AddOption("include-gl", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_INDEX:
      index_prefix = string(optarg);
      AddOption("index-prefix", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MAX_DIFF_REF:
      max_diff_ref = atoi(optarg);
      AddOption("max-diff-ref", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MAXIMAL_END_MATCH_WINDOW:
      maximal_end_match_window = atoi(optarg);
      AddOption("maximal-end-match", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MAXMAPQ:
      max_mapq = atoi(optarg);
      AddOption("mapq", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MAXMATEDIST:
      max_matedist = atoi(optarg);
      AddOption("max-matedist", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MAX_REPEATS_IN_ENDS:
      max_repeats_in_ends = atoi(optarg);
      AddOption("max-repeats-in-ends", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MIN_BORDER:
      min_border = atoi(optarg);
      AddOption("min-border", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MIN_BP_BEFORE_INDEL:
      min_bp_before_indel = atoi(optarg);
      AddOption("min-bp-before-indel", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MIN_HET_FREQ:
      min_het_freq = atof(optarg);
      AddOption("min-het-freq", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_MIN_READ_END_MATCH:
      min_read_end_match = atoi(optarg);
      AddOption("min-read-end-match", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_NOISEMODEL:
      noise_model = string(optarg);
      if ((command == "train") &&
          (fexists((noise_model+".stepmodel").c_str()) || (fexists((noise_model+".stuttermodel").c_str())))) {
        PrintMessageDieOnError("Cannot write to specified noise model file. " \
                               "This file already exists.", ERROR);
      }
      AddOption("noise-model", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_NORMDUP:
      rmdup = false;
      AddOption("normdup", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_NOWEB: // Deprecated, but left in since it is often used. Always set to true
      noweb = true;
      AddOption("noweb", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_OUTPUT:
      output_prefix = string(optarg);
      AddOption("out", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_OUTPUT_BAMS:
      output_bams = true;
      AddOption("output-bams", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_PRINT_READS:
      print_reads = true;
      AddOption("print-reads", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_QUIET:
      quiet = true;
      break;
    case OPT_REALIGN:
      realign = true;
      AddOption("realign", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_REGIONS:
      regions_file = string(optarg);
      AddOption("regions", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_REPORT_NOCALLS:
      report_nocalls = true;
      AddOption("report-nocalls", "", false, &user_defined_arguments_allelotyper);
      break;
    case OPT_STRINFO:
      strinfofile = string(optarg);
      AddOption("strinfo", string(optarg), true, &user_defined_arguments_allelotyper);
      break;
    case OPT_UNIT:
      unit = true;
      AddOption("unit", "", false, &user_defined_arguments_allelotyper);
      break;
    case 'v':
    case OPT_VERBOSE:
      my_verbose = true;
      break;
    case OPT_VERSION:
      cerr << _GIT_VERSION << endl;
      exit(0);
      break;
    case '?':
      show_help();
      exit(1);
    default:
      show_help();
      exit(1);
    }
    ch = getopt_long(argc, argv, "hv?",
                     long_options, &option_index);
  }

  // any arguments left over are extra
  if (optind < argc) {
    PrintMessageDieOnError("Unnecessary leftover arguments", ERROR);
  }
  if (command.empty()) {
    PrintMessageDieOnError("Must specify a command", ERROR);
  }
  if (command == "train") {
    if (bam_files_string.empty() || noise_model.empty()) {
      PrintMessageDieOnError("Please specify a bam file and an output prefix", ERROR);
    }
  }
  if (command == "classify") {
    if (bam_files_string.empty() || noise_model.empty()
        || output_prefix.empty()) {
      PrintMessageDieOnError("Please specify a bam file, output prefix, and noise model", ERROR);
    }
  }
  // check that parameters make sense
  if ((command == "train") &&
      (haploid_chroms_string.empty())) {
    PrintMessageDieOnError("Must specify which loci are hemizygous using --haploid", ERROR);
  }
  if (strinfofile.empty()) {
    PrintMessageDieOnError("Must specify --strinfo", ERROR);
  }
  if (index_prefix.empty()) {
    PrintMessageDieOnError("Must specify --index-prefix", ERROR);
  }
}

void LoadReference() {
  map<string, STRIntervalTree> chrom_to_region_trees;
  if (!regions_file.empty()) {
    if (my_verbose) {
      PrintMessageDieOnError("Loading regions file", PROGRESS);
    }
    map<string, vector<ReferenceSTR> > chrom_to_regions;
    TextFileReader tReader(regions_file);
    string line;
    while (tReader.GetNextLine(&line)) {
      vector<string>items(0);
      split(line, '\t', items);
      if (items.size() != 3) {
        PrintMessageDieOnError("Malformed regions file. Should have "
                               "chrom, start, end and be tab-separated", ERROR);
      }
      ReferenceSTR reg;
      const string chrom = items.at(0);
      const int start = atoi(items.at(1).c_str());
      const int stop = atoi(items.at(2).c_str());
      reg.chrom = chrom;
      reg.start = start;
      reg.stop = stop;
      if (chrom_to_regions.find(chrom) ==
          chrom_to_regions.end()) {
        vector<ReferenceSTR> regions;
        chrom_to_regions[chrom] = regions;
      }
      chrom_to_regions[chrom].push_back(reg);
    }
    for (map<string, vector<ReferenceSTR> >::const_iterator it = chrom_to_regions.begin();
         it != chrom_to_regions.end(); it++) {
      STRIntervalTree itree;
      itree.LoadIntervals(chrom_to_regions[it->first]);
      chrom_to_region_trees[it->first] = itree;
    }
  }
  if (my_verbose) {
    PrintMessageDieOnError("Loading reference STRs", PROGRESS);
  }
  // Load map of refid->loci, locus->repseq
  map<int, vector<ReferenceSTR> > refid_to_refstrs;
  TextFileReader tReader(index_prefix+"ref_map.tab");
  string line;
  while (tReader.GetNextLine(&line)) {
    vector<string>items(0);
    split(line, '\t', items);
    if (items.size() != 3) {
      PrintMessageDieOnError("Malformed map file", ERROR);
    }
    // Get refid
    int refid = atoi(items.at(0).c_str());
    vector<ReferenceSTR> refs;
    refid_to_refstrs[refid] = refs;
    // Get reference STRs
    string regions_string = items.at(2);
    vector<string>regions(0);
    split(regions_string, ';', regions);
    for (size_t i = 0; i < regions.size(); i++) {
      vector<string>region_items(0);
      split(regions.at(i), '_', region_items);
      int start = atoi(region_items.at(0).c_str());
      int stop = atoi(region_items.at(1).c_str());
      string motif = region_items.at(2);
      ReferenceSTR ref_str;
      ref_str.start = start;
      ref_str.stop = stop;
      ref_str.motif = motif;
      refid_to_refstrs[refid].push_back(ref_str);
    }
  }

  // Load nucs for each locus
  FastaFileReader faReader(index_prefix+"ref.fasta");
  MSReadRecord ref_record;
  while (faReader.GetNextRead(&ref_record)) {
    vector<string> items;
    string refstring = ref_record.ID;
    split(refstring, '$', items);
    if (items.size() == 4) {
      string chrom = items.at(1);
      int refid = atoi(items.at(0).c_str());
      int start = atoi(items.at(2).c_str());
      string nucs = ref_record.nucleotides;
      if (use_chrom.empty() || use_chrom == chrom) {
        // Get nucs for each locus at this refid
        for (vector<ReferenceSTR>::iterator it = refid_to_refstrs[refid].begin();
             it != refid_to_refstrs[refid].end(); it++) {
          it->chrom = chrom; // Set chrom, wasn't set above
          if (!regions_file.empty()) {
            if (chrom_to_region_trees.find(chrom) == chrom_to_region_trees.end()) {
              continue;
            }
            vector<ReferenceSTR> containing_regions;
            if (!chrom_to_region_trees[chrom].GetContainingRegions(it->start, it->stop, &containing_regions)) {
              continue;
            }
            if (containing_regions.size() == 0) {
              continue;
            }
          }
          pair<string, int> locus = it->GetLocus();
          int strlen = it->stop-it->start+1;
          // Get STR sequence
          ref_nucleotides[locus] = nucs.substr(it->start-start+PAD-1, strlen);
          // Get extended ref sequence
          ref_ext_nucleotides[locus] = nucs.substr(it->start-start+PAD-extend, 2*extend + strlen);
          // Add to reference STRs
          reference_strs.push_back(*it);
          // Get ref repseq
          ref_repseq[locus] = it->motif;
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
  time_t starttime, endtime;
  time(&starttime);
  /* parse command line options */
  parse_commandline_options(argc, argv);
  if (!quiet) PrintLobSTR();
  PrintMessageDieOnError("Getting run info", PROGRESS);
  run_info.Reset();
  run_info.starttime = GetTime();
  if (_GIT_VERSION != NULL) {
    run_info.gitversion = _GIT_VERSION;
  } else {run_info.gitversion = "Not available";}
  if (_MACHTYPE != NULL) {
    run_info.machtype = _MACHTYPE;
  } else {run_info.machtype = "Not available";}
  run_info.params = user_defined_arguments_allelotyper;

  if (my_verbose) {
    PrintMessageDieOnError("Running allelotype with command " + command, PROGRESS);
  }
  /* Get haploid chromosomes */
  vector<string> haploid_chroms;
  boost::split(haploid_chroms,haploid_chroms_string,boost::is_any_of(","));

  /* Load ref character and ref object for each STR */
  CheckIndexVersion();
  LoadReference();

  /* initialize noise model */
  NoiseModel nm(strinfofile, haploid_chroms, noise_model);

  /* Get list of bam files */
  vector<string>bam_files;
  boost::split(bam_files, bam_files_string, boost::is_any_of(","));

  /* Train/classify */
  ReferenceSTRContainer ref_str_container(reference_strs);
  if (command == "train") {
    ReadContainer read_container(bam_files);
    ReferenceSTR dummy_ref_str;
    vector<ReferenceSTR> haploid_str_chunk;
    for (size_t i = 0; i < haploid_chroms.size(); i++) {
      if (!ref_str_container.GetChromChunk(&haploid_str_chunk, haploid_chroms.at(i))) {
        stringstream msg;
        msg << "No reference STRs found for haploid chromosome " << haploid_chroms.at(i);
        PrintMessageDieOnError(msg.str(), WARNING);
      }
    }
    if (haploid_str_chunk.size() == 0) {
      PrintMessageDieOnError("No haploid loci found for training", ERROR);
    }
    dummy_ref_str.chrom = "NA"; dummy_ref_str.start = -1; dummy_ref_str.stop = -1;
    read_container.AddReadsFromFile(dummy_ref_str, haploid_str_chunk,
				    ref_ext_nucleotides, haploid_chroms);
    if (my_verbose) PrintMessageDieOnError("Training noise model", PROGRESS);
    nm.Train(&read_container);
  } 
  else if (command == "classify") {
    if (!nm.ReadNoiseModelFromFile())
      PrintMessageDieOnError("Problem reading noise file", ERROR);
  }
  if (command == "classify") {
    // Initialize read container
    ReadContainer str_container(bam_files);
    // Initialize genotyper
    Genotyper genotyper(&nm, haploid_chroms, &ref_nucleotides, &ref_repseq,
			output_prefix + ".vcf", str_container.samples_list, str_container.rg_id_to_sample);
    // Load annotations
    if (!annotation_files_string.empty()) {
      vector<string>annotation_files;
      boost::split(annotation_files, annotation_files_string, boost::is_any_of(","));
      if (my_verbose) {
        PrintMessageDieOnError("Loading annotations", PROGRESS);
      }
      genotyper.LoadAnnotations(annotation_files);
    }
    // Classify allelotypes
    if (my_verbose) PrintMessageDieOnError("Classifying allelotypes", PROGRESS);
    std::string current_chrom;
    // Read one chunk of refs at a time
    vector<ReferenceSTR> ref_str_chunk;
    string chrom; int begin, end;
    string prev_chrom = "NA"; int prev_begin = -1; // Keep track to avoid processing loci with duplicate entries
    while (ref_str_container.GetNextChunk(&ref_str_chunk, &chrom, &begin, &end)) {
      ReferenceSTR ref_region;
      ref_region.chrom = chrom;
      ref_region.start = begin;
      ref_region.stop = end;
      if (use_chrom.empty() || (use_chrom == chrom)) {
        str_container.AddReadsFromFile(ref_region, ref_str_chunk,
                                       ref_ext_nucleotides, vector<string>(0));
        for (size_t i = 0; i < ref_str_chunk.size(); i++) {
          // Check that we don't process the same locus twice
          if (!(ref_str_chunk.at(i).chrom==prev_chrom && ref_str_chunk.at(i).start==prev_begin)) {
            pair<string, int> coord(ref_str_chunk.at(i).chrom, ref_str_chunk.at(i).start);
            if (debug) {
              stringstream msg;
              msg << "Processing " << ref_str_chunk.at(i).chrom << ":" << ref_str_chunk.at(i).start;
              PrintMessageDieOnError(msg.str(), DEBUG);
            }
            list<AlignedRead> aligned_reads;
            list<AlignedRead> overlapping_reads;
            str_container.GetReadsAtCoord(coord, &aligned_reads, &overlapping_reads);
            if (overlapping_reads.size() > 0) {
              genotyper.Genotype(aligned_reads, overlapping_reads);
            }
          }
          prev_chrom = ref_str_chunk.at(i).chrom;
          prev_begin = ref_str_chunk.at(i).start;
        }
        str_container.ClearReads();
      }
    }
  }

  /* Return run time info */
  run_info.endtime = GetTime();
  OutputRunStatistics();
  time(&endtime);
  if (!quiet) {
    stringstream msg;
    int seconds_elapsed = difftime(endtime, starttime);
    msg << "Done! " << GetDurationString(seconds_elapsed) << " elapsed";
    PrintMessageDieOnError(msg.str(), PROGRESS);
  }
  return 0;
}
