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
#include <algorithm>
#include <map>
#include <math.h>
#include <set>
#include <string>
#include <string.h>
#include <time.h>
#include <vector>
#include <stdarg.h>

#ifdef DEBUG
#include <iostream>
#include <fstream>
#include <sstream>
#endif

#ifdef TEST
#ifndef INCLUDED_CATCH2
#define INCLUDED_CATCH2

#include "catch.hpp"

#endif
#endif


#ifndef INCLUDED_REELS_TYPES
#define INCLUDED_REELS_TYPES


/*! \brief The namespace including everything to simplify using Reels in a c++ application,

*/
namespace reels
{

#define IMAGE_BUFF_SIZE			6136					///< Makes sizeof(ImageBlock) == 6K.
#define PRIORITY_SEEN_FACTOR	2000000000				///< Close enough to 2^31 increases by 1 on visit, multiplies by this on seen.
#define DEFAULT_NUM_EVENTS		1000					///< A size to store events in an Events object by default.
#define MAX_SEQ_LEN_IN_PREDICT	1000					///< The maximum sequence length used in prediction.
#define PREDICT_MAX_TIME		(100*365.25*24*3600)	///< Hundred years when the target was never seen.
#define WEIGHT_PRECISION		10000					///< 10^ the number of digits at which weight is rounded

typedef uint64_t 						ElementHash;	///< A binary hash of a string
typedef std::string						String;			///< A dynamically allocated c++ string
typedef const char *					pChar;			///< A c string

typedef time_t							TimePoint;		///< A c 8 byte integer time point
typedef struct tm						TimeStruct;		///< A c structure of integer fields
typedef double							ExtFloat;		///< Accumulator type: Was a 128 bit float, changed to 64 for macos compatibility.


/** \brief An generic block structure to store object state in a Python-friendly way.

	This structure is 64encoded to 8K (3 input bytes == 24 bit -> 4 output chars == 24 bit). Therefore, its size is 6K.
*/
struct ImageBlock {
	int size;											///< The number of already allocated bytes inside the current block
	int block_num;										///< The current block number in the BinaryImage

	uint8_t buffer[IMAGE_BUFF_SIZE];					///< The buffer
};


typedef std::vector<ImageBlock>			BinaryImage;	///< An array of generic blocks to serialize anything
typedef BinaryImage					   *pBinaryImage;	///< A pointer to BinaryImage


/** \brief The binary representation of an event as stored in a transaction file.

	This is the intersection of transaction and event.
*/
struct BinEventPt {
	ElementHash e;										///< The "emitter". A binary hash of a string representing "owner of event".
	ElementHash d;										///< The "description". A binary hash of a string representing "the event".
	double w;											///< The "weight". A double representing a weight of the event.

	/** \brief Compare to another BinEventPt for identity to support use as a key in a map.

		\param o Another BinEventPt to which the current object is compared.

		\return	 True if identical.
	*/
	bool operator==(const BinEventPt &o) const {
		return e == o.e && d == o.d && round(WEIGHT_PRECISION*w) == round(WEIGHT_PRECISION*o.w);
	}

