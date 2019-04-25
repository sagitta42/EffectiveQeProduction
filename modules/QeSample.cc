#include "QeSample.hh"

// conversion factor from EQE (hits per event) to RQE (0.0 to 1.0)
const double factorScale = 34.4774833802;
const double factorScaleError = 0.00721240244279;
// no scaling mode
//const double factorScale = 1.;
//const double factorScaleError = 0;
// scaling for concentrators: extracted from MC
const double factorCone = 1.489078384909734;
const double factorConeError = 0.0007875838112872804;


QeSample::QeSample(string week){

    Week = week;

    HoleLabels(); // read the list of hole labels
    ChosenPmts(); // mark which PMTs are B900

    // init

    NeventsTotal = 0;
    NrunsTotal = 0;
    
    for(int h = 0; h < Nholes; h++){
        // dark rate
        DarkHits[h] = 0;
        DarkTime[h] = 0;
        
        // to check how many runs the hole was enabled
        Nruns[h] = 0;

        // from standard calculation (using B900 geo hits)
        // used in MC
        Nevents[h] = 0; // if in the end zero, means was disabled/reference/disconnected the whole week
        NhitsCandle[h] = 0;
        tCandle[h] = 0; // needed for dark hits subtraction

        // using all PMTs: needed for trigger bias correction
        NeventsAllPmts[h] = 0; // will scale for trigger bias based on hits/event, not hits, since can be different
        NhitsAllPmts[h] = 0; //DN subtraction will overwrite 
        tCandleAllPmts[h] = 0; // for dark noise subtraction

        Nhits[h] = 0;
        NhitsError[h] = 0;
        NhitsBias[h] = 0;
        NhitsBiasError[h] = 0;
        NhitsCone[h] = 0;
        NhitsConeError[h] = 0;
    }

    n_changes = 0;
    prev_profile = 0;
}


void QeSample::HoleLabels(){
    // list of hole labels
    ifstream fholes("list_of_all_hole_labels_cone.txt");
    
    if(!fholes){
        cerr << "Error: File list_of_all_hole_labels.txt not found!!" << endl;
        exit(EXIT_FAILURE);
    }

    int hlabel, conc;
    int idx = 0;
    while(fholes >> hlabel >> conc){
        HoleLabel[idx] = hlabel;
        Conc[idx] = conc;
        idx++;
    }
    fholes.close();
}


void QeSample::ChosenPmts(){
    // *** mark which channels correspond to trigger PMTs
    ifstream trg_pmts("B900.txt");
    
    if(!trg_pmts){
        cerr << "Error: File B900.txt not found!!" << endl;
        exit(EXIT_FAILURE);
    }

    int hole;
    while(trg_pmts >> hole){
        is_trigger_pmt[hole] = 1;
    }
    trg_pmts.close();
}


void QeSample::Update(Run* r){
    // get the profile of the run
    NeventsTotal += r->GetNevents(); // total 14C events in a run
    NrunsTotal++; // count this run
    
    // loop over hole labels and sum up the hits from this run
    for(int h = 0; h < Nholes; h++){
        int hole = HoleLabel[h];
        int lg = r->GetChannel(hole);
        // 0 if disabled, reference or not connected to any lg
        int nev = r->GetNevents(lg); 
       
        // count this run if was enabled
        if(nev){
            DarkHits[h] += r->GetNDarkHits(lg);
            DarkTime[h] += r->GetDarkTime(); // will also add if disabled, but we don't care cause we'll ask in the end
            
            Nruns[h]++; 
            
            Nevents[h] += nev;
            NhitsCandle[h] += r->GetNhitsCandle(lg); // simply what comes out from the event
            tCandle[h] += r->GetTCandle(lg);

            NeventsAllPmts[h] += nev;
            NhitsAllPmts[h] += r->GetNhitsAllPmts(lg); // simply what comes out from the event
            tCandleAllPmts[h] += r->GetTCandleAllPmts(lg);
        }
    }

    // add info if new profile ID encountered (or just first run)
    int pid = r->GetProfileID();
    if(prev_profile !=  pid){
        cout << "New profile: " << pid << " (run " << r->GetRunNumber() << ")" << endl;
        ProfileID[n_changes] = pid;
        RunNumber[n_changes] = r->GetRunNumber();
        for(int h = 0; h < Nholes; h++) ChannelID[n_changes][h] = r->GetChannel(HoleLabel[h]);
        n_changes++;
        prev_profile = pid;
    }

}


