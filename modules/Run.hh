/* Calculation of QE(lg, run) */
#include <fstream>
#include <string>
#include <vector>
#include <cmath> // forgot what for
#include "TFile.h"
#include "TTree.h"
#include "Database.hh"

#include "/storage/gpfs_data/borexino/users/mredchuk/offline_RefVersions/offline_c19/Echidna/event/BxEvent.hh"

using namespace std;

//TSQLServer* db; // does not work because Run.hh is included "twice" in QeSample and main

#ifndef _RUN_H
#define _RUN_H

class Run{
    public:
        // --------------------------------
        Run(string fname);
        ~Run();
        // --------------------------------
        void Geometry(); // get hole label mapping from profileMaps
        void DisabledChannels(Database* d); // get disabled channels from db
        void CollectHits(double radius); // actual calculation
        // -------------------------------- getters
        int GetRunNumber(){ return runnum; }
        int GetProfileID(){ return profileID; }
        
        int GetNevents(){ return NeventsCandle; } // total in the run
    
        int GetChannel(int hole){ return ChannelID[hole]; }
        int GetNevents(int lg);
        
        int GetNDarkHits(int lg){ return NhitsTT64[lg-1]; }
        double GetDarkTime(){ return NeventsTT64 * tGate; } // sum of NDarkEvents * tGate for given PMT
        
        int GetNhitsCandle(int lg){ return NhitsCandle[lg-1]; }
        double GetTCandle(int lg) { return tCandle[lg-1]; }
        
        double GetNhitsAllPmts(int lg) { return NhitsAllPmts[lg-1]; } // only needed if bias correction is at the end of the time window (currently at the end of run)
        double GetTCandleAllPmts(int lg) { return tCandleAllPmts[lg-1]; }



    private:
        // --- run related
        int runnum;
        int profileID;
        string cycle;
        TFile* f;
        TTree* t;
        BxEvent* ev;
        double tGate; // gate length in ns
        
        // -- info about the PMTs
        map<int, int> ChannelID; // map of hole label to channel id
        bool Disabled[2240];
        bool Reference[2240]; // 0 --> ordinary channel, 1 --> reference channel
        int NLivePmts;
        int NLivePmtsManual;
        int NLivePmtsA1000;

        // --- QE calculation related
        
        // dark rate
        int NeventsTT64;
        int NhitsTT64[2240]; // hits in TT64 trigger in each channel throughout the run
        
        // candle
        int NeventsCandle;

        int NhitsCandle[2240]; // 14C hits in each channel throughout the run, the sum of the hits in each 14C event 
        double tCandle[2240]; // [ns] sum of cluster durations of each 14C event

        // all PMTs, needed later for trigger bias correction
        double NhitsAllPmts[2240]; // similar to NhitsCandle, but when using all PMTs to select 14C
        double tCandleAllPmts[2240];
        int NeventsAllPmts; // similar to NeventsCandle, but when using all PMTs
        
        
};

#endif
