#include "Database.hh"
#include "Run.hh"
#include "QeSample.hh"

using namespace std;

void QEcalculation(string week, string input_folder, string output_folder);

const double radius = 2;

int main(int argc, char* argv[]){
	if(argc != 4){
		cout << "Format ./qe_calculation week input_folder output_folder" << endl;
		cout << "week: YYYY_MMM_DD" << endl;
		cout << "input_folder: folder in which YYYY_MMM_DD.list is stored" << endl;
		cout << "output_folder: where the outputs *_QePerRun.txt, *_RunInfo.txt and *_QE.txt will go" << endl;
		return 1;
	}

	string week = argv[1]; // list of Echidna runs
	string input_folder = argv[2];
	string output_folder = argv[3];

	QEcalculation(week, input_folder, output_folder);
}



//void QEcalculation(string flistname, string trigger_pmts_folder, int numchan, string triggerids){
void QEcalculation(string week, string input_folder, string output_folder){
	cout << "Calculating with radius: " << radius << " m" << endl;

	string flistpath = input_folder + "/" + week + ".list";
	cout << flistpath << endl;
	ifstream flist(flistpath.c_str()); // list of Echidna runs

	// final output for QE
	string qefile = output_folder + "/" + week + "_QE.txt";
	cout << "--> " << qefile << endl;

	// info for the time window
	QeSample* s = new QeSample(week);
	// database
	Database* d = new Database(); // opens DB
	string fname;
	
	while(flist >> fname){
		Run* r = new Run(fname);
		
		// read the disabled channels in this run from the database; need alredy at this stage to account for it when collecting hits on the trigger pmts
		r->DisabledChannels(d); // for now has to be done before Geometry, since in Geom I count n enabled trigger PMTs

		// get current channel-hole-conc mapping, and A1000 channels; disabled channels info (need already while collecting hits in RunClassNorm)
		r->Geometry();

		// loop through each event, determine if it's a 14C event, if yes, collect the hits; also calculate dark rate and dark hits 
		r->CollectHits(radius);

		// update the total
		s->Update(r);
		
		delete r;
	}

	cout << "DONE" << endl;
	flist.close();
	delete d;

    s->CalculateQE(); // dark noise, then chosen PMT bias, then cones
    s->SaveQE(qefile);
    
    // weekly extra: not save in "official" mode
    string qeextra = output_folder + "/" + week + "_Week.txt";
	cout << "--> " << qeextra << endl;
    s->SaveExtra(qeextra);
	delete s;
	
    cout << "SAVED" << endl;
}

