#include "Run.hh"
#include "Technical.C"

// ----------------------- constructor/destructor ----------------------- //


Run::Run(string fname){
    f = TFile::Open(fname.c_str());
    f->GetObject("bxtree",t);
    ev = new BxEvent();
    t->SetBranchAddress("events", &ev);
    t->GetEntry(0);
    runnum = ev->GetRun();
    int l = fname.size();
    cycle = fname.substr(l - 7, 2);
    profileID = Profile(runnum);
    tGate = GateLength(runnum);
    cout << "~~~ Run " << runnum << " | Cycle " << cycle << " | Profile " << profileID << "  | Gate length " << tGate << endl;
    
    // --- init
    NeventsTT64 = 0;
    NeventsCandle = 0;
    NeventsAllPmts = 0;
    NhitsTriggerPmts = 0;
    NhitsGeo = 0;
    NhitsNorm = 0;
    NLivePmtsManual = 0;

    n_enabled_trigger_channels = 0;

    
    for(int lg = 1; lg <= 2240; lg++){
        Disabled[lg-1] = 0; // by default the channel is enabled
        Reference[lg-1] = 0; // by default the channel is not a reference channel
        is_trigger_channel[lg-1] = 0; // by deafult a channel is not a trigger channel
    
        NhitsTT64[lg-1] = 0;
        
        NhitsCandle[lg-1] = 0;
        tCandle[lg-1] = 0; // ns

        NhitsAllPmts[lg-1] = 0; // collected hits; later scaled hits (overwrite)
        tCandleAllPmts[lg-1] = 0; // ns
        
    }
    
}


Run::~Run(){
    delete t;
    delete f;
    delete ev;
}



// ----------------------- calculations ----------------------- //

void Run::CollectHits(double radius){
    /* main function to calculate QE */

    int numevents = t->GetEntries();

    for(int evnum = 1001; evnum < numevents; evnum++){
        // occasional printout
        if(evnum % 100000 == 0) cout << evnum << "/" << numevents << endl;
        
        t->GetEntry(evnum);
        const BxLaben& laben = ev->GetLaben();
        int numDecHits = laben.GetNDecodedHits();
        vector<BxLabenDecodedHit> decHits = laben.GetDecodedHits();

        // dark rate
        if(ev->GetTrigger().GetTrgType() == 64 && laben.GetNClusters() == 0){
            NeventsTT64++;
            for(int hit = 0; hit < numDecHits; hit++){
                int lg = decHits[hit].GetLg();
                NhitsTT64[lg-1]++; 
            }
            continue;
        }

        // candidate for either chosen PMTs or all PMTs
        if(!IsCandleCandidate(ev,radius)) continue;

        // candle A1000 geo
//      double nhits_trigger_pmts = IsCandleA1000norm(ev, radius); // if event is not accepted, zero is returned; if it is, the hits that decided it is are returned
        double nhits_trigger_pmts = IsCandleA1000(ev); // if event is not accepted, zero is returned; if it is, the hits that decided it is are returned
        
        if(nhits_trigger_pmts){
            NeventsCandle++;
            NhitsTriggerPmts += nhits_trigger_pmts;
            NhitsGeo += laben.GetCluster(0).Normalize_geo_Pmts(laben.GetNClusteredHits());
            NhitsNorm += laben.NormalizePmts(laben.GetNClusteredHits());
            
            for(int hit = 0; hit < numDecHits; hit++){
                if(decHits[hit].GetNumCluster() == 1){
                    int lg = decHits[hit].GetLg();
                    NhitsCandle[lg-1]++;
                    tCandle[lg-1] += laben.GetClusters()[0].GetDuration(); // ns
                }
            }
        }

        // bias correctioni data: if the event is 14C according to all PMTs
        if(IsCandleAllPmts(ev)){
            NeventsAllPmts++;
            for(int hit = 0; hit < numDecHits; hit++){
                if(decHits[hit].GetNumCluster() == 1){
                    int lg = decHits[hit].GetLg();
                    NhitsAllPmts[lg-1]++;
                    tCandleAllPmts[lg-1] += laben.GetClusters()[0].GetDuration(); // ns
                }
            }
        }
        

    } // end of for loop on events 

    // live PMTs
    NLivePmts = ev->GetLaben().GetNLivePmts();
    NLivePmtsA1000 = ev->GetLaben().GetNLivePmtsA1000();

}