	/** \brief Compare to another BinEventPt for strict order to support use as a key in a map.

		\param o Another BinEventPt to which the current object is compared.

		\return	 True if strictly smaller (before in the order).
	*/
	bool operator<(const BinEventPt &o) const {
		return e < o.e || (e == o.e && d < o.d) || (e == o.e && d == o.d && round(WEIGHT_PRECISION*w) < round(WEIGHT_PRECISION*o.w));
	}
};


/** \brief The binary representation of a transaction in a transaction file.

	The class row converts the many possible source forms into this binary representation.
*/
struct BinTransaction: BinEventPt {
	ElementHash c;										///< The "client". A binary hash of a string representing "the actor".
	TimePoint t;										///< The "time". A timestamp of the event.
};


/** \brief The metrics associated to an event identified by a BinEventPt.

	The map from BinEventPt to this defines the event storage.
*/
struct EventStat {
	uint64_t seen;				///< Number of times the event has been seen in the data.
	uint64_t code;				///< A code number identifying the event.
	uint64_t priority;			///< The (unique) current priority assigned in the priority queue (set) to this event.
};


/** \brief EventMap: A map from hashes in an BinEventPt to usage data defines the info about an event.

This map is combined with a PriorityMap to update observed events.
*/
typedef std::map<BinEventPt, EventStat> EventMap;


/** \brief PriorityMap: A map with all the acceptable priority values in the EventMap as keys.

This map is the priority queue that accepts removal by least priority of current value.
*/
typedef std::map<uint64_t, BinEventPt> PriorityMap;


/** \brief EventCodeMap: A map converting the space of Event codes into a lower cardinality set for Event optimization.

*/
typedef std::map<uint64_t, uint64_t> EventCodeMap;


/** \brief CodeInTreeStatistics: A structure to compute aggregated statistics of for each code

*/
struct CodeInTreeStatistics {
	uint64_t n_succ_seen;		///< # clips that visited the parents (target and no target) of all nodes before this code in tree.
	uint64_t n_succ_target;		///< # clips with the target of all nodes before this code in tree (after in sequence).
	uint64_t n_incl_seen;		///< # clips that with the node (target and no target).
	uint64_t n_incl_target;		///< # clips that visited the node with the target.
	uint64_t sum_dep;			///< Sum of tree depth to estimate mean depth.
	int		 n_dep;				///< Number of elements sum_dep has.
};


/** \brief CodeInTreeStatMap: A map to store all the CodeInTreeStatistics by code.

*/
typedef std::map<uint64_t, CodeInTreeStatistics> CodeInTreeStatMap;


/** \brief StringUsage: A pair of String and number of times it is used.

*/
struct StringUsage {
	uint64_t seen;	///< Number of times string is used. Increase by add_str() calls to the same string, decreased/destroyed by erase_str().
	String str;		///< The string as plain text.
};


/** \brief StringUsageMap: A map from hashes to string and number of times the string is used.

This map allows doing the reverse conversion to a hash() function finding out the original string.
*/
typedef std::map<ElementHash, StringUsage> StringUsageMap;


/** \brief ClientIDs: A vector of client ID hashes.

This map is the fundamental storage of the Clients class preserving the order in which they are defined.
*/
typedef std::vector<ElementHash> ClientIDs;


/** \brief ClientIDSet: A set of client ID hashes.

This set allows fast search of a Client ID hash.
*/
typedef std::set<ElementHash> ClientIDSet;


/** \brief Clip: The clip (timeline) of a client is just a map of time points and codes.
*/
typedef std::map<TimePoint, uint64_t> Clip;


/** \brief ClipMap: A map from clients to clips.

This map is the fundamental storage of the Clips class.
*/
typedef std::map<ElementHash, Clip> ClipMap;
typedef ClipMap * pClipMap;	///< Pointer to a ClipMap


/** \brief TargetMap: A map from clients to target event TimePoints.

This map is given to the constructor of a Target object.
*/
typedef std::map<ElementHash, TimePoint> TargetMap;
typedef TargetMap * pTargetMap;					///< Pointer to a TargetMap


/** \brief TimesToTarget: A vector of predictions.

This is the return type of all predict methods.
*/
typedef std::vector<double>	TimesToTarget;


/** \brief CodeSet: A set of event codes.
*/
typedef std::set<uint64_t> CodeSet;
typedef CodeSet * pCodeSet;					///< Pointer to a CodeSet


/** \brief OptimizeEvalItem: A structure to compare predicted and observed.
*/
struct OptimizeEvalItem {
	double		t_hat;		///< The prediction (elapsed time since the last event in clip to predicted target).
	TimePoint	t_obs;		///< The observed result: Zero for not a target or elapsed time since the previous event in clip to target.
	int			seq_len;	///< The length of the predicting clip.
};


/** \brief OptimizeEval: A vector of OptimizeEvalItem.
*/
typedef std::vector<OptimizeEvalItem>	OptimizeEval;


/** \brief CodeScoreItem: A structure to sort codes by lift.
*/
struct CodeScoreItem {
	uint64_t	code;	///< The code.
	double		score;	///< The score.
};


/** \brief CodeScores: A vector of CodeScoreItem.
*/
typedef std::vector <CodeScoreItem> CodeScores;


/** \brief ChildIndex: A map to find the next child in a CodeTree.
*/
typedef std::map<uint64_t, int>	ChildIndex;


/** \brief CodeTreeNode: Each node in a fitted CodeTree.
*/
struct CodeTreeNode {
	uint64_t   n_seen;		///< The number of clips that visited the node (target and no target).
	uint64_t   n_target;	///< The number of clips that visited the node with the target.
	ExtFloat   sum_time_d;	///< Sum of time differences for the elements with a defined target.
	ChildIndex child;		///< A map of children by code (key) to index in the CodeTree.
};
typedef CodeTreeNode * pCodeTreeNode;			///< Pointer to a CodeTreeNode


/** \brief CodeTree: A tree of fitted targets.

This is the complete fitted model in a Targets object.
*/
typedef std::vector<CodeTreeNode>	CodeTree;
typedef CodeTree * pCodeTree;					///< Pointer to a CodeTree


/** \brief Transform: The transformation applied to time differences. (And inverted again in predict().)
*/
enum Transform {tr_undefined, tr_linear, tr_log};


/** \brief Aggregate: The method used to aggregate predictions for different sequence lengths.
*/
enum Aggregate {ag_undefined, ag_mean, ag_minimax, ag_longest};


// Forward declaration of utilities used in other functions.
uint64_t MurmurHash64A (const void *key, int len);
bool image_put(pBinaryImage p_bi, void *p_data, int size);
bool image_get(pBinaryImage p_bi, int &c_block, int &c_ofs, void *p_data, int size);


/** \brief A minimalist logger stored as a std::string providing sprintf functionality.
*/
class Logger {

	public:

