/*
 * Copyright (C) 2012 by Glenn Hickey (hickey@soe.ucsc.edu)
 * Copyright (C) 2012-2019 by UCSC Computational Genomics Lab
 *
 * Released under the MIT license, see LICENSE.txt
 */

/**
 * Test the Segment offset nonsense for HDF5 arrays
 */

#include "allTests.h"
#include "halCommon.h"
#include "hdf5ExternalArray.h"
#include "hdf5Genome.h"
#include "hdf5Test.h"
#include <H5Cpp.h>
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
extern "C" {
#include "commonC.h"
}

using namespace H5;
using namespace hal;
using namespace std;

static const hsize_t chunkSizes[] = {0, 4, N / 10, N / 2, N};
static const hsize_t numSizes = 5;

static void teardown() {
    hdf5TestTeardown();
}

static void setup() {
    hdf5TestSetup();
}

static char idxToDNA(hsize_t i) {
    switch (i % 10) {
    case 0:
        return 'A';
    case 1:
        return 'C';
    case 2:
        return 'G';
    case 3:
        return 'T';
    case 4:
        return 'a';
    case 5:
        return 'c';
    case 6:
        return 'g';
    case 7:
        return 't';
    case 8:
        return 'N';
    case 9:
        return 'n';
    default:
        break;
    }
    return '?';
}

void hdf5DNAPackingTest(CuTest *testCase) {
    static const size_t len = 5000;
    char array[len];
    srand(time(NULL));
    for (size_t i = 0; i < len; ++i) {
        array[i] = idxToDNA(rand());
        assert(isNucleotide(array[i]));
    }

    unsigned char packedArray[len / 2];
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = packedArray[i / 2];
        packedArray[i / 2] = dnaPack(array[i], i, c);
        c = dnaUnpack(i, packedArray[i / 2]);
        CuAssertTrue(testCase, c == array[i]);
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = dnaUnpack(i, packedArray[i / 2]);
        CuAssertTrue(testCase, c == array[i]);
    }

    for (size_t i = 0; i < len / 3; ++i) {
        size_t j = rand() % 5000;
        array[j] = idxToDNA(rand());
        unsigned char c = packedArray[j / 2];
        packedArray[j / 2] = dnaPack(array[j], j, c);
        c = dnaUnpack(j, packedArray[j / 2]);
        CuAssertTrue(testCase, c == array[j]);
    }
}

void hdf5DNATypeTest(CuTest *testCase) {
    for (hsize_t chunkIdx = 0; chunkIdx < numSizes; ++chunkIdx) {
        hsize_t chunkSize = chunkSizes[chunkIdx];
        setup();
        try {
            PredType datatype = Hdf5Genome::dnaDataType();
            H5File file(H5std_string(fileName), H5F_ACC_TRUNC);

            Hdf5ExternalArray myArray;
            DSetCreatPropList cparms;
            if (chunkSize > 0) {
                cparms.setChunk(1, &chunkSize);
            }
            hsize_t NEVEN = N % 2 ? N + 1 : N;
            myArray.create(&file, datasetName, datatype, NEVEN / 2, &cparms);
            for (hsize_t i = 0; i < NEVEN / 2; ++i) {
                unsigned char value = 0U;
                value = dnaPack(idxToDNA(i * 2), i * 2, value);
                value = dnaPack(idxToDNA((i * 2) + 1), (i * 2) + 1, value);
                myArray.setValue(i, 0, value);
            }
            myArray.write();
            file.flush(H5F_SCOPE_LOCAL);
            file.close();

            H5File rfile(H5std_string(fileName), H5F_ACC_RDONLY);
            Hdf5ExternalArray readArray;
            readArray.load(&rfile, datasetName);
            for (hsize_t i = 0; i < NEVEN / 2; ++i) {
                unsigned char value = readArray.getValue<unsigned char>(i, 0);
                char v1 = dnaUnpack(0, value);
                char v2 = dnaUnpack(1, value);
                CuAssertTrue(testCase, v1 == idxToDNA(i * 2));
                CuAssertTrue(testCase, v2 == idxToDNA((i * 2) + 1));
            }
        } catch (Exception &exception) {
            cerr << exception.getCDetailMsg() << endl;
            CuAssertTrue(testCase, 0);
        } catch (...) {
            CuAssertTrue(testCase, 0);
        }
        teardown();
    }
}

CuSuite *hdf5DNATypeTestSuite(void) {
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, hdf5DNAPackingTest);
    SUITE_ADD_TEST(suite, hdf5DNATypeTest);
    return suite;
}
