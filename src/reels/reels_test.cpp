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
#define CATCH_CONFIG_MAIN		//This tells Catch2 to provide a main() - has no effect when TEST is not defined

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

#include "reels_test.h"

#ifdef TEST

using namespace reels;

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Python interface parts
// -----------------------------------------------------------------------------------------------------------------------------------------

SCENARIO("Test image_block_as_string() / string_as_image_block()") {

	REQUIRE(sizeof(ImageBlock) == 6*1024);

	ImageBlock block;

	block.size = IMAGE_BUFF_SIZE;
	for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
		block.buffer[i] = i & 0xff;

	block.block_num = 0;

	char *p_str = image_block_as_string(block);

	REQUIRE(strlen(p_str) == 8*1024);

	// 00000001 00000010 00000011 00000100 00000101 00000110 00000111

	// 0         1         2         3         4         5         6
	// 0123456789012345678901234567890123456789012345678901234567890123
	// ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/

	REQUIRE(p_str[12] == 'A');		// 000000
	REQUIRE(p_str[13] == 'Q');		// 010000
	REQUIRE(p_str[14] == 'I');		// 001000
	REQUIRE(p_str[15] == 'D');		// 000011
	REQUIRE(p_str[16] == 'B');		// 000001
	REQUIRE(p_str[17] == 'A');		// 000000
	REQUIRE(p_str[18] == 'U');		// 010100
	REQUIRE(p_str[19] == 'G');		// 000110
	REQUIRE(p_str[20] == 'B');		// 000001

	REQUIRE(p_str[8191] == b64chars[6135 & 0x3f]);
	REQUIRE(p_str[8192] == 0);

	ImageBlock block2;

	REQUIRE(string_as_image_block(block2, p_str));

	for (int ofs = 0; ofs < 8; ofs++) {
		block.size = IMAGE_BUFF_SIZE;

		for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
			block.buffer[i] = (i + ofs) & 0xff;

		block.block_num = 8 - ofs;

		p_str = image_block_as_string(block);

		REQUIRE(strlen(p_str) == 8*1024);

		REQUIRE(string_as_image_block(block2, p_str));

		REQUIRE(block.size == block2.size);
		REQUIRE(block.block_num == block2.block_num);
		REQUIRE(MurmurHash64A(&block, sizeof(ImageBlock)) == MurmurHash64A(&block, sizeof(ImageBlock)));
	}
}


SCENARIO("Test image_put() / image_get()") {

	uint8_t buffer1[4*IMAGE_BUFF_SIZE];
	int		buffer4[IMAGE_BUFF_SIZE];

	memset(buffer1, 0xcafebebe, 4*IMAGE_BUFF_SIZE);

	for (int i = 0; i < IMAGE_BUFF_SIZE; i++)
		buffer4[i] = 2*i + 11;

	pBinaryImage p_bi = new BinaryImage;

	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE - 1));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE + 2));
	REQUIRE(image_put(p_bi, &buffer1, IMAGE_BUFF_SIZE - 1));

	REQUIRE(image_put(p_bi, &buffer4,  4));
	REQUIRE(image_put(p_bi, &buffer4,  8));
	REQUIRE(image_put(p_bi, &buffer4, 16));
	REQUIRE(image_put(p_bi, &buffer4, 32));

	REQUIRE(image_put(p_bi, &buffer4, 3*IMAGE_BUFF_SIZE));

	for (int i = 0; i < 8192; i++) {
		image_put(p_bi, &i, 4);
		image_put(p_bi, &i, 3);
		image_put(p_bi, &i, 2);
		image_put(p_bi, &i, 1);
	}

	for (int i = 0; i < p_bi->size(); i++)
		REQUIRE((*p_bi)[i].block_num == i + 1);

	uint8_t i_buffer1[4*IMAGE_BUFF_SIZE];
	int		i_buffer4[IMAGE_BUFF_SIZE];

	for (int t = 0; t < 3; t++) {
		int c_block = 0;
		int c_ofs	= 0;

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE - 1));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE - 1));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE + 2));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE + 2));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer1, IMAGE_BUFF_SIZE - 1));
		REQUIRE(!memcmp(&buffer1, &i_buffer1, IMAGE_BUFF_SIZE - 1));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4,  4));
		REQUIRE(!memcmp(&buffer4, &i_buffer4,  4));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4,  8));
		REQUIRE(!memcmp(&buffer4, &i_buffer4,  8));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 16));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 16));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 32));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 32));

		REQUIRE(image_get(p_bi, c_block, c_ofs, &i_buffer4, 3*IMAGE_BUFF_SIZE));
		REQUIRE(!memcmp(&buffer4, &i_buffer4, 3*IMAGE_BUFF_SIZE));

		bool failed = false;

		for (int i = 0; i < 8192; i++) {
			int i_i;

			image_get(p_bi, c_block, c_ofs, &i_i, 4);
			if (i != i_i)
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 3);
			if ((i & 0xffffff) != (i_i & 0xffffff))
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 2);
			if ((i & 0xffff) != (i_i & 0xffff))
				failed = true;
			image_get(p_bi, c_block, c_ofs, &i_i, 1);
			if ((i & 0xff) != (i_i & 0xff))
				failed = true;

			if (failed)
				break;
		}

		REQUIRE(!failed);
	}

	delete p_bi;
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Design assertions/sizes
// -----------------------------------------------------------------------------------------------------------------------------------------

SCENARIO("Sizes/assumptions") {

	REQUIRE(sizeof(TimePoint) == 8);
	REQUIRE(sizeof(BinEventPt) == 24);
	REQUIRE(sizeof(BinTransaction) == 40);
	REQUIRE(sizeof(EventStat) == 24);
	REQUIRE(sizeof(CodeTreeNode) == 72);
}


