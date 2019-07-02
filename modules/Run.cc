#include "Run.hh"
#include "Technical.cc"

// ----------------------------------------------------------------------------- //


Run::Run(string fname){
    f = TFile::Open(fname.c_str());

    if(!f){
        cerr << "Run " << fname << " not found!" << endl;
        exit(EXIT_FAILURE);
    }

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
    NLivePmtsManual = 0;

    for(int lg = 1; lg <= 2240; lg++){
        Disabled[lg-1] = 0; // by default the channel is enabled
        Reference[lg-1] = 0; // by default the channel is not a reference channel
    
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


// ----------------------------------------------------------------------------- //


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
        double nhits_trigger_pmts = IsCandleA1000(ev); // if event is not accepted, zero is returned; if it is, the hits that decided it is are returned
        
        if(nhits_trigger_pmts){
            NeventsCandle++;
            
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

    // if the channel was enabled but collected zero hits, it's a mistake, it is disabled but missing in the database
    cout << "DisabledChannels bug check..." << endl;
    for(int lg = 1; lg < 2241; lg++){
        if(NhitsCandle[lg-1] == 0 && !(Disabled[lg-1] || Reference[lg-1])){
            cout << "Lg " << lg << " wrongly marked as enabled" << endl;
            Disabled[lg-1] = 1;
        }
    }
}


// ----------------------------------------------------------------------------- //


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
        ChannelID[hole] = lg; // if hole is not mapped to any channel, will be zero (default in map)
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

// ----------------------------------------------------------------------------- //

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
        int lg = atoi(row->GetField(0));
        Disabled[lg-1] = 1;
    }

}


// ----------------------------------------------------------------------------- //


int Run::GetNevents(int lg){
    return !lg || Disabled[lg-1] || Reference[lg-1] ? 0 : NeventsCandle;
}