		/** \brief Logging method wrapper supporting variable arguments.

			\param fmt	A vsnprintf compatible format definition.
			\param ...	A variable list of arguments.

			This method calls the other log_printf() override with the arguments in a va_list.
		*/
		void log_printf(const char *fmt, ...) {
			va_list args;
			va_start(args, fmt);
			log_printf(fmt, args);
			va_end(args);
		}

		/** \brief Add a nicely formatted string smaller than 256 chars to the logger.

			\param fmt	A vsnprintf compatible format definition.
			\param args	A variable list of arguments stored as a va_list.
		*/
		void log_printf(const char *fmt, va_list args) {
			char buffer[256];

			vsnprintf(buffer, sizeof(buffer), fmt, args);

			log = log + buffer;
		}

		/** \brief The std::string storing the content of the Logger is public.
		*/
		String log = "";
};


class Clips;		// Forward declaration

/** \brief A container class to hold events.

	This class has two different modes, in both cases, it is constructed empty and the public properties store_strings,
	time_format and max_num_events can be set after construction.

		1. The object identifies recurring events from a sequence of transactions passed to the object using insert_row()
		2. The object is given the events as a series of define_event() calls

	To simplify the Python interface, the object has set_max_num_events() and set_store_strings() as methods.
*/
class Events {

	public:

		Events() {}

		bool store_strings	= true;						///< If true, the object stores the string values
		int	 max_num_events	= DEFAULT_NUM_EVENTS;		///< The maximum number of recurrent event stored via insert_row()


		/** \brief Process a row from a transaction file.

			\param p_e	The "emitter". A C/Python string representing "owner of event".
			\param p_d	The "description". A C/Python string representing "the event".
			\param w	The "weight". A double representing a weight of the event.

			**Caveat**: insert_row() and define_event() should not be mixed. The former is for event discovery and the latter
			for explicit definition. A set of events is build either one way or the other.
		*/
		void insert_row(pChar p_e,
						pChar p_d,
						double w);


		/** \brief Define events explicitly.

			\param p_e	The "emitter". A C/Python string representing "owner of event".
			\param p_d	The "description". A C/Python string representing "the event".
			\param w	The "weight". A double representing a weight of the event.
			\param code	A unique code number identifying the event.

			\return	 True on success.

			**Caveat**: insert_row() and define_event() should not be mixed. The former is for event discovery and the latter
			for explicit definition. A set of events is build either one way or the other.
		*/
		bool define_event(pChar	   p_e,
						  pChar	   p_d,
						  double   w,
						  uint64_t code);


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

			\param clips			 A clips object with the same codes and clips for a set of clients whose prediction we optimize.
			\param targets			 The target events in a TargetMap object in the same format expected by a Targets object. (Internally
									 a Targets object will be used to make the predictions we want to optimize.)
			\param num_steps		 The number of steps to iterate. The method will stop early if no codes are found at a step.
			\param codes_per_step	 The number of codes to be tried from the top of the priority list at each step.
			\param threshold		 A minimum threshold, below which a score change is not considered improvement.
			\param p_force_include	 An optional pointer to a set of codes that must be included before starting.
			\param p_force_exclude	 An optional pointer to a set of codes that will excluded and set to the base code.
			\param x_form			 The x_form argument to fit the internal Targets object prediction model.
			\param agg				 The agg argument to fit the internal Targets object prediction model.
			\param p				 The p argument to fit the internal Targets object prediction model.
			\param depth			 The depth argument to fit the internal Targets object prediction model.
			\param as_states		 The as_states argument to fit the internal Targets object prediction model.
			\param exponential_decay Exponential Decay Factor applied to the internal score in terms of depth. That score selects what
									 codes enter the model. The decay is applied to the average tree depth. 0 is no decay, default
									 value = 0.00693 decays to 0.5 in 100 steps.
			\param lower_bound_p	 Another p for lower bound, but applied to the scoring process rather than the model.
			\param log_lift			 A boolean to set if lift (= LB(included)/LB(after inclusion)) is log() transformed or not.

			\return	 A \n separated report string that contains either "ERROR" or "success" as the first line.
		*/
		String optimize_events (Clips &clips, TargetMap &targets, int num_steps = 10, int codes_per_step = 5, double threshold = 0.0001,
								pCodeSet p_force_include = nullptr, pCodeSet p_force_exclude = nullptr, Transform x_form = tr_linear,
								Aggregate agg = ag_longest, double p = 0.5, int depth = 1000, bool as_states = true,
								double exponential_decay = 0.00693, double lower_bound_p = 0.95, bool log_lift = true);


