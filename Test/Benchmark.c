#include "Benchmark.h"
#include "string.h"
char* fileName;
char* dsetName;
int verbose;
int main(int argc, char *argv[])
{
    printf("the num is %d\n",argc);
    if(argc !=4 )
        return 1;
    else
    fileName = argv[1];
    dsetName = argv[2];
    verbose = atoi(argv[3]);
    int choice;
    while(true)
    {
        printf("Options:\nEnter %d for peripharies of image\nEnter %d for top right bottom left corner\nEnter %d for top left bottom right corner\nEnter %d for top left to bottom right diagonal\nEnter %d for Hole\nEnter %d to quit\nEnter choice: ",
        (int) Peripheral, (int) LeftDiagonEdges, (int) RightDiagonEdges,(int) LeftDiagonSteps, (int) Hole, (int) Quit);
        scanf("%d",&choice);
        printf("You have chosen option %d\n",choice);
   
        switch(choice)
        {
            case Peripheral:
                _Peripheral();
                break;
            case LeftDiagonEdges:
                _LeftDiagonEdges();
                break;
            case RightDiagonEdges:
                _RightDiagonEdges();
                break;
            case LeftDiagonSteps:
                _LeftDiagonSteps();
                break;
            case Hole:
                _Hole();
                break;
            case Quit:
                printf("EXITTING\n");
                return 0;
            default:
                printf("You made an invalid choice please restart program\n");

        }

    }
}

void _Peripheral()
{
    int sW;
    int sH;
    int strideT;
    printf("Enter the width and height of boxes you want to extract:\nEnter Width: ");
    scanf("%d", &sW);
    printf("Enter Height: ");
    scanf("%d", &sH);
    printf("Enter the stride value in T direction: ");
    scanf("%d",&strideT);
    printf("You have chosen a width of %d and height of %d\n",sW,sH);
    hid_t file = getOpenFile();
    hid_t dataset = getDataset(file, "images");
    hid_t imageDataspace = getDataspace(dataset);
    hsize_t dims_out[4];
    hsize_t inputDims[4];
    hid_t status;
    int rank = H5Sget_simple_extent_dims (imageDataspace, inputDims, NULL);
    printf("The dataset rank is %d\n", rank);
    printf("The dimensions are %lld %lld %lld %lld\n",inputDims[0], inputDims[1], inputDims[2], inputDims[3]);
    int dataOut1 [inputDims[1]][sH][inputDims[3]];
    int dataOut2 [inputDims[1]][inputDims[2]-2*sH][sW];
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = sH;
    dims_out[3] = inputDims[3];
    memset(dataOut1, 0, sizeof(dataOut1));
    memset(dataOut2, 0, sizeof(dataOut2));
    hid_t memspace1 = H5Screate_simple (rank, dims_out, NULL);
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = inputDims[2]-2*sH;
    dims_out[3] = sW;
    hid_t memspace2 = H5Screate_simple (rank, dims_out, NULL);
    hssize_t offset[4] = {0,0,0,0};
    hssize_t readSize1[4] = {1,inputDims[1], sH,inputDims[3]};
    hssize_t readSize2[4] = {1,inputDims[1], inputDims[2]-2*sH,sW};
    status = H5Sselect_hyperslab (memspace1, H5S_SELECT_SET, offset, NULL, readSize1, NULL);
    status = H5Sselect_hyperslab (memspace2, H5S_SELECT_SET, offset, NULL, readSize2, NULL);
    while(offset[0]<inputDims[0])
    {
        offset[2] = 0;
        offset[3] = 0;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize1, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace1, imageDataspace,H5P_DEFAULT, dataOut1);
        if(verbose == 1){
            printf("TOP SECTION:\n");
            for (int l = 0; l <inputDims[1]; l++)
            {
                for (int j = 0; j < sH; j++)
                {
                    for(int k =0; k<inputDims[2]; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
            }
        }
        offset[2] = inputDims[2]-sH;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize1, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace1, imageDataspace,H5P_DEFAULT, dataOut1);
        if(verbose == 1){
        printf("BOTTOM SECTION:\n");
            for (int l = 0; l <inputDims[1]; l++){
                for (int j = 0; j < sH; j++)
                {
                    for(int k =0; k<inputDims[2]; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
            }
        }
        offset[2] = sH;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize2, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace2, imageDataspace,H5P_DEFAULT, dataOut2);

        if(verbose == 1){
        printf("LEFT SECTION:\n");
            for (int l = 0; l <inputDims[1]; l++){

                for (int j = 0; j < inputDims[2]-2*sH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut2[l][j][k]);
                    }
                    printf("\n");
                }
                                printf("\n\n\n");
            }
        }
        offset[2] = sH;
        offset[3] = inputDims[2]-sW;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize2, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace2, imageDataspace,H5P_DEFAULT, dataOut2);
        
        if(verbose == 1){
        printf("RIGHT SECTION:\n");
            for (int l = 0; l <inputDims[1]; l++)
            {
                for (int j = 0; j < inputDims[2]-2*sH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut2[l][j][k]);
                    }
                    printf("\n");
                }
                printf("\n\n\n");
            }
        }
        offset[0]+=strideT;
    }
}

