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
#include "reels.h"


#ifdef TEST
#ifndef INCLUDED_CATCH2
#define INCLUDED_CATCH2

#include "catch.hpp"

#endif
#endif


namespace reels
{

Logger REELS_logger = {};		///< Used to output stats of Events::get_top_codes added by the Python API in events_optimize_events()

#define MURMUR_SEED	  76493		///< Just a 5 digit prime

/** \brief MurmurHash2, 64-bit versions, by Austin Appleby

	(from https://sites.google.com/site/murmurhash/) a 64-bit hash for 64-bit platforms

	All code is released to the public domain. For business purposes, Murmurhash is
	under the MIT license.

	The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
	and endian-ness issues if used across multiple platforms.

	\param key address of the memory block to hash.
	\param len Number of bytes to hash.
	\return	 64-bit hash of the memory block.

*/
uint64_t MurmurHash64A (const void *key, int len) {
	const uint64_t m = 0xc6a4a7935bd1e995;
	const int	   r = 47;

	uint64_t h = MURMUR_SEED ^ (len*m);

	const uint64_t *data = (const uint64_t *) key;
	const uint64_t *end	 = data + (len/8);

	while(data != end) {
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const unsigned char *data2 = (const unsigned char*) data;

	switch(len & 7)	{
	case 7: h ^= uint64_t(data2[6]) << 48;
	case 6: h ^= uint64_t(data2[5]) << 40;
	case 5: h ^= uint64_t(data2[4]) << 32;
	case 4: h ^= uint64_t(data2[3]) << 24;
	case 3: h ^= uint64_t(data2[2]) << 16;
	case 2: h ^= uint64_t(data2[1]) << 8;
	case 1: h ^= uint64_t(data2[0]);

		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}


/** \brief A function to push arbitrary raw data into a BinaryImage

	\param p_bi		A pointer to an existing BinaryImage that receives the data
	\param p_data	Address of the memory block to store
	\param size		Number of bytes to store
	\return			True on success

*/
bool image_put(pBinaryImage p_bi, void *p_data, int size) {

	ImageBlock blk;
	int c_block = p_bi->size() - 1;

	if (c_block < 0) {
		blk.size	  = 0;
		blk.block_num = 1;

		p_bi->push_back(blk);

		c_block = 0;
	} else if ((*p_bi)[c_block].size == IMAGE_BUFF_SIZE) {
		blk.size	  = 0;
		blk.block_num = ++c_block + 1;

		p_bi->push_back(blk);
	}

	while (size > 0) {
		int uu_size = (*p_bi)[c_block].size;
		int mv_size = std::min(size, IMAGE_BUFF_SIZE - uu_size);

		if (uu_size > 0)
			blk = (*p_bi)[c_block];
		else {
			blk.size	  = 0;
			blk.block_num = c_block + 1;
		}

		memcpy(&blk.buffer[uu_size], p_data, mv_size);

		p_data	  = (char *) p_data + mv_size;
		size	 -= mv_size;
		blk.size += mv_size;

		(*p_bi)[c_block] = blk;

		if (blk.size == IMAGE_BUFF_SIZE) {
			blk.size	  = 0;
			blk.block_num = ++c_block + 1;

			p_bi->push_back(blk);
		}
	}

	return size == 0;
}


/** \brief A function to get an arbitrary raw data block from a BinaryImage

	\param p_bi		A pointer to an existing BinaryImage that was created by a series of image_put call (saved/loaded, etc.)
	\param c_block	The highest number (block number of the current cursor) that defines where to start reading from. (The normal use is
					starting at (0,0) and let this function update cursor value. Just get() the blocks in the same order ands size that
					was used to put() them.)
	\param c_ofs	The lowest number (offset inside the current block number) of the current cursor. This works together with `c_block`.
	\param p_data	Address of the memory buffer that will receive the block
	\param size		Number of bytes to get
	\return			True on success

*/
bool image_get(pBinaryImage p_bi, int &c_block, int &c_ofs, void *p_data, int size) {

	while (size > 0) {
		if (c_block < 0 || c_block >= (int) p_bi->size())
			return false;

		int uu_size = (*p_bi)[c_block].size;
		int mv_size = std::min(size, uu_size - c_ofs);

		if (mv_size <= 0) {
			if (mv_size < 0 || uu_size != IMAGE_BUFF_SIZE)
				return false;

			c_block++;
			c_ofs = 0;

			uu_size = (*p_bi)[c_block].size;
			mv_size = std::min(size, uu_size - c_ofs);
		}

		memcpy(p_data, &(*p_bi)[c_block].buffer[c_ofs], mv_size);

		p_data  = (char *) p_data + mv_size;
		size   -= mv_size;
		c_ofs  += mv_size;
	}

	return size == 0;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Events Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

void Events::insert_row(pChar p_e, pChar p_d, double w) {

	BinEventPt ept;

	ept.e = add_str(p_e);
	ept.d = add_str(p_d);
	ept.w = w;

	EventMap::iterator it = event.find(ept);

	if (it != event.end()) {
		it->second.seen++;

		uint64_t pri = it->second.priority;
		priority.erase(pri);

		pri = ++priority_low + PRIORITY_SEEN_FACTOR*it->second.seen;

		priority[pri] = ept;
		it->second.priority = pri;

		return;
	}

	if (max_num_events == (int) event.size()) {
		uint64_t   low_pri = priority.begin()->first;
		BinEventPt low_ept = priority.begin()->second;

		priority.erase(low_pri);

		erase_str(low_ept.d);
		erase_str(low_ept.e);

		event.erase(low_ept);
	}

	EventStat es;

	es.seen		= 1;
	es.priority = ++priority_low + PRIORITY_SEEN_FACTOR;
	es.code		= ++next_code;

	event[ept] = es;

	priority[es.priority] = ept;
}


bool Events::define_event(pChar	p_e, pChar p_d, double w, uint64_t code) {

	BinEventPt ept;

	ept.e = add_str(p_e);
	ept.d = add_str(p_d);
	ept.w = w;

	EventMap::iterator it = event.find(ept);

	if (it != event.end() || priority.size() != 0)
		return false;								// Event is defined or insert_row() was called before.

	EventStat es;

	es.seen		= 1;
	es.priority = 0;
	es.code		= code;

	event[ept] = es;

	return true;
}


String Events::optimize_events (Clips &clips, TargetMap &targets, int num_steps, int codes_per_step, double threshold,
								pCodeSet p_force_include, pCodeSet p_force_exclude, Transform x_form, Aggregate agg, double p,
								int depth, bool as_states, double exponential_decay, double lower_bound_p, bool log_lift) {

	// Use a logger as a debug tool and to return errors.

	Logger log = {};

	// Prerequisites: Build the list of codes from clips as a dictionary, initially to itself.

	EventCodeMap large_dict = {};

	for (ClipMap::iterator it = clips.clip_map()->begin(); it != clips.clip_map()->end(); ++it) {
		for (Clip::iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
			large_dict[jt->second] = jt->second;
		}
	}

	uint64_t code_base = large_dict.rbegin()->first + 1;		// Define the range base for the new codes

	// Prerequisites: Another dictionary, initially to code_base.

	EventCodeMap small_dict = {};

	uint64_t code_noise = code_base + 1;
	uint64_t code_new	= code_noise + 1;

	for (EventCodeMap::iterator it = large_dict.begin(); it != large_dict.end(); ++it) {
		small_dict[it->first] = code_noise;

		if (p_force_include != nullptr && p_force_include->find(it->first) != p_force_include->end())
			small_dict[it->first] = code_new++;
	}

	log.log_printf("Preprocessing:\n\n  %i codes found in clips.\n", large_dict.size());

	// Prerequisites: Check the codes in the internal EventMap, remove if in excess, fail if missing.

	CodeSet codes = {};

	int removed = 0;
	for (EventMap::const_iterator it = event.cbegin(); it != event.cend();) {
		EventCodeMap::iterator jt = large_dict.find(it->second.code);
		if (jt == large_dict.end()) {
			event.erase(it++);
			removed++;
		} else {
			codes.insert(it->second.code);
			++it;
		}
	}
	log.log_printf("  %i codes removed from internal EventMap.\n", removed);

	if (codes.size() != large_dict.size()) {
		log.log_printf("  %i codes in clips not defined in internal EventMap.\n", large_dict.size() - codes.size());

		log.log = "ERROR\n" + log.log;

		return log.log;
	}

	// First model run

	double large_score = -1, targ_prop;

	CodeInTreeStatMap codes_stat = {};

	if (!score_model(large_score, targ_prop, codes_stat, true, clips, targets, large_dict, x_form, agg, p, depth, as_states)) {
		log.log = "ERROR\nscore_model() failed!\n" + log.log;

		return log.log;
	};

	log.log_printf("  Current score = %.6f\n", large_score);

	CodeScores top_code = get_top_codes(codes_stat, targ_prop, exponential_decay, lower_bound_p, log_lift);

	// Main (over num_steps) loop

	double best_score = -1;

	int top_ix = 0;

	for (int step = 0; step < num_steps; step++) {
		log.log_printf("\nStep %i of %i\n\n", step + 1, num_steps);

		EventCodeMap dict = small_dict;
		int new_codes = 0;
		log.log_printf("  Trying:\n");
		while (new_codes < codes_per_step) {
			if (top_ix == (int) top_code.size())
				break;

			uint64_t code_try = top_code[top_ix++].code;

			if (p_force_exclude != nullptr && p_force_exclude->find(code_try) != p_force_exclude->end()) {
	 			log.log_printf("    Code %i was excluded by the caller\n", code_try);

				continue;
			}

 			log.log_printf("    Code %i as %i\n", code_try, code_new - code_base);

			dict[code_try] = code_new++;

			new_codes++;
		}
		if (new_codes == 0) {
			log.log_printf("  -- No more codes --\n");
			break;
		}

		double new_score;
		if (!score_model(new_score, targ_prop, codes_stat, false, clips, targets, dict, x_form, agg, p, depth, as_states)) {
			log.log = "ERROR\nscore_model() failed!\n" + log.log;

			return log.log;
		}
 		log.log_printf("    ---------------\n    Score = %0.6f\n", new_score);

		if (new_score - best_score >= threshold) {
			best_score = new_score;
			small_dict = dict;
			log.log_printf("    Best score so far.\n");
		} else
			log.log_printf("    Threshold (%.6f) not met (diff = %.6f)\n", threshold, new_score - best_score);
	}

	log.log_printf("\n== F I N A L ==\n\n");
	log.log_printf("  Final score      = %.6f\n", best_score);
	log.log_printf("  Final dictionary = {");

	EventCodeMap::iterator it_small = small_dict.begin();
	for (int i = 1; i < (int) small_dict.size(); i++) {
		log.log_printf("%i:%i, ", it_small->first, it_small->second - code_base);

		++it_small;
	}
	log.log_printf("%i:%i}\n", it_small->first, it_small->second - code_base);

	for (EventMap::iterator it = event.begin(); it != event.end(); ++it)
		it->second.code = small_dict[it->second.code] - code_base;

	log.log = "SUCCESS\n" + log.log;

	return log.log;
}


/** \brief Compare two OptimizeEvalItem structures for sorting.

	\param a  The first structure
	\param b  The second one

	\return	True if a < b.
*/
bool compare_optimize_eval(const OptimizeEvalItem a, const OptimizeEvalItem b) {
	if (a.t_hat != b.t_hat)
		return a.t_hat < b.t_hat;

	return a.seq_len < b.seq_len;
}


bool Events::score_model(double &score, double &targ_prop, CodeInTreeStatMap &codes_stat, bool calc_tree_stats, Clips &clips,
						 TargetMap &targets, EventCodeMap code_dict, Transform x_form, Aggregate agg, double p, int depth, bool as_states) {

	// Creates t_clips as a copy and renames its codes.
	Clips t_clips = clips;

	for (ClipMap::iterator it = t_clips.clip_map()->begin(); it != t_clips.clip_map()->end(); ++it)
		for (Clip::iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
			jt->second = code_dict[jt->second];

	// Creates a Targets with the renamed codes and fits it to the targets.
	Targets targ = Targets(t_clips.clip_map(), targets);

	if (as_states)
		t_clips.collapse_to_states();

	if (!targ.fit(x_form, agg, p, depth, false))
		return false;

	targ_prop = targ.p_tree()->at(0).n_seen > 0 ? targ.p_tree()->at(0).n_target/targ.p_tree()->at(0).n_seen : 0;

	// Predicts, builds an OptimizeEval with (observed and predicted), sorts it and computes score.
	TimesToTarget t_hat = targ.predict();

	OptimizeEval ev = {};
	int i = 0;
	for (ClipMap::iterator it_client = t_clips.clip_map()->begin(); it_client != t_clips.clip_map()->end(); ++it_client) {
		TimePoint elapsed = 0;

		TargetMap::iterator it_target = targets.find(it_client->first);
		if (it_target != targets.end()) {
			for (Clip::reverse_iterator it_point = it_client->second.rbegin(); it_point != it_client->second.rend(); it_point++) {
				TimePoint et = it_target->second - it_point->first;
				if (et > 0) {
					elapsed = et;
					break;
				}
			}
			elapsed++;	// Adds 1 second to avoid the case when the client is a target but no events before ir are recorded.
		}

		OptimizeEvalItem itm = {t_hat[i], elapsed, (int) it_client->second.size()};

		ev.push_back(itm);

		++i;
	}
	std::sort(ev.begin(), ev.end(), compare_optimize_eval);

	int tp = 0;
	int fp = 0;
	int tot_targ = targets.size();

	for (int i = 0; i < tot_targ; i++)
		if (ev[i].t_obs != 0) tp++; else fp++;

	// This is the F1 score: The harmonic mean of precision and recall.
	// In general: F1 = 2*tp / (2*tp + fp + fn)
	// In this case, fn == fp -> 2*tp / (2*tp + 2*fp)
	score = tp/(1.0*tp + fp);

	if (tp < tot_targ && tp > 0) {		// Score adjustment by correlation
		double max_diff = ((tp + 1)/(tp + 1.0 + fp) - (tp - 1)/(tp - 1.0 + fp))/2;	// The maximum score adjustment size is the mean
																					// difference between tp+/-1 scores.
		double pearson_corr = linear_correlation(ev);

		score = score + max_diff*pearson_corr;
	}

	// Exit if tree statistics are not required.
	if (!calc_tree_stats)
		return true;

	// Builds an empty CodeInTreeStatMap
	CodeInTreeStatistics void_code_stat = {0, 0, 0, 0, 0, 0};

	for (EventCodeMap::iterator it = code_dict.begin(); it != code_dict.end(); ++it)
		codes_stat[it->first] = void_code_stat;

	// Calls the Targets recurse_tree_stats() to update the tree statistics
	return targ.recurse_tree_stats(0, 0, -1, 0, codes_stat);
}


/** \brief Compare two CodeScoreItem structures for sorting.

	\param a  The first structure
	\param b  The second one

	\return	True if a < b.
*/
bool compare_score_item(const CodeScoreItem a, const CodeScoreItem b) {
	return b.score < a.score;
}


CodeScores Events::get_top_codes(CodeInTreeStatMap &codes_stat, double targ_prop, double exponential_decay, double lower_bound_p, bool log_lift) {

	CodeScores	ev = {};
	ClipMap		clm	= {};
	TargetMap	tm	= {};
	Targets		tar(&clm, tm);

	tar.fit(tr_linear, ag_mean, lower_bound_p, 10, false);

#ifdef DEBUG
	std::ofstream f_stream;
	std::filebuf *f_buff = f_stream.rdbuf();

	String fn = "/home/jadmin/bbva/x2/source/logs/get_top_codes.log";

	f_buff->open(fn, std::ios::out);
#endif

	REELS_logger.log = "";

	REELS_logger.log_printf("n_succ_seen\tn_succ_target\tn_incl_seen\tn_incl_target\tsum_dep\tn_dep\tedf\tprop_succ\tprop_incl\tlift\tscore\tcode\n");

	for (CodeInTreeStatMap::iterator it = codes_stat.begin(); it != codes_stat.end(); ++it) {
		double edf = it->second.n_dep > 0 ? exp(-exponential_decay*it->second.sum_dep/it->second.n_dep) : 0;
		double succ = std::max((double) 0.0, tar.agresti_coull_lower_bound(it->second.n_succ_target, it->second.n_succ_seen));
		double incl = std::max((double) 0.0, tar.agresti_coull_lower_bound(it->second.n_incl_target, it->second.n_incl_seen));
		double lift = succ > 0.001 ? incl/succ : 0;

		lift = tar.agresti_coull_upper_bound(it->second.n_incl_target, it->second.n_incl_seen) < targ_prop ? 0 : log_lift ? log(lift + 1) : lift;

		double score = edf*incl*lift;

		REELS_logger.log_printf("%i\t%i\t%i\t%i\t%i\t%i\t%0.6f\t%0.6f\t%0.6f\t%0.6f\t%0.6f\t%i\n",
								it->second.n_succ_seen,
								it->second.n_succ_target,
								it->second.n_incl_seen,
								it->second.n_incl_target,
								it->second.sum_dep,
								it->second.n_dep,
								edf, succ, incl, lift, score, it->first);

		CodeScoreItem itm = {it->first, score};

		ev.push_back(itm);
	}

#ifdef DEBUG
	f_buff->sputn(REELS_logger.log.c_str(), REELS_logger.log.length());

	f_stream.flush();
	f_stream.close();
#endif

	std::sort(ev.begin(), ev.end(), compare_score_item);

	int top_n = ev.size();

	for (int i = 0; i < top_n; i++) {
		if (ev[i].score < 5e-7) {
			top_n = i;
			break;
		}
	}

	ev.resize(top_n);

	return ev;
}


bool Events::load(pBinaryImage &p_bi) {
	int c_block = 0, c_ofs = 0;

	return load(p_bi, c_block, c_ofs);
}


bool Events::load(pBinaryImage &p_bi, int &c_block, int &c_ofs) {

	String		section = "events";
	ElementHash hs;
	char		buffer[8192];

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if (hs != MurmurHash64A(section.c_str(), section.length()))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &store_strings, sizeof(store_strings)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &max_num_events, sizeof(max_num_events)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &priority_low, sizeof(priority_low)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &next_code, sizeof(next_code)))
		return false;

	section = "names_map";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (names_map.size() != 0))
		return false;

	int len;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		ElementHash hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		StringUsage su;
		if (!image_get(p_bi, c_block, c_ofs, &su.seen, sizeof(su.seen)))
			return false;

		int ll;

		if (!image_get(p_bi, c_block, c_ofs, &ll, sizeof(ll)))
			return false;

		if ((ll < 0) || (ll >= 8192))
			return false;

		if (ll == 0)
			su.str = (char *) "";
		else {
			if (!image_get(p_bi, c_block, c_ofs, &buffer, ll))
				return false;

			buffer[ll] = 0;

			su.str = buffer;
		}

		names_map[hh] = su;
	}

	section = "event";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (event.size() != 0))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		BinEventPt ev;
		if (!image_get(p_bi, c_block, c_ofs, &ev, sizeof(ev)))
			return false;

