#include "Database.hh"
#include "Run.hh"
#include "QeSample.hh"

using namespace std;

void QEcalculation(string week, string input_folder, string output_folder, int rmin, int rmax);

const double radius = 2;

int main(int argc, char* argv[]){
	if(argc != 4 && argc != 6){
		cout << "Format ./qe_calculation week input_folder output_folder" << endl;
		cout << "Or ./qe_calculation week input_folder output_folder run_min run_max" << endl;
		cout << "week: YYYY_MMM_DD" << endl;
		cout << "input_folder: folder in which YYYY_MMM_DD.list is stored" << endl;
		cout << "output_folder: where the outputs *_QePerRun.txt, *_RunInfo.txt and *_QE.txt will go" << endl;
        cout << "run_min and run_max: definition of the 'boundaries' of the week (in case of a stretched week)" << endl;
		return 1;
	}

	string week = argv[1]; // list of Echidna runs
	string input_folder = argv[2];
	string output_folder = argv[3];
    int rmin = argc > 4 ? atoi(argv[4]) : 0;
    int rmax = argc > 4 ? atoi(argv[5]) : 0;

	QEcalculation(week, input_folder, output_folder, rmin, rmax);
}



//void QEcalculation(string flistname, string trigger_pmts_folder, int numchan, string triggerids){
void QEcalculation(string week, string input_folder, string output_folder, int rmin, int rmax){
	cout << "Calculating with radius: " << radius << " m" << endl;

	string flistpath = input_folder + "/" + week + ".list";
	cout << flistpath << endl;
	ifstream flist(flistpath.c_str()); // list of Echidna runs

    if(!flist){
        cerr << "qe_calculation Error: list " << flistpath << " not found!" << endl;
        exit(EXIT_FAILURE);
    }

	// final output for QE
	string qefile = output_folder + "/" + week + "_QE.txt";
	cout << "--> " << qefile << endl;

	// info for the time window
	QeSample* s = new QeSample(week, rmin, rmax);
	// database
	Database* d = new Database(); // opens DB
	string fname;

    cout << "~~~ Collecting data from runs" << endl;
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

    cout << "~~~ Calculating QE" << endl;
    s->CalculateQE(d); // dark noise, then chosen PMT bias, then cones
    s->SaveQE(qefile);
    
    // weekly extra: not save in "official" mode
    string qeextra = output_folder + "/" + week + "_Week.txt";
	cout << "--> " << qeextra << endl;
    s->SaveExtra(qeextra);
	
    cout << "SAVED" << endl;
	flist.close();
	delete s;
	delete d;
}

