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
        Run(string fname);
        ~Run();
        
        void Geometry(); // get hole label mapping from profileMaps
        void DisabledChannels(Database* d); // get disabled channels from db
        void CollectHits(double radius); // actual calculation
        void ScaleCone(); // scale for cones after the calculation
        void CorrectBias(); // correct for the trigger PMT bias
        void Save(ofstream& out, ofstream& extra); // writing to file
        
        // ------ getters
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
        int HoleLabel[2240]; // map of channel to hole label
        map<int, int> ChannelID; // map of hole label to channel id
        bool Conc[2240]; // in principle, it's bool; have a concentrator or not
        bool is_trigger_channel[2240]; // a bool that tells if a given channel is a trigger PMT or not 
        int n_enabled_trigger_channels; // how many of the trigger PMTs themselves were enabled
        bool Disabled[2240];
        bool Reference[2240]; // 0 --> ordinary channel, 1 --> reference channel
        int NLivePmts;
        int NLivePmtsManual;
        int NLivePmtsA1000;

        // --- QE calculation related
        
        // dark rate
        int NeventsTT64;
        int NhitsTT64[2240]; // hits in TT64 trigger in each channel throughout the run
        double darkRate[2240]; // per ns, NhitsTT64 / NeventsTT64 / gate
        double darkRateError[2240]; 
        
        // candle
        int NeventsCandle;
        double NhitsTriggerPmts; // sum of the hits that decide of event is 14C in this run
        double NhitsGeo; // sum of geonorm hits of each 14C event
        double NhitsNorm; // sum of norm hits of each 14C event

        int NhitsCandle[2240]; // 14C hits in each channel throughout the run, the sum of the hits in each 14C event 
        double tCandle[2240]; // [ns] sum of cluster durations of each 14C event
        double Nhits[2240]; // NhitsCandle - NhitsDark
        double NhitsError[2240]; // NhitsCandle - NhitsDark

        // scaling for cone
        double NhitsScaled[2240]; // scaled accounting for cones 
        double NhitsScaledError[2240]; // associated error
        double NhitsWoc[2240];
        // needed for QeWoc error propagation
        double RatioCNC; // ratio Nhits of PMTs with cone to without cone 
        double RatioCNCAllPmts; // ratio Nhits of PMTs with coneto without cone based on hits from allgeo selection

        // bias correction
        double NhitsCorrected[2240]; // after bias correction
        double NhitsCorrectedError[2240]; // associated error
        double NhitsAllPmts[2240]; // similar to NhitsCandle, but when using all PMTs to select 14C
        double NhitsAllPmtsError[2240]; // associated error of the hits above after scaling for cones
        double tCandleAllPmts[2240];
        int NeventsAllPmts; // similar to NeventsCandle, but when using all PMTs
        double RatioTNT; // ratio NhitsScaled of trigger to non-trigger PMTs using trigger PMTs to select 14C ("biased ratio")
        double RatioTNTAllPmts; // ratio NhitsScaled of trigger to non-trigger PMTs using all PMTs to select 14C ("true ratio")
        
        
};

#endif