void QeSample::CalculateQE(){
    // *** subtract dark noise
    DarkNoise();

    //*** calculate trigger bias correction 
    // i would still prefer to do it AFTER the cone scaling, but let's see
    // after dark noise subtraction!
    ChosenPmtBias();

    // *** scale for cones based on average in the week
    ScaleConeMC();

    // we are NOT scaling the cone here, just calculating the ratio from data, so later we can check its evolution
    ConeRatioData();

}


void QeSample::DarkNoise(){
    for(int h = 0; h < Nholes; h++){
        if(Nevents[h]){
            // *** subtract dark noise
            DarkRate[h] = DarkHits[h] * 1. / DarkTime[h];
            double DarkRateErrorSquare = DarkRate[h] / DarkTime[h];
        
            /// standard QE hits
            double NhitsNoise = DarkRate[h] * tCandle[h];
            double NhitsNoiseErrorSquare = DarkRateErrorSquare * pow(tCandle[h],2);
            Nhits[h] = NhitsCandle[h] - NhitsNoise;
            NhitsError[h] = sqrt(NhitsCandle[h] + NhitsNoiseErrorSquare);

            // in all PMTs (needed for trigger bias correction)
            double NhitsNoiseAllPmts = DarkRate[h] * tCandleAllPmts[h];
            double NhitsNoiseAllPmtsErrorSquare = DarkRateErrorSquare * pow(tCandleAllPmts[h],2);
            NhitsAllPmtsError[h] = sqrt(NhitsAllPmts[h] + NhitsNoiseAllPmtsErrorSquare);
            NhitsAllPmts[h] = NhitsAllPmts[h] - NhitsNoiseAllPmts;
        }
    }
}



void QeSample::ChosenPmtBias(){
    /* calculate the ratio for bias correction */
    
    // sum the hits in trigger and non-trigger PMTs
    // using all PMTs to select 14C ("true" ratio)
    double sum_true_B900 = 0;
    double sum_true_B900_error_square = 0;
    double sum_true_nonB900 = 0;
    double sum_true_nonB900_error_square = 0;
    // using chosen PMTs (biased ratio)
    double sum_biased_B900 = 0;
    double sum_biased_B900_error_square = 0;
    double sum_biased_nonB900 = 0;
    double sum_biased_nonB900_error_square = 0;

    for(int h = 0; h < Nholes; h++){
        if(Nevents[h]){
            int hole = HoleLabel[h];
            if(is_trigger_pmt[hole]){
                // summing hits after dark noise subtraction
                sum_true_B900 += NhitsAllPmts[h] / NeventsAllPmts[h];
                sum_true_B900_error_square += pow(NhitsAllPmtsError[h]/NeventsAllPmts[h],2);
                sum_biased_B900 += Nhits[h] / Nevents[h];
                sum_biased_B900_error_square += pow(NhitsError[h]/Nevents[h],2);
            }
            else{
                sum_true_nonB900 += NhitsAllPmts[h] / NeventsAllPmts[h];
                sum_true_nonB900_error_square += pow(NhitsAllPmtsError[h]/NeventsAllPmts[h],2);
                sum_biased_nonB900 += Nhits[h] / Nevents[h];
                sum_biased_nonB900_error_square += pow(NhitsError[h]/Nevents[h],2);
            }
        }
    }

    // rescale such that the new chosen/non-chosen ratio is the same as the "true" one
    // no need to normalize by n_trigger and n_nontrigger because it cancels out anyway
    Ratio_true = sum_true_B900 * 1. / sum_true_nonB900;
    double Ratio_true_error_square_relative = sum_true_B900_error_square / pow(sum_true_B900,2) + sum_true_nonB900_error_square / pow(sum_true_nonB900,2); // true ratio, independent
    Ratio_biased = sum_biased_B900 * 1. / sum_biased_nonB900; // biased ratio
    double Ratio_biased_error_square_relative = sum_biased_B900_error_square / pow(sum_biased_B900,2) + sum_biased_nonB900_error_square / pow(sum_biased_nonB900,2); // true ratio, independent

    for(int h = 0; h < Nholes; h++){
        // if the channel is disabled, NhitsCorrected (and Error) will stay at 0
        if(Nevents[h]){
            int hole = HoleLabel[h];
            if(is_trigger_pmt[hole]){
                NhitsBias[h] = Nhits[h];
                NhitsBiasError[h] = NhitsError[h];
            }
            else{
                if(!Nhits[h]){
                    cout << "Nhits zero: " << HoleLabel[h] << endl;
                    continue;
                }

                NhitsBias[h] = Nhits[h] / Ratio_true * Ratio_biased;
                NhitsBiasError[h] = NhitsBias[h] * sqrt( abs( pow(NhitsError[h]/Nhits[h],2) + Ratio_true_error_square_relative + Ratio_biased_error_square_relative - 2*pow(NhitsError[h],2)/sum_biased_nonB900/Nhits[h] ) ); // complex like this because sum_chosen_nontrigger includes NhitsScaled[lg-1] itself
            }
        }
    }
    
    // saving extra
    Ratio_true_error = sqrt(Ratio_true_error_square_relative) * Ratio_true;
    Ratio_biased_error = sqrt(Ratio_biased_error_square_relative) * Ratio_biased;
    RatioBT = Ratio_biased / Ratio_true;
    RatioBTError = RatioBT * sqrt( Ratio_true_error_square_relative + Ratio_biased_error_square_relative );  
}