		EventStat es;
		if (!image_get(p_bi, c_block, c_ofs, &es, sizeof(es)))
			return false;

		event[ev] = es;
	}

	section = "priority";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (priority.size() != 0))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		uint64_t hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		BinEventPt ev;
		if (!image_get(p_bi, c_block, c_ofs, &ev, sizeof(ev)))
			return false;

		priority[hh] = ev;
	}

	section = "end";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	return hs == MurmurHash64A(section.c_str(), section.length());
}


bool Events::save(pBinaryImage &p_bi) {

	String section = "events";
	ElementHash hs = MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	image_put(p_bi, &store_strings, sizeof(store_strings));
	image_put(p_bi, &max_num_events, sizeof(max_num_events));
	image_put(p_bi, &priority_low, sizeof(priority_low));
	image_put(p_bi, &next_code, sizeof(next_code));

	section = "names_map";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len = names_map.size();

	image_put(p_bi, &len, sizeof(len));

	for (StringUsageMap::iterator it = names_map.begin(); it != names_map.end(); ++it) {
		ElementHash hh = it->first;
		image_put(p_bi, &hh, sizeof(hh));
		uint64_t su_seen = it->second.seen;

		image_put(p_bi, &su_seen, sizeof(su_seen));

		int ll = it->second.str.length();

		image_put(p_bi, &ll, sizeof(ll));
		image_put(p_bi, (void *) it->second.str.c_str(), ll);
	}

	section = "event";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	len = event.size();

	image_put(p_bi, &len, sizeof(len));

	for (EventMap::iterator it = event.begin(); it != event.end(); ++it) {
		BinEventPt ev = it->first;
		image_put(p_bi, &ev, sizeof(ev));
		EventStat es = it->second;
		image_put(p_bi, &es, sizeof(es));
	}

	section = "priority";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	len = priority.size();

	image_put(p_bi, &len, sizeof(len));

	for (PriorityMap::iterator it = priority.begin(); it != priority.end(); ++it) {
		uint64_t hh = it->first;
		image_put(p_bi, &hh, sizeof(hh));
		BinEventPt ev = it->second;
		image_put(p_bi, &ev, sizeof(ev));
	}

	section = "end";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Clients Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

void Clients::add_client_id(pChar p_cli) {

	ElementHash hash = hash_client_id(p_cli);

	id.push_back(hash);
	id_set.insert(hash);
}


bool Clients::load(pBinaryImage &p_bi) {
	int c_block = 0, c_ofs = 0;

	return load(p_bi, c_block, c_ofs);
}


bool Clients::load(pBinaryImage &p_bi, int &c_block, int &c_ofs) {

	String		section = "clients";
	ElementHash hs;

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (id.size() != 0) || (id_set.size() != 0))
		return false;

	int len;

	if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
		return false;

	for (int i = 0; i < len; i++) {
		ElementHash hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		id.push_back(hh);
		id_set.insert(hh);
	}

	section = "end";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	return hs == MurmurHash64A(section.c_str(), section.length());
}


