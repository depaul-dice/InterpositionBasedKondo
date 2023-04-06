from copy import deepcopy
import os
import sys
# from turtle import towards
from parse import *
from intervaltree import Interval, IntervalTree
import time

def getLines(filename):
    """Given a filename  open the file and read all lines from it
    and return it as a list"""
    with open(filename, "r") as fin:
        return fin.readlines()


def getReadList(Lines):
    """Given a list of lines from the log file return a list of dictionaries
     of offsets of data that were read"""
    ret = IntervalTree()

    # Skip to read calls
    if(Lines[1].strip() != "ReadList"):
        print("Error in file order")
        sys.exit(1)
    for i in range(2, len(Lines)):
        curLine = Lines[i].strip()
        # Break when we reach call details
        if(curLine == "CallList"):
            break
        format_string = "{}:{}"
        parsed = parse(format_string, curLine)
        # ret[.append({"start": parsed[0], "end": parsed[1]})]
        ret[int(parsed[0]):int(parsed[1])] = {"data": None}
    return ret


def getCallList(Lines):
    """ Given a list of lines from the log file return a list of dictionaries
    of read and write calls made to the file"""
    index = 0
    ret = IntervalTree()
    # Skip to start of read calls
    for i in range(len(Lines)):
        curLine = Lines[i].strip()
        if(curLine == "CallList"):
            index = i
            break
    for i in range(index+1, len(Lines)):
        curLine = Lines[i].strip()
        # BReak when we reazach backup info 
        if(curLine == "Backups"):
            break
        format_string = "{}:{}:{}:{}:{}"
        parsed = parse(format_string, curLine)
        if parsed[4] == "1":
            ret[int(parsed[1]):int(parsed[1])+int(parsed[2])] = {            
                "location": parsed[0],
                "offset": parsed[1],
                "OpSize": parsed[2],
                "timestamp": parsed[3],
                "type": parsed[4]
                }
    return ret


def readFromFile(readList, path):
    """Given a list of offsets that were read and path to the file read from
    return a dictionary with the offsets and the data"""
    fd = os.open(path, os.O_RDONLY)
    for read in readList:
        newread = deepcopy(read)
        os.lseek(fd, read[0], os.SEEK_SET)
        read.data["data"] = os.read(fd, read[1]-read[0])
    return readList


def storeSubset(readData, filename, pathTrace):
    """Given the read data create a subset file of the original file"""
    os.chdir(pathTrace)
    os.chdir("subsets")
    fd = os.open(filename, os.O_WRONLY | os.O_CREAT)
    curPos = 0
    for val in readData:
        s = val.begin
        e = val.end
        d = val.data["data"]
        os.write(fd, d)
        val.data["offsetBackup"] = curPos
        curPos += (e-s)
    return readData


def checkOverlap(s1, e1, s2, e2):
    """Given two sintervals return 0 if no overlap else return size of overlap"""
    return max(0, min(e1, e2)-max(s1, s2))


def getOverlap(s1, e1, s2, e2):
    """Return overlapting segment between two intervals"""
    s3 = max(s1, s2)
    e3 = min(e1, e2)
    return (s3, e3)


def getAdditions(s, e, s1, e1):
    """Given two intervals return the intervals obtained by subtracting interval 2 from 
    interval 1"""
    if s1 <= s and e1 >= e:
        return []
    if s1 < s:
        if e1 >= e:
            return []
        return [(e1, e)]
    else:
        if e1 >= e:
            return [(s, s1)]
    return [(s, s1), (e1, e)]


def createPointers(subsetTree, backupTree, callList):
    print("In Create pointers")
    # loop through all calls
    # callList = sorted(callList, key = lambda x: int(x.data["timestamp"]))
    for call in callList:
        desc = []
        # check for all the overlaps with backup and then remove everything
        # that has a timestamp lesser than it
        curCall = IntervalTree([Interval(call.begin, call.end)])
        callTimestamp = int(call.data["timestamp"])
        backupOverlap = list(sorted(backupTree[call.begin:call.end], key= lambda x: int(x.data["timestamp"])))
        backupOverlap = list(filter( lambda x: False if(int(x.data["timestamp"])<=int(call.data["timestamp"]))  else True,backupOverlap))
        for backupObject in backupOverlap:
            curOverlap = list(curCall[backupObject.begin:backupObject.end])
            for overlap in curOverlap:
                overlapRegion = getOverlap(overlap.begin, overlap.end, backupObject.begin, backupObject.end)
                curCall.chop(overlapRegion[0],overlapRegion[1])
                desc.append({
                    "originalStart": overlapRegion[0],
                    "backup": 1,
                    "offset": (overlapRegion[0]-int(backupObject.data["originalStart"]))+int(backupObject.data["backupStart"]),
                    "size": overlapRegion[1]-overlapRegion[0]
                })
                if len(curCall) == 0:
                    break
            if len(curCall) == 0:
                break
        fileOverlap = subsetTree[call.begin:call.end]
        for subsetObject in fileOverlap:
            curOverlap = list(curCall[subsetObject.begin:subsetObject.end])
            for overlap in curOverlap:
                overlapRegion = getOverlap(overlap.begin, overlap.end, subsetObject.begin, subsetObject.end)
                curCall.chop(overlapRegion[0],overlapRegion[1])
                desc.append({
                    "originalStart": overlapRegion[0],
                    "backup": 0,
                    "offset": subsetObject.data["offsetBackup"]+overlapRegion[0]-subsetObject.begin,
                    "size": overlapRegion[1]-overlapRegion[0]
                })
                if len(curCall) == 0:
                    break
            if len(curCall) == 0:
                    break
        call.data["pointers"]  = desc
    return callList

