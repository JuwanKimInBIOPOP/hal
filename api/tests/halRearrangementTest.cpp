/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 * Copyright (C) 2012-2019 by UCSC Computational Genomics Lab
 *
 * Released under the MIT license, see LICENSE.txt
 */
#include "halRearrangementTest.h"
#include "halBottomSegmentTest.h"
#include "halTopSegmentTest.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;
using namespace hal;

void addIdenticalParentChild(Alignment *alignment, size_t numSequences, size_t numSegmentsPerSequence, size_t segmentLength) {
    vector<Sequence::Info> seqVec(numSequences);

    BottomSegmentIteratorPtr bi;
    BottomSegmentStruct bs;
    TopSegmentIteratorPtr ti;
    TopSegmentStruct ts;

    Genome *parent = alignment->addRootGenome("parent");
    Genome *child = alignment->addLeafGenome("child", "parent", 1);

    for (size_t i = 0; i < numSequences; ++i) {
        seqVec[i] = Sequence::Info("Sequence" + std::to_string(i), segmentLength * numSegmentsPerSequence,
                                   numSegmentsPerSequence, numSegmentsPerSequence);
    }
    parent->setDimensions(seqVec);
    child->setDimensions(seqVec);

    for (bi = parent->getBottomSegmentIterator(); not bi->atEnd(); bi->toRight()) {
        bs.set(bi->getBottomSegment()->getArrayIndex() * segmentLength, segmentLength);
        bs._children.clear();
        bs._children.push_back(pair<hal_size_t, bool>(bi->getBottomSegment()->getArrayIndex(), false));
        bs.applyTo(bi);
    }

    for (ti = child->getTopSegmentIterator(); not ti->atEnd(); ti->toRight()) {
        ts.set(ti->getTopSegment()->getArrayIndex() * segmentLength, segmentLength, ti->getTopSegment()->getArrayIndex());
        ts.applyTo(ti);
    }
}

// doesn't currently work on sequence endpoints
void makeInsertion(BottomSegmentIteratorPtr bi) {
    assert(bi->getBottomSegment()->isLast() == false);
    TopSegmentIteratorPtr ti = bi->getBottomSegment()->getGenome()->getTopSegmentIterator();
    ti->toChild(bi, 0);
    hal_index_t pi = ti->getTopSegment()->getParentIndex();
    ti->getTopSegment()->setParentIndex(NULL_INDEX);
    ti->toRight();
    ti->getTopSegment()->setParentIndex(pi);

    hal_index_t ci = bi->getBottomSegment()->getChildIndex(0);
    bi->getBottomSegment()->setChildIndex(0, ci + 1);
    bi->toRight();
    bi->getBottomSegment()->setChildIndex(0, NULL_INDEX);
}

// designed to be used only on alignments made with
// addIdenticalParentChild()
void makeInsGap(TopSegmentIteratorPtr ti) {
    Genome *genome = ti->getTopSegment()->getGenome();
    Genome *parent = genome->getParent();
    BottomSegmentIteratorPtr bi = parent->getBottomSegmentIterator();
    assert(ti->tseg()->hasParent() == true);
    bi->toParent(ti);
    while (not bi->atEnd()) {
        hal_index_t childIndex = bi->getBottomSegment()->getChildIndex(0);
        if (childIndex == (hal_index_t)(genome->getNumTopSegments() - 1)) {
            bi->getBottomSegment()->setChildIndex(0, NULL_INDEX);
        } else {
            bi->getBottomSegment()->setChildIndex(0, childIndex + 1);
        }
        bi->getReversed() ? bi->toLeft() : bi->toRight();
    }
    TopSegmentIteratorPtr topIt = ti->clone();
    topIt->getTopSegment()->setParentIndex(NULL_INDEX);
    topIt->toRight();
    while (not topIt->atEnd()) {
        hal_index_t parentIndex = topIt->getTopSegment()->getParentIndex();
        topIt->getTopSegment()->setParentIndex(parentIndex - 1);
        topIt->getReversed() ? topIt->toLeft() : topIt->toRight();
    }
}