bool Clients::save(pBinaryImage &p_bi) {

	String section = "clients";
	ElementHash hs = MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len = id.size();

	image_put(p_bi, &len, sizeof(len));

	for (int i = 0; i < len; i++) {
		ElementHash hh = id[i];
		image_put(p_bi, &hh, sizeof(hh));
	}

	section = "end";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Clips Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

bool Clips::scan_event(pChar p_e, pChar p_d, double w, pChar p_c, pChar p_t) {

	// Is it a client that should be tracked?

	int ll = strlen(p_c);

	if (ll == 0)
		return false;

	ElementHash client_hash = MurmurHash64A(p_c, ll);

	if (clients.id_set.size() > 0) {
		ClientIDSet::iterator it = clients.id_set.find(client_hash);
		if (it == clients.id_set.end())
			return false;						// clients.id_set is not empty and the client is not in it.
	}

	// Is it an event that should be tracked?

	BinEventPt ept;

	ept.e = events.add_str(p_e);
	ept.d = events.add_str(p_d);
	ept.w = w;

	uint64_t code = events.event_code(ept);

	if (code == 0)
		return false;

	// Is the timestamp valid?

	TimePoint time_pt = get_time(p_t);

	if (time_pt < 0)
		return false;	// Times before the epoch are not supported, format error returns -1.

	// We have everything: client_hash, code, time_pt. From here on, we insert the point in a clip and return true.

	insert_event(client_hash, code, time_pt);

	return true;
}


bool Clips::load(pBinaryImage &p_bi) {

	int c_block = 0, c_ofs = 0;

	String		section = "clips";
	ElementHash hs;

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if (hs != MurmurHash64A(section.c_str(), section.length()))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &time_format, sizeof(time_format)))
		return false;

	if (!clients.load(p_bi, c_block, c_ofs))
		return false;

	if (!events.load(p_bi, c_block, c_ofs))
		return false;

	section = "clip_map";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (clips.size() != 0))
		return false;

	int len_clips;
	if (!image_get(p_bi, c_block, c_ofs, &len_clips, sizeof(len_clips)))
		return false;

	for (int i = 0; i < len_clips; i++) {
		ElementHash hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		int len;
		if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
			return false;

		Clip clip = {};

		for (int j = 0; j < len; j++) {
			TimePoint tp;
			if (!image_get(p_bi, c_block, c_ofs, &tp, sizeof(tp)))
				return false;

			uint64_t ev;
			if (!image_get(p_bi, c_block, c_ofs, &ev, sizeof(ev)))
				return false;

			clip[tp] = ev;
		}

		clips[hh] = clip;
	}

	section = "end";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	return hs == MurmurHash64A(section.c_str(), section.length());
}