def createNewRead(subsetData, backupData, callList):
    """Given data from the subsets and versions and all the read calls
    return a list of dictionary where each dictioanry is the composition of those calls
    from the subset and backupdata"""
    ret = []
    index = 0
    for call in callList:
        if(index%1000==0):
            print("Finished {}".format(index))
        index+=1
        callDescriptor = []
        if(int(call["type"]) == 1):
            s = int(call["offset"])
            e = int(call["OpSize"])+s
            offsets = [(s, e)]
            status = 1
            for backup in backupData:
                backS = int(backup["originalStart"])
                backE = int(backup["Size"]) + backS
                i = 0
                while (i < len(offsets)):
                    cur = offsets[i]
                    if checkOverlap(cur[0], cur[1], backS, backE) and int(call["timestamp"]) < int(backup["timestamp"]):
                        oS, oE = getOverlap(cur[0], cur[1], backS, backE)
                        callDescriptor.append({
                            "originalStart": oS,
                            "backup": 1,
                            "offset": (oS-int(backup["originalStart"]))+int(backup["backupStart"]),
                            "size": oE-oS
                        })
                        ext = getAdditions(cur[0], cur[1], oS, oE)
                        del offsets[i]
                        offsets.extend(ext)
                    else:
                        i += 1
            for subset in subsetData:
                i = 0
                while(i < len(offsets)):
                    cur = offsets[i]
                    subS = subset["start"]
                    subE = subset["end"]
                    if checkOverlap(cur[0], cur[1], subS, subE):
                        oS, oE = getOverlap(cur[0], cur[1], subS, subE)
                        totalOff = subset["offsetBackup"]+(oS-subS)
                        size = oE-oS
                        callDescriptor.append({
                            "originalStart": oS,
                            "backup": 0,
                            "offset": totalOff,
                            "size": size
                        })
                    i += 1
            newCall = deepcopy(call)
            callDescriptor.sort(key=lambda x: x["originalStart"])
            newCall["pointers"] = callDescriptor
            ret.append(newCall)
    return ret


def flushToFile(data, filename, pathTrace, size):
    """Flus the new mapping of calls to a file"""
    os.chdir(pathTrace)
    os.chdir("pointers")
    fd = os.open(filename, os.O_RDWR | os.O_CREAT)
    os.write(fd, ("{}\n".format(size)).encode())
    data = sorted(data, key = lambda x: int(x.data["timestamp"]))
    for d in data:
        timestamp = d.data["timestamp"]
        l = len(d.data["pointers"])
        os.write(fd, (str(timestamp)+":"+str(l)+":"+d.data["OpSize"]+"\n").encode())
        for pointer in d.data["pointers"]:
            os.write(fd, ("{}:{}:{}\n".format(
                pointer["backup"], pointer["offset"], pointer["size"])).encode())
    os.close(fd)
    os.chdir("..")


def getBackupList(Lines):
    """Given a list of lines from log file read all the backed up info
    metadata"""
    index = 0
    ret = IntervalTree()
    for i in range(len(Lines)):
        curLine = Lines[i].strip()
        if(curLine == "Backups"):
            index = i
            break
    for i in range(index+1, len(Lines)):
        curLine = Lines[i].strip()
        format_string = "{}:{}:{}:{}"
        parsed = parse(format_string, curLine)
        ret[int(parsed[1]):int(parsed[1])+int(parsed[2])]={
            "backupStart": parsed[0],
            "originalStart": parsed[1],
            "Size": parsed[2],
            "timestamp": parsed[3],
        }
    return ret


def readFromBackup(filename, backupList, pathBackup):
    """Read the backed up data from the backup file"""
    ret = []
    os.chdir(pathBackup)
    fd = os.open(filename, os.O_RDONLY)
    for backup in backupList:
        newBackup = deepcopy(backup)
        os.lseek(fd, int(newBackup["backupStart"]), os.SEEK_SET)
        newBackup["data"] = os.read(fd, int(newBackup["Size"]))
        ret.append(newBackup)
    return ret


def processFiles(pathTrace, pathBackup):
    """Process each file in the given path"""
    for filename in os.listdir(pathTrace):
        if(filename == "subsets" or filename == "pointers"):
            continue
        if os.path.isdir(filename):
            continue
        tic = time.perf_counter()
        Lines = getLines(filename)
        parsed = parse("{}:{}", Lines[0].strip())
        origPath = parsed[0]
        size = parsed[1]
        readLisst = getReadList(Lines)
        callList = getCallList(Lines)
        backupList = getBackupList(Lines)
        read_Data = readFromFile(readLisst, origPath)
        subsetData = storeSubset(read_Data, filename, pathTrace)
        readPointers = createPointers(subsetData, backupList, callList)
        #for item in readPointers:
         #   print(item)
        flushToFile(readPointers, filename, pathTrace, size)
        toc = time.perf_counter()
        print("Time taken to process {} is {}\n".format(filename, toc-tic))

def main():
    pathTrace = sys.argv[1]
    pathBackup =  sys.argv[2]
    os.chdir(pathTrace)
    os.makedirs("pointers", exist_ok=True)
    os.makedirs("subsets", exist_ok=True)
    processFiles(pathTrace, pathBackup)


if __name__ == "__main__":
    main()