// ****** not used, MC scaling used instead
void QeSample::ConeRatioData(){
    double sum_cone = 0;
    double sum_cone_error_square = 0;
    int Ncone = 0;
    double sum_nocone = 0;
    double sum_nocone_error_square = 0;
    int Nnocone = 0;

    // special extra for Alessio, to check evolution in time
    double sum_cone_B900 = 0;
    double sum_cone_B900_error_square = 0;
    int NconeB900 = 0;
    double sum_nocone_B900 = 0;
    double sum_nocone_B900_error_square = 0;
    int NnoconeB900 = 0;
    
    for(int h = 0; h < Nholes; h++){
        int hole = HoleLabel[h];
        if(Nevents[h]){
            // *** scale for cones
            if(Conc[h]){
                sum_cone += NhitsBias[h] / Nevents[h];
                sum_cone_error_square += pow(NhitsBiasError[h] / Nevents[h], 2);
                Ncone++;

                // special extra for Alessio, to check evolution in time
                if(is_trigger_pmt[hole]){
                    sum_cone_B900 += NhitsBias[h] / Nevents[h];
                    sum_cone_B900_error_square += pow(NhitsBiasError[h] / Nevents[h], 2);
                    NconeB900++;
                }
            }
            else{
                sum_nocone += NhitsBias[h] / Nevents[h];
                sum_nocone_error_square += pow(NhitsBiasError[h] / Nevents[h], 2);
                Nnocone++;
                
                // special extra for Alessio, to check evolution in time
                if(is_trigger_pmt[hole]){
                    sum_nocone_B900 += NhitsBias[h] / Nevents[h];
                    sum_nocone_B900_error_square += pow(NhitsBiasError[h] / Nevents[h], 2);
                    NnoconeB900++;
                }
            }
        }
    }


    double avg_cone = sum_cone / Ncone;
    double avg_nocone = sum_nocone / Nnocone;
    // sigma_avg_cone / avg_cone = sigma_sum_cone / sum_cone (Ncone is a constant)
    RatioCNC = avg_cone / avg_nocone;
    RatioCNCError = RatioCNC * sqrt(sum_cone_error_square / pow(sum_cone,2) + sum_nocone_error_square / pow(sum_nocone,2) );
                
    // special extra for Alessio, to check evolution in time 
    double avg_cone_B900 = sum_cone_B900 / NconeB900;
    double avg_nocone_B900 = sum_nocone_B900 / NnoconeB900;
    RatioCNCB900 = avg_cone_B900 / avg_nocone_B900;
    RatioCNCB900Error = RatioCNCB900 * sqrt(sum_cone_B900_error_square / pow(sum_cone_B900,2) + sum_nocone_B900_error_square / pow(sum_nocone_B900,2) );

//    for(int h = 0; h < Nholes; h++){
//        if(Nevents[h]){
//            // *** scale for cones
//            if(Conc[h]){
//                if(!Nhits[h]){
//                    cout << "Nhits zero: " << HoleLabel[h] << endl;
//                    continue;
//                }
//
//                NhitsCone[h] = NhitsBias[h] / RatioCNC;
//                NhitsConeError[h] = NhitsCone[h] * sqrt( abs( pow(NhitsBiasError[h]/NhitsBias[h],2) + sum_cone_error_square / pow(sum_cone,2) + sum_nocone_error_square / pow(sum_nocone,2) -  2*pow(NhitsBiasError[h],2)/sum_cone/NhitsBias[h] ) ); 
//            }
//            else{
//                NhitsCone[h] = NhitsBias[h];
//                NhitsConeError[h] = NhitsBiasError[h];
//            }
//        }
//    }

}


