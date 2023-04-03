%module py_reels

%{
	extern int  new_events();
	extern bool destroy_events(int id);
	extern bool events_insert_row(int id, char *p_e, char *p_d, double w);
	extern bool events_define_event(int id, char *p_e, char *p_d, double w, int code);
	extern char *events_optimize_events(int id, int id_clips, int id_targets, int num_steps, int codes_per_step, double threshold,
										char *force_include, char *force_exclude, char *x_form, char *agg, double p, int depth,
										int as_states, double exponential_decay, double lower_bound_p, bool log_lift);
	extern bool events_load_block(int id, char *p_block);
	extern int	events_save(int id);
	extern char *events_describe_next_event(int id, char *prev_event);
	extern bool events_set_max_num_events(int id, int max_events);
	extern bool events_set_store_strings(int id, bool store);
	extern int  events_num_events(int id);

	extern int  new_clients();
	extern bool destroy_clients(int id);
	extern char *clients_hash_client_id(int id, char *p_cli);
	extern bool clients_add_client_id(int id, char *p_cli);
	extern char *clients_hash_by_index(int id, int idx);
	extern int	clients_num_clients(int id);
	extern bool clients_load_block(int id, char *p_block);
	extern int	clients_save(int id);

	extern int  new_clips(int id_clients, int id_events);
	extern bool destroy_clips(int id);
	extern bool clips_set_time_format(int id, char *fmt);
	extern bool clips_scan_event(int id, char *p_e, char *p_d, double w, char *p_c, char *p_t);
	extern char *clips_hash_by_previous(int id, char *prev_hash);
	extern bool clips_load_block(int id, char *p_block);
	extern int	clips_save(int id);
	extern char *clips_describe_clip(int id, char *client_id);
	extern int	clips_num_clips(int id);
	extern int	clips_num_events(int id);
	extern char *clips_test_sequence(int seq_num, bool target);

	extern int  new_targets(int id_clips);
	extern bool destroy_targets(int id);
	extern bool targets_set_time_format(int id, char *fmt);
	extern bool targets_insert_target(int id, char *p_c, char *p_t);
	extern bool targets_fit(int id, char *x_form, char *agg, double p, int depth, int as_states);
	extern int	targets_predict_clients(int id, int id_clients);
	extern int	targets_predict_clips(int id, int id_clips);
	extern bool targets_load_block(int id, char *p_block);
	extern int	targets_save(int id);
	extern int	targets_num_targets(int id);
	extern int	targets_tree_node_idx(int id, int parent_idx, int code);
	extern char *targets_tree_node_children(int id, int idx);
	extern char *targets_describe_tree_node(int id, int idx);
	extern char *targets_describe_tree(int id);

	extern int	  size_result_iterator(int iter_id);
	extern double next_result_iterator(int iter_id);
	extern void	  destroy_result_iterator(int iter_id);

	extern int  size_binary_image_iterator(int image_id);
	extern char *next_binary_image_iterator(int image_id);
	extern bool destroy_binary_image_iterator(int image_id);
%}

extern int  new_events();
extern bool destroy_events(int id);
extern bool events_insert_row(int id, char *p_e, char *p_d, double w);
extern bool events_define_event(int id, char *p_e, char *p_d, double w, int code);
extern char *events_optimize_events(int id, int id_clips, int id_targets, int num_steps, int codes_per_step, double threshold,
									char *force_include, char *force_exclude, char *x_form, char *agg, double p, int depth,
									int as_states, double exponential_decay, double lower_bound_p, bool log_lift);
extern bool events_load_block(int id, char *p_block);
extern int	events_save(int id);
extern char *events_describe_next_event(int id, char *prev_event);
extern bool events_set_max_num_events(int id, int max_events);
extern bool events_set_store_strings(int id, bool store);
extern int  events_num_events(int id);

extern int  new_clients();
extern bool destroy_clients(int id);
extern char *clients_hash_client_id(int id, char *p_cli);
extern bool clients_add_client_id(int id, char *p_cli);
extern char *clients_hash_by_index(int id, int idx);
extern int	clients_num_clients(int id);
extern bool clients_load_block(int id, char *p_block);
extern int	clients_save(int id);

extern int  new_clips(int id_clients, int id_events);
extern bool destroy_clips(int id);
extern bool clips_set_time_format(int id, char *fmt);
extern bool clips_scan_event(int id, char *p_e, char *p_d, double w, char *p_c, char *p_t);
extern char *clips_hash_by_previous(int id, char *prev_hash);
extern bool clips_load_block(int id, char *p_block);
extern int	clips_save(int id);
extern char *clips_describe_clip(int id, char *client_id);
extern int	clips_num_clips(int id);
extern int	clips_num_events(int id);
extern char *clips_test_sequence(int seq_num, bool target);

extern int  new_targets(int id_clips);
extern bool destroy_targets(int id);
extern bool targets_set_time_format(int id, char *fmt);
extern bool targets_insert_target(int id, char *p_c, char *p_t);
extern bool targets_fit(int id, char *x_form, char *agg, double p, int depth, int as_states);
extern int	targets_predict_clients(int id, int id_clients);
extern int	targets_predict_clips(int id, int id_clips);
extern bool targets_load_block(int id, char *p_block);
extern int	targets_save(int id);
extern int	targets_num_targets(int id);
extern int	targets_tree_node_idx(int id, int parent_idx, int code);
extern char *targets_tree_node_children(int id, int idx);
extern char *targets_describe_tree_node(int id, int idx);
extern char *targets_describe_tree(int id);

extern int	  size_result_iterator(int iter_id);
extern double next_result_iterator(int iter_id);
extern void	  destroy_result_iterator(int iter_id);

extern int  size_binary_image_iterator(int image_id);
extern char *next_binary_image_iterator(int image_id);
extern bool destroy_binary_image_iterator(int image_id);