bool Clips::save(pBinaryImage &p_bi) {

	String section = "clips";
	ElementHash hs = MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	image_put(p_bi, &time_format, sizeof(time_format));

	if (!clients.save(p_bi))
		return false;

	if (!events.save(p_bi))
		return false;

	section = "clip_map";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len_clips = clips.size();

	image_put(p_bi, &len_clips, sizeof(len_clips));

	for (ClipMap::iterator it_clip = clips.begin(); it_clip != clips.end(); ++it_clip) {
		ElementHash hh = it_clip->first;
		image_put(p_bi, &hh, sizeof(hh));

		int len = it_clip->second.size();
		image_put(p_bi, &len, sizeof(len));

		for (Clip::iterator it = it_clip->second.begin(); it != it_clip->second.end(); ++it) {
			TimePoint tp = it->first;
			image_put(p_bi, &tp, sizeof(tp));

			uint64_t ev = it->second;
			image_put(p_bi, &ev, sizeof(ev));
		}
	}

	section = "end";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	return true;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Targets Implementation
// -----------------------------------------------------------------------------------------------------------------------------------------

bool Targets::insert_target(pChar p_c, pChar p_t) {

	int ll = strlen(p_c);

	if (ll == 0)
		return false;

	TimePoint time_pt = get_time(p_t);

	if (time_pt < 0)
		return false;	// Times before the epoch are not supported, format error returns -1.

	ElementHash client_hash = MurmurHash64A(p_c, ll);

	if (target.find(client_hash) != target.end())
		return false;

	target[client_hash] = time_pt;

	return true;
}


bool Targets::fit(Transform x_form, Aggregate agg, double p, int depth, bool as_states) {

	// Check if already fitted

	if (tree.size() != 1)
		return false;

	// Validate arguments

	transform = x_form == tr_linear ? tr_linear : tr_log;

	switch (agg) {
	case ag_mean:
		aggregate = ag_mean;
		break;

	case ag_longest:
		aggregate = ag_longest;
		break;

	default:
		aggregate = ag_minimax;
	}

	tree_depth = std::max(1, std::min(MAX_SEQ_LEN_IN_PREDICT, depth));

	pClipMap p_clip_map = p_clips;
	Clips state_clips;

	if (as_states) {
		state_clips = Clips(*p_clip_map);
		state_clips.collapse_to_states();
		p_clip_map = state_clips.clip_map();
	}

	ClipMap   clm	= {};
	TargetMap tm	= {};

	Targets tt(&clm, tm);

	p = std::max(0.0, std::min(0.9999, p));

	double x0 = -5, x1 = 5, cum_p = p/2 + 0.5;

	while (x1 - x0 > 1e-6) {
		binomial_z = (x0 + x1)/2;

		if (tt.normal_cdf(binomial_z) < cum_p)
			x0 = binomial_z;
		else
			x1 = binomial_z;
	}

	binomial_z_sqr		 = binomial_z*binomial_z;
	binomial_z_sqr_div_2 = binomial_z_sqr/2;

	// Fill the tree

	for (ClipMap::iterator it_client = p_clip_map->begin(); it_client != p_clip_map->end(); ++it_client) {

		// Find the client in TargetMap
		TargetMap::iterator it_target = target.find(it_client->first);

		TimePoint target_time = it_target == target.end() ? 0 : it_target->second;
		ExtFloat time_d;
		int n = 0, parent_idx = 0;

		for (Clip::reverse_iterator it_point = it_client->second.rbegin(); it_point != it_client->second.rend(); it_point++) {
			if (target_time == 0) {
				parent_idx = update_node(parent_idx, it_point->second, false, 0);

				if (++n == tree_depth)
					break;

			} else {
				TimePoint elapsed_sec = target_time - it_point->first;
				if (elapsed_sec > 0) {
					if (n == 0)
						time_d = transform == tr_linear ? elapsed_sec : log(elapsed_sec);

					parent_idx = update_node(parent_idx, it_point->second, true, time_d);

					if (++n == tree_depth)
						break;
				}
			}
		}
	}

	return true;
}


TimesToTarget Targets::predict() {

	TimesToTarget ret = {};

	if (tree.size() > 1 && tree[0].n_seen > 0) {
		for (ClipMap::iterator it = p_clips->begin(); it != p_clips->end(); ++it) {
			double t = predict_clip(it->second);

			ret.push_back(t);
		}
	}

	return ret;
}


TimesToTarget Targets::predict(Clients clients) {

	TimesToTarget ret = {};

	if (tree.size() > 1 && tree[0].n_seen > 0) {
		double t_not_found = predict_time(tree[0]);

		for (int i = 0; i < (int) clients.id.size(); i++) {
			ClipMap::iterator it = p_clips->find(clients.id[i]);

			double t = it == p_clips->end() ? t_not_found : predict_clip(it->second);

			ret.push_back(t);
		}
	}

	return ret;
}


TimesToTarget Targets::predict(pClipMap p_clips) {

	TimesToTarget ret = {};

	if (tree.size() > 1 && tree[0].n_seen > 0) {
		for (ClipMap::iterator it = p_clips->begin(); it != p_clips->end(); ++it) {
			double t = predict_clip(it->second);

			ret.push_back(t);
		}
	}

	return ret;
}


void Targets::verbose_predict_clip(const ElementHash &client,
								   Clip				 &clip,
								   TimePoint		 &obs_time,
								   bool				 &target_yn,
								   int				 &longest_seq,
								   uint64_t			 &n_visits,
								   uint64_t			 &n_targets,
								   double			 &targ_mean_t) {

	TargetMap::iterator it_target = target.find(client);

	target_yn = it_target != target.end();

	TimePoint target_time = target_yn ? it_target->second : 0;

	obs_time	= 0;
	longest_seq	= 0;
	targ_mean_t	= 0;

	int idx = 0;

	for (Clip::reverse_iterator it_point = clip.rbegin(); it_point != clip.rend(); it_point++) {
		if (target_yn) {
			TimePoint t = target_time - it_point->first;
			if (t < 0)
				continue;

			if (longest_seq == 0)
				obs_time = t;
		}

		ChildIndex::iterator it = tree[idx].child.find(it_point->second);

		if (it == tree[idx].child.end())
			break;

		++longest_seq;
		idx = it->second;
	}

	n_visits	= tree[idx].n_seen;
	n_targets	= tree[idx].n_target;

	if (n_targets)
		targ_mean_t = transform == tr_linear ? ((double) tree[idx].sum_time_d)/n_targets : exp(((double) tree[idx].sum_time_d)/n_targets);
}


bool Targets::recurse_tree_stats(int depth, int idx, int parent_idx, uint64_t code, CodeInTreeStatMap &codes_stat) {

	int ts = tree.size();

	if (depth >= ts || idx < 0 || idx >= ts)
		return false;

	if (parent_idx >= 0 && parent_idx < ts) {
		CodeInTreeStatistics *p_stat = &codes_stat[code];
		pCodeTreeNode		  p_incl = &tree[idx];
		pCodeTreeNode		  p_succ = &tree[parent_idx];

		p_stat->n_incl_seen   += p_incl->n_seen;
		p_stat->n_incl_target += p_incl->n_target;
		p_stat->n_succ_seen   += p_succ->n_seen;
		p_stat->n_succ_target += p_succ->n_target;
		p_stat->sum_dep		  += depth;
		p_stat->n_dep		  += 1;
	}

	for (ChildIndex::iterator it = tree[idx].child.begin(); it != tree[idx].child.end(); ++it)
		if (!recurse_tree_stats(depth + 1, it->second, idx, it->first, codes_stat))
			return false;

	return true;
}


bool Targets::load(pBinaryImage &p_bi) {

	int c_block = 0, c_ofs = 0;

	String		section = "targets";
	ElementHash hs;

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if (hs != MurmurHash64A(section.c_str(), section.length()))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &time_format, sizeof(time_format)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &transform, sizeof(transform)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &aggregate, sizeof(aggregate)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &binomial_z, sizeof(binomial_z)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &binomial_z_sqr, sizeof(binomial_z_sqr)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &binomial_z_sqr_div_2, sizeof(binomial_z_sqr_div_2)))
		return false;

	if (!image_get(p_bi, c_block, c_ofs, &tree_depth, sizeof(tree_depth)))
		return false;

	section = "clip_map";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (p_clips != nullptr))
		return false;

	int len_clips;
	if (!image_get(p_bi, c_block, c_ofs, &len_clips, sizeof(len_clips)))
		return false;

	p_clips = new ClipMap;

	for (int i = 0; i < len_clips; i++) {
		ElementHash hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		int len;
		if (!image_get(p_bi, c_block, c_ofs, &len, sizeof(len)))
			return false;

		Clip clip = {};

		for (int j = 0; j < len; j++) {
			TimePoint tp;
			if (!image_get(p_bi, c_block, c_ofs, &tp, sizeof(tp)))
				return false;

			uint64_t ev;
			if (!image_get(p_bi, c_block, c_ofs, &ev, sizeof(ev)))
				return false;

			clip[tp] = ev;
		}

		p_clips->insert(std::pair<ElementHash, Clip>(hh, clip));
	}

	section = "target";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (target.size() != 0))
		return false;

	int len_targets;

	if (!image_get(p_bi, c_block, c_ofs, &len_targets, sizeof(len_targets)))
		return false;

	for (int i = 0; i < len_targets; i++) {
		ElementHash hh;
		if (!image_get(p_bi, c_block, c_ofs, &hh, sizeof(hh)))
			return false;

		TimePoint tp;
		if (!image_get(p_bi, c_block, c_ofs, &tp, sizeof(tp)))
			return false;

		target[hh] = tp;
	}

	section = "tree";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	if ((hs != MurmurHash64A(section.c_str(), section.length())) || (tree.size() != 1))
		return false;

	int len_tree;

	if (!image_get(p_bi, c_block, c_ofs, &len_tree, sizeof(len_tree)))
		return false;

	for (int i = 0; i < len_tree; i++) {
		CodeTreeNode nd = {0, 0, 0, {}};

		if (!image_get(p_bi, c_block, c_ofs, &nd.n_seen, sizeof(uint64_t)))
			return false;

		if (!image_get(p_bi, c_block, c_ofs, &nd.n_target, sizeof(uint64_t)))
			return false;

		if (!image_get(p_bi, c_block, c_ofs, &nd.sum_time_d, sizeof(ExtFloat)))
			return false;

		int ll;

		if (!image_get(p_bi, c_block, c_ofs, &ll, sizeof(ll)))
			return false;

		for (int j = 0; j < ll; j++) {
			uint64_t key;
			if (!image_get(p_bi, c_block, c_ofs, &key, sizeof(key)))
				return false;

			int idx;
			if (!image_get(p_bi, c_block, c_ofs, &idx, sizeof(idx)))
				return false;

			nd.child[key] = idx;
		}

		if (i == 0)
			tree[0] = nd;
		else
			tree.push_back(nd);
	}

	section = "end";

	if (!image_get(p_bi, c_block, c_ofs, &hs, sizeof(hs)))
		return false;

	return hs == MurmurHash64A(section.c_str(), section.length());
}


bool Targets::save(pBinaryImage &p_bi) {

	String section = "targets";
	ElementHash hs = MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	image_put(p_bi, &time_format, sizeof(time_format));
	image_put(p_bi, &transform, sizeof(transform));
	image_put(p_bi, &aggregate, sizeof(aggregate));
	image_put(p_bi, &binomial_z, sizeof(binomial_z));
	image_put(p_bi, &binomial_z_sqr, sizeof(binomial_z_sqr));
	image_put(p_bi, &binomial_z_sqr_div_2, sizeof(binomial_z_sqr_div_2));
	image_put(p_bi, &tree_depth, sizeof(tree_depth));

	section = "clip_map";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len_clips = p_clips->size();

	image_put(p_bi, &len_clips, sizeof(len_clips));

	for (ClipMap::iterator it_clip = p_clips->begin(); it_clip != p_clips->end(); ++it_clip) {
		ElementHash hh = it_clip->first;
		image_put(p_bi, &hh, sizeof(hh));

		int len = it_clip->second.size();
		image_put(p_bi, &len, sizeof(len));

		for (Clip::iterator it = it_clip->second.begin(); it != it_clip->second.end(); ++it) {
			TimePoint tp = it->first;
			image_put(p_bi, &tp, sizeof(tp));

			uint64_t ev = it->second;
			image_put(p_bi, &ev, sizeof(ev));
		}
	}

	section = "target";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len_targets = target.size();

	image_put(p_bi, &len_targets, sizeof(len_targets));

	for (TargetMap::iterator it = target.begin(); it != target.end(); ++it) {
		ElementHash hh = it->first;
		image_put(p_bi, &hh, sizeof(hh));

		TimePoint tp = it->second;
		image_put(p_bi, &tp, sizeof(tp));
	}

	section = "tree";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	int len_tree = tree.size();

	image_put(p_bi, &len_tree, sizeof(len_tree));

	for (int i = 0; i < len_tree; i++) {
		pCodeTreeNode pn = &tree[i];
		image_put(p_bi, &pn->n_seen, sizeof(uint64_t));
		image_put(p_bi, &pn->n_target, sizeof(uint64_t));
		image_put(p_bi, &pn->sum_time_d, sizeof(ExtFloat));

		int ll = pn->child.size();
		image_put(p_bi, &ll, sizeof(ll));

		for (ChildIndex::iterator it = pn->child.begin(); it != pn->child.end(); ++it) {
			uint64_t key = it->first;
			image_put(p_bi, &key, sizeof(key));
			int idx =  it->second;
			image_put(p_bi, &idx, sizeof(idx));
		}
	}

	section = "end";
	hs		= MurmurHash64A(section.c_str(), section.length());

	image_put(p_bi, &hs, sizeof(hs));

	return true;
}

} // namespace reels

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: Common utils
// -----------------------------------------------------------------------------------------------------------------------------------------

// Forward declaration of utilities used in other functions.

bool destroy_binary_image_iterator (int image_id);

using namespace reels;

typedef Events		  *pEvents;
typedef Clients		  *pClients;
typedef Clips		  *pClips;
typedef Targets		  *pTargets;
typedef TimesToTarget *pIterTimes;

typedef std::map<int, pEvents>		EventsServer;
typedef std::map<int, pClients>		ClientsServer;
typedef std::map<int, pClips>		ClipsServer;
typedef std::map<int, pTargets>		TargetsServer;
typedef std::map<int, pIterTimes>	IterTimesServer;
typedef std::map<int, pBinaryImage>	BinaryImageServer;

int events_num	 = 0;
int clients_num	 = 0;
int clips_num	 = 0;
int targets_num	 = 0;
int it_times_num = 0;

EventsServer	  events   = {};
ClientsServer	  clients  = {};
ClipsServer		  clips	   = {};
TargetsServer	  targets  = {};
IterTimesServer	  it_times = {};
BinaryImageServer image	   = {};

char answer_buffer [8192];	// Used by x_describe_x.
char answer_block  [8208];	// 4K + final zero aligned to 16 bytes

char	b64chars[]		= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
uint8_t	b64inverse[256]	= {0};

#include "thousand_sequences.hpp"