void QeSample::ScaleConeMC(){
    for(int h = 0; h < Nholes; h++){
        if(Nevents[h]){
            // *** scale for cones
            if(Conc[h]){
                NhitsCone[h] = NhitsBias[h] / factorCone;
                NhitsConeError[h] = NhitsCone[h] * sqrt( pow(NhitsBiasError[h]/NhitsBias[h],2) + pow(factorConeError/factorCone,2) );
            }
            else{
                NhitsCone[h] = NhitsBias[h];
                NhitsConeError[h] = NhitsBiasError[h];
            }
        }
    }
}



void QeSample::SaveQE(string output_file){        
    string sep = ","; // separator

    // calculate and save qe
    cout << "--> " << output_file << endl;
    ofstream out(output_file.c_str());
    out << "RunNumber,DST_Index,HoleLabel,ProfileID,ChannelID,Qe,QeError,Status,Noise,QeWoc,QeWocError,NhitsCollected,NhitsCone,NhitsBias,Nevents,NeventsFraction,NrunsFraction" << endl;

    // calculate for each hole ID and save
    for(int h = 0; h < Nholes; h++){
        double Qe = Nevents[h] ? NhitsCone[h] * 1. / Nevents[h] * factorScale: 0; 
        double QeError = Nevents[h] ? Qe * sqrt( NhitsConeError[h]/pow(NhitsCone[h],2) + pow(factorScaleError/factorScale,2) ) : 0; //
        // saturation
        if(Qe > 1.0) Qe = 1.0;

        double QeWoc = Nevents[h] ? NhitsBias[h] * 1. / Nevents[h] * factorScale: 0;
        double QeWocError = Nevents[h] ? sqrt( pow(NhitsBiasError[h]/NhitsBias[h],2) + pow(factorScaleError/factorScale,2) ) / Nevents[h] : 0; //

        int Status = Nevents[h] ? 1 : 2; // for now only "enabled" and "disabled" with no "discarded"


        for(int ch = 0; ch < n_changes; ch++){
            out << RunNumber[ch]
                << sep << Week
                << sep << HoleLabel[h]
                << sep << ProfileID[ch]
                << sep << ChannelID[ch][h]
                << sep << Qe
                << sep << QeError
                << sep << Status
                << sep << DarkRate[h]
                << sep << QeWoc
                << sep << QeWocError
                << sep << NhitsCandle[h]
                << sep << NhitsCone[h]
                << sep << NhitsBias[h]
                << sep << Nevents[h]
                << sep << Nevents[h]*1. / NeventsTotal
                << sep << Nruns[h]*1. / NrunsTotal
                << endl; 
        }
    }

    out.close();
    
}


void QeSample::SaveExtra(string qeextra){
    // extra once per week
    ofstream extra(qeextra.c_str());
    extra << "Week,Ratio_true,Ratio_true_error,Ratio_biased,Ratio_biased_error,RatioBT,RatioBTError,RatioCNC,RatioCNCError,RatioCNCB900,RatioCNCB900Error" << endl;
    string sep = ",";
    extra << Week
        << sep << Ratio_true
        << sep << Ratio_true_error
        << sep << Ratio_biased
        << sep << Ratio_biased_error
        << sep << RatioBT
        << sep << RatioBTError
        << sep << RatioCNC
        << sep << RatioCNCError
        << sep << RatioCNCB900
        << sep << RatioCNCB900Error
        << endl;

    extra.close();
}