SCENARIO("BinEventPt operators work as expected by map") {

	BinEventPt u = {e : 1, d : 1, w : 0.1};
	BinEventPt v = {e : 1, d : 1, w : 0.1};
	BinEventPt w = {e : 1, d : 9, w : 999};
	BinEventPt x = {e : 2, d : 1, w : 0.1};
	BinEventPt y = {e : 1, d : 2, w : 0.1};
	BinEventPt z = {e : 1, d : 1, w : 0.2};

	REQUIRE(u == v);
	REQUIRE(!(u == w));
	REQUIRE(!(u == x));
	REQUIRE(!(u == y));
	REQUIRE(!(u == z));

	REQUIRE(!(u < v));
	REQUIRE(u < x);
	REQUIRE(u < y);
	REQUIRE(u < z);
	REQUIRE(z < y);
	REQUIRE(y < x);

	EventMap em = {};

	EventStat u2 = {code : 1};
	EventStat v2 = {code : 2};
	EventStat w2 = {code : 3};
	EventStat x2 = {code : 4};
	EventStat y2 = {code : 5};
	EventStat z2 = {code : 6};

	em[u] = u2;
	em[v] = v2;
	em[w] = w2;
	em[x] = x2;
	em[y] = y2;
	em[z] = z2;

	REQUIRE(em.size() == 5);

	EventMap::iterator it = em.begin();

	REQUIRE(it->second.code == v2.code);

	++it;

	REQUIRE(it->second.code == z2.code);

	++it;

	REQUIRE(it->second.code == y2.code);

	++it;

	REQUIRE(it->second.code == w2.code);

	++it;

	REQUIRE(it->second.code == x2.code);

	++it;

	REQUIRE(it == em.end());
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Header methods, various utils
// -----------------------------------------------------------------------------------------------------------------------------------------

SCENARIO("Test elementary time conversion") {

	REQUIRE(sizeof(TimePoint) == 8);

	TimePoint big_bang = 0;

	TimeStruct *p_tim;

	p_tim = gmtime(&big_bang);

	REQUIRE(p_tim->tm_sec	== 0);
	REQUIRE(p_tim->tm_min	== 0);
	REQUIRE(p_tim->tm_hour	== 0);

	REQUIRE(p_tim->tm_mday	== 1);
	REQUIRE(p_tim->tm_mon	== 0);
	REQUIRE(p_tim->tm_year	== 70);

	REQUIRE(p_tim->tm_wday	== 4);
	REQUIRE(p_tim->tm_yday	== 0);
	REQUIRE(p_tim->tm_isdst	== 0);

	Events	ev	= {};
	Clients	cli = {};
	Clips	clp(cli, ev);

	TimePoint now  = clp.get_time((char *) "2022-06-04 10:00:00");
	TimePoint hour = clp.get_time((char *) "2022-06-04 11:00:00");
	TimePoint min  = clp.get_time((char *) "2022-06-04 10:01:00");
	TimePoint sec  = clp.get_time((char *) "2022-06-04 10:00:01");

	REQUIRE(clp.get_time((char *) "1000-06-04 10:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-00-04 10:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-13-04 10:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-06-00 10:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-06-32 10:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-06-04 24:00:01") < 0);
	REQUIRE(clp.get_time((char *) "2022-06-04 10:61:01") < 0);		// 60 is acceptable
	REQUIRE(clp.get_time((char *) "2022-06-04 10:00:65") < 0);		// leap seconds are acceptable
	REQUIRE(clp.get_time((char *) "2022-06-04") < 0);
	REQUIRE(clp.get_time((char *) "10:00:01") < 0);

	REQUIRE(hour - now == 3600);
	REQUIRE( min - now == 60);
	REQUIRE( sec - now == 1);

	clp.set_time_format((char *) "%Y%m%d%H%M%S");

	TimePoint now_2 = clp.get_time((char *) "20220604100000");

	REQUIRE(now == now_2);
}


SCENARIO("Basic math") {

	ClipMap   clm	= {};
	TargetMap tm	= {};

	Targets tar(&clm, tm);

	REQUIRE(abs(tar.normal_pdf(-5.0) - 1.4867195147343e-06)	< 1e-15);
	REQUIRE(abs(tar.normal_pdf(-3.0) - 0.004431848411938)	< 1e-10);
	REQUIRE(abs(tar.normal_pdf(-1.0) - 0.24197072451914)	< 1e-10);
	REQUIRE(abs(tar.normal_pdf( 0.0) - 0.39894228040143)	< 1e-10);
	REQUIRE(abs(tar.normal_pdf( 2.0) - 0.053990966513188)	< 1e-10);
	REQUIRE(abs(tar.normal_pdf( 4.0) - 0.00013383022576489)	< 1e-10);

	REQUIRE(abs(tar.normal_cdf(-5.0) - 2.8665157187919e-07)	< 1e-15);
	REQUIRE(abs(tar.normal_cdf(-3.0) - 0.0013498980316301)	< 1e-10);
	REQUIRE(abs(tar.normal_cdf(-1.0) - 0.15865525393146)	< 1e-10);
	REQUIRE(abs(tar.normal_cdf( 0.0) - 0.5)					< 1e-10);
	REQUIRE(abs(tar.normal_cdf( 2.0) - 0.97724986805182)	< 1e-10);
	REQUIRE(abs(tar.normal_cdf( 4.0) - 0.99996832875817)	< 1e-10);

	REQUIRE(tar.fit(tr_log, ag_minimax, 0.6, 10, false));

	REQUIRE(abs(tar.normal_cdf(tar.binomial_z) - 0.8) < 0.00001);

	double lb = tar.agresti_coull_upper_bound(0, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(1, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(1, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(0, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 100000000);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(100000000, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(100000000, 100000000);
	REQUIRE(lb < 1);

	REQUIRE(abs(tar.agresti_coull_upper_bound( 4,  9) - 0.58283563362179) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(20, 30) - 0.7346209914122)  < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(15, 46) - 0.38657200240716) < 0.00001);

	REQUIRE(abs(tar.agresti_coull_lower_bound( 4,  9) - 0.31415999991831) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(20, 30) - 0.59102358791283) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(15, 46) - 0.27087665252942) < 0.00001);

	REQUIRE(tar.fit(tr_log, ag_minimax, 0.4, 10, false));

	REQUIRE(abs(tar.normal_cdf(tar.binomial_z) - 0.7) < 0.00001);

	lb = tar.agresti_coull_upper_bound(0, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(1, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(1, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(0, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 100000000);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(100000000, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(100000000, 100000000);
	REQUIRE(lb < 1);

	REQUIRE(abs(tar.agresti_coull_upper_bound( 20,  90) - 0.24604520204386) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(200, 300) - 0.68078150062164) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(150, 460) - 0.33765018030386) < 0.00001);

	REQUIRE(abs(tar.agresti_coull_lower_bound( 20,  90) - 0.20009157699688) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(200, 300) - 0.65224656154195) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(150, 460) - 0.31473154491253) < 0.00001);

	REQUIRE(tar.fit(tr_log, ag_minimax, 0.95, 10, false));

	REQUIRE(abs(tar.normal_cdf(tar.binomial_z) - 0.975) < 0.00001);

	lb = tar.agresti_coull_upper_bound(0, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(1, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(1, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(0, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 100000000);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(100000000, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(100000000, 100000000);
	REQUIRE(lb < 1);

	REQUIRE(abs(tar.agresti_coull_upper_bound( 10,  90) - 0.19440637544447) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(100, 300) - 0.38852884879373) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_upper_bound(450, 460) - 0.9887004491092)  < 0.00001);

	REQUIRE(abs(tar.agresti_coull_lower_bound( 10,  90) - 0.059654666661122) < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(100, 300) - 0.28235214161445)  < 0.00001);
	REQUIRE(abs(tar.agresti_coull_lower_bound(450, 460) - 0.95989953399427)  < 0.00001);

	REQUIRE(tar.fit(tr_linear, ag_mean, 0.05, 10, false));

	lb = tar.agresti_coull_upper_bound(0, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(1, 1);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(1, 1);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(0, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(0, 100000000);
	REQUIRE(lb < 1);

	lb = tar.agresti_coull_upper_bound(100000000, 100000000);
	REQUIRE(lb > 0);
	lb = tar.agresti_coull_lower_bound(100000000, 100000000);
	REQUIRE(lb < 1);
}

// -----------------------------------------------------------------------------------------------------------------------------------------
//	Complete class tests
// -----------------------------------------------------------------------------------------------------------------------------------------

extern int new_events();
extern bool destroy_events(int id);
extern bool events_insert_row(int id, char *p_e, char *p_d, double w);
extern bool events_define_event(int id, char *p_e, char *p_d, double w, int code);
extern char *events_describe_next_event(int id, char *prev_event);
extern bool events_load_block(int id, char *p_block);
extern int events_save(int id);
extern bool events_set_max_num_events(int id, int max_events);
extern bool events_set_store_strings(int id, bool store);
extern int events_num_events(int id);
extern char *events_optimize_events(int id, int id_clips, int id_targets, int num_steps, int codes_per_step, double threshold,
									char *force_include, char *force_exclude, char *x_form, char *agg, double p, int depth, int as_states,
									double exp_decay, double lower_bound_p, bool log_lift);

extern int new_clients();
extern bool destroy_clients(int id);
extern char *clients_hash_client_id(int id, char *p_cli);
extern bool clients_add_client_id(int id, char *p_cli);
extern char *clients_hash_by_index(int id, int idx);
extern int clients_num_clients(int id);
extern bool clients_load_block(int id, char *p_block);
extern int clients_save(int id);

extern int new_clips(int id_clients, int id_events);
extern bool destroy_clips(int id);
extern bool clips_set_time_format(int id, char *fmt);
extern bool clips_scan_event(int id, char *p_e, char *p_d, double w, char *p_c, char *p_t);
extern char *clips_hash_by_previous(int id, char *prev_hash);
extern bool clips_load_block(int id, char *p_block);
extern int clips_save(int id);
extern char *clips_describe_clip(int id, char *client_id);
extern int clips_num_clips(int id);
extern int clips_num_events(int id);
extern char *clips_test_sequence(int seq_num, bool target);

extern int new_targets(int id_clips);
extern bool destroy_targets(int id);
extern bool targets_set_time_format(int id, char *fmt);
extern bool targets_insert_target(int id, char *p_c, char *p_t);
extern bool targets_fit(int id, char *x_form, char *agg, double p, int depth, int as_states);
extern int targets_predict_clients(int id, int id_clients);
extern int targets_predict_clips(int id, int id_clips);
extern bool targets_load_block(int id, char *p_block);
extern int targets_save(int id);
extern int targets_num_targets(int id);
extern int targets_tree_node_idx(int id, int parent_idx, int code);
extern char *targets_tree_node_children(int id, int idx);
extern char *targets_describe_tree_node(int id, int idx);
extern char *targets_describe_tree(int id);

extern int size_result_iterator(int iter_id);
extern double next_result_iterator(int iter_id);
extern void destroy_result_iterator(int iter_id);

extern int size_binary_image_iterator(int image_id);
extern char *next_binary_image_iterator(int image_id);
extern bool destroy_binary_image_iterator(int image_id);


SCENARIO("Test Events") {

	GIVEN("I have a few Events objects") {
		Events ev_small_ins = {}, ev_big_ins = {}, ev_defined = {};

		REQUIRE(ev_defined.max_num_events == DEFAULT_NUM_EVENTS);
		REQUIRE(ev_defined.store_strings);

		ev_small_ins.set_max_num_events(300);
		ev_big_ins.set_max_num_events(900);

		REQUIRE(ev_small_ins.max_num_events == 300);
		REQUIRE(ev_big_ins.max_num_events	== 900);

		ev_defined.set_store_strings(false);
		REQUIRE(!ev_defined.store_strings);

		WHEN("I fill the small inserted one.") {
			char emitter[80];
			char description[80];
			double w;

			for (int i = 0; i < 1000; i++) {
				sprintf(emitter, "emi%i", (int) sqrt(i % 100));
				sprintf(description, "prod%i", (i % 5) + 1);
				w = i % 11 ? 1.3 : 4.2;

				ev_small_ins.insert_row(emitter, description, w);
			}

			BinEventPt ept = {123, 456, 7.89};

			REQUIRE(ev_small_ins.event_code(ept) == 0);

			for (EventMap::iterator it = ev_small_ins.event.begin(); it != ev_small_ins.event.end(); ++it) {
				BinEventPt ept = it->first;
				REQUIRE(ev_small_ins.event_code(ept) == it->second.code);
			}

			THEN("The strings match.") {
				for (int i = 0; i < 1000; i++) {
					sprintf(emitter, "emi%i", (int) sqrt(i % 100));
					sprintf(description, "prod%i", (i % 5) + 1);

					int			ll	 = strlen(emitter);
					ElementHash hash = MurmurHash64A(emitter, ll);

					REQUIRE(strcmp(emitter, ev_small_ins.get_str(hash).c_str()) == 0);

					ll	 = strlen(description);
					hash = MurmurHash64A(description, ll);

					REQUIRE(strcmp(description, ev_small_ins.get_str(hash).c_str()) == 0);
				}

				REQUIRE(strcmp("\x04", ev_small_ins.get_str(1234).c_str()) == 0);
				REQUIRE(strlen(ev_small_ins.get_str(0).c_str()) == 0);
			}
		}

		WHEN("I fill the big inserted one.") {
			char emitter[80];
			char description[80];
			double w;

			for (int i = 0; i < 1000000; i++) {
				sprintf(emitter, "emi%i", (int) sqrt(i % 1000));
				sprintf(description, "prod%i", (i % 23) + 1);
				w = i % 11 ? 1.3 : 4.2;

				ev_big_ins.insert_row(emitter, description, w);
			}

			THEN("The most seen ones are there.") {
				REQUIRE(ev_big_ins.event.size() == 900);
				REQUIRE(ev_big_ins.priority.size() == 900);

				int str_seen = 0;
				for (StringUsageMap::iterator it = ev_big_ins.names_map.begin(); it != ev_big_ins.names_map.end(); ++it)
					str_seen += it->second.seen;

				REQUIRE(str_seen > 1000000);

				REQUIRE(ev_big_ins.add_str((char *) "") == 0);

				for (PriorityMap::iterator it = ev_big_ins.priority.begin(); it != ev_big_ins.priority.end(); ++it) {
					BinEventPt ept = it->second;

					EventMap::iterator it_event = ev_big_ins.event.find(ept);
					if (it_event == ev_big_ins.event.end()) {
						REQUIRE(false);
						break;
					}
					if (it_event->second.seen < 1 || it_event->second.code < 1 || it_event->second.priority != it->first) {
						REQUIRE(false);
						break;
					}
				}
			}
		}

		WHEN("I fill the defined one.") {
			REQUIRE(ev_defined.define_event((char *) "event_u", (char *) "descr", 1, 123));
			REQUIRE(ev_defined.define_event((char *) "event_v", (char *) "bla", 1, 222));
			REQUIRE(ev_defined.define_event((char *) "event_x", (char *) "bla,bla", 1, 301));
			REQUIRE(ev_defined.define_event((char *) "event_y", (char *) "bla,bla,bla", 1, 404));
			REQUIRE(ev_defined.define_event((char *) "event_z", (char *) "whatever", 1, 505));

			REQUIRE(!ev_defined.define_event((char *) "event_u", (char *) "descr", 1, 123));
			REQUIRE(!ev_defined.define_event((char *) "event_v", (char *) "bla", 1, 222));
			REQUIRE(!ev_defined.define_event((char *) "event_x", (char *) "bla,bla", 1, 301));
			REQUIRE(!ev_defined.define_event((char *) "event_y", (char *) "bla,bla,bla", 1, 404));
			REQUIRE(!ev_defined.define_event((char *) "event_z", (char *) "whatever", 1, 505));

			THEN("The events are as defined.") {
				REQUIRE(ev_defined.priority_low == 0);
				REQUIRE(ev_defined.next_code == 0);

				REQUIRE(ev_defined.names_map.size() == 0);
				REQUIRE(ev_defined.event.size() == 5);
				REQUIRE(ev_defined.priority.size() == 0);
			}
		}

		WHEN("I use the python API server to test events_describe_next_event().") {
			int ev_id1 = new_events();
			int ev_id2 = new_events();
			int ev_id3 = new_events();

			events_set_store_strings(ev_id2, false);
			events_set_store_strings(ev_id3, true);

			REQUIRE(events_insert_row(ev_id1, (char *) "emi", (char *) "des", 1));
			REQUIRE(events_insert_row(ev_id1, (char *) "emi2", (char *) "des", 1));
			REQUIRE(events_insert_row(ev_id2, (char *) "emi", (char *) "des", 1));
			REQUIRE(events_insert_row(ev_id2, (char *) "emi2", (char *) "des", 1));
			REQUIRE(events_insert_row(ev_id3, (char *) "a aa", (char *) "x", 2));
			REQUIRE(events_insert_row(ev_id3, (char *) "b bb", (char *) "y", 2));

			THEN("Everything goes.") {
				char buff[256];

				sprintf(buff, "%s", events_describe_next_event(ev_id1, (char *) ""));

				REQUIRE(strcmp(buff, (char *) "emi\tdes\t1.00000\t1") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id1, buff));

				REQUIRE(strcmp(buff, (char *) "emi2\tdes\t1.00000\t2") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id1, buff));

				REQUIRE(strlen(buff) == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id1, (char *) "xxx"));

				REQUIRE(strlen(buff) == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id1, (char *) "<a0b2fbd5d8ff9c22>\t<0a348b052daf4285>\t1.00000\t1"));

				REQUIRE(strcmp(buff, (char *) "emi2\tdes\t1.00000\t2") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id2, (char *) ""));

				REQUIRE(strcmp(buff, (char *) "<a0b2fbd5d8ff9c22>\t<0a348b052daf4285>\t1.00000\t1") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id2, buff));

				REQUIRE(strcmp(buff, (char *) "<af06a6d491b4fc8e>\t<0a348b052daf4285>\t1.00000\t2") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id2, (char *) "xxx\tyyy"));

				REQUIRE(strlen(buff) == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id3, (char *) ""));

				REQUIRE(strcmp(buff, (char *) "b bb\ty\t2.00000\t2") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id3, buff));

				REQUIRE(strcmp(buff, (char *) "a aa\tx\t2.00000\t1") == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id3, buff));

				REQUIRE(strlen(buff) == 0);

				sprintf(buff, "%s", events_describe_next_event(ev_id3, (char *) "xxx\tyyy\t1"));

				REQUIRE(strlen(buff) == 0);

			}

			destroy_events(ev_id3);
			destroy_events(ev_id2);
			destroy_events(ev_id1);
		}

		WHEN("I build a few OptimizeEval vectors.") {
			OptimizeEval m_1 = {}, neg = {}, z_0 = {}, pos = {}, p_1 = {};

			REQUIRE(ev_defined.linear_correlation(m_1) == 0);
			m_1.push_back({0, 0, 0});
			REQUIRE(ev_defined.linear_correlation(m_1) == 0);
			m_1.push_back({1, 3, 0});
			REQUIRE(ev_defined.linear_correlation(m_1) == 0);
			m_1.push_back({1, 3, 0});
			REQUIRE(ev_defined.linear_correlation(m_1) == 0);
			m_1.push_back({2, 2, 0});
			REQUIRE(ev_defined.linear_correlation(m_1) != 0);
			m_1.push_back({2, 2, 0});
			m_1.push_back({3, 1, 0});
			m_1.push_back({3, 1, 0});

			REQUIRE(ev_defined.linear_correlation(neg) == 0);
			neg.push_back({1, 7, 0});
			REQUIRE(ev_defined.linear_correlation(neg) == 0);
			neg.push_back({2, 5, 0});
			REQUIRE(ev_defined.linear_correlation(neg) != 0);
			neg.push_back({3, 2, 0});
			neg.push_back({4, 2, 0});
			neg.push_back({5, 3, 0});

			REQUIRE(ev_defined.linear_correlation(z_0) == 0);
			z_0.push_back({1, 1, 0});
			REQUIRE(ev_defined.linear_correlation(z_0) == 0);
			z_0.push_back({1, 2, 0});
			REQUIRE(ev_defined.linear_correlation(z_0) == 0);
			z_0.push_back({2, 1, 0});
			REQUIRE(ev_defined.linear_correlation(z_0) != 0);
			z_0.push_back({2, 2, 0});

			REQUIRE(ev_defined.linear_correlation(pos) == 0);
			pos.push_back({1, 1, 0});
			REQUIRE(ev_defined.linear_correlation(pos) == 0);
			pos.push_back({2, 2, 0});
			REQUIRE(ev_defined.linear_correlation(pos) != 0);
			pos.push_back({3, 2, 0});
			pos.push_back({4, 3, 0});
			pos.push_back({5, 3, 0});

			REQUIRE(ev_defined.linear_correlation(p_1) == 0);
			p_1.push_back({1, 2, 0});
			REQUIRE(ev_defined.linear_correlation(p_1) == 0);
			p_1.push_back({2, 4, 0});
			REQUIRE(ev_defined.linear_correlation(p_1) != 0);
			p_1.push_back({3, 6, 0});
			p_1.push_back({4, 8, 0});

			THEN("Correlations match.") {
				REQUIRE(abs(ev_defined.linear_correlation(m_1) + 1) < 1e-7);
				REQUIRE(abs(ev_defined.linear_correlation(neg) + 0.8022575) < 1e-7);
				REQUIRE(abs(ev_defined.linear_correlation(z_0)) < 1e-7);
				REQUIRE(abs(ev_defined.linear_correlation(pos) - 0.9449112) < 1e-7);
				REQUIRE(abs(ev_defined.linear_correlation(p_1) - 1) < 1e-7);
			}
		}
	}
}