/** \brief Serializes a complete binary block (fixed size) as a base-64 string which is Python compatible.

	\param p_in	The address of the ImageBlock as a generic unsigned pointer.

	\return	The address of a fixed buffer big enough to hold the output.
*/
char *image_block_as_string(uint8_t *p_in) {

	char *p_out	  = (char *) &answer_block;

	uint8_t v, w;
	for (int i = 0; i < 2048; i++) {
		v = *(p_in++);
		*(p_out++) = b64chars[v >> 2];

		w = *(p_in++);
		*(p_out++) = b64chars[((v << 4) & 0x30) | w >> 4];

		v = *(p_in++);
		*(p_out++) = b64chars[((w << 2) & 0x3c) | v >> 6];
		*(p_out++) = b64chars[v & 0x3f];
	}
	*p_out = 0;

	return (char *) &answer_block;
}


/** \brief Serializes a complete binary block (fixed size) as a base-64 string which is Python compatible.

	\param blk	An ImageBlock passed by reference.

	\return	The address of a fixed buffer big enough to hold the output.
*/
char *image_block_as_string(ImageBlock &blk) {
	uint8_t *p_in = (uint8_t *) &blk;

	return image_block_as_string(p_in);
}


/** \brief Deserializes a base-64 string which is Python compatible as a complete binary ImageBlock (fixed size).

	\param blk	An ImageBlock passed by reference.
	\param p_in	The address of the ImageBlock as a generic unsigned pointer.

	\return	The address of a fixed buffer big enough to hold the output.
*/
bool string_as_image_block(ImageBlock &blk, char *p_in) {

	if (b64inverse[0] != 0xc0) {
		// create the inverse of b64chars[] in b64inverse[] just once if not exists
		memset(&b64inverse, 0xc0c0c0c0, sizeof(b64inverse));

		for (int i = 0; i < 64; i++)
			b64inverse[(int) b64chars[i]] = i;
	}
	uint8_t *p_out = (uint8_t *) &blk;

	uint8_t v, w;
	for (int i = 0; i < 2048; i++) {
		v = b64inverse[(int) *(p_in++)];
		if ((v & 0xc0) != 0)
			return false;

		w = b64inverse[(int) *(p_in++)];
		if ((w & 0xc0) != 0)
			return false;

		*(p_out++) = (v << 2) | (w >> 4);

		v = b64inverse[(int) *(p_in++)];
		if ((v & 0xc0) != 0)
			return false;

		*(p_out++) = (w << 4) | (v >> 2);

		w = b64inverse[(int) *(p_in++)];
		if ((w & 0xc0) != 0)
			return false;

		*(p_out++) = (v << 6) | w;
	}

	return (uint_fast64_t) p_out - (uint_fast64_t) &blk == sizeof(ImageBlock);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: EventsServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Create a new Events object that can be used via the Python interface.

	\return	A unique ID that can be passed as the id parameter for any method in the Python interface.

To free the resources allocated by this ID, the (python) caller must call destroy_events() with the id and never use the same
id after that.
*/
int new_events() {

	events[++events_num] = new Events();

	return events_num;
}


/** \brief Destroy a Events object that was used via the Python interface.

	\param id  The id returned by a previous new_events() call.

	\return	 True on success.
*/
bool destroy_events(int id) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return false;

	delete it->second;

	events.erase(it);

	return true;
}


/** \brief Process a row from a transaction file in an Events object stored by the EventsServer.

	\param id	The id returned by a previous new_events() call.
	\param p_e	The "emitter". A C/Python string representing "owner of event".
	\param p_d	The "description". A C/Python string representing "the event".
	\param w	The "weight". A double representing a weight of the event.

	\return	 True on success.
*/
bool events_insert_row(int id, char *p_e, char *p_d, double w) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return false;

	it->second->insert_row(p_e, p_d, w);

	return true;
}


/** \brief Define events explicitly in an Events object stored by the EventsServer.

	\param id	The id returned by a previous new_events() call.
	\param p_e	The "emitter". A C/Python string representing "owner of event".
	\param p_d	The "description". A C/Python string representing "the event".
	\param w	The "weight". A double representing a weight of the event.
	\param code	A unique code number identifying the event.

	\return	 True on success.
*/
bool events_define_event(int id, char *p_e, char *p_d, double w, int code) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return false;

	return it->second->define_event(p_e, p_d, w, code);
}


/** \brief Events optimizer.

	Optimizes the events to maximize prediction signal. (F1 score over same number of positives.)
	It converts code values many-to-one trying to group event codes into categories that represent similar events.

	Before starting, a non-optimized Events object must be populated with an initial set of codes we want to reduce by assigning
	new many-to-one codes to them.

	The algorithm initially removes all codes not found in the clips object. This completely removes them.

	The algorithm builds a list of most promising (not already used) codes at the beginning of each step by full tree search.
	From that list, each code is tried downwards as {noise, new_code, last_code} for score improvement above threshold
	up to codes_per_step steps. And assigned a new code accordingly.
	The codes assigned become part of the internal EventCodeMap and in the next step they will replace their old values.

	When the algorithm finishes, the internal EventCodeMap is used to rename the object codes and the whole process is reported.

	\param id				The id returned by a previous new_events() call. The object must be empty (never called).
	\param id_clips			The id of a clips object with the same codes and clips for a set of clients whose prediction we optimize.
	\param id_targets		The id of a Targets object whose internal TargetMap defines the targets. (Internally a new Targets object
							will be used to make the predictions we want to optimize.)
	\param num_steps		The number of steps to iterate. The method will stop early if no codes are found at a step.
	\param codes_per_step	The number of codes to be tried from the top of the priority list at each step.
	\param threshold		A minimum threshold, below which a score change is not considered improvement.
	\param p_force_include	An optional pointer to a set of codes that must be included before starting.
	\param p_force_exclude	An optional pointer to a set of codes that will excluded and set to the base code.
	\param x_form			The x_form argument to fit the internal Targets object prediction model.
	\param agg				The agg argument to fit the internal Targets object prediction model.
	\param p				The p argument to fit the internal Targets object prediction model.
	\param depth			The depth argument to fit the internal Targets object prediction model.
	\param as_states		The as_states argument to fit the internal Targets object prediction model.
	\param exp_decay		Exponential Decay Factor applied to the internal score in terms of depth. That score selects what
							codes enter the model. The decay is applied to the average tree depth. 0 is no decay, default
							value = 0.00693 decays to 0.5 in 100 steps.
	\param lower_bound_p	Another p for lower bound, but applied to the scoring process rather than the model.
	\param log_lift			A boolean to set if lift (= LB(included)/LB(after inclusion)) is log() transformed or not.

	\return					A lf separated report string that contains either "ERROR" or "success" as the first line.
*/
char *events_optimize_events(int id, int id_clips, int id_targets, int num_steps, int codes_per_step, double threshold,
							 char *force_include, char *force_exclude, char *x_form, char *agg, double p, int depth, int as_states,
							 double exponential_decay, double lower_bound_p, bool log_lift) {
	answer_buffer[0] = 0;

	EventsServer::iterator it = events.find(id);

	if (it == events.end()) {
		sprintf(answer_buffer, "ERROR\nEvents object not found.");

		return answer_buffer;
	}

	ClipsServer::iterator it_clip = clips.find(id_clips);

	if (it_clip == clips.end()) {
		sprintf(answer_buffer, "ERROR\nClips object not found.");

		return answer_buffer;
	}

	TargetsServer::iterator it_targ = targets.find(id_targets);

	if (it_targ == targets.end()) {
		sprintf(answer_buffer, "ERROR\nTargets object not found.");

		return answer_buffer;
	}

	pCodeSet p_force_include = new CodeSet();
	pCodeSet p_force_exclude = new CodeSet();

	while (force_include[0] != 0) {
		uint64_t code;
		int l = sscanf(force_include, "%li", &code);
		if (l < 1)
			break;
		p_force_include->insert(code);
		force_include += l;
		if (force_include[0] == ',')
			force_include++;
	}

	while (force_exclude[0] != 0) {
		uint64_t code;
		int l = sscanf(force_exclude, "%li", &code);
		if (l < 1)
			break;
		p_force_exclude->insert(code);
		force_exclude += l;
		if (force_exclude[0] == ',')
			force_exclude++;
	}

	Transform xfm = strcmp("log", x_form) == 0 ? tr_log : tr_linear;

	Aggregate ag = strcmp("mean", agg) == 0 ? ag_mean : strcmp("longest", agg) == 0 ? ag_longest : ag_minimax;

	String ret = it->second->optimize_events(*it_clip->second, *it_targ->second->p_target(), num_steps, codes_per_step, threshold,
 											 p_force_include, p_force_exclude, xfm, ag, p, depth, as_states, exponential_decay,
											 lower_bound_p, log_lift);

	REELS_logger.log = ret + "\ncode_performance\n" + REELS_logger.log;

	delete p_force_exclude;
	delete p_force_include;

	return (char *) REELS_logger.log.c_str();
}


/** \brief Pushes raw image blocks into an initially empty Events object and finally creates it already populated with the binary image.

	\param id		The id returned by a previous new_events() call. The object must be empty (never called).
	\param p_block	If non-empty, a block in the right order making a binary image obtained for a previous events_save() call.
					If empty, an order to .load() the Events object and destroyed the previously stored blocks.

	\return	 True on success.
*/
bool events_load_block(int id, char *p_block) {

	EventsServer::iterator it_events = events.find(id);

	if (it_events == events.end())
		return false;

	if (p_block[0] == 0) {
		BinaryImageServer::iterator it_image = image.find(id);

		if (it_image == image.end())
			return false;

		bool ok = it_events->second->load(it_image->second);

		destroy_binary_image_iterator(id);

		return ok;
	}

	ImageBlock blk;

	if (!string_as_image_block(blk, p_block)) {
		destroy_binary_image_iterator(id);

		return false;
	}

	BinaryImageServer::iterator it_image = image.find(id);

	if (it_image == image.end()) {
		if (blk.block_num != 1)
			return false;

		pBinaryImage p_bi = new BinaryImage;

		p_bi->push_back(blk);

		image[id] = p_bi;

		return true;
	}

	ImageBlock *p_last = &it_image->second->back();

	if (blk.block_num != p_last->block_num + 1)
		return false;

	it_image->second->push_back(blk);

	return true;
}


