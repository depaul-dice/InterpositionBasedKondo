import h5py as h5

def openFile(path, mode):
    return h5.File(path, mode)

def openDataset(file, datasetName):
    return file[datasetName]

def main():
    fileObj = openFile("/home/ubuntu/Research/InterpositionBasedMiDas/Test/Datasets/NEONDSImagingSpectrometerData.h5","r")
    dataset = openDataset(fileObj, "Reflectance")
    print(dataset.shape)
if __name__ == "__main__":
    main()

# f = h5.File()
# reflectance = f["Reflectance"]
# print(reflectance.shape)