void _LeftDiagonEdges()
{
    printf("In Left Diag Edges\n");
    int sW;
    int sH;
    int strideT;
    printf("Enter the width and height of boxes you want to extract:\nEnter Width: ");
    scanf("%d", &sW);
    printf("Enter Height: ");
    scanf("%d", &sH);
    printf("Enter the stride value in T direction: ");
    scanf("%d",&strideT);
    printf("You have chosen a width of %d and height of %d\n",sW,sH);
    hid_t file = getOpenFile();
    hid_t dataset = getDataset(file, "images");
    hid_t imageDataspace = getDataspace(dataset);
    hsize_t dims_out[4];
    hsize_t inputDims[4];
    hid_t status;
    int rank = H5Sget_simple_extent_dims (imageDataspace, inputDims, NULL);
    int dataOut [inputDims[1]][sW][sH];
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = sW;
    dims_out[3] = sH;
    memset(dataOut, 0, sizeof(dataOut));
    hid_t memspace = H5Screate_simple (rank, dims_out, NULL);
    hssize_t offset[4] = {0,0,0,0};
    hssize_t readSize[4] = {1,inputDims[1],sW,sH};
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset, NULL, readSize, NULL);
    printf("The dataset rank is %d\n", rank);
    printf("The dimensions are %lld %lld %lld %lld\n",inputDims[0], inputDims[1], inputDims[2], inputDims[3]);
    while(offset[0]<inputDims[0])
    {
        offset[2] = 0;
        offset[3] = inputDims[3]-readSize[3];
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace, imageDataspace,H5P_DEFAULT, dataOut);

        if(verbose == 1){
        printf("Reading from image number t=%lld\n",offset[0]);
        printf("TOP RIGHT:\n");
        for (int l = 0; l <inputDims[1]; l++)
        {
            for (int j = 0; j < dims_out[2]; j++)
            {
                for(int k =0; k<dims_out[3]; k++){
                    printf("%d ",dataOut[l][j][k]);
                }
                printf("\n");
            }
        }
        }
        offset[2]= inputDims[2]-readSize[2];
        offset[3]= 0;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace, imageDataspace,H5P_DEFAULT, dataOut);
        if(verbose == 1){
        printf("BOTTOM LEFT:\n");
        for (int l = 0; l <inputDims[1]; l++)
        {
            for (int j = 0; j < dims_out[2]; j++)
            {
                for(int k =0; k<dims_out[3]; k++){
                    printf("%d ",dataOut[l][j][k]);
                }
                printf("\n");
            }
        }
        printf("\n\n\n");
        }
        offset[0]+=strideT;
    }
}

void _RightDiagonEdges()
{
    printf("In Right Diag Edges\n");
    
    int sW;
    int sH;
    int strideT;
    printf("Enter the width and height of boxes you want to extract:\nEnter Width: ");
    scanf("%d", &sW);
    printf("Enter Height: ");
    scanf("%d", &sH);
    printf("Enter the stride value in T direction: ");
    scanf("%d",&strideT);
    printf("You have chosen a width of %d and height of %d\n",sW,sH);
  
    hid_t file = getOpenFile();
    hid_t dataset = getDataset(file, "images");
    hid_t imageDataspace = getDataspace(dataset);
    hsize_t dims_out[4];
    hsize_t inputDims[4];
    hid_t status;
    int rank = H5Sget_simple_extent_dims (imageDataspace, inputDims, NULL);
    int dataOut [inputDims[1]][sW][sH];
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = sW;
    dims_out[3] = sH;
    memset(dataOut, 0, sizeof(dataOut));
    hid_t memspace = H5Screate_simple (rank, dims_out, NULL);
    hssize_t offset[4] = {0,0,0,0};
    hssize_t readSize[4] = {1,inputDims[1],sW,sH};
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset, NULL, readSize, NULL);
    printf("The dataset rank is %d\n", rank);
    printf("The dimensions are %lld %lld %lld %lld\n",inputDims[0], inputDims[1], inputDims[2], inputDims[3]);
    while(offset[0]<inputDims[0])
    {
        offset[2] = 0;
        offset[3] = 0;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace, imageDataspace,H5P_DEFAULT, dataOut);
        if(verbose == 1){
        printf("Reading from image number %lld\n",offset[0]);
        printf("TOP LEFT:\n");
        for( int l = 0 ; l < inputDims[1]; l++)
        {
            for (int j = 0; j < dims_out[2]; j++)
            {
                for(int k =0; k<dims_out[3]; k++){
                    printf("%d ",dataOut[l][j][k]);
                }
                printf("\n");
            }
            printf("\n\n\n");
        }
        }
        offset[2]= inputDims[2]-readSize[2];
        offset[3]= inputDims[3]-readSize[3];
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace, imageDataspace,H5P_DEFAULT, dataOut);
        if(verbose == 1){
        printf("BOTTOM RIGHT:\n");
        for( int l = 0 ; l < inputDims[1]; l++)
        {
            for (int j = 0; j < dims_out[2]; j++)
            {
                for(int k =0; k<dims_out[3]; k++){
                    printf("%d ",dataOut[l][j][k]);
                }
                printf("\n");
            }
            printf("\n\n\n");
        }
        printf("********\n");
        }
        offset[0]+=strideT;
    }
}

