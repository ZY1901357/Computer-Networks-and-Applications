#!/usr/bin/env python3
#***********************************************
#Computer Network and Application Assignment 3 
# Student: Zhen Yu
# Student ID: a1901357
# Language: Python 3.12.0 
#***********************************************

'''
Part 1 (DV algorithm)

You are to produce a program that:

1) Reads information about a topology/updates to the topology from the standard input.
2) Uses DV algorithm or DV with SH algorithm, as appropriate, to bring the simulated routers to convergence.
        - Output the distance tables in the required format for each router at each step/round.
        - Output the final routing tables in the required format once convergence is reached.
3) Repeats the above steps until no further input is provided.

The DV algorithm program you are to provide should be named DistanceVector.

'''
#_______________________________________________

#----- Import Liabaries ----- 
import sys 

