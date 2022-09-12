#!/bin/bash
Help()
{
   # Display Help
   echo "Syntax: ./log.sh <Absolute path to Executable> <path to base directory of code> \"<command to execute>\""
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
rm -rf tracelog/ >> /dev/null
mkdir tracelog/
echo "LD_PRELOAD=$2/Log/libwrappers.so:$2/Log/libFileData.so $3"
LD_PRELOAD=$2/Log/libwrappers.so:$2/Log/libFileData.so $3