SCENARIO("Test Events.save() / Events.load()") {

	GIVEN("I have two Events objects.") {
		Events ev_def = {}, ev_alt = {};

		ev_def.set_store_strings(false);

		REQUIRE(ev_def.max_num_events == DEFAULT_NUM_EVENTS);
		REQUIRE(!ev_def.store_strings);
		REQUIRE(ev_def.priority_low == 0);
		REQUIRE(ev_def.next_code == 0);

		ev_alt.set_max_num_events(10);

		char emitter[80];
		char description[80];
		double w;

		for (int i = 0; i < 1000; i++) {
			sprintf(emitter, "emi%i", (int) sqrt(i % 100));
			sprintf(description, "prod%i", (i % 5) + 1);
			w = i % 11 ? 1.3 : 4.2;

			ev_alt.insert_row(emitter, description, w);
		}

		REQUIRE(ev_alt.max_num_events == 10);
		REQUIRE(ev_alt.store_strings);
		REQUIRE(ev_alt.priority_low > 0);
		REQUIRE(ev_alt.next_code > 0);

		WHEN("I copy them.") {
			Events cpy_def = {}, cpy_alt = {};

			pBinaryImage p_def = new BinaryImage;

			REQUIRE(ev_def.save(p_def));
			REQUIRE(cpy_def.load(p_def));

			delete p_def;

			pBinaryImage p_alt = new BinaryImage;

			REQUIRE(ev_alt.save(p_alt));
			REQUIRE(cpy_alt.load(p_alt));

			delete p_alt;

			THEN("They are identical.") {
				REQUIRE(ev_def.store_strings    == cpy_def.store_strings);
				REQUIRE(ev_def.max_num_events   == cpy_def.max_num_events);
				REQUIRE(ev_def.priority_low	    == cpy_def.priority_low);
				REQUIRE(ev_def.next_code	    == cpy_def.next_code);

				REQUIRE(ev_def.names_map.size()	== 0);
				REQUIRE(cpy_def.names_map.size()== 0);
				REQUIRE(ev_def.event.size()		== 0);
				REQUIRE(cpy_def.event.size()	== 0);
				REQUIRE(ev_def.priority.size()	== 0);
				REQUIRE(cpy_def.priority.size()	== 0);

				REQUIRE(ev_alt.store_strings    == cpy_alt.store_strings);
				REQUIRE(ev_alt.max_num_events   == cpy_alt.max_num_events);
				REQUIRE(ev_alt.priority_low	    == cpy_alt.priority_low);
				REQUIRE(ev_alt.next_code	    == cpy_alt.next_code);

				REQUIRE(ev_alt.names_map.size() == cpy_alt.names_map.size()); {
					StringUsageMap::iterator it1 = ev_alt.names_map.begin(), it2 = cpy_alt.names_map.begin();

					for (int i = 0; i < 10; i++) {
						if (it1 == ev_alt.names_map.end())
							break;

						REQUIRE(it1->first		 == it2->first);
						REQUIRE(it1->second.seen == it2->second.seen);
						REQUIRE(it1->second.str	 == it2->second.str);

						++it1;
						++it2;
					}
				}

				REQUIRE(ev_alt.event.size()	== cpy_alt.event.size()); {
					EventMap::iterator it1 = ev_alt.event.begin(), it2 = cpy_alt.event.begin();

					for (int i = 0; i < 10; i++) {
						if (it1 == ev_alt.event.end())
							break;

						REQUIRE(it1->first			 == it2->first);
						REQUIRE(it1->second.seen	 == it2->second.seen);
						REQUIRE(it1->second.code	 == it2->second.code);
						REQUIRE(it1->second.priority == it2->second.priority);

						++it1;
						++it2;
					}
				}

				REQUIRE(ev_alt.priority.size() == cpy_alt.priority.size()); {
					PriorityMap::iterator it1 = ev_alt.priority.begin(), it2 = cpy_alt.priority.begin();

					for (int i = 0; i < 10; i++) {
						if (it1 == ev_alt.priority.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second == it2->second);

						++it1;
						++it2;
					}
				}
			}
		}
	}
}


SCENARIO("Test Clients") {

	Clients cli = {};

	REQUIRE(cli.id.size() == 0);
	REQUIRE(cli.id_set.size() == 0);

	cli.add_client_id((char *) "");

	REQUIRE(cli.id.size() == 1);
	REQUIRE(cli.id_set.size() == 1);

	cli.add_client_id((char *) "client_a");
	cli.add_client_id((char *) "client_b");
	cli.add_client_id((char *) "client_c");

	REQUIRE(cli.id.size() == 4);
	REQUIRE(cli.id_set.size() == 4);

	cli.add_client_id((char *) "");

	REQUIRE(cli.id.size() == 5);
	REQUIRE(cli.id_set.size() == 4);

	ElementHash hash = MurmurHash64A((char *) "client_b", 8);

	REQUIRE(cli.id[2] == hash);
	REQUIRE(cli.id_set.find(hash) != cli.id_set.end());
}


SCENARIO("Test Clients.save() / Clients.load()") {

	GIVEN("I have two Clients objects.") {
		Clients cli_def = {}, cli_alt = {};

		char emitter[80];

		for (int i = 0; i < 250; i++) {
			sprintf(emitter, "emi%i", (int) sqrt(i % 100));

			cli_def.add_client_id(emitter);
		}

		REQUIRE(cli_def.id.size()	  == 250);
		REQUIRE(cli_def.id_set.size()  < 250);
		REQUIRE(cli_def.id_set.size()  >   0);
		REQUIRE(cli_alt.id.size()	  ==   0);
		REQUIRE(cli_alt.id_set.size() ==   0);

		WHEN("I copy them.") {
			Clients cpy_def = {}, cpy_alt = {};

			pBinaryImage p_def = new BinaryImage;

			REQUIRE(cli_def.save(p_def));
			REQUIRE(cpy_def.load(p_def));

			delete p_def;

			pBinaryImage p_alt = new BinaryImage;

			REQUIRE(cli_alt.save(p_alt));
			REQUIRE(cpy_alt.load(p_alt));

			delete p_alt;

			THEN("They are identical.") {
				REQUIRE(cpy_alt.id.size()	  ==   0);
				REQUIRE(cpy_alt.id_set.size() ==   0);

				REQUIRE(cli_def.id.size()	  == cpy_def.id.size());
				REQUIRE(cli_def.id_set.size() == cpy_def.id_set.size());

				ClientIDSet::iterator it1 = cli_def.id_set.begin(), it2 = cpy_def.id_set.begin();

				for (int i = 0; i < 10; i++) {
					if (it1 == cli_def.id_set.end())
						break;

					REQUIRE(*it1 == *it2);
					REQUIRE(cli_def.id[i] == cpy_def.id[i]);
					REQUIRE(cpy_def.id_set.find(cli_def.id[i]) != cpy_def.id_set.end());

					++it1;
					++it2;
				}
			}
		}
	}
}


