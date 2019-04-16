#ifndef _QESAMPLE_H
#define _QESAMPLE_H

#include "Run.hh"

const int Nholes = 2212;

class QeSample{
    public:
        QeSample(string week);
        ~QeSample(){};

        void HoleLabels();
        void ChosenPmts();

        void DarkNoise();
        void ChosenPmtBias();
        void ScaleCone();

        void Update(Run* r); // update hits and Nevents seen by each hole label
        void CalculateQE();
        void SaveQE(string output_file);
        void SaveExtra(string qeextra);

    private:
        string Week;

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
        double NhitsBias[Nholes]; // trigger bias corrected
        double NhitsBiasError[Nholes];
        double NhitsCone[Nholes]; // hits after correcting for cones
        double NhitsConeError[Nholes]; 
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

        int n_changes; // how many changes there were of profileID
        int prev_profile;
        int RunNumber[25]; // [0]: first run number of the week, next: if profile changes
        int ProfileID[25]; // profile ID corr to RunNumber
        int ChannelID[25][Nholes]; // channel ID corr. to each hole label; will store info for each profile ID here

};

#endif
