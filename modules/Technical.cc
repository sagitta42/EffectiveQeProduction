#include <iostream>

using namespace std;

int Profile(int runnum){
    /*
     * Determine which profile the run number belogs to. Used in channel to hole mapping.
     * Info taken from table "Run" in database daq_config
     */

    if(runnum >= 29090) return 25;
    else if(runnum >= 29059) return 24;
    else if(runnum >= 29010) return 23;
    else if(runnum >= 28928) return 22;
    else if(runnum >= 28884) return 21;
    else if(runnum >= 28740) return 20;
    else if(runnum >= 18000) return 19;
    else if(runnum >= 12001) return 18;
    else if(runnum >= 6413) return 17;
    else if(runnum >= 5542) return 16;
    else if(runnum >= 3231) return 15;
    else{
        cerr << "Error: Run number " << runnum << " < 3231 is not supported! [Technical.C]" << endl;
        exit(EXIT_FAILURE);
    }
}


double GateLength(int runnum){
    /*
     * Return gate length in ns.
     * Info taken from table "TriggerParameters" in database bx_calib
     */

    if(runnum >= 6545) return 16500;
    else if(runnum >= 6443) return 6880;
    else if(runnum >= 6395) return 16500;
    else if(runnum >= 6167) return 6880;
    else if(runnum >= 6165) return 16500;
    else if(runnum >= 5281) return 6880;
    else if(runnum >= 5000) return 6873; // ab run 3398, but we only consider > 5000
    else{
        cerr << "Error: Run number " << runnum << " < 5000 is not supported! [Technical.C]" << endl;
        exit(EXIT_FAILURE);
    }
}


bool IsCandleCandidate(BxEvent* ev, double rad){
    /* Determine if the event is a possible candle event (14C in given radius). Conditions:
     * - TT1BTB0
     * - 1 cluster
     * - r < rad
     */
    
    if(ev->GetTrigger().GetTrgType() != 1) return false;
    if(ev->GetTrigger().GetBtbInputs() != 0) return false;
    
    const BxLaben& laben = ev->GetLaben();    

    if(laben.GetNClusters() != 1) return false;
    
    double x, y, z, r;
    const BxLabenCluster& cluster = laben.GetCluster(0);
    x = cluster.GetPositionLNGS().GetX();
    y = cluster.GetPositionLNGS().GetY();
    z = cluster.GetPositionLNGS().GetZ();
    r = sqrt( pow(x,2) + pow(y,2) + pow(z,2) );

    return r < rad;
}


bool IsCandleAllPmts(BxEvent* ev){
    /* !! Apply after the candidate is already a possible candle event i.e. IsCandleCandidate() returned true!!
     * Determine if the event is a candle event (14C in given radius) as judged by all PMTs. Condition:
     * - 50 < geonorm hits < 100
     * Return the number of hits (used for analysing the selected candle events)
     */
    
    const BxLaben& laben = ev->GetLaben();    

    // simple condition: all geo hits
    double nhits = laben.GetCluster(0).Normalize_geo_Pmts(laben.GetNClusteredHits());
    return (nhits < 100) && (nhits >= 50);
}



double IsCandleA1000(BxEvent* ev){
    /* Determine if the event is a 14C event according to all PMTs
     * 14C: TT1BTB0, 1 cluster
     * 50 < geonorm hits in B900 built-in < 100
     * r < rad
     */
    
    const BxLaben& laben = ev->GetLaben();    
    const BxLabenCluster& cluster = laben.GetCluster(0);
    
    // simple condition: all geo hits
    double nhits = laben.GetCluster(0).Normalize_geo_Pmts_A1000(cluster.GetNHitsA1000());
    return nhits < 100 && nhits >= 50 ? nhits : 0;

}