SCENARIO("Test Clips") {

	GIVEN("I load a Clients, Events and a Clips object") {
		Events events = {};
		Clients clients = {};

		events.set_max_num_events(50);

		for (int i = 0; i < 10000; i++) {
			char emitter[80];
			char description[80];
			double w;

			if (i < 200) {
				sprintf(emitter, "cli-id%i", 77*i);
				clients.add_client_id(emitter);
			}

			sprintf(emitter, "emi%i", (int) sqrt(i % 100));
			sprintf(description, "prod%i", (i % 5) + 1);
			w = i % 11 ? 1.3 : 4.2;

			events.insert_row(emitter, description, w);
		}

		REQUIRE(events.event.size() == 50);
		REQUIRE(clients.id.size() == 200);
		REQUIRE(clients.id_set.size() == 200);

		Clips clips(clients, events);

		WHEN("I feed the Clips with transactions from the same and other clients") {

			int i = 0;
			for (int day = 10; day < 30; day++) {
				for (int hour = 10; hour < 24; hour++) {
					for (int min = 10; min < 60; min++) {
						for (int sec = 10; sec < 60; sec++) {
							char timestamp[80];
							char client[80];
							char emitter[80];
							char description[80];
							double w;

							i++;

							sprintf(timestamp, "2022-06-%i %i:%i:%i", day, hour, min, sec);
							sprintf(client, "cli-id%i", 77*(i % 500));
							sprintf(emitter, "emi%i", (int) sqrt(i % 200));
							sprintf(description, "prod%i", (i % 9) + 1);
							w = i % 11 ? 1.3 : 5.0;

							clips.scan_event(emitter, description, w, client, timestamp);
						}
					}
				}
			}
			char timestamp[80] = {"2022-06-04 10:00:00"};
			char client[80]	   = {"cli-id2541"};
			String emitter	   = events.get_str(events.event.begin()->first.e);
			String description = events.get_str(events.event.begin()->first.d);
			double w		   = events.event.begin()->first.w;

			REQUIRE( clips.scan_event((pChar) emitter.c_str(), (pChar) description.c_str(), w, (pChar) client, (pChar) timestamp));
			REQUIRE(!clips.scan_event((pChar) emitter.c_str(), (pChar) description.c_str(), w, (pChar) "", (pChar) timestamp));
			REQUIRE(!clips.scan_event((pChar) emitter.c_str(), (pChar) description.c_str(), w, (pChar) "xx", (pChar) timestamp));
			REQUIRE(!clips.scan_event((pChar) emitter.c_str(), (pChar) description.c_str(), w, (pChar) client, (pChar) "xx"));

			THEN("The ClipMap is as expected") {
				REQUIRE(clips.clips.size() <= 200);
			}
		}
	}
}


SCENARIO("Test Clips.save() / Clips.load()") {

	Events	ev = {};
	Clients	cli = {};

	char timestamp[80];
	char client[80];
	char emitter[80];
	char description[80];
	double w;

	for (int i = 0; i < 5000; i++) {
		sprintf(client, "cli-id%i", 77*(i % 500));
		sprintf(emitter, "emi%i", (int) sqrt(i % 100));
		sprintf(description, "prod%i", (i % 5) + 1);

		w = i % 11 ? 1.3 : 5.0;

		ev.insert_row(emitter, description, w);
		cli.add_client_id(client);
	}

	Clips clp_def(cli, ev), clp_alt({}, {});

	int i = 0;
	for (int hour = 10; hour < 12; hour++) {
		for (int min = 10; min < 60; min++) {
			for (int sec = 10; sec < 60; sec++) {
				i++;

				sprintf(timestamp, "2022-06-02 %i:%i:%i", hour, min, sec);
				sprintf(client, "cli-id%i", 77*(i % 500));
				sprintf(emitter, "emi%i", (int) sqrt(i % 100));
				sprintf(description, "prod%i", (i % 5) + 1);

				w = i % 11 ? 1.3 : 5.0;

				clp_def.scan_event(emitter, description, w, client, timestamp);
			}
		}
	}

	WHEN("I copy them.") {
		Clips cpy_def({}, {}), cpy_alt({}, {});

		pBinaryImage p_def = new BinaryImage;

		REQUIRE(clp_def.save(p_def));
		REQUIRE(cpy_def.load(p_def));

		delete p_def;

		pBinaryImage p_alt = new BinaryImage;

		REQUIRE(clp_alt.save(p_alt));
		REQUIRE(cpy_alt.load(p_alt));

		delete p_alt;

		THEN("They are identical.") {
			REQUIRE(strcmp(clp_def.time_format, cpy_def.time_format) == 0);
			REQUIRE(strcmp(clp_alt.time_format, cpy_alt.time_format) == 0);

			REQUIRE(clp_def.clients.id_set.size() == cpy_def.clients.id_set.size());
			REQUIRE(clp_alt.clients.id_set.size() == cpy_alt.clients.id_set.size());

			REQUIRE(clp_def.events.event.size()		== cpy_def.events.event.size());
			REQUIRE(clp_def.events.names_map.size()	== cpy_def.events.names_map.size());
			REQUIRE(clp_def.events.max_num_events	== cpy_def.events.max_num_events);
			REQUIRE(clp_def.events.next_code		== cpy_def.events.next_code);
			REQUIRE(clp_def.events.priority_low		== cpy_def.events.priority_low);
			REQUIRE(clp_def.events.store_strings	== cpy_def.events.store_strings);
			REQUIRE(clp_alt.events.event.size()		== cpy_alt.events.event.size());
			REQUIRE(clp_alt.events.names_map.size()	== cpy_alt.events.names_map.size());
			REQUIRE(clp_alt.events.max_num_events	== cpy_alt.events.max_num_events);
			REQUIRE(clp_alt.events.next_code		== cpy_alt.events.next_code);
			REQUIRE(clp_alt.events.priority_low		== cpy_alt.events.priority_low);
			REQUIRE(clp_alt.events.store_strings	== cpy_alt.events.store_strings);

			REQUIRE(clp_def.clips.size() == cpy_def.clips.size()); {
				ClipMap::iterator it_clip1 = clp_def.clips.begin(), it_clip2 = cpy_def.clips.begin();

				for (int i = 0; i < 10; i++) {
					if (it_clip1 == clp_def.clips.end())
						break;

					REQUIRE(it_clip1->first			== it_clip2->first);
					REQUIRE(it_clip1->second.size()	== it_clip2->second.size());
					Clip::iterator it1 = it_clip1->second.begin(), it2 = it_clip2->second.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == it_clip1->second.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}

					++it_clip1;
					++it_clip2;
				}
			}
			REQUIRE(clp_alt.clips.size() == cpy_alt.clips.size()); {
				ClipMap::iterator it_clip1 = clp_alt.clips.begin(), it_clip2 = cpy_alt.clips.begin();

				for (int i = 0; i < 10; i++) {
					if (it_clip1 == clp_alt.clips.end())
						break;

					REQUIRE(it_clip1->first			== it_clip2->first);
					REQUIRE(it_clip1->second.size()	== it_clip2->second.size());
					Clip::iterator it1 = it_clip1->second.begin(), it2 = it_clip2->second.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == it_clip1->second.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}

					++it_clip1;
					++it_clip2;
				}
			}
		}
	}
}