/** \brief Saves an Events object as a BinaryImage.

	\param id  The id returned by a previous new_events() call.

	\return	 0 on error, or a binary_image_id > 0 which is the same as id and must be destroyed using destroy_binary_image_iterator()
*/
int events_save(int id) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return 0;

	pBinaryImage p_bi = new BinaryImage;

	if (!it->second->save(p_bi)) {
		delete p_bi;

		return 0;
	}

	destroy_binary_image_iterator(id);

	image[id] = p_bi;

	return id;
}


bool parse_bin_event_pt(char *line, BinEventPt &ev) {

	char *pt = strchr(line, '\t');

	if (pt == nullptr)
		return false;

	uint64_t l = (uint64_t) pt - (uint64_t) line;

	if (line[0] != '<' || l != 18 || sscanf(line, "<%016lx>\t", &ev.e) != 1)
		ev.e = MurmurHash64A(line, l);

	char *pt2 = strchr(++pt, '\t');

	if (pt2 == nullptr)
		return false;

	l = (uint64_t) pt2 - (uint64_t) pt;

	if (pt[0] != '<' || l != 18 || sscanf(pt, "<%016lx>\t", &ev.d) != 1)
		ev.d = MurmurHash64A(pt, l);

	char *pt3 = strchr(++pt2, '\t');

	if (pt3 == nullptr)
		return false;

	return sscanf(pt2, "%lf\t", &ev.w) == 1;
}


/** \brief Describes the internal representation of an event. (Iterate through all.)

	\param id		  The id returned by a previous new_events() call.
	\param prev_event The previous description, either complete or the (emitter, description, weight) triple alone, or empty for the first.

	\return	On success, it will return either tab separated emitter, description, weight, code (if store_strings is true) or
			the same with hashes corresponding to strings instead of strings if false.
*/
char *events_describe_next_event(int id, char *prev_event) {

	answer_buffer[0] = 0;

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return answer_buffer;

	int ll = strlen(prev_event);

	BinEventPt ev;
	EventMap::iterator it_ev_end = it->second->events_end();
	EventMap::iterator it_ev = it_ev_end;

	if (ll == 0)
		it_ev = it->second->events_begin();
	else if (parse_bin_event_pt(prev_event, ev)) it_ev = it->second->events_next_after_find(ev);

	if (it_ev != it_ev_end) {
		double round_w = round(WEIGHT_PRECISION*it_ev->first.w)/WEIGHT_PRECISION;
		if (it->second->store_strings) {
			String e = it->second->get_str(it_ev->first.e);
			String d = it->second->get_str(it_ev->first.d);
			sprintf(answer_buffer, "%s\t%s\t%.5f\t%li", e.c_str(), d.c_str(), round_w, it_ev->second.code);
		} else
			sprintf(answer_buffer, "<%016lx>\t<%016lx>\t%.5f\t%li", it_ev->first.e, it_ev->first.d, round_w, it_ev->second.code);
	}

	return answer_buffer;
}


/** \brief Sets the public property max_num_events in an Events object stored by the EventsServer.

	\param id		  The id returned by a previous new_events() call.
	\param max_events The value to apply to max_num_events.

	\return	 True on success.
*/
bool events_set_max_num_events(int id, int max_events) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return false;

	it->second->set_max_num_events(max_events);

	return true;
}


/** \brief Sets the public property store_strings in an Events object stored by the EventsServer.

	\param id	 The id returned by a previous new_events() call.
	\param store True for storing the string contents.

	\return	 True on success.
*/
bool events_set_store_strings(int id, bool store) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return false;

	it->second->set_store_strings(store);

	return true;
}


/** \brief Return the number of events stored in an Events object stored by the EventsServer.

	\param id	The id returned by a previous new_events() call.

	\return	The size of the internal EventMap() or a negative error code on failure.
*/
int events_num_events(int id) {

	EventsServer::iterator it = events.find(id);

	if (it == events.end())
		return -1;

	return it->second->num_events();
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: ClientsServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Create a new Clients object that can be used via the Python interface.

	\return	A unique ID that can be passed as the id parameter for any method in the Python interface.

To free the resources allocated by this ID, the (python) caller must call destroy_clients() with the id and never use the same
id after that.
*/
int new_clients() {

	clients[++clients_num] = new Clients();

	return clients_num;
}


/** \brief Destroy a Clients object that was used via the Python interface.

	\param id  The id returned by a previous new_clients() call.

	\return	 True on success.
*/
bool destroy_clients(int id) {

	ClientsServer::iterator it = clients.find(id);

	if (it == clients.end())
		return false;

	delete it->second;

	clients.erase(it);

	return true;
}


/** \brief Return the hash of a client ID as a decimal string.

	\param id	 The id returned by a previous new_clients() call.
	\param p_cli The "client". A string representing "the actor".

	\return	 The hash of the selected client if as a decimal string.
*/
char *clients_hash_client_id(int id, char *p_cli) {

	ClientsServer::iterator it = clients.find(id);

	answer_buffer[0] = 0;

	if (it != clients.end())
		sprintf(answer_buffer, "<%016lx>", it->second->hash_client_id(p_cli));

	return answer_buffer;
}


/** \brief Add a client ID to a Clients object stored by the ClientsServer only if new.

	\param id	 The id returned by a previous new_clients() call.
	\param p_cli The "client". A string representing "the actor".

	\return	 True on success. False on id not found or p_cli is empty or already a client.
*/
bool clients_add_client_id(int id, char *p_cli) {

	ClientsServer::iterator it = clients.find(id);

	if (it == clients.end())
		return false;

	int ll = strlen(p_cli);

	if (ll == 0)
		return false;

	ElementHash client_hash = MurmurHash64A(p_cli, ll);

	if (it->second->id_set.find(client_hash) != it->second->id_set.end())
		return false;

	it->second->add_client_id(p_cli);

	return true;
}


/** \brief Return the hash of a stored client ID by index as a decimal string.

	\param id	The id returned by a previous new_clients() call.
	\param idx	The index of the client in the container (In range 0..size() - 1).

	\return	 The hash of the selected client id as a decimal string.
*/
char *clients_hash_by_index(int id, int idx) {

	ClientsServer::iterator it = clients.find(id);

	answer_buffer[0] = 0;

	if (it != clients.end() && idx >= 0 && idx < (int) it->second->id.size())
		sprintf(answer_buffer, "<%016lx>", it->second->id[idx]);

	return answer_buffer;
}


/** \brief Return the number of clients stored in an Clients object stored by the ClientsServer.

	\param id The id returned by a previous new_clients() call.

	\return	The size of the internal ClientIDSet() or a negative error code on failure.
*/
int clients_num_clients(int id) {

	ClientsServer::iterator it = clients.find(id);

	if (it == clients.end())
		return -1;

	return it->second->id.size();
}


/** \brief Pushes raw image blocks into an initially empty Clients object and finally creates it already populated with the binary image.

	\param id		The id returned by a previous new_clients() call. The object must be empty (never called).
	\param p_block	If non-empty, a block in the right order making a binary image obtained for a previous clients_save() call.
					If empty, an order to .load() the Clients object and destroyed the previously stored blocks.

	\return	 True on success.
*/
bool clients_load_block(int id, char *p_block) {

	ClientsServer::iterator it_clients = clients.find(id);

	if (it_clients == clients.end())
		return false;

	if (p_block[0] == 0) {
		BinaryImageServer::iterator it_image = image.find(id);

		if (it_image == image.end())
			return false;

		bool ok = it_clients->second->load(it_image->second);

		destroy_binary_image_iterator(id);

		return ok;
	}

	ImageBlock blk;

	if (!string_as_image_block(blk, p_block)) {
		destroy_binary_image_iterator(id);

		return false;
	}

	BinaryImageServer::iterator it_image = image.find(id);

	if (it_image == image.end()) {
		if (blk.block_num != 1)
			return false;

		pBinaryImage p_bi = new BinaryImage;

		p_bi->push_back(blk);

		image[id] = p_bi;

		return true;
	}

	ImageBlock *p_last = &it_image->second->back();

	if (blk.block_num != p_last->block_num + 1)
		return false;

	it_image->second->push_back(blk);

	return true;
}


/** \brief Saves an Clients object as a BinaryImage.

	\param id  The id returned by a previous new_clients() call.

	\return	 0 on error, or a binary_image_id > 0 which is the same as id and must be destroyed using destroy_binary_image_iterator()
*/
int clients_save(int id) {

	ClientsServer::iterator it = clients.find(id);

	if (it == clients.end())
		return 0;

	pBinaryImage p_bi = new BinaryImage;

	if (!it->second->save(p_bi)) {
		delete p_bi;

		return 0;
	}

	destroy_binary_image_iterator(id);

	image[id] = p_bi;

	return id;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: ClipsServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Create a new Clips object that can be used via the Python interface.

	\param id_clients The id returned by a previous new_clients() call passed to the constructor.
	\param id_events  The id returned by a previous new_events() call passed to the constructor.

	\return	A unique ID that can be passed as the id parameter for any method in the Python interface or -1 on error.

To free the resources allocated by this ID, the (python) caller must call destroy_clips() with the id and never use the same
id after that.
*/
int new_clips(int id_clients, int id_events) {

	ClientsServer::iterator it_clients = clients.find(id_clients);

	if (it_clients == clients.end())
		return -1;

	EventsServer::iterator it_events = events.find(id_events);

	if (it_events == events.end())
		return -1;

	clips[++clips_num] = new Clips(*it_clients->second, *it_events->second);

	return clips_num;
}


/** \brief Destroy a Clips object that was used via the Python interface.

	\param id  The id returned by a previous new_clips() call.

	\return	 True on success.
*/
bool destroy_clips(int id) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return false;

	delete it->second;

	clips.erase(it);

	return true;
}


/** \brief Sets the public property time_format to simplify the python interface in a Clips object stored by the ClipsServer.

	\param fmt The time format in standard calendar time format
					http://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html

	\return	 True on valid id.
*/
bool clips_set_time_format(int id, char *fmt) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return false;

	it->second->set_time_format(fmt);

	return true;
}


