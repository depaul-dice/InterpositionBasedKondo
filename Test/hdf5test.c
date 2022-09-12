/************************************************************
  
  This example shows how to write and read a hyperslab.  It 
  is derived from the h5_read.c and h5_write.c examples in 
  the "Introduction to HDF5".

 ************************************************************/
 
#include "hdf5.h"

#define FILE        "sds.h5"
#define DATASETNAME "IntArray" 
#define NX_SUB  3                      /* hyperslab dimensions */ 
#define NY_SUB  4 
#define NX 7                           /* output buffer dimensions */ 
#define NY 7 
#define NZ  3 
#define RANK         2
#define RANK_OUT     3
#define NUMIMAGES 3
#define IMAGE_X 5
#define IMAGE_Y 5
#define X     5                        /* dataset dimensions */
#define Y     6

int
main (void)
{
    hsize_t     dimsf[2];              /* dataset dimensions */
    int         data[X][Y];            /* data to write */

    /* 
     * Data  and output buffer initialization. 
     */
    hid_t       file, dataset;         /* handles */
    hid_t       dataspace;   
    hid_t       memspace; 
    hsize_t     dimsm[3];              /* memory space dimensions */
    hsize_t     dims_out[3];           /* dataset dimensions */      
    herr_t      status;                             

    int         data_out[NUMIMAGES][IMAGE_X][IMAGE_Y]; /* output buffer */
   
    hsize_t     count[3];              /* size of the hyperslab in the file */
    hssize_t    offset[3];             /* hyperslab offset in the file */
    hsize_t     count_out[3];          /* size of the hyperslab in memory */
    hssize_t    offset_out[3];         /* hyperslab offset in memory */
    int         i, j, k, status_n, rank;


 

/*************************************************************  

  This reads the hyperslab from the sds.h5 file just 
  created, into a 2-dimensional plane of the 3-dimensional 
  array.

 ************************************************************/  

    for (j = 0; j < NUMIMAGES; j++) {
	for (i = 0; i < IMAGE_X; i++) {
	    for (k = 0; k < IMAGE_Y ; k++)
		data_out[j][i][k] = 0;
	}
    } 
 
    /*
     * Open the file and the dataset.
     */
    file = H5Fopen ("/home/ubuntu/researchDev3/InterpositionClean/Test/Datasets/data.h5", H5F_ACC_RDONLY, H5P_DEFAULT);

    dataset = H5Dopen (file, "images");

    dataspace = H5Dget_space (dataset);    /* dataspace handle */
    rank      = H5Sget_simple_extent_ndims (dataspace);
    // printf("\nReading the dims\n");
    status_n  = H5Sget_simple_extent_dims (dataspace, dims_out, NULL);
    printf("\nRank: %d\nDimensions: %lu x %lu x %lu \n", rank,
	   (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]), (unsigned long)(dims_out[2]));


    // memory dataspace
    dimsm[0] = NUMIMAGES;
    dimsm[1] = IMAGE_X;
    dimsm[2] = IMAGE_Y;
    memspace = H5Screate_simple (RANK_OUT, dimsm, NULL);   
    offset_out[0]=0;
    offset_out[1]=0;
    offset_out[2]=0;
    count_out[0]=NUMIMAGES;
    count_out[1]=IMAGE_X;
    count_out[2]=IMAGE_Y;
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset_out, NULL, 
                                    count_out, NULL);
    offset[0]=10;
    offset[1]=100;
    offset[2]=10;

    count[0]=NUMIMAGES;
    count[1]=IMAGE_X;
    count[2]=IMAGE_Y;
    status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count, NULL);

    status = H5Dread (dataset, H5T_NATIVE_INT, memspace, dataspace,
                        H5P_DEFAULT, data_out);
    printf ("Data:\n ");
    for (j = 0; j < IMAGE_X; j++) {
    for (i = 0; i < IMAGE_Y; i++) printf("%d ", data_out[0][i][j]);
    printf("\n ");
    }
    /*
    * Close and release resources.
    */
    H5Dclose (dataset);
    H5Sclose (dataspace);
    H5Sclose (memspace);
    H5Fclose (file);

}
