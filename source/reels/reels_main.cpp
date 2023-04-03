/* Mercury-Reels

	Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

	This product includes software developed at

	BBVA (https://www.bbva.com/)

	  Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	  Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>


#include "reels.h"
#include "reels_main.h"


using namespace reels;
using namespace std;


int do_all(String trans_fn,
		   int	  max_events,
		   String event_fn,
		   String client_fn,
		   String target_fn,
		   String train_fn,
		   String test_fn,
		   String output,
		   String transform,
		   String aggregate,
		   double fit_p,
		   int	  depth,
		   bool	  as_states) {

	chrono::steady_clock::time_point time_origin = chrono::steady_clock::now();
	uint64_t num_transactions = 0;

	cout << "Processing ...\n\n";


	Events events = {};
	events.set_store_strings(false);

	if (event_fn != "") {
		if (max_events > 0)
			cout << "WARNING: 'max_events' is defined and ignored because 'events' is given.\n\n";

		ifstream fh(event_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'events' file.\n\n";

			return 1;
		}

		String emi, des, wei_s, cod_s;

		while (!fh.eof()) {
			getline(fh, emi, '\t');
			getline(fh, des, '\t');
			getline(fh, wei_s, '\t');
			double wei = stod(wei_s);
			getline(fh, cod_s);
			int code = stoi(cod_s);

			events.define_event(emi.c_str(), des.c_str(), wei, code);
		}
		fh.close();
	} else {
		if (max_events <= 0) {
			cout << "ERROR: 'max_events' is required.\n\n";

			return 1;
		}

		ifstream fh(trans_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'transactions' file.\n\n";

			return 1;
		}

		events.set_max_num_events(max_events);

		String emi, des, wei_s, rest;

		while (!fh.eof()) {
			getline(fh, emi, '\t');
			getline(fh, des, '\t');
			getline(fh, wei_s, '\t');
			double wei = stod(wei_s);
			getline(fh, rest);

			events.insert_row(emi.c_str(), des.c_str(), wei);

			num_transactions++;
		}
		fh.close();
	}

	double elapsed_events = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_origin).count())/1000000.0;
	chrono::steady_clock::time_point time_step = chrono::steady_clock::now();
	cout << "Events events is loaded.\n";


	Clients clients = {};

	if (client_fn != "") {
		ifstream fh(client_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'clients' file.\n\n";

			return 1;
		}

		String cli;

		while (!fh.eof()) {
			getline(fh, cli);

			clients.add_client_id(cli.c_str());
		}
		fh.close();
	}

	double elapsed_clients = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_step).count())/1000000.0;
	time_step = chrono::steady_clock::now();
	cout << "Clients clients is loaded.\n";


	Clips clips = {clients, events};

	String clips_fn = train_fn == "" ? trans_fn : train_fn;

	if (clips_fn == "") {
		cout << "ERROR: No 'train' or 'transactions' file given.\n\n";

		return 1;
	} else {
		ifstream fh(clips_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'train' or 'transactions' file.\n\n";

			return 1;
		}

		String emi, des, wei_s, cli, tim;

		while (!fh.eof()) {
			getline(fh, emi, '\t');
			getline(fh, des, '\t');
			getline(fh, wei_s, '\t');
			double wei = stod(wei_s);
			getline(fh, cli, '\t');
			getline(fh, tim);

			clips.scan_event(emi.c_str(), des.c_str(), wei, cli.c_str(), tim.c_str());
		}
		fh.close();
	}

	double elapsed_clips = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_step).count())/1000000.0;
	time_step = chrono::steady_clock::now();
	cout << "Clips clips is loaded.\n";


	TargetMap target = {};
	Targets targets = {clips.clip_map(), target};

	if (target_fn == "") {
		cout << "ERROR: No 'targets' file given.\n\n";

		return 1;
	} else {
		ifstream fh(target_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'targets' file.\n\n";

			return 1;
		}

		String cli, tim;

		while (!fh.eof()) {
			getline(fh, cli, '\t');
			getline(fh, tim);

			targets.insert_target(cli.c_str(), tim.c_str());
		}
		fh.close();
	}

	double elapsed_target_map = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_step).count())/1000000.0;
	time_step = chrono::steady_clock::now();
	cout << "TargetMap target is loaded.\n";


	Transform x_form = tr_log;

	if (transform != "log") {
		if (transform == "linear")
			x_form = tr_linear;
		else {
			cout << "ERROR: Unknown 'transform'. (Use linear or leave the default.)\n\n";

			return 1;
		}
	}

	Aggregate agg = ag_minimax;

	if (aggregate != "minimax") {
		if (aggregate == "mean")
			agg = ag_mean;
		else if (aggregate == "longest")
			agg = ag_longest;
		else {
			cout << "ERROR: Unknown 'aggregate'. (Use mean, longest or leave the default.)\n\n";

			return 1;
		}
	}

	if (!targets.fit(x_form, agg, fit_p, depth, as_states)) {
		cout << "ERROR: targets.fit() failed.\n\n";

		return 1;
	}

	double elapsed_target_fit = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_step).count())/1000000.0;
	time_step = chrono::steady_clock::now();
	cout << "Targets targets is fitted.\n";


	Clips clips_test = {clients, events};
	if (test_fn != "") {
		ifstream fh(test_fn);

		if (!fh.is_open()) {
			cout << "ERROR: Could not read 'test' file.\n\n";

			return 1;
		}

		String emi, des, wei_s, cli, tim;

		while (!fh.eof()) {
			getline(fh, emi, '\t');
			getline(fh, des, '\t');
			getline(fh, wei_s, '\t');
			double wei = stod(wei_s);
			getline(fh, cli, '\t');
			getline(fh, tim);

			clips_test.scan_event(emi.c_str(), des.c_str(), wei, cli.c_str(), tim.c_str());
		}
		fh.close();

		cout << "Clips clips_test is loaded.\n";
	}

	TimesToTarget pred_T = test_fn == "" ? targets.predict() : targets.predict(clips_test.clip_map());

	double elapsed_target_predict = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_step).count())/1000000.0;
	double elapsed_total = (chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - time_origin).count())/1000000.0;
	cout << "Targets targets predict()ed.\n";



	struct stat info;

	if (stat(output.c_str(), &info) == 0) {
		cout << "ERROR: File or folder " << output << " already exists.\n\n";

		return 1;
	}

	mkdir(output.c_str(), 0700);

	cout << "Writing output ...";

	ofstream f_stream;
	filebuf *f_buff = f_stream.rdbuf();


	String fn = output + "/RESULTS.md";

	f_buff->open(fn, std::ios::out | std::ios::app);

	if (!f_buff->is_open()) {
		cout << "ERROR: Could not write to: " << fn << "\n\n";

		return 1;
	}

	char buffer[1024];

	sprintf(buffer, "REELS\n");										f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "-----\n\n");									f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "Command line arguments given:\n\n");			f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "  transactions : %s\n", trans_fn.c_str());		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  max_events   : %i\n", max_events);			f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  events       : %s\n", event_fn.c_str());		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clients      : %s\n", client_fn.c_str());	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  targets      : %s\n", target_fn.c_str());	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  train        : %s\n", train_fn.c_str());		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  test         : %s\n", test_fn.c_str());		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  output       : %s\n", output.c_str());		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  transform    : %s\n", transform.c_str());	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  aggregate    : %s\n", aggregate.c_str());	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  fit_p        : %0.3f\n", fit_p);				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  depth        : %i\n", depth);				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  as_states    : %i\n\n", as_states);			f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "Running times (sec):\n\n");	f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "  building events     : %0.3f\n", elapsed_events);				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  loading clients     : %0.3f\n", elapsed_clients);			f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  building clips      : %0.3f\n", elapsed_clips);				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  building target map : %0.3f\n", elapsed_target_map);			f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  fitting tree        : %0.3f\n", elapsed_target_fit);			f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  predicting times    : %0.3f\n\n", elapsed_target_predict);	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  total               : %0.3f\n\n", elapsed_total);			f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "Object sizes:\n\n");							f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "  transactions.num_rows : %li\n", num_transactions);		 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  events.num_events     : %i\n", events.num_events());		 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clients.num_clients   : %li\n", clients.id.size());		 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clips.num_clips       : %li\n", clips.clip_map()->size()); 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clips.num_events      : %li\n", clips.num_events());		 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clips_test.num_clips  : %li\n", clips_test.clip_map()->size());	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  clips_test.num_events : %li\n", clips_test.num_events()); 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  targets.num_targets   : %i\n", targets.num_targets());	 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  targets.tree_size     : %i\n", targets.tree_size());		 		f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  pred_time.size()      : %li\n\n", pred_T.size());			 	f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "Legend of PREDICTIONS.tsv:\n\n");				f_buff->sputn(buffer, strlen(buffer));

	sprintf(buffer, "  client_id   : The id of the client predicted (test or transactions).\n");	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  obs_time    : Time from last event to target (observed).\n");				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  target_yn   : The client hit the target (yes/no).\n");						f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  pred_time   : Time from last event to target (predicted).\n");				f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  longest_seq : Longest event sequence in the tree.\n");						f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  n_visits    : # of visits for the longest sequence.\n");						f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  n_targets   : # of clients who hit the target for the longest sequence.\n");	f_buff->sputn(buffer, strlen(buffer));
	sprintf(buffer, "  targ_mean_t : Mean observed time for those who hit (also longest seq).\n");	f_buff->sputn(buffer, strlen(buffer));

	f_stream.flush();
	f_stream.close();


	fn = output + "/PREDICTIONS.tsv";

	f_buff->open(fn, std::ios::out | std::ios::app);

	if (!f_buff->is_open()) {
		cout << "ERROR: Could not write to: " << fn << "\n\n";

		return 1;
	}

	sprintf(buffer, "client_id\tobs_time\ttarget_yn\tpred_time\tlongest_seq\tn_visits\tn_targets\ttarg_mean_t\n");
	f_buff->sputn(buffer, strlen(buffer));

	pClipMap p_clips = test_fn == "" ? targets.clip_map() : clips_test.clip_map();

	int i = 0;
	for (ClipMap::iterator it = p_clips->begin(); it != p_clips->end(); ++it) {
		TimePoint obs_time;
		double targ_mean_t;
		uint64_t n_visits, n_targets;
		int longest_seq;
		bool target_yn;

		targets.verbose_predict_clip(it->first, it->second, obs_time, target_yn, longest_seq, n_visits, n_targets, targ_mean_t);

		sprintf(buffer, "%lu\t%li\t%i\t%0.1f\t%i\t%ld\t%ld\t%0.1f\n",
				it->first, obs_time, target_yn, pred_T[i++], longest_seq, n_visits, n_targets, targ_mean_t);

		f_buff->sputn(buffer, strlen(buffer));
	}

	f_stream.flush();
	f_stream.close();

	cout << " Ok.\n\nDone.\n";

	return 0;
}


void help() {

	cout << "Usage: reels <arguments>\n\n";
	cout << "  Arguments:\n\n";
	cout << "    transactions=path : Tab separated text file containing: (emitter, description, weight, client, time).\n";
	cout << "    max_events=number : The maximum number of events to auto detect if 'events' is not given.\n";
	cout << "    events=path       : Optional tab separated text file containing: (emitter, description, weight, code).\n";
	cout << "    clients=path      : Optional text file with client ids to define which clients are fitted.\n";
	cout << "    targets=path      : Tab separated text file containing: (client, time) of the target to predict.\n";
	cout << "    train=path        : An optional alternative (to transactions) for sequence detection and fitting.\n";
	cout << "    test=path         : An optional alternative (to transactions) for sequence detection and predicting.\n";
	cout << "    output=folder     : A folder to write the output of the execution.\n";
	cout << "    transform=linear  : Fit the time without any transformation (default is 'log').\n";
	cout << "    aggregate=mean    : Fit aggregation (default is 'minimax'), 'longest' is also a valid option.\n";
	cout << "    fit_p=0.9         : Fit probability of the binomial interval. (0 is no interval, 0.9 default is 0.05|0.9|0.05)\n";
	cout << "    tree_depth=8      : Fit tree depth == maximum learned sequence length. Default is 8.\n";
	cout << "    as_states=1       : Fit as states rather than events == removing consecutive same codes. Default is 'false'.\n\n";

	cout << "  (All times must be \"%Y-%m-%d %H:%M:%S\".)\n";
}


String parse(String name, int argc, char* argv[]) {

	for (int i = 0; i < argc; i++) {
		String s(argv[i]);

		if (s.rfind(name, 0) == 0) {
			return s.substr(name.length());
		}
	}
	return "";
}


int main(int argc, char* argv[]) {

	cout << "-- Reels command line interface --\n\n";

	if (argc < 4) {
		help();
		return 1;
	}

	String transact	= parse("transactions=", argc, argv);
	String max_ev_s	= parse("max_events=", argc, argv);
	String events	= parse("events=", argc, argv);
	String clients	= parse("clients=", argc, argv);
	String targets	= parse("targets=", argc, argv);
	String train	= parse("train=", argc, argv);
	String test		= parse("test=", argc, argv);
	String output	= parse("output=", argc, argv);
	String transf	= parse("transform=", argc, argv);
	String agg		= parse("aggregate=", argc, argv);
	String fit_p_s	= parse("fit_p=", argc, argv);
	String depth_s	= parse("tree_depth=", argc, argv);
	String states_s	= parse("as_states=", argc, argv);

	if (transf == "")
		transf = "log";

	if (agg == "")
		agg = "minimax";

	int max_events = max_ev_s != "" ? stoi(max_ev_s) : -1;
	double fit_p   = fit_p_s  != "" ? stod(fit_p_s)	 : 0.9;
	int tree_depth = depth_s  != "" ? stoi(depth_s)	 : 8;
	bool as_states = states_s != "" ? stoi(states_s) : false;

	return do_all(transact, max_events, events, clients, targets, train, test, output, transf, agg, fit_p, tree_depth, as_states);
};
