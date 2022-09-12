Help()
{
   # Display Help
   echo "Function:"
   echo "   1) Create the subset databse using output of Logging phase"
   echo "Syntax:"
   echo "./createDataStore.sh <Absolute path to executable's directory> <path to base directory of code>"  
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
cd $2
cd DataStoreCreator
python3 readFile.py $1/tracelog $1/backups