		/** \brief Internal: Do one step of the optimize_events() method.

			\param score			Returns score by reference.
			\param targ_prop		Returns the targets/seen proportion at the tree root by reference (used by get_top_codes).
			\param codes_stat		Returns a complete CodeInTreeStatMap if calc_tree_stats is true.
			\param calc_tree_stats	Complete a tree search (if true) or just evaluate the score if not.
			\param clips			A clips object with the same codes and clips for a set of clients whose prediction we optimize.
			\param targets			The target events in a TargetMap object in the same format expected by a Targets object. (Internally
									a Targets object will be used to make the predictions we want to optimize.)
			\param code_dict		A dictionary of code transformations to be applied to a copy of the clips before fitting.
			\param x_form			The x_form argument to fit the internal Targets object prediction model.
			\param agg				The agg argument to fit the internal Targets object prediction model.
			\param p				The p argument to fit the internal Targets object prediction model.
			\param depth			The depth argument to fit the internal Targets object prediction model.
			\param as_states		The as_states argument to fit the internal Targets object prediction model.

			\return	 False on error.
		*/
		bool score_model(double &score, double &targ_prop, CodeInTreeStatMap &codes_stat, bool calc_tree_stats, Clips &clips,
						 TargetMap &targets, EventCodeMap code_dict, Transform x_form, Aggregate agg, double p, int depth, bool as_states);


		/** \brief Internal: Extract the top top_n codes by lift from a CodeInTreeStatMap map.

			\param codes_stat		 A complete CodeInTreeStatMap computed by score_model().
			\param targ_prop		 The targets/seen proportion at the tree root.
			\param exponential_decay Exponential Decay Factor applied to the internal score in terms of depth. That score selects what
									 codes enter the model. The decay is applied to the average tree depth. 0 is no decay, default
									 value = 0.00693 decays to 0.5 in 100 steps.
			\param lower_bound_p	 Another p for lower bound, but applied to the scoring process rather than the model.
			\param log_lift			 A boolean to set if lift (= LB(included)/LB(after inclusion)) is log() transformed or not.

			\return	 A sorted (by lift) vector of codes top first.
		*/
		CodeScores get_top_codes(CodeInTreeStatMap &codes_stat, double targ_prop, double exponential_decay, double lower_bound_p,
								 bool log_lift);


		/** \brief Compute Pearson linear correlation between predicted and observed in an OptimizeEval

			\param ev The vector of OptimizeEvalItem containing t_obs (observed) and t_hat (predicted) values.

			\return	The linear correlation.
		*/
		double linear_correlation(OptimizeEval &ev) {
			ExtFloat s_h = 0, s_o = 0, sho = 0, ssh = 0, sso = 0;
			int n = 0;

			for (OptimizeEval::iterator it = ev.begin(); it != ev.end(); ++it) {
				if (it->t_obs != 0) {
					s_h += it->t_hat;
					s_o += it->t_obs;
					sho += it->t_hat*it->t_obs;
					ssh += it->t_hat*it->t_hat;
					sso += it->t_obs*it->t_obs;

					n++;
				}
			}
			if (n == 0)
				return 0;

			double d2 = (n*ssh - s_h*s_h)*(n*sso - s_o*s_o);

			if (d2 <= 1e-20)
				return 0;

			return (n*sho - s_h*s_o)/sqrt(d2);
		}


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi);


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi	   The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.
			\param c_block The current reading cursor (block number) required only for nested use of load().
			\param c_ofs   The current reading cursor (offset in block) required only for nested use of load().

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi, int &c_block, int &c_ofs);


		/** \brief Save the state of an object into a base64 mercury-dynamics serialization using image_put()

			\param p_bi The address of a BinaryImage stream that is either empty or has been used only for writing.

			\return	 True on success (Most likely error is allocation).
		*/
		bool save(pBinaryImage &p_bi);


		/** \brief Sets the public property max_num_events to simplify the python interface.

			\param max_events The value to apply to max_num_events.
		*/
		inline void set_max_num_events(int max_events) {
			max_num_events = max_events;
		}


		/** \brief Sets the public property store_strings to simplify the python interface.

			\param store True for storing the string contents.
		*/
		inline void set_store_strings(bool store) {
			store_strings = store;
		}


		/** \brief Define a new string and push it into the StringUsageMap.

			\param p_str	The string to be added.

			\return	The hash.
		*/
		inline ElementHash add_str(pChar p_str) {
			int ll = strlen(p_str);

			if (!ll)
				return 0;

			ElementHash hash = MurmurHash64A(p_str, ll);

			if (!store_strings)
				return hash;

			StringUsageMap::iterator it = names_map.find(hash);

			if (it != names_map.end())
				it->second.seen++;
			else {
				StringUsage su = {1, p_str};

				names_map[hash] = su;
			}
			return hash;
		}


		/** \brief Remove a string from the StringUsageMap by decreasing its use count and destroying it if not used anymore.

			\param hash	hash(key)

		*/
		inline void erase_str(ElementHash hash) {

			if (store_strings) {
				StringUsageMap::iterator it = names_map.find(hash);

				if (it != names_map.end()) {
					if (--it->second.seen == 0)
						names_map.erase(it);
				}
			}
		}


		/** \brief Get a string content from its hash value.

			\param hash	hash(key)

			\return	The string if found, or a single EOT (End of transmission) character if not found. (An empty string is usable.)
		*/
		inline String get_str(ElementHash hash) {

			if (store_strings) {
				if (!hash)
					return "";

				StringUsageMap::iterator it = names_map.find(hash);

				if (it != names_map.end())
					return it->second.str;
			}
			return "\x04";
		}


		/** \brief Return the code associated to an BinEventPt if found in the object.

			\param ept	The BinEventPt searched.

			\return	The code if found, or zero if not.
		*/
		inline uint64_t event_code(BinEventPt &ept) {
			EventMap::iterator it = event.find(ept);

			if (it == event.end())
				return 0;

			return it->second.code;
		}


		/** \brief Return the number of events stored in the object.

			\return	The size of the internal EventMap().
		*/
		inline int num_events() {
			return event.size();
		}


		/** \brief Return the EventMap::iterator to the first elements in the private variable .events.

			\return	The iterator to the first element.
		*/
		inline EventMap::iterator events_begin() {
			return event.begin();
		}


		/** \brief Return the EventMap::iterator to past-the-end in the private variable .events.

			\return	The iterator to the first element past-the-end.
		*/
		inline EventMap::iterator events_end() {
			return event.end();
		}


		/** \brief Return the EventMap::iterator to the next BinEventPt after matching ev or nullptr if not found or is last.

			\param ept	The BinEventPt searched.

			\return	The iterator to the next element after the matching element or events_end() when no next element exists.
		*/
		inline EventMap::iterator events_next_after_find(BinEventPt &ept) {
			EventMap::iterator it = event.find(ept);

			if (it != event.end())
				++it;

			return it;
		}

