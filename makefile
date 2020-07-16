# *****************************************************
# Variables to control Makefile operation

CXX = g++
CXXFLAGS = -Wall -O2 `root-config --cflags --glibs` /storage/gpfs_data/borexino/users/alessre/offline_RefVersions/offline_c19/Echidna/rootechidna.so
#CXXFLAGS = -Wall -O2 `root-config --cflags --glibs` /storage/gpfs_data/borexino/users/mredchuk/offline_RefVersions/offline_c19/Echidna/rootechidna.so

MPATH=modules
INCLUDES= -I$(MPATH) # so that #include in main.cc looks for the files in this folder

# ****************************************************
# Targets needed to bring the executable up to date

qe_calculation: qe_calculation.o $(MPATH)/Database.o $(MPATH)/Run.o $(MPATH)/QeSample.o
	$(CXX) $(INCLUDES) $(CXXFLAGS) -o qe_calculation qe_calculation.o $(MPATH)/Database.o $(MPATH)/Run.o $(MPATH)/QeSample.o

# The main.o target can be written more simply

qe_calculation.o: qe_calculation.cc $(MPATH)/Database.hh $(MPATH)/Run.hh $(MPATH)/QeSample.hh
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c qe_calculation.cc

$(MPATH)/Database.o: $(MPATH)/Database.hh

$(MPATH)/Run.o: $(MPATH)/Run.hh $(MPATH)/Database.hh

$(MPATH)/QeSample.o: $(MPATH)/QeSample.hh $(MPATH)/Run.hh $(MPATH)/Database.hh