// ----------------------- geometry  ----------------------- //


void Run::Geometry(){

    // *** map channel to hole label
    char filename[100];
    sprintf(filename, "profile_channel_to_hole/profile%imap.txt",profileID);
    ifstream fmap(filename);

    if(!fmap){
        cerr << "Error: File " << filename << " not found!!" << endl;
        exit(EXIT_FAILURE);
    }
    
    int lg, hole, conc = 0;
    
    while (fmap >> lg >> hole >> conc){
        HoleLabel[lg-1] = hole;
        ChannelID[hole] = lg; // if hole is not mapped to any channel, will be zero (default in map)
        Conc[lg-1] = conc;
        // if hole label = 0, mark as disabled immediately
        if(!hole) Disabled[lg-1] = 1;
    }
    fmap.close();

    // *** mark which channels are reference channels
    sprintf(filename, "reference_channels/reference_channels%i.txt", profileID);
    ifstream ref(filename);
    
    if(!ref){
        cerr << "Error: File " << filename << " not found!!" << endl;
        exit(EXIT_FAILURE);
    }
    
    while(ref >> lg) Reference[lg-1] = 1;
    ref.close();

    
    // live channels
    for(int lg = 1; lg < 2241; lg++){
        if(!Disabled[lg-1] && !Reference[lg-1]) NLivePmtsManual++;
    }

}

// ----------------------- electronics  ----------------------- //

void Run::DisabledChannels(Database* d){
    if (!d->db){
        cerr << "Error connecting to bx_calib" << endl;
        delete d;
        exit(EXIT_FAILURE);
    }
    
    // last valid evnum
    int last_valid_evnum = d->LastValidEvnum(runnum);
    
    char query[200];
    sprintf(query, "select \"ChannelID\" from \"DisabledChannels\" where \"Cycle\" = %s and \"RunNumber\" = %i and \"Timing\" = 1 and \"EvNum\" < %i", cycle.c_str(), runnum, last_valid_evnum);
    
    TSQLResult* result = d->db->Query(query);
    
    if (!result){
        cerr << "Error querying DisabledChannels" << endl;
        delete d;
        exit(EXIT_FAILURE);
    }

    int nrows = result->GetRowCount();

    if(!nrows){
        cerr << "Error: DisabledChannels has 0 rows" << endl;
        delete d;
        exit(EXIT_FAILURE);
    }

    cout << nrows << " disabled channels" << endl;
    for(int i = 0; i < nrows; i++){
        TSQLRow* row = result->Next();
        // lg of a disabled channel
        int lg = atol(row->GetField(0));
        Disabled[lg-1] = 1;
    }

}




// ---------------------------------------------------------------- //

void Run::Save(ofstream& out, ofstream& extra){
    /* save calculations for every lg into the output file */

    for(int lg = 1; lg < 2241; lg++){
        out << runnum << " "
            << lg << " "
            << HoleLabel[lg-1] << " "
            << Conc[lg-1] << " "
            << Disabled[lg-1] << " "
            << Reference[lg-1] << " "
            << NhitsCandle[lg-1] << " "
            << darkRate[lg-1] << " "
            << tCandle[lg-1] << " "
            << Nhits[lg-1] << " "
            << NhitsScaled[lg-1] << " "
            << NhitsScaledError[lg-1] << " "
            << NhitsCorrected[lg-1] << " "
            << NhitsCorrectedError[lg-1] << endl;
    }

    extra << runnum
            << " " << NeventsCandle
            << " " << NhitsTriggerPmts
            << " " << NhitsGeo
            << " " << NhitsNorm
            << " " << RatioCNC
            << " " << RatioCNCAllPmts
            << " " << RatioTNT
            << " " << RatioTNTAllPmts
            << " " << NeventsAllPmts
            << " " << NLivePmts
            << " " << NLivePmtsManual
            << " " << NLivePmtsA1000
            << " " << n_enabled_trigger_channels
            << endl;
}

// ----------------------- getters ----------------------- //


int Run::GetNevents(int lg){
    return !lg || Disabled[lg-1] || Reference[lg-1] ? 0 : NeventsCandle;
}
