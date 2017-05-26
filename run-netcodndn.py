#!/usr/bin/env python
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

from subprocess import call
from sys import argv
import os
import subprocess
import workerpool
import multiprocessing
import argparse

N_RUN = 1

######################################################################
######################################################################
######################################################################

parser = argparse.ArgumentParser(description='Simulation runner')
parser.add_argument('scenarios', metavar='scenario', type=str, nargs='*',
                    help='Scenario to run')

parser.add_argument('-l', '--list', dest="list", action='store_true', default=False,
                    help='Get list of available scenarios')

parser.add_argument('-s', '--simulate', dest="simulate", action='store_true', default=False,
                    help='Run simulation and postprocessing (false by default)')

parser.add_argument('-g', '--no-graph', dest="graph", action='store_false', default=False,
                    help='Do not build a graph for the scenario (builds a graph by default)')

args = parser.parse_args()

if not args.list and len(args.scenarios)==0:
    print "ERROR: at least one scenario need to be specified"
    parser.print_help()
    exit (1)

if args.list:
    print "Available scenarios: "
else:
    if args.simulate:
        # Clean Memory
        print "Cleaning memory"
        cmdline = ["sync"]
        subprocess.call (cmdline)
        cmdline = "echo 3 > /proc/sys/vm/drop_caches"
        subprocess.call (cmdline,shell=True)
        # Compile
        print "Compiling the following scenarios: " + ",".join (args.scenarios)
        cmdline = ["./waf"]
        subprocess.call (cmdline)

        # Simulate
        print "Simulating the following scenarios: " + ",".join (args.scenarios)
        for s in args.scenarios:
            for i in range(N_RUN):
                cmdline = ["./build/" + s, "--runid=" + str(i), "--RngRun=" + str(i)]
                print (cmdline)
                subprocess.call (cmdline)

    if args.graph:
        print "Building graphs for the following scenarios: " + ",".join (args.scenarios)
