import h5py as h5

def openFile(path, mode):
    return h5.File(path, mode)

def openDataset(file, datasetName):
    return file[datasetName]

def main():
    fileObj = openFile("/home/rohan/Research/InterpositionBasedMiDas/Test/Datasets/Mat1000x1000Comp.h5","r")
    dataset = openDataset(fileObj, "matrix")
    print(dataset.chunks)
    print(dataset[:,:])
    #print(dataset[0:3,0:2])
    #0:100,0:100
    #950:1000,400:700 
    #print(dataset[90,95])
    #print(dataset[45,67])
    #print(dataset[0,4])
if __name__ == "__main__":
    main()

# f = h5.File()
# reflectance = f["Reflectance"]
# print(reflectance.shape)
"""

import h5py as h5
File = h5.File("matrix.h5", mode ="r")
matrix = File["matrix"]
print(matrix[0,0])"""