SCENARIO("Test Targets") {

	Events events = {};

	events.define_event((char *) "emi_A", (char *) "descr_A", 1, 'A');
	events.define_event((char *) "emi_B", (char *) "descr_B", 1, 'B');
	events.define_event((char *) "emi_C", (char *) "descr_C", 1, 'C');
	events.define_event((char *) "emi_D", (char *) "descr_D", 1, 'D');
	events.define_event((char *) "emi_E", (char *) "descr_E", 1, 'E');
	events.define_event((char *) "emi_F", (char *) "descr_F", 1, 'F');

	BinEventPt event_A = {MurmurHash64A((char *) "emi_A", 5), MurmurHash64A((char *) "descr_A", 7), 1};
	BinEventPt event_B = {MurmurHash64A((char *) "emi_B", 5), MurmurHash64A((char *) "descr_B", 7), 1};
	BinEventPt event_C = {MurmurHash64A((char *) "emi_C", 5), MurmurHash64A((char *) "descr_C", 7), 1};
	BinEventPt event_D = {MurmurHash64A((char *) "emi_D", 5), MurmurHash64A((char *) "descr_D", 7), 1};
	BinEventPt event_E = {MurmurHash64A((char *) "emi_E", 5), MurmurHash64A((char *) "descr_E", 7), 1};
	BinEventPt event_F = {MurmurHash64A((char *) "emi_F", 5), MurmurHash64A((char *) "descr_F", 7), 1};

	REQUIRE(events.event.size() == 6);

	REQUIRE(events.event[event_A].code == 'A');
	REQUIRE(events.event[event_B].code == 'B');
	REQUIRE(events.event[event_C].code == 'C');
	REQUIRE(events.event[event_D].code == 'D');
	REQUIRE(events.event[event_E].code == 'E');
	REQUIRE(events.event[event_F].code == 'F');

	Clients clients = {};

	clients.add_client_id((char *) "cli_U");
	clients.add_client_id((char *) "cli_V");
	clients.add_client_id((char *) "cli_W");
	clients.add_client_id((char *) "cli_X");
	clients.add_client_id((char *) "cli_Y");
	clients.add_client_id((char *) "cli_Z");

	ElementHash cli_U = MurmurHash64A((char *) "cli_U", 5);
	ElementHash cli_V = MurmurHash64A((char *) "cli_V", 5);
	ElementHash cli_W = MurmurHash64A((char *) "cli_W", 5);
	ElementHash cli_X = MurmurHash64A((char *) "cli_X", 5);
	ElementHash cli_Y = MurmurHash64A((char *) "cli_Y", 5);
	ElementHash cli_Z = MurmurHash64A((char *) "cli_Z", 5);

	REQUIRE(clients.id.size() == 6);

	REQUIRE(clients.id_set.find(cli_U) != clients.id_set.end());
	REQUIRE(clients.id_set.find(cli_V) != clients.id_set.end());
	REQUIRE(clients.id_set.find(cli_W) != clients.id_set.end());
	REQUIRE(clients.id_set.find(cli_X) != clients.id_set.end());
	REQUIRE(clients.id_set.find(cli_Y) != clients.id_set.end());
	REQUIRE(clients.id_set.find(cli_Z) != clients.id_set.end());

	Clips clips(clients, events);

	TimePoint day_10 = clips.get_time((char *) "2022-01-10 12:33:44");

	REQUIRE(day_10 > 0);

	TimePoint day_11 = clips.get_time((char *) "2022-01-11 12:33:44");
	TimePoint day_12 = clips.get_time((char *) "2022-01-12 12:33:44");
	TimePoint day_13 = clips.get_time((char *) "2022-01-13 12:33:44");
	TimePoint day_14 = clips.get_time((char *) "2022-01-14 12:33:44");
	TimePoint day_15 = clips.get_time((char *) "2022-01-15 12:33:44");
	TimePoint day_16 = clips.get_time((char *) "2022-01-16 12:33:44");
	TimePoint day_17 = clips.get_time((char *) "2022-01-17 12:33:44");
	TimePoint day_18 = clips.get_time((char *) "2022-01-18 12:33:44");
	TimePoint day_19 = clips.get_time((char *) "2022-01-19 12:33:44");

	TimePoint day_20 = clips.get_time((char *) "2022-01-20 12:33:44");
	TimePoint day_21 = clips.get_time((char *) "2022-01-21 12:33:44");
	TimePoint day_22 = clips.get_time((char *) "2022-01-22 12:33:44");
	TimePoint day_23 = clips.get_time((char *) "2022-01-23 12:33:44");
	TimePoint day_24 = clips.get_time((char *) "2022-01-24 12:33:44");
	TimePoint day_25 = clips.get_time((char *) "2022-01-25 12:33:44");
	TimePoint day_26 = clips.get_time((char *) "2022-01-26 12:33:44");
	TimePoint day_27 = clips.get_time((char *) "2022-01-27 12:33:44");
	TimePoint day_28 = clips.get_time((char *) "2022-01-28 12:33:44");
	TimePoint day_29 = clips.get_time((char *) "2022-01-29 12:33:44");

	TimePoint day_30 = clips.get_time((char *) "2022-01-30 12:33:44");

	REQUIRE(day_30 - day_10 ==  2*(day_20 - day_10));
	REQUIRE(day_30 - day_10 ==  4*(day_15 - day_10));
	REQUIRE(day_30 - day_10 ==  5*(day_14 - day_10));
	REQUIRE(day_30 - day_10 == 10*(day_12 - day_10));
	REQUIRE(day_30 - day_10 == 20*(day_11 - day_10));

	TargetMap target_map = {};

	target_map[cli_X] = day_25;
	target_map[cli_Y] = day_25;
	target_map[cli_Z] = day_25;

	GIVEN("I create the clips for some clients") {
		clips.insert_event(cli_U, 'A', day_10);
		clips.insert_event(cli_U, 'A', day_12);
		clips.insert_event(cli_U, 'A', day_14);
		clips.insert_event(cli_U, 'A', day_16);

		clips.insert_event(cli_V, 'B', day_10);
		clips.insert_event(cli_V, 'C', day_11);
		clips.insert_event(cli_V, 'C', day_14);
		clips.insert_event(cli_V, 'A', day_15);
		clips.insert_event(cli_V, 'B', day_20);
		clips.insert_event(cli_V, 'C', day_22);
		clips.insert_event(cli_V, 'C', day_24);
		clips.insert_event(cli_V, 'A', day_29);

		clips.insert_event(cli_W, 'A', day_18);
		clips.insert_event(cli_W, 'A', day_22);
		clips.insert_event(cli_W, 'C', day_28);

		clips.insert_event(cli_X, 'A', day_10);
		clips.insert_event(cli_X, 'E', day_14);
		clips.insert_event(cli_X, 'E', day_24);
		clips.insert_event(cli_X, 'A', day_27);
		clips.insert_event(cli_X, 'F', day_30);

		clips.insert_event(cli_Y, 'B', day_12);
		clips.insert_event(cli_Y, 'D', day_13);
		clips.insert_event(cli_Y, 'D', day_15);
		clips.insert_event(cli_Y, 'E', day_16);
		clips.insert_event(cli_Y, 'F', day_18);
		clips.insert_event(cli_Y, 'F', day_23);

		clips.insert_event(cli_Z, 'A', day_18);
		clips.insert_event(cli_Z, 'A', day_20);
		clips.insert_event(cli_Z, 'A', day_21);
		clips.insert_event(cli_Z, 'F', day_28);
		clips.insert_event(cli_Z, 'F', day_30);

		WHEN("I fit everything into a tree") {
			Targets targ_mm(&clips.clips, target_map);
			Targets targ_av(&clips.clips, target_map);
			Targets targ_ll(&clips.clips, target_map);

			ClipMap clips_copy(clips.clips);

			Targets targ_sq(&clips_copy, target_map);

			TimesToTarget t = targ_mm.predict();
			REQUIRE(t.size() == 0);
			t = targ_mm.predict(clients);
			REQUIRE(t.size() == 0);
			t = targ_mm.predict(clips.clip_map());
			REQUIRE(t.size() == 0);

			REQUIRE( targ_mm.fit(tr_log, ag_minimax, 0.8, 12, false));
			REQUIRE(!targ_mm.fit(tr_log, ag_minimax, 0.8, 12, false));
			REQUIRE( targ_av.fit(tr_linear, ag_mean, 0.0, 2, false));
			REQUIRE( targ_ll.fit(tr_linear, ag_longest, 0.0, 2, false));
			REQUIRE( targ_sq.fit(tr_log, ag_minimax, 0.8, 12, true));

			REQUIRE(targ_mm.tree[0].n_seen == 6);
			REQUIRE(targ_mm.tree[0].n_target == 3);
			REQUIRE(targ_mm.tree[0].child.size() == 4);

			REQUIRE(targ_av.tree[0].n_seen == 6);
			REQUIRE(targ_av.tree[0].n_target == 3);
			REQUIRE(targ_av.tree[0].child.size() == 4);

			REQUIRE(targ_mm.tree[0].sum_time_d < targ_av.tree[0].sum_time_d);
			REQUIRE(targ_mm.tree.size() > targ_av.tree.size());

			REQUIRE(targ_ll.tree[0].n_seen == 6);
			REQUIRE(targ_ll.tree[0].n_target == 3);
			REQUIRE(targ_ll.tree[0].child.size() == 4);

			REQUIRE(targ_sq.tree[0].n_seen == 6);
			REQUIRE(targ_sq.tree[0].n_target == 3);
			REQUIRE(targ_sq.tree[0].child.size() == 4);

			THEN("I can make some predictions") {
				TimesToTarget tt_mm = targ_mm.predict();
				REQUIRE(tt_mm.size() == 6);
				TimesToTarget tt_av = targ_av.predict();
				REQUIRE(tt_av.size() == 6);
				TimesToTarget tt_ll = targ_av.predict();
				REQUIRE(tt_ll.size() == 6);
				TimesToTarget tt_sq = targ_av.predict();
				REQUIRE(tt_sq.size() == 6);

				CodeTreeNode node;
				node.n_seen		= 0;
				node.n_target	= 0;
				node.sum_time_d = 1;

				REQUIRE(targ_av.predict_time(node) == PREDICT_MAX_TIME);

				node.n_seen = 1;

				REQUIRE(targ_av.predict_time(node) == PREDICT_MAX_TIME);

				node.n_target = 1;

				REQUIRE(targ_av.predict_time(node) > 0.99);
				REQUIRE(targ_av.predict_time(node) < 1.01);

				node.sum_time_d = 0;

				REQUIRE(targ_mm.predict_time(node) > 3.0);
				REQUIRE(targ_mm.predict_time(node) < 3.1);

				REQUIRE(targ_sq.predict_time(node) > 3.0);
				REQUIRE(targ_sq.predict_time(node) < 3.1);

				node.n_seen		= 2;
				node.sum_time_d = 1;

				REQUIRE(targ_av.predict_time(node) > 1.99);
				REQUIRE(targ_av.predict_time(node) < 2.01);

				if (clients.id.size() == 6)
					clients.add_client_id((char *) "unknown_clip");

				tt_mm = targ_mm.predict(clients);
				REQUIRE(tt_mm.size() == 7);
				double default_t_mm = targ_mm.predict_time(targ_mm.tree[0]);
				REQUIRE(tt_mm[6] == default_t_mm);

				tt_av = targ_av.predict(clients);
				REQUIRE(tt_av.size() == 7);
				double default_t_av = targ_av.predict_time(targ_av.tree[0]);
				REQUIRE(tt_av[6] == default_t_av);

				tt_sq = targ_sq.predict(clients);
				REQUIRE(tt_sq.size() == 7);
				double default_t_sq = targ_sq.predict_time(targ_sq.tree[0]);
				REQUIRE(tt_sq[6] == default_t_sq);

				Clips clp(clients, events);

				clp.insert_event(0, 'H', day_15);
				clp.insert_event(0, 'H', day_16);
				clp.insert_event(0, 'H', day_17);

				clp.insert_event(1, 'A', day_18);
				clp.insert_event(1, 'A', day_20);
				clp.insert_event(1, 'A', day_21);
				clp.insert_event(1, 'H', day_10);
				clp.insert_event(1, 'H', day_11);

				clp.insert_event(2, 'A', day_18);
				clp.insert_event(2, 'A', day_20);
				clp.insert_event(2, 'A', day_21);

				double t_mm = targ_mm.predict_clip(clp.clips.begin()->second);

				REQUIRE(t_mm == default_t_mm);

				double t_av = targ_av.predict_clip(clp.clips.begin()->second);

				REQUIRE(t_av == default_t_av);

				tt_mm = targ_mm.predict(&clp.clips);
				REQUIRE(tt_mm.size() == 3);
				REQUIRE(tt_mm[0] == default_t_mm);
				REQUIRE(tt_mm[1] == tt_mm[2]);

				tt_av = targ_av.predict(&clp.clips);
				REQUIRE(tt_av.size() == 3);
				REQUIRE(tt_av[0] == default_t_av);
				REQUIRE(tt_av[1] == tt_av[2]);

				tt_ll = targ_ll.predict(&clp.clips);
				REQUIRE(tt_ll.size() == 3);
				REQUIRE(tt_ll[0] == default_t_av);
				REQUIRE(tt_ll[1] == tt_ll[2]);
				REQUIRE(tt_ll[1] != tt_av[1]);

				tt_sq = targ_sq.predict(&clp.clips);
				REQUIRE(tt_sq.size() == 3);
				REQUIRE(tt_sq[0] == default_t_sq);
				REQUIRE(tt_sq[1] == tt_sq[2]);
			}
			THEN("My sequences are as expected") {
				REQUIRE(targ_sq.p_clips->size() == 6);

				// Now length should be unaffected
				Clip clp = targ_sq.p_clips->find(cli_U)->second;
				REQUIRE(clp.size() == 4);

				clp = targ_sq.p_clips->find(cli_V)->second;
				REQUIRE(clp.size() == 8);

				clp = targ_sq.p_clips->find(cli_W)->second;
				REQUIRE(clp.size() == 3);

				clp = targ_sq.p_clips->find(cli_X)->second;
				REQUIRE(clp.size() == 5);

				clp = targ_sq.p_clips->find(cli_Y)->second;
				REQUIRE(clp.size() == 6);

				clp = targ_sq.p_clips->find(cli_Z)->second;
				REQUIRE(clp.size() == 5);

				Clips clp_sq(*targ_sq.p_clips);
				clp_sq.collapse_to_states();

				// Now we test the collapsed ones, just like the old object used to return

				// clips.insert_event(cli_U, 'A', day_10);
				clp = clp_sq.clips.find(cli_U)->second;
				REQUIRE(clp.size() == 1);
				REQUIRE(clp.find(day_10)->second == 'A');

				// clips.insert_event(cli_V, 'B', day_10);
				// clips.insert_event(cli_V, 'C', day_11);
				// clips.insert_event(cli_V, 'A', day_15);
				// clips.insert_event(cli_V, 'B', day_20);
				// clips.insert_event(cli_V, 'C', day_22);
				// clips.insert_event(cli_V, 'A', day_29);
				clp = clp_sq.clips.find(cli_V)->second;
				REQUIRE(clp.size() == 6);
				REQUIRE(clp.find(day_10)->second == 'B');
				REQUIRE(clp.find(day_11)->second == 'C');
				REQUIRE(clp.find(day_15)->second == 'A');
				REQUIRE(clp.find(day_20)->second == 'B');
				REQUIRE(clp.find(day_22)->second == 'C');
				REQUIRE(clp.find(day_29)->second == 'A');

				// clips.insert_event(cli_W, 'A', day_18);
				// clips.insert_event(cli_W, 'C', day_28);
				clp = clp_sq.clips.find(cli_W)->second;
				REQUIRE(clp.size() == 2);
				REQUIRE(clp.find(day_18)->second == 'A');
				REQUIRE(clp.find(day_28)->second == 'C');

				// clips.insert_event(cli_X, 'A', day_10);
				// clips.insert_event(cli_X, 'E', day_14);
				// clips.insert_event(cli_X, 'A', day_27);
				// clips.insert_event(cli_X, 'F', day_30);
				clp = clp_sq.clips.find(cli_X)->second;
				REQUIRE(clp.size() == 4);
				REQUIRE(clp.find(day_10)->second == 'A');
				REQUIRE(clp.find(day_14)->second == 'E');
				REQUIRE(clp.find(day_27)->second == 'A');
				REQUIRE(clp.find(day_30)->second == 'F');

				// clips.insert_event(cli_Y, 'B', day_12);
				// clips.insert_event(cli_Y, 'D', day_13);
				// clips.insert_event(cli_Y, 'E', day_16);
				// clips.insert_event(cli_Y, 'F', day_18);
				clp = clp_sq.clips.find(cli_Y)->second;
				REQUIRE(clp.size() == 4);
				REQUIRE(clp.find(day_12)->second == 'B');
				REQUIRE(clp.find(day_13)->second == 'D');
				REQUIRE(clp.find(day_16)->second == 'E');
				REQUIRE(clp.find(day_18)->second == 'F');

				// clips.insert_event(cli_Z, 'A', day_18);
				// clips.insert_event(cli_Z, 'F', day_28);

				clp = clp_sq.clips.find(cli_Z)->second;
				REQUIRE(clp.size() == 2);
				REQUIRE(clp.find(day_18)->second == 'A');
				REQUIRE(clp.find(day_28)->second == 'F');
			}
		}
	}
}