void makeDelGap(BottomSegmentIteratorPtr botIt) {
    Genome *parent = botIt->getBottomSegment()->getGenome();
    Genome *genome = parent->getChild(0);
    BottomSegmentIteratorPtr bi = botIt->clone();
    BottomSegmentIteratorPtr bi2 = botIt->clone();
    TopSegmentIteratorPtr ti = genome->getTopSegmentIterator();
    TopSegmentIteratorPtr ti2 = genome->getTopSegmentIterator();
    assert(bi->bseg()->hasChild(0) == true);
    hal_index_t prevIndex = bi->getBottomSegment()->getChildIndex(0);
    bool prevReversed = bi->getBottomSegment()->getChildReversed(0);
    ti->toChild(bi, 0);
    bi->getBottomSegment()->setChildIndex(0, NULL_INDEX);
    bi->toRight();
    while (not bi->atEnd()) {
        hal_index_t prevIndex2 = bi->getBottomSegment()->getChildIndex(0);
        bool prevReversed2 = bi->getBottomSegment()->getChildReversed(0);
        bi2 = bi->clone();
        assert(bi2->getReversed() == false);
        bi2->toLeft();
        bi->getBottomSegment()->setChildIndex(0, prevIndex);
        bi->getBottomSegment()->setChildReversed(0, prevReversed);
        bi->getReversed() ? bi->toLeft() : bi->toRight();
        swap(prevIndex, prevIndex2);
        swap(prevReversed, prevReversed2);
    }

    while (not ti->atEnd()) {
        hal_index_t parentIndex = ti->getTopSegment()->getParentIndex();
        if (parentIndex == (hal_index_t)(parent->getNumBottomSegments() - 1)) {
            ti->getTopSegment()->setParentIndex(NULL_INDEX);
        } else {
            ti2 = ti->clone();
            if (ti->getTopSegment()->isLast() == false) {
                ti2->getReversed() ? ti2->toLeft() : ti2->toRight();
                hal_index_t nextIndex = ti2->getTopSegment()->getParentIndex();
                bool nextRev = ti2->getTopSegment()->getParentReversed();
                ti->getTopSegment()->setParentIndex(nextIndex);
                ti->getTopSegment()->setParentReversed(nextRev);
            }
        }
        ti->getReversed() ? ti->toLeft() : ti->toRight();
    }
}

void makeInversion(TopSegmentIteratorPtr ti, hal_size_t len) {
    Genome *child = ti->getTopSegment()->getGenome();
    Genome *parent = child->getParent();
    hal_index_t first = ti->getTopSegment()->getArrayIndex();
    hal_index_t last = ti->getTopSegment()->getArrayIndex() + len - 1;
    for (size_t i = 0; i < len; ++i) {
        TopSegmentIteratorPtr t = child->getTopSegmentIterator(first + i);
        BottomSegmentIteratorPtr b = parent->getBottomSegmentIterator(last - i);
        t->getTopSegment()->setParentIndex(last - i);
        t->getTopSegment()->setParentReversed(true);
        b->getBottomSegment()->setChildIndex(0, first + i);
        b->getBottomSegment()->setChildReversed(0, true);
    }
}

void RearrangementInsertionTest::createCallBack(Alignment *alignment) {
    size_t numSequences = 3;
    size_t numSegmentsPerSequence = 10;
    size_t segmentLength = 50;

    addIdenticalParentChild(alignment, numSequences, numSegmentsPerSequence, segmentLength);

    Genome *parent = alignment->openGenome("parent");
    Genome *child = alignment->openGenome("child");

    BottomSegmentIteratorPtr bi = parent->getBottomSegmentIterator();

    // insertion smaller than gap threshold
    bi->toRight();
    makeInsertion(bi);

    // stagger insertions to prevent gapped iterators from being larger
    // than a segment
    size_t count = 0;
    for (bi = parent->getBottomSegmentIterator(); not bi->atEnd(); bi->toRight()) {
        if (bi->bseg()->hasChild(0)) {
            bi->getBottomSegment()->setChildReversed(0, count % 2);
            TopSegmentIteratorPtr ti = child->getTopSegmentIterator();
            ti->toChild(bi, 0);
            ti->getTopSegment()->setParentReversed(count % 2);
            ++count;
        }
    }

    // insertion larger than gap threshold but that contains gaps
}