/** \brief Process a row from a transaction file, to add the event to the client's timeline in a Clips object stored by the ClipsServer.

	\param id	The id returned by a previous new_clips() call.
	\param p_e	The "emitter". A C/Python string representing "owner of event".
	\param p_d	The "description". A C/Python string representing "the event".
	\param w	The "weight". A double representing a weight of the event.
	\param p_c	The "client". A C/Python string representing "the actor".
	\param p_t	The "time". A timestamp of the event as a C/Python string. (The format is given via set_time_format().)

	\return	 True on insertion. False usually just means, the event is not in events or the client is not in clients.
			 Occasionally, it may be a time parsing error or id not found.
*/
bool clips_scan_event(int id, char *p_e, char *p_d, double w, char *p_c, char *p_t) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return false;

	return it->second->scan_event(p_e, p_d, w, p_c, p_t);
}


/** \brief Return the hash of the client ID of a clip defined the previous value or zero for the first one as a decimal string.

	\param id			The id returned by a previous new_clients() call.
	\param prev_hash	The previous hash returned by this function to use as an iterator.

	\return	 The hash of the selected client id as a hexadecimal string.
*/
char *clips_hash_by_previous(int id, char *prev_hash) {

	ClipsServer::iterator it = clips.find(id);

	answer_buffer[0] = 0;

	if (it == clips.end())
		return answer_buffer;

	ElementHash hh = 0;

	int ll = strlen(prev_hash);

	if (prev_hash[0] == '<' && ll == 18 && sscanf(prev_hash, "<%016lx>", &hh) != 1)
		hh = 0;

	ClipMap::iterator it_clip;
	if (hh == 0)
		it_clip = it->second->clip_map()->begin();
	else {
		it_clip = it->second->clip_map()->find(hh);

		if (it_clip == it->second->clip_map()->end())
			return answer_buffer;

		++it_clip;

		if (it_clip == it->second->clip_map()->end())
			return answer_buffer;
	}

	sprintf(answer_buffer, "<%016lx>", it_clip->first);

	return answer_buffer;
}


/** \brief Pushes raw image blocks into an initially empty Clips object and finally creates it already populated with the binary image.

	\param id		The id returned by a previous new_clips() call. The object must be empty (never called).
	\param p_block	If non-empty, a block in the right order making a binary image obtained for a previous clips_save() call.
					If empty, an order to .load() the Clips object and destroyed the previously stored blocks.

	\return	 True on success.
*/
bool clips_load_block(int id, char *p_block) {

	ClipsServer::iterator it_clips = clips.find(id);

	if (it_clips == clips.end())
		return false;

	if (p_block[0] == 0) {
		BinaryImageServer::iterator it_image = image.find(id);

		if (it_image == image.end())
			return false;

		bool ok = it_clips->second->load(it_image->second);

		destroy_binary_image_iterator(id);

		return ok;
	}

	ImageBlock blk;

	if (!string_as_image_block(blk, p_block)) {
		destroy_binary_image_iterator(id);

		return false;
	}

	BinaryImageServer::iterator it_image = image.find(id);

	if (it_image == image.end()) {
		if (blk.block_num != 1)
			return false;

		pBinaryImage p_bi = new BinaryImage;

		p_bi->push_back(blk);

		image[id] = p_bi;

		return true;
	}

	ImageBlock *p_last = &it_image->second->back();

	if (blk.block_num != p_last->block_num + 1)
		return false;

	it_image->second->push_back(blk);

	return true;
}


/** \brief Saves an Clips object as a BinaryImage.

	\param id  The id returned by a previous new_clips() call.

	\return	 0 on error, or a binary_image_id > 0 which is the same as id and must be destroyed using destroy_binary_image_iterator()
*/
int clips_save(int id) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return 0;

	pBinaryImage p_bi = new BinaryImage;

	if (!it->second->save(p_bi)) {
		delete p_bi;

		return 0;
	}

	destroy_binary_image_iterator(id);

	image[id] = p_bi;

	return id;
}


/** \brief Describes the internal representation of a clip.

	\param id		 The id returned by a previous new_clips() call.
	\param client_id The hash identifying a client or the client id.

	\return	On success, it will return a tab separated list of codes corresponding to the clip.
*/
char *clips_describe_clip(int id, char *client_id) {

	answer_buffer[0] = 0;

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return answer_buffer;

	ElementHash hh = 0;

	int ll = strlen(client_id);

	if (client_id[0] == '<' && ll == 18 && sscanf(client_id, "<%016lx>", &hh) != 1)
		hh = 0;

	if (hh == 0 && ll > 0)
		hh = MurmurHash64A(client_id, ll);

	if (hh == 0)
		return answer_buffer;

	ClipMap::iterator it_clip_map = it->second->clip_map()->find(hh);

	if (it_clip_map == it->second->clip_map()->end())
		return answer_buffer;

	bool first = true;

	char *pt = answer_buffer;

	for (Clip::iterator it_clip = it_clip_map->second.begin(); it_clip != it_clip_map->second.end(); ++it_clip) {
		if (!first)
			*pt++ = '\t';

		first = false;

		int l = sprintf(pt, "%li", it_clip->second);

		pt += l;
	}

	return answer_buffer;
}


/** \brief Return the number of clips stored in the internal ClipMap in a Clips object stored by the ClipsServer.

	\param id	The id returned by a previous new_clips() call.

	\return	The number clips in the internal ClipMap.
*/
int clips_num_clips(int id) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return false;

	return it->second->clip_map()->size();
}


/** \brief Return the number of events stored in the internal ClipMap in a Clips object stored by the ClipsServer.

	\param id	The id returned by a previous new_clips() call.

	\return	The total count of events aggregating all the clips in the internal ClipMap or -1 on error.
*/
int clips_num_events(int id) {

	ClipsServer::iterator it = clips.find(id);

	if (it == clips.end())
		return -1;

	return it->second->num_events();
}


/** \brief Generates a constant sequence of codes for testing the Event Optimizer.

	This returns one of the 500 non target sequences or one of the 500 target sequences.

	\param seq_num	The sequence id (in range 0.499).
	\param target	True for one of the target sequences, false for non target.

	\return	 A sequence of integer codes as a comma separated string. Empty if seq_num is out of range.
*/
char *clips_test_sequence(int seq_num, bool target) {
	answer_buffer[0] = 0;

	if (seq_num >= 0 && seq_num < 500) {
		if (target)
			seq_num += 500;

		sprintf(answer_buffer, "%s", TEST_SEQ[seq_num].c_str());
	}

	return answer_buffer;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: TargetsServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Create a new Targets object that can be used via the Python interface.

	\param id_clips	The id returned by a previous new_clips() call passed to the constructor.

	\return	A unique ID that can be passed as the id parameter for any method in the Python interface or -1 on error.

To free the resources allocated by this ID, the (python) caller must call destroy_targets() with the id and never use the same
id after that.
*/
int new_targets(int id_clips) {

	ClipsServer::iterator it_clips = clips.find(id_clips);

	if (it_clips == clips.end())
		return -1;

	if (it_clips->second->clip_map()->size() == 0)
		targets[++targets_num] = new Targets(nullptr, {});
	else
		targets[++targets_num] = new Targets(it_clips->second->clip_map(), {});

	return targets_num;
}


/** \brief Destroy a Targets object that was used via the Python interface.

	\param id  The id returned by a previous new_targets() call.

	\return	 True on success.
*/
bool destroy_targets(int id) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return false;

	delete it->second;

	targets.erase(it);

	return true;
}


/** \brief Sets the public property time_format to simplify the python interface in a Targets object stored by the TargetsServer.

	\param fmt The time format in standard calendar time format
					http://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html

	\return	 True on valid id.
*/
bool targets_set_time_format(int id, char *fmt) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return false;

	it->second->set_time_format(fmt);

	return true;
}


/** \brief Utility to fill the internal TargetMap target in a Targets object stored by the TargetsServer.

	The TargetMap can be initialized and given to the constructor, or an empty TargetMap can be given to the constructor ans
	initialized by this method. From the python interface only the latter is available.

	\param id	The id returned by a previous new_targets() call.
	\param p_c	The "client". A C/Python string representing "the actor".
	\param p_t	The "time". A timestamp of the event as a C/Python string. (The format is given via set_time_format().)

	\return	 True on new clients. False if the client is already in the TargetMap or the time is in the wrong format.
			 Also false if the id is not found.
*/
bool targets_insert_target(int id, char *p_c, char *p_t) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return false;

	return it->second->insert_target(p_c, p_t);
}


/** \brief Fit the prediction model in a Targets object stored by the TargetsServer.

	\param x_form	 A possible transformation of the times. (Currently "log" or "linear".)
	\param agg		 The mechanism used for the aggregation. (Currently "minimax", "mean" or "longest".)
	\param p		 The width of the confidence interval for the binomial proportion used to calculate the lower bound.
					 (E.g., p = 0.5 will estimate a lower bound of a symmetric CI with coverage of 0.5.)
	\param depth	 The maximum depth of the tree (maximum sequence length learned).
	\param as_states Treat events as states by removing repeated ones from the ClipMap keeping the time of the first instance only.
					 When used, the ClipMap passed to the constructor by reference will be converted to states as a side effect.

	Fit can only be called once in the life of a Targets object and predict() cannot be called before fit().

	\return	 True on success. Error if already fitted, wrong arguments or the id is not found.
*/
bool targets_fit(int id, char *x_form, char *agg, double p, int depth, int as_states) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return false;

	Transform xfm = strcmp("log", x_form) == 0 ? tr_log : tr_linear;

	Aggregate ag = strcmp("mean", agg) == 0 ? ag_mean : strcmp("longest", agg) == 0 ? ag_longest : ag_minimax;

	return it->second->fit(xfm, ag, p, depth, as_states);
}


/** \brief Predict time to target for all the clients in a given Clients object whose clips have been used to fit the model in a Targets
object stored by the TargetsServer.

	predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

	\param id		  The id returned by a previous new_targets() call.
	\param id_clients The id returned by a previous new_clients() call passed to the constructor.

	\return	 The index of an iterator (valid for size_result_iterator(), next_result_iterator() and destroy_result_iterator() calls)
	with the times of the predictions in seconds or -1 on error such as id not found.
*/
int	targets_predict_clients(int id, int id_clients) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return -1;

	ClientsServer::iterator it_clients = clients.find(id_clients);

	if (it_clients == clients.end())
		return -1;

	TimesToTarget ret = it->second->predict(*it_clients->second);

	if (ret.size() == 0)
		return -1;

	it_times[++it_times_num] = new TimesToTarget(ret);

	return it_times_num;
}