void _LeftDiagonSteps()
{
    printf("In Left Diag Steps\n");
    int sW;
    int sH;
    int strideX;
    int strideY;
    int strideT;
    
    printf("Enter the width and height of boxes you want to extract:\nEnter Width: ");
    scanf("%d", &sW);
    printf("Enter Height: ");
    scanf("%d", &sH);
    printf("Enter the stride value in each of the directions:\nEnter strideX: ");
    scanf("%d",&strideX);
    printf("Enter strideY: ");
    scanf("%d",&strideY);
    printf("Enter strideT: ");
    scanf("%d",&strideT);
    
    printf("You have chosen a width of %d and height of %d\n",sW,sH);
    printf("And strides of X: %d, Y: %d, T: %d\n",strideX, strideY, strideT);

    hid_t file = getOpenFile();
    hid_t dataset = getDataset(file, "images");
    hid_t imageDataspace = getDataspace(dataset);
    hsize_t dims_out[4];
    hsize_t inputDims[4];
    hid_t status;
    int rank = H5Sget_simple_extent_dims (imageDataspace, inputDims, NULL);
    int dataOut[1][inputDims[1]][sW][sH];
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = sW;
    dims_out[3] = sH;
    memset(dataOut, 0, sizeof(dataOut));
    hid_t memspace = H5Screate_simple (rank, dims_out, NULL);
    hssize_t offset[4] = {0,0,0,0};
    hssize_t readSize[4] = {1,inputDims[1],sW,sH};
    
    status = H5Sselect_hyperslab (memspace, H5S_SELECT_SET, offset, NULL, readSize, NULL);
    printf("The dataset rank is %d\n", rank);
    printf("The dimensions are %lld %lld %lld %lld\n",inputDims[0], inputDims[1], inputDims[2], inputDims[3]);

    while(offset[0]<inputDims[0] && (offset[2]+sW)<inputDims[2] && (offset[3]+sH)<inputDims[3])
    {
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, offset, NULL,readSize, NULL);
        
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace, imageDataspace,
                            H5P_DEFAULT, dataOut);
        if(verbose == 1){
        printf("Reading from t:%lld x:%lld y:%lld\n",offset[0],offset[2],offset[3]);
        
        for( int l = 0 ; l < inputDims[1]; l++)
        {
            for (int j = 0; j < dims_out[2]; j++)
            {
                for(int k =0; k<dims_out[3]; k++){
                    printf("%d ",dataOut[0][l][j][k]);
                }
                printf("\n");
            }
            printf("\n\n\n");
        }
        printf("\n*****\n");
        }
        offset[0]+=strideT;
        offset[2]+=strideX;
        offset[3]+=strideY;
    }    
}
void _Hole()
{
    int sW;
    int sH;
    int strideX;
    int strideY;
    int strideT;
    int innersW;
    int innersH;
    printf("Enter the width and height of boxes you want to extract:\nEnter Width: ");
    scanf("%d", &sW);
    printf("Enter Height: ");
    scanf("%d", &sH);
    printf("Enter Inner Height: ");
    scanf("%d", &innersH);
    printf("Enter Inner Width: ");
    scanf("%d", &innersW);
    printf("Enter the stride value in X direction: ");
    scanf("%d",&strideX);
    printf("Enter the stride value in Y direction: ");
    scanf("%d",&strideY);
    printf("Enter the stride value in T direction: ");
    scanf("%d",&strideT);
    printf("You have chosen a width of %d and height of %d for the outer box\n",sW,sH);
    printf("You have chosen a width of %d and height of %d for the inner box\n",innersW,innersH);
    printf("You have chosen a stride of %d %d %d in the 3 Dimensions T, X, Y\n",strideT, strideX, strideY);
    hid_t file = getOpenFile();
    hid_t dataset = getDataset(file, "images");
    hid_t imageDataspace = getDataspace(dataset);
    hsize_t dims_out[4];
    hsize_t inputDims[4];
    hid_t status;
    int rank = H5Sget_simple_extent_dims (imageDataspace, inputDims, NULL);
    int dataOut1 [inputDims[1]][innersH][sW];
    int dataOut2 [inputDims[1]][sH-2*innersH][innersW];
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = innersH;
    dims_out[3] = sW;
    memset(dataOut1, 0, sizeof(dataOut1));
    memset(dataOut2, 0, sizeof(dataOut2));
    hid_t memspace1 = H5Screate_simple (rank, dims_out, NULL);
    dims_out[0] = 1;
    dims_out[1] = inputDims[1];
    dims_out[2] = sH-2*innersH;
    dims_out[3] = innersW;
    hid_t memspace2 = H5Screate_simple (rank, dims_out, NULL);
    hssize_t offset[4] = {0,0,0,0};
    hssize_t innerOffset[4] = {0,0,0,0};
    hssize_t readSize1[4] = {1,inputDims[1],innersH,sW};
    hssize_t readSize2[4] = {1,inputDims[1],sH-2*innersH,innersW};
    status = H5Sselect_hyperslab (memspace1, H5S_SELECT_SET, offset, NULL, readSize1, NULL);
    status = H5Sselect_hyperslab (memspace2, H5S_SELECT_SET, offset, NULL, readSize2, NULL);
    printf("The dataset rank is %d\n", rank);
    printf("The dimensions are %lld %lld %lld %lld\n",inputDims[0], inputDims[1], inputDims[2], inputDims[3]);
    while(offset[0]<inputDims[0] && (offset[2]+sW)<(inputDims[2]+1) && (offset[3]+sH)<(inputDims[3]+1))
    {
        innerOffset[0] = offset[0];
        innerOffset[1] = offset[1];
        innerOffset[2] = offset[2];
        innerOffset[3] = offset[3];
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, innerOffset, NULL,readSize1, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace1, imageDataspace,H5P_DEFAULT, dataOut1);
        if(verbose == 1){
        printf("Reading from offsets T:%lld X:%lld Y:%lld\n\n",offset[0],offset[2],offset[3]);
        printf("TOP SECTION:\n");
            for(int l = 0; l < inputDims[1]; l++)
            {
                for (int j = 0; j < innersH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
        innerOffset[2] = offset[2]+sH-innersH;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, innerOffset, NULL,readSize1, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace1, imageDataspace,H5P_DEFAULT, dataOut1);
        if(verbose == 1){
        printf("BOTTOM SECTION:\n");
            for(int l = 0; l < inputDims[1]; l++)
            {
                for (int j = 0; j < innersH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
        innerOffset[2] = offset[2]+innersH;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, innerOffset, NULL,readSize2, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace2, imageDataspace,H5P_DEFAULT, dataOut2);
        if(verbose == 1){
        printf("LEFT SECTION:\n");
            for(int l = 0; l < inputDims[1]; l++)
            {
                for (int j = 0; j < innersH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
        innerOffset[3]=offset[3]+sW-innersW;
        status = H5Sselect_hyperslab (imageDataspace, H5S_SELECT_SET, innerOffset, NULL,readSize2, NULL);
        status = H5Dread (dataset, H5T_NATIVE_INT, memspace2, imageDataspace,H5P_DEFAULT, dataOut2);
        if(verbose == 1){
        printf("RIGHT SECTION:\n");
            for(int l = 0; l < inputDims[1]; l++)
            {
                for (int j = 0; j < innersH; j++)
                {
                    for(int k =0; k<sW; k++){
                        printf("%d ",dataOut1[l][j][k]);
                    }
                    printf("\n");
                }
                printf("\n");
            }
        }
        offset[0]+=strideT;
        offset[2]+=strideX;
        offset[3]+=strideY;
    }
    
}

hid_t getOpenFile()
{
    return H5Fopen (fileName , H5F_ACC_RDONLY, H5P_DEFAULT);  
}

hid_t getDataset(hid_t file, char* DatasetName)
{
    return H5Dopen (file, dsetName);
}

hid_t getDataspace(hid_t dataset)
{
    return H5Dget_space (dataset);    
}