void RearrangementInsertionTest::checkCallBack(const Alignment *alignment) {
    BottomSegmentIteratorPtr bi;
    TopSegmentIteratorPtr ti;

    const Genome *child = alignment->openGenome("child");

    RearrangementPtr r = child->getRearrangement(0, 10, 1);
    do {
        hal_index_t leftIdx = r->getLeftBreakpoint()->getTopSegment()->getArrayIndex();
        if (leftIdx == 1) {
            CuAssertTrue(_testCase, r->getID() == Rearrangement::Insertion);
        } else if (leftIdx == 2) {
            // side effect of makeInsertion causes a deletion right next door
            CuAssertTrue(_testCase, r->getID() == Rearrangement::Deletion);
        } else {
            CuAssertTrue(_testCase, r->getID() != Rearrangement::Insertion && r->getID() != Rearrangement::Deletion);
        }
    } while (r->identifyNext() == true);
}

void RearrangementSimpleInversionTest::createCallBack(Alignment *alignment) {
    size_t numSequences = 3;
    size_t numSegmentsPerSequence = 10;
    size_t segmentLength = 50;

    addIdenticalParentChild(alignment, numSequences, numSegmentsPerSequence, segmentLength);

    Genome *parent = alignment->openGenome("parent");
    Genome *child = alignment->openGenome("child");

    // inversion on 1st segment (interior)
    BottomSegmentIteratorPtr bi = parent->getBottomSegmentIterator(1);
    TopSegmentIteratorPtr ti = child->getTopSegmentIterator(1);
    bi->getBottomSegment()->setChildReversed(0, true);
    ti->getTopSegment()->setParentReversed(true);

    // inversion on last segment of first sequence
    bi = parent->getBottomSegmentIterator(numSegmentsPerSequence - 1);
    ti = child->getTopSegmentIterator(numSegmentsPerSequence - 1);
    bi->getBottomSegment()->setChildReversed(0, true);
    ti->getTopSegment()->setParentReversed(true);

    // inversion on first segment of 3rd sequence
    bi = parent->getBottomSegmentIterator(numSegmentsPerSequence * 2);
    ti = child->getTopSegmentIterator(numSegmentsPerSequence * 2);
    bi->getBottomSegment()->setChildReversed(0, true);
    ti->getTopSegment()->setParentReversed(true);
}

void RearrangementSimpleInversionTest::checkCallBack(const Alignment *alignment) {
    BottomSegmentIteratorPtr bi;
    TopSegmentIteratorPtr ti;

    const Genome *child = alignment->openGenome("child");

    size_t numSegmentsPerSequence = child->getSequenceIterator()->getSequence()->getNumTopSegments();

    RearrangementPtr r = child->getRearrangement(0, 10, 1);

    r = child->getRearrangement(0, 10, 1);
    do {
        hal_index_t leftIdx = r->getLeftBreakpoint()->getTopSegment()->getArrayIndex();
        if (leftIdx == 1 || leftIdx == (hal_index_t)(numSegmentsPerSequence - 1) ||
            leftIdx == (hal_index_t)(numSegmentsPerSequence * 2)) {
            CuAssertTrue(_testCase, r->getID() == Rearrangement::Inversion);
        } else {
            CuAssertTrue(_testCase, r->getID() != Rearrangement::Inversion);
        }
    } while (r->identifyNext() == true);
}

