import h5py as h5


f = h5.File("/home/rohan/Documents/Research/InterpositionBasedMiDas/Test/Datasets/NEONDSImagingSpectrometerData.h5","r")
reflectance = f["Reflectance"]
reflectanceData = reflectance[:,49,392]
print(reflectanceData.shape)