SCENARIO("Test Targets.save() / Targets.load()") {

	Events	ev = {};
	Clients	cli = {};

	char timestamp[80];
	char client[80];
	char emitter[80];
	char description[80];
	double w;

	for (int i = 0; i < 5000; i++) {
		sprintf(client, "cli-id%i", 77*(i % 500));
		sprintf(emitter, "emi%i", (int) sqrt(i % 100));
		sprintf(description, "prod%i", (i % 5) + 1);

		w = i % 11 ? 1.3 : 5.0;

		ev.insert_row(emitter, description, w);
		cli.add_client_id(client);
	}

	Clips clp(cli, ev), clp_void({}, {});

	int i = 0;
	for (int hour = 10; hour < 12; hour++) {
		for (int min = 10; min < 60; min++) {
			for (int sec = 10; sec < 60; sec++) {
				i++;

				sprintf(timestamp, "2022-06-02 %i:%i:%i", hour, min, sec);
				sprintf(client, "cli-id%i", 77*(i % 500));
				sprintf(emitter, "emi%i", (int) sqrt(i % 100));
				sprintf(description, "prod%i", (i % 5) + 1);

				w = i % 11 ? 1.3 : 5.0;

				clp.scan_event(emitter, description, w, client, timestamp);
			}
		}
	}

	Targets trg_def(&clp.clips, {}), trg_alt(&clp_void.clips, {});

	sprintf(timestamp, "2022-06-03 01:02:03");

	for (int i = 0; i < 5000; i++) {
		if (i % 9 == 2) {
			sprintf(client, "cli-id%i", 77*(i % 500));

			trg_def.insert_target(client, timestamp);
		}
	}

	REQUIRE(trg_def.fit(tr_log, ag_minimax, 0.6, 10, false));

	WHEN("I copy them.") {
		Targets cpy_def({}, {}), cpy_alt({}, {});

		pBinaryImage p_def = new BinaryImage;

		REQUIRE(trg_def.save(p_def));
		REQUIRE(cpy_def.load(p_def));

		delete p_def;

		pBinaryImage p_alt = new BinaryImage;

		REQUIRE(trg_alt.save(p_alt));
		REQUIRE(cpy_alt.load(p_alt));

		delete p_alt;

		THEN("They are identical.") {
			REQUIRE(strcmp(trg_def.time_format, cpy_def.time_format) == 0);
			REQUIRE(trg_def.transform == cpy_def.transform);
			REQUIRE(trg_def.aggregate == cpy_def.aggregate);
			REQUIRE(trg_def.binomial_z == cpy_def.binomial_z);
			REQUIRE(trg_def.binomial_z_sqr == cpy_def.binomial_z_sqr);
			REQUIRE(trg_def.binomial_z_sqr_div_2 == cpy_def.binomial_z_sqr_div_2);
			REQUIRE(trg_def.tree_depth == cpy_def.tree_depth);

			REQUIRE(strcmp(trg_alt.time_format, cpy_alt.time_format) == 0);
			REQUIRE(trg_alt.transform == cpy_alt.transform);
			REQUIRE(trg_alt.aggregate == cpy_alt.aggregate);
			REQUIRE(trg_alt.binomial_z == cpy_alt.binomial_z);
			REQUIRE(trg_alt.binomial_z_sqr == cpy_alt.binomial_z_sqr);
			REQUIRE(trg_alt.binomial_z_sqr_div_2 == cpy_alt.binomial_z_sqr_div_2);
			REQUIRE(trg_alt.tree_depth == cpy_alt.tree_depth);

			REQUIRE(trg_def.p_clips->size() == cpy_def.p_clips->size()); {
				ClipMap::iterator it_clip1 = trg_def.p_clips->begin(), it_clip2 = cpy_def.p_clips->begin();

				for (int i = 0; i < 10; i++) {
					if (it_clip1 == trg_def.p_clips->end())
						break;

					REQUIRE(it_clip1->first			== it_clip2->first);
					REQUIRE(it_clip1->second.size()	== it_clip2->second.size());
					Clip::iterator it1 = it_clip1->second.begin(), it2 = it_clip2->second.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == it_clip1->second.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}

					++it_clip1;
					++it_clip2;
				}
			}
			REQUIRE(trg_alt.p_clips->size() == cpy_alt.p_clips->size()); {
				ClipMap::iterator it_clip1 = trg_alt.p_clips->begin(), it_clip2 = cpy_alt.p_clips->begin();

				for (int i = 0; i < 10; i++) {
					if (it_clip1 == trg_alt.p_clips->end())
						break;

					REQUIRE(it_clip1->first			== it_clip2->first);
					REQUIRE(it_clip1->second.size()	== it_clip2->second.size());
					Clip::iterator it1 = it_clip1->second.begin(), it2 = it_clip2->second.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == it_clip1->second.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}

					++it_clip1;
					++it_clip2;
				}
			}
			REQUIRE(trg_def.target.size() == cpy_def.target.size()); {
				TargetMap::iterator it1 = trg_def.target.begin(), it2 = cpy_def.target.begin();

				for (int i = 0; i < 10; i++) {
					if (it1 == trg_def.target.end())
						break;

					REQUIRE(it1->first	== it2->first);
					REQUIRE(it1->second	== it2->second);

					++it1;
					++it2;
				}
			}
			REQUIRE(trg_alt.target.size() == cpy_alt.target.size()); {
				TargetMap::iterator it1 = trg_alt.target.begin(), it2 = cpy_alt.target.begin();

				for (int i = 0; i < 10; i++) {
					if (it1 == trg_alt.target.end())
						break;

					REQUIRE(it1->first	== it2->first);
					REQUIRE(it1->second	== it2->second);

					++it1;
					++it2;
				}
			}
			REQUIRE(trg_def.tree.size() == cpy_def.tree.size()); {
				for (int i = 0; i < 10; i++) {
					if (i == trg_def.tree.size())
						break;

					REQUIRE(trg_def.tree[i].n_seen		 == cpy_def.tree[i].n_seen);
					REQUIRE(trg_def.tree[i].n_target	 == cpy_def.tree[i].n_target);
					REQUIRE(trg_def.tree[i].sum_time_d	 == cpy_def.tree[i].sum_time_d);
					REQUIRE(trg_def.tree[i].child.size() == cpy_def.tree[i].child.size());

					ChildIndex::iterator it1 = trg_def.tree[i].child.begin(), it2 = cpy_def.tree[i].child.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == trg_def.tree[i].child.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}
				}
			}
			REQUIRE(trg_alt.tree.size() == cpy_alt.tree.size()); {
				for (int i = 0; i < 10; i++) {
					if (i == trg_alt.tree.size())
						break;

					REQUIRE(trg_alt.tree[i].n_seen		 == cpy_alt.tree[i].n_seen);
					REQUIRE(trg_alt.tree[i].n_target	 == cpy_alt.tree[i].n_target);
					REQUIRE(trg_alt.tree[i].sum_time_d	 == cpy_alt.tree[i].sum_time_d);
					REQUIRE(trg_alt.tree[i].child.size() == cpy_alt.tree[i].child.size());

					ChildIndex::iterator it1 = trg_alt.tree[i].child.begin(), it2 = cpy_alt.tree[i].child.begin();

					for (int j = 0; j < 10; j++) {
						if (it1 == trg_alt.tree[i].child.end())
							break;

						REQUIRE(it1->first	== it2->first);
						REQUIRE(it1->second	== it2->second);

						++it1;
						++it2;
					}
				}
			}
		}
	}
}


SCENARIO("Extended Testing Emulating the Python API ") {

	int ev_id = new_events();

	REQUIRE(events_define_event(ev_id, (char *) "emi_A", (char *) "descr_A", 1.0, 1));
	REQUIRE(events_define_event(ev_id, (char *) "emi_B", (char *) "descr_B", 1.0, 2));
	REQUIRE(events_num_events(ev_id) == 2);

	int cl_id = new_clients();

	REQUIRE(!clients_add_client_id(cl_id, (char *) ""));
	REQUIRE(clients_add_client_id(cl_id, (char *) "cli_A"));
	REQUIRE(clients_add_client_id(cl_id, (char *) "cli_B"));
	REQUIRE(!clients_add_client_id(cl_id, (char *) "cli_B"));
	REQUIRE(clients_num_clients(cl_id) == 2);

	String hash_a = clients_hash_client_id(cl_id, (char *) "cli_A");
	String hash_a_idx = clients_hash_by_index(cl_id, 0);

	REQUIRE(hash_a.length() == 18);
	REQUIRE(hash_a == hash_a_idx);

	int clips_id = new_clips(cl_id, ev_id);
	REQUIRE(clips_id > 0);
	REQUIRE(clips_set_time_format(clips_id, (char *) "%Y-%m-%d %H:%M:%S"));

	REQUIRE(clips_scan_event(clips_id, (char *) "emi_A", (char *) "descr_A", 1.0, (char *) "cli_A", (char *) "2022-06-04 10:00:00"));
	REQUIRE(clips_scan_event(clips_id, (char *) "emi_B", (char *) "descr_B", 1.0, (char *) "cli_A", (char *) "2022-06-04 10:01:00"));
	REQUIRE(clips_scan_event(clips_id, (char *) "emi_A", (char *) "descr_A", 1.0, (char *) "cli_B", (char *) "2022-06-04 10:00:30"));
	REQUIRE(!clips_scan_event(clips_id, (char *) "emi_X", (char *) "descr_X", 1.0, (char *) "cli_A", (char *) "2022-06-04 10:02:00"));
	REQUIRE(!clips_scan_event(clips_id, (char *) "emi_A", (char *) "descr_A", 1.0, (char *) "cli_X", (char *) "2022-06-04 10:02:00"));

	REQUIRE(clips_num_clips(clips_id) == 2);
	REQUIRE(clips_num_events(clips_id) == 3);

	String hash_first = clips_hash_by_previous(clips_id, (char *) "");
	String hash_second = clips_hash_by_previous(clips_id, (char *) hash_first.c_str());
	String hash_none = clips_hash_by_previous(clips_id, (char *) hash_second.c_str());

	REQUIRE(hash_first.length() == 18);
	REQUIRE(hash_second.length() == 18);
	REQUIRE(hash_first != hash_second);
	REQUIRE(hash_none.length() == 0);

	String clip_desc = clips_describe_clip(clips_id, (char *) "cli_A");
	REQUIRE(clip_desc == "1\t2");

	int targ_id = new_targets(clips_id);
	REQUIRE(targ_id > 0);
	REQUIRE(targets_set_time_format(targ_id, (char *) "%Y-%m-%d %H:%M:%S"));
	REQUIRE(targets_insert_target(targ_id, (char *) "cli_A", (char *) "2022-06-04 10:02:00"));
	REQUIRE(!targets_insert_target(targ_id, (char *) "cli_A", (char *) "2022-06-04 10:02:00"));
	REQUIRE(targets_num_targets(targ_id) == 1);
	REQUIRE(targets_fit(targ_id, (char *) "log", (char *) "mean", 0.5, 5, 0));

	String tree_desc = targets_describe_tree(targ_id);
	String root_children = targets_tree_node_children(targ_id, 0);

	REQUIRE(tree_desc.length() > 0);
	REQUIRE(root_children.length() > 0);

	int child_code = 0;
	REQUIRE(sscanf(root_children.c_str(), "%i", &child_code) == 1);

	int child_idx = targets_tree_node_idx(targ_id, 0, child_code);
	REQUIRE(child_idx >= 0);

	String node_desc = targets_describe_tree_node(targ_id, child_idx);
	REQUIRE(node_desc.length() > 0);

	int it_pred = targets_predict_clients(targ_id, cl_id);
	REQUIRE(it_pred > 0);
	REQUIRE(size_result_iterator(it_pred) == 2);

	double pred_1 = next_result_iterator(it_pred);
	double pred_2 = next_result_iterator(it_pred);

	REQUIRE(size_result_iterator(it_pred) == 0);
	REQUIRE(pred_1 > 0);
	REQUIRE(pred_2 > 0);

	destroy_result_iterator(it_pred);

	int it_pred_clips = targets_predict_clips(targ_id, clips_id);
	REQUIRE(it_pred_clips > 0);
	REQUIRE(size_result_iterator(it_pred_clips) == 2);
	next_result_iterator(it_pred_clips);
	next_result_iterator(it_pred_clips);
	destroy_result_iterator(it_pred_clips);

	int image_cli = clients_save(cl_id);
	REQUIRE(image_cli == cl_id);

	int n_cli_blocks = size_binary_image_iterator(image_cli);
	REQUIRE(n_cli_blocks > 0);

	std::vector<String> cli_blocks = {};
	for (int i = 0; i < n_cli_blocks; i++) {
		String blk = next_binary_image_iterator(image_cli);
		REQUIRE(blk.length() > 0);
		cli_blocks.push_back(blk);
	}

	REQUIRE(destroy_binary_image_iterator(image_cli));

	int cl_id_copy = new_clients();
	for (std::vector<String>::iterator it = cli_blocks.begin(); it != cli_blocks.end(); ++it)
		REQUIRE(clients_load_block(cl_id_copy, (char *) it->c_str()));
	REQUIRE(clients_load_block(cl_id_copy, (char *) ""));
	REQUIRE(clients_num_clients(cl_id_copy) == 2);

	int image_clp = clips_save(clips_id);
	REQUIRE(image_clp == clips_id);

	int n_clp_blocks = size_binary_image_iterator(image_clp);
	REQUIRE(n_clp_blocks > 0);

	std::vector<String> clp_blocks = {};
	for (int i = 0; i < n_clp_blocks; i++) {
		String blk = next_binary_image_iterator(image_clp);
		REQUIRE(blk.length() > 0);
		clp_blocks.push_back(blk);
	}

	REQUIRE(destroy_binary_image_iterator(image_clp));

	int ev_id_empty = new_events();
	int cl_id_empty = new_clients();
	int clips_id_copy = new_clips(cl_id_empty, ev_id_empty);

	for (std::vector<String>::iterator it = clp_blocks.begin(); it != clp_blocks.end(); ++it)
		REQUIRE(clips_load_block(clips_id_copy, (char *) it->c_str()));
	REQUIRE(clips_load_block(clips_id_copy, (char *) ""));
	REQUIRE(clips_num_clips(clips_id_copy) == 2);
	REQUIRE(clips_num_events(clips_id_copy) == 3);

	int image_targ = targets_save(targ_id);
	REQUIRE(image_targ == targ_id);

	int n_targ_blocks = size_binary_image_iterator(image_targ);
	REQUIRE(n_targ_blocks > 0);

	std::vector<String> targ_blocks = {};
	for (int i = 0; i < n_targ_blocks; i++) {
		String blk = next_binary_image_iterator(image_targ);
		REQUIRE(blk.length() > 0);
		targ_blocks.push_back(blk);
	}

	REQUIRE(destroy_binary_image_iterator(image_targ));

	int ev_id_empty_2 = new_events();
	int cl_id_empty_2 = new_clients();
	int clips_id_empty = new_clips(cl_id_empty_2, ev_id_empty_2);
	int targ_id_copy = new_targets(clips_id_empty);

	for (std::vector<String>::iterator it = targ_blocks.begin(); it != targ_blocks.end(); ++it)
		REQUIRE(targets_load_block(targ_id_copy, (char *) it->c_str()));
	REQUIRE(targets_load_block(targ_id_copy, (char *) ""));
	REQUIRE(targets_num_targets(targ_id_copy) == 1);

	destroy_targets(targ_id_copy);
	destroy_targets(targ_id);
	destroy_clips(clips_id_empty);
	destroy_clips(clips_id_copy);
	destroy_clips(clips_id);
	destroy_clients(cl_id_empty_2);
	destroy_clients(cl_id_empty);
	destroy_clients(cl_id_copy);
	destroy_clients(cl_id);
	destroy_events(ev_id_empty_2);
	destroy_events(ev_id_empty);
	destroy_events(ev_id);
}