#ifndef TEST
	private:
#endif

		uint64_t priority_low = 0;
		uint64_t next_code	  = 0;

		StringUsageMap names_map = {};
		EventMap	   event	 = {};
		PriorityMap	   priority	 = {};
};


/** \brief A container class to hold client ids.

*/
class Clients {

	public:

		Clients() {}


		/** \brief Return the hash of a client ID as an ElementHash.

			\param p_cli The "client". A string representing "the actor".

			\return	 The hash.
		*/
		inline ElementHash hash_client_id(pChar p_cli) {
			int ll = strlen(p_cli);

			return ll == 0 ? 0 : MurmurHash64A(p_cli, ll);
		}


		/** \brief Add a client ID to this container.

			\param p_cli The "client". A string representing "the actor".
		*/
		void add_client_id(pChar p_cli);


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi);


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi	   The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.
			\param c_block The current reading cursor (block number) required only for nested use of load().
			\param c_ofs   The current reading cursor (offset in block) required only for nested use of load().

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi, int &c_block, int &c_ofs);


		/** \brief Save the state of an object into a base64 mercury-dynamics serialization using image_put()

			\param p_bi The address of a BinaryImage stream that is either empty or has been used only for writing.

			\return	 True on success (Most likely error is allocation).
		*/
		bool save(pBinaryImage &p_bi);

		ClientIDs	id	   = {};	///< The vector containing client ids as hashes in the order of definition.
		ClientIDSet id_set = {};	///< The set of the same hashes for fast search.
};


/** \brief A common ancestor of Clips and Targets to avoid duplicating time management.
*/
class TimeUtil {

	public:

		TimeUtil() {}

		char time_format[128] = "%Y-%m-%d %H:%M:%S";	///< Date and time format for insert_row() and define_event()


		/** \brief Convert time as a string to a TimePoint (using the object's time_format).

			\param p_t	A string containing a valid date and time in the object's time_format.

			\return	The TimePoint corresponding to that time or a negative value on error.
		*/
		inline TimePoint get_time(pChar p_t) {

			TimeStruct ts = {0};

			if (strptime(p_t, time_format, &ts) == nullptr)
				return -1;

			return timegm(&ts);
		}


		/** \brief Sets the public property time_format to simplify the python interface.

			\param fmt The time format in standard calendar time format
					   http://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html
		*/
		inline void set_time_format(pChar fmt) {
			strncpy(time_format, fmt, sizeof(time_format) - 1);
		}
};


/** \brief A container class to hold clips (sequences of events).

	To simplify the Python interface, the object has a set_time_format() method.
*/
class Clips : public TimeUtil {

	public:

		/** \brief Default construct a Clips object as an abstract method.
			This is required for declaring a Clips object that will later be copy constructed based on conditional statements.
		*/
		Clips() {}


		/** \brief Construct a Clips object from a Clients and an Events objects.

			\param clients	The list of all the clients to be processed. If empty, all the clients will be considered.
			\param events	An initialized Events object created either by auto-detection (insert_row) or definition (define_event).
		*/
		Clips(Clients clients, Events events) : clients(clients), events(events) {}


		/** \brief Construct a Clips object from a ClipMap to be copied.

			\param clip_map	The Clips object to be copied.
		*/
		Clips(const ClipMap &clip_map) {
			clients = Clients();
			events	= Events();

			clips = clip_map;
		}


		/** \brief Copy-construct a Clips object.

			\param o_clips	The Clips object to be copied.
		*/
		Clips(Clips &o_clips) {
			pBinaryImage p_bi = new BinaryImage;

			o_clips.save(p_bi);

			load(p_bi);

			delete p_bi;
		}


