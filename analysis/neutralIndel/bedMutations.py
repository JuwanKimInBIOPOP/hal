#!/usr/bin/env python

#Copyright (C) 2012 by Glenn Hickey
#
#Released under the MIT license, see LICENSE.txt
#!/usr/bin/env python

"""Scan a BED file of mutations
"""
import sys
import os

class BedMutations:

    # the official location of these definitions is in
    # hal/mutations/src/halBranchMutations.cpp
    inversionBedTag = "V"
    insertionBedTag = "I"
    deletionBedTag = "D"
    deletionBreakBedTag = "DB"
    transpositionBedTag = "P"
    duplicationBedTag = "U"
    gapInsertionBedTag = "GI"
    gapDeletionBedTag = "GD"
    gapDeletionBreakBedTag = "GDB"
    substitutionBedTag = "S_"

    # keep lenient by default, everything but dupes and transpositions
    defaultEvents = (insertionBedTag, gapInsertionBedTag,
                     deletionBedTag, deletionBreakBedTag,
                     gapDeletionBedTag, gapDeletionBreakBedTag,
                     inversionBedTag, substitutionBedTag)
    
    def __init__(self):
        pass

    # read bed file line by line, storing relevant info in class members
    def scan(bedPath, events = self.defaultEvents):
        bedFile = open(bedPath, "r")
        self.sequence = None
        self.prevSequence = None
        self.range = None
        self.prevRange = None
        self.op = None
        self.prevOp = None
        self.events = set(events)
        
        for line in bedFile:
            tokens = line.split()
            if len(tokens) == 0 or tokens[0][0] == "#":
                continue
            assert len(tokens) >= 4
            if not __testIgnore(tokens[3]):
                self.prevSequence = self.sequence
                self.prevRange = self.range
                self.prevOp = self.op
                self.sequence = tokens[0]
                self.range = (tokens[1], tokens[2])
                self.op = tokens[3]
                if self.prevSequence != self.sequence:
                    self.prevSequence = None
                    self.prevRange = None
                    self.prevOp = None
                yield

    # return distance from previous operation
    def distance():
        if self.prevRange is None or self.range is None:
            return None
        return self.range[0] - self.prevRange[1]

    def __testIgnore(token):
        if token.find(self.substitutionBedTag) == 0:
            return self.substitutionBedTag in self.events
        return token in self.events