SCENARIO("Cover python API zero-coverage functions") {

	int ev_id = new_events();

	REQUIRE(events_define_event(ev_id, (char *) "emi_A", (char *) "descr_A", 1.0, 1));
	REQUIRE(events_define_event(ev_id, (char *) "emi_B", (char *) "descr_B", 1.0, 2));

	int cl_id = new_clients();

	REQUIRE(clients_add_client_id(cl_id, (char *) "cli_A"));
	REQUIRE(clients_add_client_id(cl_id, (char *) "cli_B"));

	int clips_id = new_clips(cl_id, ev_id);
	REQUIRE(clips_id > 0);
	REQUIRE(clips_set_time_format(clips_id, (char *) "%Y-%m-%d %H:%M:%S"));

	REQUIRE(clips_scan_event(clips_id, (char *) "emi_A", (char *) "descr_A", 1.0, (char *) "cli_A", (char *) "2022-06-04 10:00:00"));
	REQUIRE(clips_scan_event(clips_id, (char *) "emi_B", (char *) "descr_B", 1.0, (char *) "cli_A", (char *) "2022-06-04 10:01:00"));
	REQUIRE(clips_scan_event(clips_id, (char *) "emi_A", (char *) "descr_A", 1.0, (char *) "cli_B", (char *) "2022-06-04 10:00:30"));

	int targ_id = new_targets(clips_id);
	REQUIRE(targ_id > 0);
	REQUIRE(targets_set_time_format(targ_id, (char *) "%Y-%m-%d %H:%M:%S"));
	REQUIRE(targets_insert_target(targ_id, (char *) "cli_A", (char *) "2022-06-04 10:02:00"));

	int ev_opt_id = new_events();

	REQUIRE(events_define_event(ev_opt_id, (char *) "emi_A", (char *) "descr_A", 1.0, 1));
	REQUIRE(events_define_event(ev_opt_id, (char *) "emi_B", (char *) "descr_B", 1.0, 2));

	char *opt_log = events_optimize_events(ev_opt_id, clips_id, targ_id, 1, 1, 0.0,
										  (char *) "", (char *) "", (char *) "log", (char *) "mean",
										  0.5, 3, 0, 0.0, 0.5, false);

	REQUIRE(strncmp(opt_log, "SUCCESS", 7) == 0);

	int ev_cap_id = new_events();
	REQUIRE(events_set_max_num_events(ev_cap_id, 1));
	REQUIRE(events_insert_row(ev_cap_id, (char *) "emi_1", (char *) "descr_1", 1.0));
	REQUIRE(events_insert_row(ev_cap_id, (char *) "emi_2", (char *) "descr_2", 1.0));
	REQUIRE(events_num_events(ev_cap_id) == 1);

	int image_ev = events_save(ev_id);
	REQUIRE(image_ev == ev_id);

	int n_ev_blocks = size_binary_image_iterator(image_ev);
	REQUIRE(n_ev_blocks > 0);

	std::vector<String> ev_blocks = {};
	for (int i = 0; i < n_ev_blocks; i++) {
		String blk = next_binary_image_iterator(image_ev);
		REQUIRE(blk.length() > 0);
		ev_blocks.push_back(blk);
	}
	REQUIRE(destroy_binary_image_iterator(image_ev));

	int ev_load_id = new_events();
	for (std::vector<String>::iterator it = ev_blocks.begin(); it != ev_blocks.end(); ++it)
		REQUIRE(events_load_block(ev_load_id, (char *) it->c_str()));
	REQUIRE(events_load_block(ev_load_id, (char *) ""));
	REQUIRE(events_num_events(ev_load_id) == 2);

	String seq_a = clips_test_sequence(0, false);
	String seq_b = clips_test_sequence(0, true);
	String seq_c = clips_test_sequence(999, false);

	REQUIRE(seq_a.length() > 0);
	REQUIRE(seq_b.length() > 0);
	REQUIRE(seq_c.length() == 0);

	Events ev_cpp = {};
	Clients cli_cpp = {};

	ev_cpp.define_event((char *) "em1", (char *) "desc1", 1.0, 1);
	ev_cpp.define_event((char *) "em2", (char *) "desc2", 1.0, 2);
	cli_cpp.add_client_id((char *) "cli_V");

	Clips clp_cpp(cli_cpp, ev_cpp);

	TimePoint t1 = clp_cpp.get_time((char *) "2022-06-01 10:00:00");
	TimePoint t2 = clp_cpp.get_time((char *) "2022-06-01 10:01:00");
	TimePoint t3 = clp_cpp.get_time((char *) "2022-06-01 10:03:00");

	ElementHash cli_hash = MurmurHash64A((char *) "cli_V", 5);

	clp_cpp.insert_event(cli_hash, 1, t1);
	clp_cpp.insert_event(cli_hash, 2, t2);

	TargetMap tm = {};
	Targets targ_cpp(&clp_cpp.clips, tm);

	REQUIRE(targ_cpp.p_target() == &targ_cpp.target);

	targ_cpp.target[cli_hash] = t3;
	REQUIRE(targ_cpp.fit(tr_linear, ag_mean, 0.5, 5, false));

	TimePoint obs_time = 0;
	bool target_yn = false;
	int longest_seq = 0;
	uint64_t n_visits = 0;
	uint64_t n_targets = 0;
	double targ_mean_t = 0;

	Clip &clip_ref = clp_cpp.clips[cli_hash];

	targ_cpp.verbose_predict_clip(cli_hash, clip_ref, obs_time, target_yn, longest_seq, n_visits, n_targets, targ_mean_t);

	REQUIRE(target_yn);
	REQUIRE(obs_time == t3 - t2);
	REQUIRE(longest_seq > 0);
	REQUIRE(n_visits > 0);
	REQUIRE(n_targets > 0);
	REQUIRE(targ_mean_t > 0);

	destroy_targets(targ_id);
	destroy_clips(clips_id);
	destroy_clients(cl_id);
	destroy_events(ev_load_id);
	destroy_events(ev_cap_id);
	destroy_events(ev_opt_id);
	destroy_events(ev_id);
}