		/** \brief Process a row from a transaction file, to add the event to the client's timeline (clip).

			\param p_e	The "emitter". A C/Python string representing "owner of event".
			\param p_d	The "description". A C/Python string representing "the event".
			\param w	The "weight". A double representing a weight of the event.
			\param p_c	The "client". A C/Python string representing "the actor".
			\param p_t	The "time". A timestamp of the event as a C/Python string. (The format is given via set_time_format().)

			\return	 True on insertion. False usually just means, the event is not in events or the client is not in clients.
					 Occasionally, it may be a time parsing error.
		*/
		bool scan_event(pChar p_e,
						pChar p_d,
						double w,
						pChar p_c,
						pChar p_t);


		/** \brief The kernel of a scan_event() made inline, when all checks and conversion to binary are successful.

			\param client_hash	The "client". Already verified for insertion and converted into hash.
			\param code			The code number identifying the event already found in events.
			\param time_pt		The "time" already verified and converted into a TimePoint.

		*/
		inline void insert_event(ElementHash client_hash,
								 uint64_t	 code,
								 TimePoint	 time_pt) {

			ClipMap::iterator it = clips.find(client_hash);

			if (it == clips.end()) {
				Clip clip = {};

				clip[time_pt] = code;

				clips[client_hash] = clip;

			} else
				it->second[time_pt] = code;
		};


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi);


		/** \brief Save the state of an object into a base64 mercury-dynamics serialization using image_put()

			\param p_bi The address of a BinaryImage stream that is either empty or has been used only for writing.

			\return	 True on success (Most likely error is allocation).
		*/
		bool save(pBinaryImage &p_bi);


		/** \brief The address of the internal ClipMap to be accessed from a Targets object.

			\return	 The address.
		*/
		inline pClipMap clip_map() {
			return &clips;
		}


		/** \brief Return the number of events stored in the internal ClipMap.

			\return	The total count of events aggregating all the clips in the internal ClipMap.
		*/
		inline uint64_t num_events() {

			uint64_t ret = 0;

			for (ClipMap::iterator it = clips.begin(); it != clips.end(); ++it)
				ret += it->second.size();

			return ret;
		}

		/** \brief Collapse the ClipMap to states.

			This removes identical consecutive codes from all the clips in the ClipMap keeping the time of the first instance.
		*/
		inline void collapse_to_states() {
			for (ClipMap::iterator it_client = clips.begin(); it_client != clips.end(); ++it_client) {
				uint64_t last_code = 0xA30BdefacedCabal;
				for (Clip::const_iterator it = it_client->second.cbegin(); it != it_client->second.cend();) {
					uint64_t code = it->second;
					if (code == last_code)
						it_client->second.erase(it++);
					else
						++it;
					last_code = code;
				}
			}
		}

#ifndef TEST
	private:
#endif

		Clients clients;
		Events	events;
		ClipMap	clips	= {};
};


/** \brief A container class to hold target events and do predictions based on clips.

*/
class Targets	: public TimeUtil {

	public:

		/** \brief Construct a Targets object from a Clips object and a TargetMap.

			\param p_clips	The address of a Clips object initialized with the clips of a set of clients.
			\param target	A TargetMap with the even times for a subset of the same clients (those who experienced the target).
		*/
		Targets(pClipMap p_clips, TargetMap target) : p_clips(p_clips), target(target) {
			CodeTreeNode root = {0, 0, 0, {}};
			tree.push_back(root);
		}


		/** \brief Utility to fill the internal TargetMap target

			The TargetMap can be initialized and given to the constructor, or an empty TargetMap can be given to the constructor ans
			initialized by this method.

			\param p_c	The "client". A C/Python string representing "the actor".
			\param p_t	The "time". A timestamp of the event as a C/Python string. (The format is given via set_time_format().)

			\return	 True on new clients. False if the client is already in the TargetMap or the time is in the wrong format.
		*/
		bool insert_target(pChar p_c, pChar p_t);


		/** \brief Fit the prediction model

			\param x_form	 A possible transformation of the times. (Currently "log" or "linear".)
			\param agg		 The mechanism used for the aggregation. (Currently "minimax", "mean" or "longest".)
			\param p		 The width of the confidence interval for the binomial proportion used to calculate the lower bound.
							 (E.g., p = 0.5 will estimate a lower bound of a symmetric CI with coverage of 0.5.)
			\param depth	 The maximum depth of the tree (maximum sequence length learned).
			\param as_states Treat events as states by removing repeated ones from the ClipMap keeping the time of the first instance only.
							 When used, the ClipMap passed to the constructor by reference will be converted to states as a side effect.

			Fit can only be called once in the life of a Targets object and predict() cannot be called before fit().

			\return	 True on success. Error is already fitted or wrong arguments.
		*/
		bool fit(Transform x_form, Aggregate agg, double p, int depth, bool as_states);


		/** \brief Predict time to target for all the clients in the Clips object used to fit the model.

			predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

			\return	 A vector with the times.
		*/
		TimesToTarget predict();


		/** \brief Predict time to target for all the clients in a given Clients object whose clips have been used to fit the model.

			It will predict the prediction of the zero-length clip (accumulated in the root node) if the client is not found.

			\param clients A Clients object with a subset of the clients used to fit the model.

			predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

			\return	 A vector with the times.
		*/
		TimesToTarget predict(Clients clients);


