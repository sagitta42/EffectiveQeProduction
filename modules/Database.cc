#include "Database.hh"

using namespace std;

Database::Database(){
    db = TSQLServer::Connect("pgsql://bxdb.lngs.infn.it/bx_calib","borex_guest","xyz");
    dbrun = TSQLServer::Connect("pgsql://bxdb.lngs.infn.it/bx_runvalidation","borex_guest","xyz");
}

Database::~Database(){
    db->Close();
    dbrun->Close();
    delete db;
    delete dbrun;
}

int Database::LastValidEvnum(int runnum){
    // --- find out last valid event in this run
    if (!dbrun){
        cerr << "Error connecting to DB bx_runvalidation"  << endl;
        dbrun->Close();
        exit(EXIT_FAILURE);
    }
    
    char query[100];
    sprintf(query, "select \"LastValidEvent\" from \"ValidRuns\" where \"RunNumber\" = %i", runnum);
    TSQLResult* result = dbrun->Query(query);
    if (!result){
        cerr << "Error querying ValidRuns" << endl;
        dbrun->Close();
        exit(EXIT_FAILURE);
    }
    
    if( result->GetRowCount() == 1) {
        TSQLRow* row = result->Next();
        return atol(row->GetField(0));
    }
    else{
        cerr << "Error: ValidRuns LastValidEvent has 0 rows or > 1 row" << endl;
        dbrun->Close();
        exit(EXIT_FAILURE);
    }
}

