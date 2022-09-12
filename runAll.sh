#!/bin/bash
Help()
{
   # Display Help
   echo "Function:"
   echo ""
   echo "Script to:"
   echo "   1) Compile all code"
   echo "   2) Run Logging (phase 1) on the executable provided"
   echo "   3) Create subset data store (phase 2)"
   echo "   4) Re execute executable using subsetted data (phase 3)"
   echo ""
   echo "Syntax:" 
   echo "./runAll.sh <Absolute path to Executable's directory> <path to base directory of code> <name of executable>"
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
./compile.sh $2 >> /dev/null
./log.sh $1 $2 $3
./createDataStore.sh $1 $2
./reExecute.sh $1 $2 $3