/** \brief Predict time to target for a set of clients whose clips are given in a ClipMap in a Targets object stored by the TargetsServer.

	predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

	\param id	The id returned by a previous new_targets() call.
	\param id_clips	The id returned by a previous new_clips() call passed to the constructor.

	\return	 The index of an iterator (valid for size_result_iterator(), next_result_iterator() and destroy_result_iterator() calls)
	with the times of the predictions in seconds or -1 on error such as id not found.
*/
int	targets_predict_clips(int id, int id_clips) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return -1;

	ClipsServer::iterator it_clips = clips.find(id_clips);

	if (it_clips == clips.end())
		return -1;

	TimesToTarget ret = it->second->predict(it_clips->second->clip_map());

	if (ret.size() == 0)
		return -1;

	it_times[++it_times_num] = new TimesToTarget(ret);

	return it_times_num;
}


/** \brief Pushes raw image blocks into an initially empty Targets object and finally creates it already populated with the binary image.

	\param id		The id returned by a previous new_targets() call. The object must be empty (never called).
	\param p_block	If non-empty, a block in the right order making a binary image obtained for a previous targets_save() call.
					If empty, an order to .load() the Targets object and destroyed the previously stored blocks.

	\return	 True on success.
*/
bool targets_load_block(int id, char *p_block) {

	TargetsServer::iterator it_targets = targets.find(id);

	if (it_targets == targets.end())
		return false;

	if (p_block[0] == 0) {
		BinaryImageServer::iterator it_image = image.find(id);

		if (it_image == image.end())
			return false;

		bool ok = it_targets->second->load(it_image->second);

		destroy_binary_image_iterator(id);

		return ok;
	}

	ImageBlock blk;

	if (!string_as_image_block(blk, p_block)) {
		destroy_binary_image_iterator(id);

		return false;
	}

	BinaryImageServer::iterator it_image = image.find(id);

	if (it_image == image.end()) {
		if (blk.block_num != 1)
			return false;

		pBinaryImage p_bi = new BinaryImage;

		p_bi->push_back(blk);

		image[id] = p_bi;

		return true;
	}

	ImageBlock *p_last = &it_image->second->back();

	if (blk.block_num != p_last->block_num + 1)
		return false;

	it_image->second->push_back(blk);

	return true;
}


/** \brief Saves an Targets object as a BinaryImage.

	\param id  The id returned by a previous new_targets() call.

	\return	 0 on error, or a binary_image_id > 0 which is the same as id and must be destroyed using destroy_binary_image_iterator()
*/
int targets_save(int id) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return 0;

	pBinaryImage p_bi = new BinaryImage;

	if (!it->second->save(p_bi)) {
		delete p_bi;

		return 0;
	}

	destroy_binary_image_iterator(id);

	image[id] = p_bi;

	return id;
}


/** \brief Returns the number of target points stored in the internal target variable.

	\param id  The id returned by a previous new_targets() call.

	\return	 -1 on error (wrong id) or the size.
*/
int targets_num_targets(int id) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return -1;

	return it->second->num_targets();
}


/** \brief Returns the index of a tree node by parent node and code.

	\param id		  The id returned by a previous new_targets() call.
	\param parent_idx The index of the parent node which is either 0 for root or returned from a previous targets_tree_node_idx() call.
	\param code		  The code that leads in the tree from the parent node to the child node.

	\return	On success, i.e, if both the parent index exists and contains the code, it will return the index of the child (-1 otherwise).
*/
int	targets_tree_node_idx(int id, int parent_idx, int code) {

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return -1;

	pCodeTree pT = it->second->p_tree();

	if (parent_idx < 0 || parent_idx >= (int) pT->size())
		return -1;

	pCodeTreeNode pN = &pT->at(parent_idx);

	ChildIndex::iterator jt = pN->child.find(code);

	if (jt == pN->child.end())
		return -1;

	return jt->second;
}


/** \brief Lists the children of a tree node.

	\param id	The id returned by a previous new_targets() call.
	\param idx	The index of the node in the tree, typically navigated using targets_tree_node_idx().

	\return	On success, it will return a tab separated list of integer codes.
*/
char *targets_tree_node_children(int id, int idx) {

	answer_buffer[0] = 0;

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return answer_buffer;

	pCodeTree pT = it->second->p_tree();

	if (idx < 0 || idx >= (int) pT->size())
		return answer_buffer;

	pCodeTreeNode pN = &pT->at(idx);

	bool first = true;

	char *pt = answer_buffer;

	for (ChildIndex::iterator it_child = pN->child.begin(); it_child != pN->child.end(); ++it_child) {
		if (!first)
			*pt++ = '\t';

		first = false;

		int l = sprintf(pt, "%li", it_child->first);

		pt += l;
	}

	return answer_buffer;
}


/** \brief Describes the internal representation of a tree node.

	\param id	The id returned by a previous new_targets() call.
	\param idx	The index of the node in the tree, typically navigated using targets_tree_node_idx().

	\return	On success, it will return a tab separated list n_seen, n_target, sum_time_d, num_codes.
*/
char *targets_describe_tree_node(int id, int idx) {

	answer_buffer[0] = 0;

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return answer_buffer;

	pCodeTree pT = it->second->p_tree();

	if (idx < 0 || idx >= (int) pT->size())
		return answer_buffer;

	pCodeTreeNode pN = &pT->at(idx);

	sprintf(answer_buffer, "%lu\t%lu\t%f\t%lu", pN->n_seen, pN->n_target, (double) pN->sum_time_d, pN->child.size());

	return answer_buffer;
}


/** \brief Describes some statistics of a fitted tree inside a Targets object.

	\param id	The id returned by a previous new_targets() call.

	\return	 On success, it will return a descriptive text with node count numbers for different node content.
*/
char *targets_describe_tree(int id) {

	answer_buffer[0] = 0;

	TargetsServer::iterator it = targets.find(id);

	if (it == targets.end())
		return answer_buffer;

	pCodeTree pT = it->second->p_tree();

	int siz = pT->size();

	pChar pC = (pChar) &answer_buffer;

	pC += sprintf((char *) pC, "tree.size() : %i\n\n", siz);

	int nt_zero  = 0;
	int nt_one   = 0;
	int nt_more  = 0;
	int nt_final = 0;
	int tt_one   = 0;
	int tt_more  = 0;
	int tt_final = 0;

	for (int i = 0; i < siz; i++) {
		if (pT->at(i).n_seen == 0)
			nt_zero++;
		else {
			if (pT->at(i).n_seen == 1) {
				if (pT->at(i).n_target > 0) tt_one++; else nt_one++;
			} else {
				if (pT->at(i).n_target > 0) tt_more++; else nt_more++;
			}
		}
		if (pT->at(i).child.size() == 0) {
			if (pT->at(i).n_target > 0) tt_final++; else nt_final++;
		}
	}

	pC += sprintf((char *) pC, "num_of_nodes_with_zero_visits : %i\n", nt_zero);
	pC += sprintf((char *) pC, "num_of_no_targets_one_visit   : %i\n", nt_one);
	pC += sprintf((char *) pC, "num_of_has_target_one_visit   : %i\n", tt_one);
	pC += sprintf((char *) pC, "num_of_no_targets_more_visits : %i\n", nt_more);
	pC += sprintf((char *) pC, "num_of_has_target_more_visits : %i\n", tt_more);
	pC += sprintf((char *) pC, "num_of_no_targets_final_node  : %i\n", nt_final);
	pC += sprintf((char *) pC, "num_of_has_target_final_node  : %i\n", tt_final);

	return answer_buffer;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: IterTimesServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Return the number of unread items in an iterator returned by predict().

	\param iter_id  The iter_id returned by a previous predict() call.

	\return			The number of unread items in the iterator.
*/
int size_result_iterator(int iter_id) {

	IterTimesServer::iterator it = it_times.find(iter_id);

	if (it == it_times.end())
		return 0;

	return it->second->size();
}


/** \brief Return the first unread item in an iterator returned by predict().

	\param iter_id  The iter_id returned by a previous predict() call.

	\return			The str_id of an insert()-ed set that matches the query.
*/
double next_result_iterator(int iter_id) {

	IterTimesServer::iterator it = it_times.find(iter_id);

	if ((it == it_times.end()) || it->second->size() == 0)
		return 0;

	double d = *it->second->data();

	it->second->erase(it->second->begin());

	return d;
}


/** \brief Destroy an iterator returned by predict().

	\param iter_id  The iter_id returned by a previous predict() call.
*/
void destroy_result_iterator(int iter_id) {

	IterTimesServer::iterator it = it_times.find(iter_id);

	if (it == it_times.end())
		return;

	delete it->second;

	it_times.erase(it);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python Implementation: BinaryImageServer
// -----------------------------------------------------------------------------------------------------------------------------------------

/** \brief Return the number of unread binary images blocks in an iterator returned by a save_as_binary_image() call.

	\param image_id	The image_id returned by a previous save_as_binary_image() call.

	\return			The number of unread blocks in the iterator.
*/
int size_binary_image_iterator(int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return 0;

	return it->second->size();
}


/** \brief Return the first unread binary images block in an iterator returned by a save_as_binary_image() call.

	\param image_id	The image_id returned by a previous save_as_binary_image() call.

	\return			The the binary image block serialized as base64 or an empty string on failure.
*/
char *next_binary_image_iterator(int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return (char *) "";

	uint8_t *p_in = (uint8_t *) it->second->data();

	char *p_ret = image_block_as_string(p_in);

	it->second->erase(it->second->begin());

	return p_ret;
}


/** \brief Destroy an iterator for a binary image(returned by save_as_binary_image()).

	\param image_id  The image_id returned by a previous save_as_binary_image() call.

	\return	 True on success.
*/
bool destroy_binary_image_iterator(int image_id) {

	BinaryImageServer::iterator it = image.find(image_id);

	if (it == image.end())
		return false;

	delete it->second;

	image.erase(it);

	return true;
}
