#!/bin/bash
Help()
{
   # Display Help
   echo "Syntax: ./reExecute.sh <Absolute path to Executable's directory> <path to base directory of code> <\"Command to execute\">"
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
LD_PRELOAD=$2/ReExecute/libwrappers.so:$2/ReExecute/libReExec.so "$3"