		/** \brief Predict time to target for a set of clients whose clips are given in a ClipMap.

			\param p_clips A ClipMap of clients and clips to be used in prediction.

			predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

			\return	 A vector with the times.
		*/
		TimesToTarget predict(pClipMap p_clips);


		/** \brief Predict time for a single Clip returning all kind of prediction related information.

			\param client		The client hash (needed to see if he fits the target).
			\param clip			The clip we want to predict
			\param obs_time		A variable to store the time between the last even and the target when the target is hit.
			\param target_yn	A variable to store if the target was hit (true) or not (false).
			\param longest_seq	A variable to store the longest aligned matching sequence stored in the tree.
			\param n_visits		A variable to store the number of visits for the longest sequence.
			\param n_targets	A variable to store the number of target hits for the longest sequence.
			\param targ_mean_t	A variable to store average observed time for hits in the longest sequence.
		*/
		void verbose_predict_clip(const ElementHash &client,
								  Clip				&clip,
								  TimePoint			&obs_time,
								  bool				&target_yn,
								  int				&longest_seq,
								  uint64_t			&n_visits,
								  uint64_t			&n_targets,
								  double			&targ_mean_t);


		/** \brief Load the state of an object from a base64 mercury-dynamics serialization using image_get()

			\param p_bi The address of a BinaryImage stream containing a previously save()-ed image at the cursor position.

			\return	 True on success (Most likely error is a wrong stream).
		*/
		bool load(pBinaryImage &p_bi);


		/** \brief Save the state of an object into a base64 mercury-dynamics serialization using image_put()

			\param p_bi The address of a BinaryImage stream that is either empty or has been used only for writing.

			\return	 True on success (Most likely error is allocation).
		*/
		bool save(pBinaryImage &p_bi);


		/** \brief Update (fit) the CodeTree inserting new nodes as necessary.

			\param idx_parent The index of the parent node. For the first insertion, root == 0. For more, returned values of this.
			\param code		  The node will be the child of the parent node whose code is this code.
			\param target	  The target was matched in the clip or not.
			\param time_d	  The time difference from the code to the target (when there is a target, must be 0 otherwise).

			\return	The index of the current node.
		*/
		inline int update_node(int idx_parent, uint64_t code, bool target, ExtFloat time_d) {

			if (idx_parent == 0) {		// The root node contains the prediction of the zero-length clip.
				tree[0].n_seen++;
				if (target) {
					tree[0].n_target++;
					tree[0].sum_time_d += time_d;
				}
			}

			ChildIndex::iterator it = tree[idx_parent].child.find(code);

			if (it != tree[idx_parent].child.end()) {
				int idx = it->second;

				tree[idx].n_seen++;
				if (target) {
					tree[idx].n_target++;
					tree[idx].sum_time_d += time_d;
				}

				return idx;
			}

			CodeTreeNode node = {1, target, time_d, {}};

			tree.push_back(node);

			int idx = tree.size() - 1;

			tree[idx_parent].child[code] = idx;

			return idx;
		}


		/** \brief Density (pdf) for the normal distribution with mean 0 and standard deviation 1.

			\param x	The quantile.

			\return		The density.
		*/
		inline double normal_pdf(double x) {
			// https://stackoverflow.com/questions/10847007/using-the-gaussian-probability-density-function-in-c

			static const double inv_sqrt_2pi = 0.3989422804014327;

			return exp(-0.5*x*x)*inv_sqrt_2pi;
		}


		/** \brief Cumulative distribution (cdf) for the normal distribution with mean 0 and standard deviation 1.

			\param x	The quantile.

			\return		The cumulative probability.
		*/
		inline double normal_cdf(double x) {
			// https://stackoverflow.com/questions/2328258/cumulative-normal-distribution-function-in-c-c
			// https://cplusplus.com/reference/cmath/erfc/

			static const double m_sqrt_dot_5 = 0.7071067811865476;

			return 0.5*erfc(-x*m_sqrt_dot_5);
		}


		/** \brief Upper bound of the Agresti-Coull confidence interval for a binomial proportion.

			The confidence level is passed as an argument to the fit() method, the fit method computes some "binomial_z.*" variables
			used in this function.

			\param n_hits	The number of successes.
			\param n_total	The number of independent trials.

			\return	The upper bound of the AC confidence interval.
		*/
		inline double agresti_coull_upper_bound(uint64_t n_hits, uint64_t n_total) {
			// https://github.com/msn0/agresti-coull-interval/blob/master/src/agresti.js

			double n_tilde = n_total + binomial_z_sqr;
			double p_tilde = (n_hits + binomial_z_sqr_div_2)/n_tilde;
			double a	   = binomial_z*sqrt(p_tilde*(1 - p_tilde)/n_tilde);

			return p_tilde + a;
		}


