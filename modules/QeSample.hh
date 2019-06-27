#ifndef _QESAMPLE_H
#define _QESAMPLE_H

#include "Database.hh"
#include "Run.hh"

const int Nholes = 2212;

class QeSample{
    public:
        QeSample(string week, int run_min=0, int run_max=0);
        ~QeSample(){};

        void HoleLabels();
        void ChosenPmts();

        void CalculateQE(Database* d); // calculation with the steps in the methods below:

        void DarkNoise(); // subtract dark noise hits
        void ChosenPmtBias(); // correct for trigger PMT bias
        void ScaleConeMC(); // scale according to cone presence 
        void ScaleQe(); // scale to relative QE from 0.0 to 1.0
        void DiscardPmts(Database* d); // assign status (enabled, disabled, discarded) and value from prev week
        
        void ConeRatioData(); // not used now, used only to get cone to no cone ratio (optional)

        void Update(Run* r); // update hits and Nevents seen by each hole label
        void SaveQE(string output_file);
        void SaveExtra(string qeextra);

    private:
        string Week;
        int rmin, rmax; // 0 in case of a normal week, defined in case of a stretched week

        int HoleLabel[Nholes]; // each hole label has an index from 0 to Nholes in the array
        int Conc[Nholes];
        map<int,int> is_trigger_pmt; // bool didn't work for map somehow
        
        int NeventsTotal; // total number of events in all runs in this time window
        int NrunsTotal; // total number of runs in this time window
        
        int DarkHits[Nholes]; // sum of dark hits collected over all runs in the week
        double DarkTime[Nholes]; // sum of (NdarkEvents * tGate) from all runs
        double DarkRate[Nholes];
        
        int Nruns[Nholes]; // how many runs each hole was enabled
        
        // params corresponding to each HoleID
        int Nevents[Nholes]; // sum of number of events in each run in which given PMT was enabled
        int NhitsCandle[Nholes]; // simply what laben spits out
        double tCandle[Nholes]; // total summed time of 14C clusters (when enabled)
        double Nhits[Nholes]; // subtracting dark noise
        double NhitsError[Nholes]; 
        double Qe[Nholes];
        double QeError[Nholes];
        double QeBias[Nholes];// after bias correction
        double QeBiasError[Nholes];
        double QeCone[Nholes];// after cone scaling
        double QeConeError[Nholes]; 
        double QeFinal[Nholes]; // after scaling to relative QE between 0.0 and 1.0
        double QeFinalError[Nholes]; 
        double QeWoc[Nholes]; // relative scale, but without cone correction
        double QeWocError[Nholes]; 
        int Status[Nholes]; // 1 = enabled, 2 = disabled, 3 = discarded
        
        // extra
        double Ratio_true;
        double Ratio_true_error;
        double Ratio_biased;
        double Ratio_biased_error;
        double RatioBT;
        double RatioBTError;
        double RatioCNC;
        double RatioCNCError;
        // special extra for Alessio, to check evolution in time 
        double RatioCNCB900;
        double RatioCNCB900Error;
        
        // chosen PMT bias correction ("trigger bias")
        int NeventsAllPmts[Nholes];
        double NhitsAllPmts[Nholes]; // needed for the "true" ratio
        double NhitsAllPmtsError[Nholes]; // needed for the "true" ratio
        double tCandleAllPmts[Nholes]; // total summed time of 14C clusters (when enabled)
        double QeAllPmts[Nholes];
        double QeAllPmtsError[Nholes];

        int n_changes; // how many changes there were of profileID
        int prev_profile;
        int RunNumber[25]; // [0]: first run number of the week, next: if profile changes
        int ProfileID[25]; // profile ID corr to RunNumber
        int ChannelID[25][Nholes]; // channel ID corr. to each hole label; will store info for each profile ID here

};

#endif
