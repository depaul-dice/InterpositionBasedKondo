
Help()
{
   # Display Help
   echo "Function:"
   echo "   1) Create the subset databse using output of Logging phase"
   echo "Syntax:"
   echo "./createDataStore.sh <path to base directory of code>"  
}

while getopts ":h" option; do
   case $option in
      h) # display Help
         Help
         exit;;
     \?) # incorrect option
         echo "Error: Invalid option"
         exit;;
   esac
done

cd $1
cd Log
clang -I ../ -shared -g -ldl -fPIC tracer.c -o libwrappers.so
clang -I ../ -shared -g -ldl -fPIC traceDataHandler.c -o libFileData.so
cd ../ReExecute
clang -I ../ -shared -g -ldl -fPIC wrappers.c -o libwrappers.so
clang -I ../ -shared -g -ldl -fPIC reExecute.c -o libReExec.so
