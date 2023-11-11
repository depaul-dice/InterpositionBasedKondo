#include "hdf5.h"

#include <stdio.h>
#include <stdlib.h>
#define FileName "/home/rohan/Documents/Research/HDF5SyntheticBenchmark/Synthetic10.h5"


enum AccessPattern{ Peripheral, LeftDiagonEdges, RightDiagonEdges, LeftDiagonSteps, Hole,  Quit};

void _Peripheral();

void _LeftDiagonEdges();

void _RightDiagonEdges();

void _LeftDiagonSteps();

void _Hole();

hid_t getOpenFile();
hid_t getDataset(hid_t file, char* DatasetName);
hid_t getDataspace(hid_t dataset);