void RearrangementGappedInversionTest::createCallBack(Alignment *alignment) {
    size_t numSequences = 3;
    size_t numSegmentsPerSequence = 10;
    size_t segmentLength = 5;

    addIdenticalParentChild(alignment, numSequences, numSegmentsPerSequence, segmentLength);

    Genome *parent = alignment->openGenome("parent");
    Genome *child = alignment->openGenome("child");

    // 4-segment inversion.
    // parent has gap-deletions at 2nd and 5th posstions
    // child has gap-insertions positions 3 and 5
    BottomSegmentIteratorPtr bi = parent->getBottomSegmentIterator(1);
    TopSegmentIteratorPtr ti = child->getTopSegmentIterator(1);
    bi->getBottomSegment()->setChildReversed(0, true);
    bi->getBottomSegment()->setChildIndex(0, 6);
    ti->getTopSegment()->setParentReversed(true);
    ti->getTopSegment()->setParentIndex(6);

    bi = parent->getBottomSegmentIterator(2);
    ti = child->getTopSegmentIterator(2);
    bi->getBottomSegment()->setChildIndex(0, NULL_INDEX);
    ti->getTopSegment()->setParentReversed(true);
    ti->getTopSegment()->setParentIndex(4);

    bi = parent->getBottomSegmentIterator(3);
    ti = child->getTopSegmentIterator(3);
    bi->getBottomSegment()->setChildReversed(0, true);
    bi->getBottomSegment()->setChildIndex(0, 4);
    ti->getTopSegment()->setParentIndex(NULL_INDEX);

    bi = parent->getBottomSegmentIterator(4);
    ti = child->getTopSegmentIterator(4);
    bi->getBottomSegment()->setChildReversed(0, true);
    bi->getBottomSegment()->setChildIndex(0, 2);
    ti->getTopSegment()->setParentIndex(3);
    ti->getTopSegment()->setParentReversed(true);

    bi = parent->getBottomSegmentIterator(5);
    ti = child->getTopSegmentIterator(5);
    bi->getBottomSegment()->setChildIndex(0, NULL_INDEX);
    ti->getTopSegment()->setParentIndex(NULL_INDEX);

    bi = parent->getBottomSegmentIterator(6);
    ti = child->getTopSegmentIterator(6);
    bi->getBottomSegment()->setChildReversed(0, true);
    bi->getBottomSegment()->setChildIndex(0, 1);
    ti->getTopSegment()->setParentIndex(1);
    ti->getTopSegment()->setParentReversed(true);
}

void RearrangementGappedInversionTest::checkCallBack(const Alignment *alignment) {
    BottomSegmentIteratorPtr bi;
    TopSegmentIteratorPtr ti;

    const Genome *child = alignment->openGenome("child");

    RearrangementPtr r = child->getRearrangement(0, 10, 1);
    do {
        hal_index_t leftIdx = r->getLeftBreakpoint()->getTopSegment()->getArrayIndex();
        if (leftIdx == 1) {
            CuAssertTrue(_testCase, r->getID() == Rearrangement::Inversion);
        } else {
            CuAssertTrue(_testCase, r->getID() != Rearrangement::Inversion);
        }
    } while (r->identifyNext() == true);
}

void halRearrangementInsertionTest(CuTest *testCase) {
    RearrangementInsertionTest tester;
    tester.check(testCase);
}

void halRearrangementSimpleInversionTest(CuTest *testCase) {
    RearrangementSimpleInversionTest tester;
    tester.check(testCase);
}

void halRearrangementGappedInversionTest(CuTest *testCase) {
    RearrangementGappedInversionTest tester;
    tester.check(testCase);
}

CuSuite *halRearrangementTestSuite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, halRearrangementInsertionTest);
    SUITE_ADD_TEST(suite, halRearrangementSimpleInversionTest);
    SUITE_ADD_TEST(suite, halRearrangementGappedInversionTest);
    return suite;
}
