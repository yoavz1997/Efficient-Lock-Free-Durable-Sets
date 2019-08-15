# importing the required module
import matplotlib.pyplot as plt
# import numpy as np
from functools import reduce
import argparse
import os

def extractXaxis(filename):
    file = open(filename, "r")
    xAxis = []
    for line in file:
        if not line[0].isalpha():
            continue
        xPoint = int(line.split(":")[1].split(" ")[1])
        xAxis.append(xPoint)
    file.close()
    return xAxis


def extractYaxis(filename):
    file = open(filename, "r")
    yAxis = []
    avg = []
    for line in file:
        if line[0].isalpha():
            if len(avg) == 0:
                continue
            num = len(avg)
            yAxis.append((reduce(lambda x, y: x + y, avg) / num) / 1000)
            avg = []
        else:
            avg.append(float(line.split("\n")[0]))
    if len(avg) != 0:
        num = len(avg)
        yAxis.append((sum(avg) / num)/ 1000)
    file.close()
    return yAxis

def getColor(algoName):
    if algoName.startswith("LinkFree"):
        return ('#bf00ff')
    elif algoName.startswith('SOFT'):
        return ('#ff5800')
    return ('#000000')

def getLabel(filename):
    return filename.split("/")[-1].split("-")[0]

def getMarker(algoName):
    if algoName.startswith("LinkFree"):
        return 'o'
    elif algoName.startswith('SOFT'):
        return 's'
    return 'x'

def getRawTitle(filename):
    filenameNoExtension = filename.split("/")[-1].split(".")[0]
    p1 = filenameNoExtension.split("-")[2]
    p2 = filenameNoExtension.split("-")[4]
    return p1, p2


def getTitle(filename, testNum):
    p1, p2 = getRawTitle(filename)
    if testNum == 1:
        return str(p1) + '% reads, with ' + str(p2) + ' keys'
    elif testNum == 2:
        return str(p1) + '% reads, with ' + str(p2) + ' threads'
    elif testNum == 3:
        return str(p2) + ' threads, with ' + str(p1) + ' keys'
    return 'default string'
    
def getDS(algoName):
    if algoName.endswith("SkipList"):
        return 'sl'
    elif algoName.endswith("List"):
        return 'list'
    elif algoName.endswith("Table"):
        return 'hash'

parser = argparse.ArgumentParser(description='Proccess test results and print a graph')
parser.add_argument('-T', metavar='testnum', help='The Number of the Test (1..3)')
parser.add_argument('-D', metavar='dir', help='The directory with the result files')

args = vars(parser.parse_args())
for _, value in args.items():
	if value is None:
		parser.print_help()
		exit()

resultsDir = args['D']
testNum = int(args['T'])
graphsDir = resultsDir + '/../'
dsName = ''
for filename in os.listdir(resultsDir):
    if not filename.endswith('.txt'):
        continue
    x = extractXaxis(resultsDir + '/' + filename)
    y = extractYaxis(resultsDir + '/' + filename)
    algoName = getLabel(filename)
    plt.plot(x, y, label=algoName, marker=getMarker(algoName), color=getColor(algoName))
    plt.title(getTitle(filename, testNum))
    p1, p2 = getRawTitle(filename)
    dsName = getDS(algoName)
    
if testNum == 1:
  plt.xlabel("#Threads")
elif testNum == 2:
  plt.xlabel("Key Range")
  plt.xscale("log")
elif testNum == 3:
  plt.xlabel("%Reads")
plt.ylabel("#Operations per ms [K]")

plt.legend()

if not os.path.exists(graphsDir):
    os.mkdir(graphsDir)
plt.savefig(
    graphsDir + "/test" + str(testNum) + "-" + dsName + "-" + str(p1) + "-" + str(p2) + ".png", dpi=700)
# plt.show()