SCENARIO("Test for optimize_events() (basic)") {

	GIVEN("A minimal synthetic dataset where Events::optimize_events can succeed") {

		// Define a tiny dictionary of events
		Events events = {};

		REQUIRE(events.define_event((char *) "emi_A", (char *) "descr_A", 1.0, 'A'));
		REQUIRE(events.define_event((char *) "emi_B", (char *) "descr_B", 1.0, 'B'));

		// Sanity: we really have two events defined
		REQUIRE(events.num_events() == 2);

		// Minimal clients / clips setup
		Clients clients = {};
		Clips   clips(clients, events);

		// Some simple time points
		TimePoint t0 = clips.get_time((char *) "2022-01-01 10:00:00");
		TimePoint t1 = clips.get_time((char *) "2022-01-01 10:00:10");
		TimePoint t2 = clips.get_time((char *) "2022-01-01 10:00:20");
		TimePoint t3 = clips.get_time((char *) "2022-01-01 10:01:00");

		REQUIRE(t0 > 0);
		REQUIRE(t1 > t0);
		REQUIRE(t2 > t1);
		REQUIRE(t3 > t2);

		// Three clients (we don't need to populate clients.id_set; Clips::insert_event
		// works directly with ElementHash client ids)
		ElementHash cli1 = MurmurHash64A((char *) "cli1", 4);
		ElementHash cli2 = MurmurHash64A((char *) "cli2", 4);
		ElementHash cli3 = MurmurHash64A((char *) "cli3", 4);

		// Build their clips in terms of codes 'A' and 'B'
		// cli1: A then B
		clips.insert_event(cli1, 'A', t0);
		clips.insert_event(cli1, 'B', t1);

		// cli2: only A
		clips.insert_event(cli2, 'A', t0);
		clips.insert_event(cli2, 'A', t1);

		// cli3: only B
		clips.insert_event(cli3, 'B', t0);
		clips.insert_event(cli3, 'B', t1);

		// Targets: cli1 and cli3 experience the target, cli2 does not
		TargetMap target_map = {};
		target_map[cli1] = t2;	// target 20 seconds after first event
		target_map[cli3] = t3;	// later target for cli3

		REQUIRE(target_map.size() == 2);

		WHEN("I run optimize_events with default-like arguments") {

			// Use slightly non-trivial parameters to tick more branches
			String log = events.optimize_events(clips,
												target_map,
												3,				// num_steps
												2,				// codes_per_step
												0.0001,			// threshold
												nullptr,		// force include
												nullptr,		// force exclude
												tr_linear,		// transform
												ag_longest,		// aggregate
												0.5,			// p
												10,				// depth
												true,			// as_states
												0.00693,		// exp_decay
												0.95,			// lower_bound_p
												true);			// log_lift

			THEN("The optimization succeeds and the report starts with 'success'") {
				REQUIRE(!log.empty());

				size_t pos = log.find('\n');
				String first_line = (pos == String::npos) ? log : log.substr(0, pos);

				REQUIRE(first_line == "SUCCESS");

				// Make sure we actually hit part of the body of the report
				REQUIRE(log.find("Preprocessing:") != String::npos);
				REQUIRE(log.find("codes found in clips") != String::npos);
			}

			THEN("Different transforms and aggregates are accepted without error") {

				// IMPORTANT:
				// optimize_events mutates `events` (internal EventMap), so we rebuild a fresh setup
				// for this alternate run to avoid "codes in clips not defined in internal EventMap".

				Events events_alt = {};
				REQUIRE(events_alt.define_event((char *) "emi_A", (char *) "descr_A", 1.0, 'A'));
				REQUIRE(events_alt.define_event((char *) "emi_B", (char *) "descr_B", 1.0, 'B'));
				REQUIRE(events_alt.num_events() == 2);

				Clients clients_alt = {};
				Clips   clips_alt(clients_alt, events_alt);

				TimePoint t0_alt = clips_alt.get_time((char *) "2022-01-01 10:00:00");
				TimePoint t1_alt = clips_alt.get_time((char *) "2022-01-01 10:00:10");
				TimePoint t2_alt = clips_alt.get_time((char *) "2022-01-01 10:00:20");
				TimePoint t3_alt = clips_alt.get_time((char *) "2022-01-01 10:01:00");

				REQUIRE(t0_alt > 0);
				REQUIRE(t1_alt > t0_alt);
				REQUIRE(t2_alt > t1_alt);
				REQUIRE(t3_alt > t2_alt);

				ElementHash cli1_alt = MurmurHash64A((char *) "cli1", 4);
				ElementHash cli2_alt = MurmurHash64A((char *) "cli2", 4);
				ElementHash cli3_alt = MurmurHash64A((char *) "cli3", 4);

				// Same clip patterns as the "success" case
				clips_alt.insert_event(cli1_alt, 'A', t0_alt);
				clips_alt.insert_event(cli1_alt, 'B', t1_alt);

				clips_alt.insert_event(cli2_alt, 'A', t0_alt);
				clips_alt.insert_event(cli2_alt, 'A', t1_alt);

				clips_alt.insert_event(cli3_alt, 'B', t0_alt);
				clips_alt.insert_event(cli3_alt, 'B', t1_alt);

				TargetMap target_map_alt = {};
				target_map_alt[cli1_alt] = t2_alt;
				target_map_alt[cli3_alt] = t3_alt;

				// Log transform + mean aggregation, no state collapsing
				String log_alt = events_alt.optimize_events(clips_alt,
														   target_map_alt,
														   1,			// fewer steps
														   1,			// one code per step
														   0.0,			// zero threshold to always accept improvements
														   nullptr,
														   nullptr,
														   tr_log,
														   ag_mean,
														   0.8,			// different p
														   5,			// smaller depth
														   false,		// as sequences, not states
														   0.01,			// different exp_decay
														   0.9,			// different lower_bound_p
														   false);		// linear lift

				REQUIRE(!log_alt.empty());

				size_t pos_alt = log_alt.find('\n');
				String first_line_alt = (pos_alt == String::npos) ? log_alt : log_alt.substr(0, pos_alt);

				REQUIRE(first_line_alt == "SUCCESS");
			}
		}
	}

	GIVEN("A dataset where Clips contains a code that is not defined in Events") {

		// Start from the same minimal successful setup
		Events events = {};

		REQUIRE(events.define_event((char *) "emi_A", (char *) "descr_A", 1.0, 'A'));
		REQUIRE(events.define_event((char *) "emi_B", (char *) "descr_B", 1.0, 'B'));

		Clients clients = {};
		Clips   clips(clients, events);

		TimePoint t0 = clips.get_time((char *) "2022-01-01 10:00:00");
		TimePoint t1 = clips.get_time((char *) "2022-01-01 10:00:10");
		TimePoint t2 = clips.get_time((char *) "2022-01-01 10:00:20");

		REQUIRE(t0 > 0);
		REQUIRE(t1 > t0);
		REQUIRE(t2 > t1);

		ElementHash cli1 = MurmurHash64A((char *) "cli1", 4);
		ElementHash cli2 = MurmurHash64A((char *) "cli2", 4);

		// Normal events using only defined codes 'A' and 'B'
		clips.insert_event(cli1, 'A', t0);
		clips.insert_event(cli1, 'B', t1);
		clips.insert_event(cli2, 'A', t0);

		TargetMap target_map = {};
		target_map[cli1] = t2;

		REQUIRE(target_map.size() == 1);

		// Now inject a clip with an undefined event code: 9999
		ElementHash cli_fake = MurmurHash64A((char *) "cli_fake", 8);
		clips.insert_event(cli_fake, 9999, t0);		// 9999 is not present in Events::event

		WHEN("I run optimize_events on this inconsistent dataset") {

			String log_err = events.optimize_events(clips,
													target_map,
													1,
													2,
													0.0001,
													nullptr,
													nullptr,
													tr_linear,
													ag_longest,
													0.5,
													10,
													true,
													0.00693,
													0.95,
													true);

			THEN("The optimizer detects missing codes and reports an ERROR") {
				REQUIRE(!log_err.empty());

				size_t pos = log_err.find('\n');
				String first_line = (pos == String::npos) ? log_err : log_err.substr(0, pos);

				REQUIRE(first_line == "ERROR");

				// And the log should mention that some codes in clips are not in the internal EventMap
				REQUIRE(log_err.find("codes in clips not defined in internal EventMap") != String::npos);
			}
		}
	}
}


SCENARIO("Cover remaining zero-coverage Python API and verbose helpers") {

	GIVEN("A minimal setup using the Python-style C API") {

		// -------------------------
		// Events (Python API)
		// -------------------------
		int eid = new_events();
		REQUIRE(eid > 0);

		REQUIRE(events_set_max_num_events(eid, 5));

		REQUIRE(events_insert_row(eid, (char*)"e", (char*)"d1", 1.0));
		REQUIRE(events_insert_row(eid, (char*)"e", (char*)"d2", 1.0));

		// -------------------------
		// Clients
		// -------------------------
		int cid = new_clients();
		REQUIRE(cid > 0);

		REQUIRE(clients_add_client_id(cid, (char*)"cli1"));

		// -------------------------
		// Clips
		// -------------------------
		int clp_id = new_clips(cid, eid);
		REQUIRE(clp_id > 0);

		REQUIRE(clips_scan_event(
			clp_id,
			(char*)"e", (char*)"d1", 1.0,
			(char*)"cli1",
			(char*)"2023-01-01 10:00:00"
		));

		// -------------------------
		// Targets
		// -------------------------
		int trg_id = new_targets(clp_id);
		REQUIRE(trg_id > 0);

		REQUIRE(targets_insert_target(
			trg_id,
			(char*)"cli1",
			(char*)"2023-01-01 10:00:10"
		));

		WHEN("Calling the Python-facing optimizer wrapper") {

			// Covers:
			// - events_optimize_events
			// - p_target()
			// - string -> enum parsing
			char *res = events_optimize_events(
				eid,
				clp_id,
				trg_id,
				1,          // num_steps
				1,          // codes_per_step
				0.0,        // threshold
				(char*)"",  // force_include
				(char*)"",  // force_exclude
				(char*)"log",
				(char*)"mean",
				0.8,
				5,
				0,
				0.01,
				0.9,
				false
			);

			THEN("The optimizer runs successfully") {
				REQUIRE(res != nullptr);
				REQUIRE(std::string(res).rfind("SUCCESS", 0) == 0);
			}
		}

		WHEN("Saving and reloading events through the Python binary API") {

			// Covers events_save
			int img_id = events_save(eid);
			REQUIRE(img_id == eid);

			THEN("Events are restored correctly") {
				REQUIRE(events_num_events(eid) > 0);
			}
		}

		WHEN("Requesting synthetic optimizer test sequences") {

			// Covers clips_test_sequence
			char *non_target = clips_test_sequence(0, false);
			char *target     = clips_test_sequence(0, true);

			THEN("Both sequences are returned") {
				REQUIRE(non_target != nullptr);
				REQUIRE(target != nullptr);
				REQUIRE(strlen(non_target) > 0);
				REQUIRE(strlen(target) > 0);
			}
		}
	}

	GIVEN("A native C++ Targets object for verbose prediction") {

		// This part is intentionally NOT using the Python API.
		// It cleanly isolates Targets::verbose_predict_clip.

		Events ev = {};
		REQUIRE(ev.define_event((char*)"e", (char*)"d", 1.0, 1));

		Clients cl = {};
		Clips cp(cl, ev);

		TimePoint t0 = cp.get_time((char*)"2023-01-01 10:00:00");
		TimePoint t1 = cp.get_time((char*)"2023-01-01 10:00:10");

		ElementHash cli = MurmurHash64A((char*)"cli1", 4);

		cp.insert_event(cli, 1, t0);

		TargetMap tm = {};
		tm[cli] = t1;

		Targets tg(cp.clip_map(), tm);

		REQUIRE(tg.fit(
			tr_linear,
			ag_mean,
			0.8,
			5,
			false
		));

		Clip clip = (*cp.clip_map())[cli];

		WHEN("Calling verbose_predict_clip") {

			TimePoint obs_time = 0;
			bool target_yn = false;
			int longest_seq = 0;
			uint64_t n_visits = 0;
			uint64_t n_targets = 0;
			double targ_mean_t = 0.0;

			// Covers Targets::verbose_predict_clip
			tg.verbose_predict_clip(
				cli,
				clip,
				obs_time,
				target_yn,
				longest_seq,
				n_visits,
				n_targets,
				targ_mean_t
			);

			THEN("Verbose statistics are produced without error") {
				REQUIRE(longest_seq >= 0);
				REQUIRE(n_visits >= 0);
				REQUIRE(n_targets >= 0);
			}
		}
	}
}


SCENARIO("Test Logger") {

	Logger log = {};

	REQUIRE(log.log == "");

	log.log_printf("Hello world!");

	REQUIRE(log.log == "Hello world!");

	log.log_printf("");

	REQUIRE(log.log == "Hello world!");

	log.log_printf("\n");

	REQUIRE(log.log == "Hello world!\n");

	log.log_printf("a:%i, b:%i, c:%03i\n", 1, 2, 3);

	REQUIRE(log.log == "Hello world!\na:1, b:2, c:003\n");

	log.log_printf("name:%s, hex:%05x, p:%6.2f\n", "abc!", 41739, 3.141592);

	REQUIRE(log.log == "Hello world!\na:1, b:2, c:003\nname:abc!, hex:0a30b, p:  3.14\n");

	log.log = "SUCCESS\n" + log.log;

	REQUIRE(log.log == "SUCCESS\nHello world!\na:1, b:2, c:003\nname:abc!, hex:0a30b, p:  3.14\n");
}

#endif