		/** \brief Lower bound of the Agresti-Coull confidence interval for a binomial proportion.

			The confidence level is passed as an argument to the fit() method, the fit method computes some "binomial_z.*" variables
			used in this function.

			\param n_hits	The number of successes.
			\param n_total	The number of independent trials.

			\return	The upper bound of the AC confidence interval.
		*/
		inline double agresti_coull_lower_bound(uint64_t n_hits, uint64_t n_total) {
			// https://github.com/msn0/agresti-coull-interval/blob/master/src/agresti.js

			double n_tilde = n_total + binomial_z_sqr;
			double p_tilde = (n_hits + binomial_z_sqr_div_2)/n_tilde;
			double a	   = binomial_z*sqrt(p_tilde*(1 - p_tilde)/n_tilde);

			return p_tilde - a;
		}


		/** \brief Predict the time to target for a sub-clip that starts at a node.

			This method assumes uniform distribution over time of the events (like Poisson distribution) but weighted by evidence.

			Basically, we assume that the observed maximum likelihood time (mu_hat) is the mean (therefore the half) of a uniform
			distribution U(0, 2*mu_hat) and the whole "event space", if estimated without bias, would be U(0, 2*mu_hat*n_seen/n_target).

			Caveat 1. We don't want it unbiased, but biased by evidence, because many estimates will be done, "lucky paths" are a real
			thing, especially with low number of visits. That is why we use agresti_coull_lower_bound() and not n_seen/n_target.
			Note that, of course, as you reduce the confidence (the p argument of fit), you will make it approach n_seen/n_target.

			Caveat 2. We, intentionally bias towards underestimating the urgency. With much evidence, it will be close to unbiased, but
			with little evidence, if will underrepresent the urgency (proximity of the event happening). This is why minimax will select
			the most urgent (min) of the biased up values (max).

			Caveat 3. mu_hat is numerically more stable if the transformation is log (although the aggregation it is computed in
			128 bit floating point arithmetic which should be stable either way) but, in that case mu_hat will not be the mean time,
			but its geometric mean (since adding logs is multiplying).

			\param node	The node in the tree defining the sub-clip.

			\return	The predicted time to the target event.
		*/
		inline double predict_time(CodeTreeNode &node) {

			if (node.n_target <= 0)
				return PREDICT_MAX_TIME;

			double lb	  = std::max(1e-4, agresti_coull_lower_bound(node.n_target, node.n_seen));
			double mu_hat = transform == tr_linear ? ((double) node.sum_time_d)/node.n_target : exp(((double) node.sum_time_d)/node.n_target);

			return mu_hat/lb;
		}


		/** \brief Predict the time to target for a clip.

			\param clip	A clip containing a sequence of event codes.

			\return	The predicted time to the target event.
		*/
		inline double predict_clip(Clip clip) {

			int idx = 0, n = 0;

			double t[MAX_SEQ_LEN_IN_PREDICT];

			for (Clip::reverse_iterator it = clip.rbegin(); it != clip.rend(); it++) {
				ChildIndex::iterator jt = tree[idx].child.find(it->second);

				if (jt == tree[idx].child.end())
					break;

				idx = jt->second;

				t[n++] = predict_time(tree[idx]);
			}

			if (n == 0)
				return predict_time(tree[0]);

			if (aggregate == ag_longest)
				return t[n - 1];

			double ret = t[0];

			if (aggregate == ag_mean) {
				for (int i = 1; i < n; i++)
					ret += t[i];

				return ret/n;
			}

			for (int i = 1; i < n; i++)
				ret = std::min(ret, t[i]);

			return ret;
		}


		/** \brief Recursive tree exploration updating a CodeInTreeStatMap map.

			\param depth	  The recursion depth
			\param idx		  The index of the current node
			\param parent_idx The index of the parent node
			\param code		  The current code
			\param codes_stat The CodeInTreeStatMap being updated

			\return	False on error.
		*/
		bool recurse_tree_stats(int depth, int idx, int parent_idx, uint64_t code, CodeInTreeStatMap &codes_stat);


		/** \brief Return the size of the internal TargetMap.

			\return	Size of the internal TargetMap.
		*/
		inline int num_targets() {
			return target.size();
		}


		/** \brief Return the size of the internal CodeTree.

			\return	Size of the internal CodeTree.
		*/
		inline int tree_size() {
			return tree.size();
		}


		/** \brief The address of the internal ClipMap.

			\return	 The address.
		*/
		inline pClipMap clip_map() {
			return p_clips;
		}


		/** \brief The address of the internal CodeTree.

			\return	 The address.
		*/
		inline pCodeTree p_tree() {
			return &tree;
		}


		/** \brief The address of the internal TargetMap.

			\return	 The address.
		*/
		inline pTargetMap p_target() {
			return &target;
		}

#ifndef TEST
	private:
#endif

		pClipMap   p_clips;
		TargetMap  target;
		CodeTree   tree					= {};
		Transform  transform			= tr_undefined;
		Aggregate  aggregate			= ag_undefined;
		double	   binomial_z			= 0;
		double	   binomial_z_sqr		= 0;
		double	   binomial_z_sqr_div_2	= 0;
		int		   tree_depth			= 0;
};

} // namespace reels

#endif // ifndef INCLUDED_REELS_TYPES
