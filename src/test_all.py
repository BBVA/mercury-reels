# Mercury-Reels

#     Copyright 2023 Banco Bilbao Vizcaya Argentaria, S.A.

#     This product includes software developed at

#     BBVA (https://www.bbva.com/)

#       Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

#       Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

import pickle

import datetime

import reels

from reels.Events import events_describe_next_event


def test_events():
	evn = reels.Events(max_num_events = 5)
	assert evn.num_events() == 0

	st = evn.__str__()
	assert st == evn.__repr__()
	assert st.startswith('reels.Events')
	assert st.endswith(' 0 events')

	assert evn.insert_row('emi', 'des', 1)
	assert evn.num_events() == 1

	assert evn.insert_row('emi', 'des', 1)
	assert evn.num_events() == 1

	assert evn.insert_row('emi2', 'des', 1)
	assert evn.num_events() == 2

	assert events_describe_next_event(-1, '') == ''

	ev_1 = events_describe_next_event(evn.ev_id, '')

	assert ev_1 == 'emi\tdes\t1.00000\t1'

	cli = reels.Clients()

	assert events_describe_next_event(evn.ev_id, 'xxx') == ''

	ll = ev_1.split('\t')

	assert len(ll) == 4

	ev_2 = events_describe_next_event(evn.ev_id, ev_1)
	ev_x = events_describe_next_event(evn.ev_id, '\t'.join(ll))

	assert ev_2 == 'emi2\tdes\t1.00000\t2'
	assert ev_2 == ev_x

	ev_x = events_describe_next_event(evn.ev_id, '\t'.join(ll))

	assert ev_2 == ev_x

	ev_x = events_describe_next_event(evn.ev_id, '%s\t%s\t%s\t' % (cli.hash_client_id(ll[0]), ll[1], ll[2]))

	assert ev_2 == ev_x

	ev_x = events_describe_next_event(evn.ev_id, '%s\t%s\t%s\t' % (ll[0], cli.hash_client_id(ll[1]), ll[2]))

	assert ev_2 == ev_x

	ev_x = events_describe_next_event(evn.ev_id, '%s\t%s\t%s\t' % (cli.hash_client_id(ll[0]), cli.hash_client_id(ll[1]), ll[2]))

	assert ev_2 == ev_x

	assert events_describe_next_event(evn.ev_id, ev_x) == ''

	tc = 0
	for tp in evn.describe_events():
		e, d, w, c = tp

		if (c == 1):
			assert e == 'emi' and d == 'des' and w == 1
		else:
			assert e == 'emi2' and d == 'des' and w == 1 and c == 2

		tc += c

	assert tc == 3

	im = evn.save_as_binary_image()

	ev2 = reels.Events(binary_image = im)

	assert ev2.num_events() == 2

	pkk = pickle.dumps(evn)
	ev3 = pickle.loads(pkk)

	assert ev3.num_events() == 2

	assert evn.insert_row('emi2', 'des3', 1)
	assert evn.num_events() == 3

	assert evn.insert_row('emi2', 'des4', 1)
	assert evn.num_events() == 4

	assert evn.insert_row('emi2', 'des5', 1)
	assert evn.num_events() == 5

	assert evn.insert_row('emi2', 'des6', 1)
	assert evn.num_events() == 5

	st = evn.__str__()
	assert st == evn.__repr__()
	assert st.startswith('reels.Events')
	assert st.endswith(' 5 events')

	evn = reels.Events()
	assert evn.num_events() == 0

	assert evn.define_event('e', 'd', 1.0, 1)
	assert evn.define_event('ee', 'dd', 1.1, 2)
	assert evn.define_event('eee', 'ddd', 1.2, 3)
	assert evn.num_events() == 3

	tc = 0
	for tp in evn.describe_events():
		e, d, w, c = tp

		if c == 1:
			assert e == 'e' and d == 'd' and w == 1
		elif c == 2:
			assert e == 'ee' and d == 'dd' and w == 1.1
		else:
			assert e == 'eee' and d == 'ddd' and w == 1.2 and c == 3

		tc += c

	assert tc == 6

	assert not evn.load_from_binary_image(im)

	assert evn.num_events() == 0

	ev4 = reels.Events()

	assert not ev4.load_from_binary_image('hi')

	ev4.ev_id += 999

	assert ev4.save_as_binary_image() is None


def test_clients():
	cli = reels.Clients()

	st = cli.__str__()
	assert st == cli.__repr__()
	assert st.startswith('Empty reels.Clients')

	assert cli.add_client_id('cli1')
	assert cli.add_client_id('cli2')
	assert cli.add_client_id('cli3')
	assert cli.add_client_id('cli4')
	assert cli.add_client_id('cli5')

	ll = list(cli.client_hashes())

	st = cli.__str__()
	assert st == cli.__repr__()
	assert st.startswith('reels.Clients')
	assert st.endswith(' 5 clients')

	assert len(ll) == 5

	assert ll[0] == cli.hash_client_id('cli1')
	assert ll[4] == cli.hash_client_id('cli5')

	assert not cli.add_client_id('cli4')
	assert not cli.add_client_id('cli5')

	ll = list(cli.client_hashes())

	assert len(ll) == 5

	assert ll[3] == cli.hash_client_id('cli4')
	assert ll[4] == cli.hash_client_id('cli5')

	im = cli.save_as_binary_image()

	cl2 = reels.Clients(im)

	ll2 = list(cl2.client_hashes())

	assert ll2[3] == cl2.hash_client_id('cli4')
	assert ll2[4] == cl2.hash_client_id('cli5')

	pkk = pickle.dumps(cli)
	cl3 = pickle.loads(pkk)

	ll3 = list(cl3.client_hashes())

	assert ll3[3] == cl3.hash_client_id('cli4')
	assert ll3[4] == cl3.hash_client_id('cli5')

	assert not cl3.load_from_binary_image(im)

	assert cl3.num_clients() == 0

	cl4 = reels.Clients()

	assert not cl4.load_from_binary_image('hi')

	cl4.cl_id += 999

	assert cl4.save_as_binary_image() is None


def test_clips():
	evn = reels.Events()
	assert evn.define_event('em1', 'des', 1, 1)
	assert evn.define_event('em2', 'des', 1, 2)
	assert evn.define_event('em3', 'des', 1, 3)

	cli = reels.Clients()
	assert cli.add_client_id('cli1')
	assert cli.add_client_id('cli2')
	assert cli.add_client_id('cli3')
	assert cli.add_client_id('cli4')
	assert cli.add_client_id('cli5')

	clp = reels.Clips(cli, evn)
	assert clp.num_clips() == 0
	assert clp.num_events() == 0

	st = clp.__str__()
	assert st == clp.__repr__()
	assert st.startswith('reels.Clips')
	assert st.endswith(' 0 events')

	assert clp.scan_event('em1', 'des', 1, 'cli1', '2022-06-01 09:00:00')
	assert clp.scan_event('em2', 'des', 1, 'cli1', '2022-06-01 09:01:00')
	assert clp.scan_event('em1', 'des', 1, 'cli2', '2022-06-01 09:00:00')
	assert clp.scan_event('em1', 'des', 1, 'cli2', '2022-06-01 10:00:00')
	assert clp.scan_event('em3', 'des', 1, 'cli2', '2022-06-01 10:30:00')
	assert clp.scan_event('em2', 'des', 1, 'cli3', '2022-06-01 09:00:00')
	assert clp.scan_event('em3', 'des', 1, 'cli3', '2022-06-01 10:30:00')

	assert clp.num_clips() == 3
	assert clp.num_events() == 7

	st = clp.__str__()
	assert st == clp.__repr__()
	assert st.startswith('reels.Clips')
	assert st.endswith(' 7 events')

	assert clp.describe_clip('cl99') is None

	hh = cli.hash_client_id('cl99')

	assert clp.describe_clip(hh) is None

	ii = clp.describe_clip('cli1')

	assert ii == [1, 2]

	ii = clp.describe_clip('cli2')

	assert ii == [1, 1, 3]

	ii = clp.describe_clip('cli3')

	assert ii == [2, 3]

	hh = cli.hash_client_id('cli3')
	ii = clp.describe_clip(hh)

	assert ii == [2, 3]

	hh = cli.hash_client_id('cli2')
	ii = clp.describe_clip(hh)

	assert ii == [1, 1, 3]

	hh = cli.hash_client_id('cli1')
	ii = clp.describe_clip(hh)

	assert ii == [1, 2]

	ll1 = list(clp.clips_client_hashes())
	ll2 = ll2 = [cli.hash_client_id('cli%i' % i) for i in range(1, 4)]
	ll1.sort()
	ll2.sort()

	assert ll1 == ll2

	im = clp.save_as_binary_image()

	clp2 = reels.Clips(reels.Clients(), reels.Events(), binary_image = im)

	ll3 = list(clp2.clips_client_hashes())
	ll3.sort()

	assert ll1 == ll3

	pkk = pickle.dumps(clp)
	clp3 = pickle.loads(pkk)

	ll4 = list(clp3.clips_client_hashes())
	ll4.sort()

	assert ll1 == ll4

	clp = reels.Clips(cli, evn, time_format = '%Y%m%d%H%M')
	assert clp.scan_event('em1', 'des', 1, 'cli1', '202206010900')
	assert clp.scan_event('em2', 'des', 1, 'cli1', '202206010901')
	assert clp.scan_event('em1', 'des', 1, 'cli2', '202206010900')
	assert clp.scan_event('em1', 'des', 1, 'cli2', '202206011000')
	assert clp.scan_event('em3', 'des', 1, 'cli2', '202206011030')
	assert clp.scan_event('em2', 'des', 1, 'cli3', '202206010900')
	assert clp.scan_event('em3', 'des', 1, 'cli3', '202206011030')

	assert clp.num_clips() == 3
	assert clp.num_events() == 7

	assert not clp.load_from_binary_image(im)

	assert clp.num_clips() == 0
	assert clp.num_events() == 0

	clp4 = reels.Clips(reels.Clients(), reels.Events())

	assert not clp4.load_from_binary_image('hi')

	clp4.cp_id += 999

	assert clp4.save_as_binary_image() is None

	assert clp.test_sequence(-1, False) is None
	assert clp.test_sequence(-1, True) is None
	assert clp.test_sequence(500, False) is None
	assert clp.test_sequence(500, True) is None

	assert clp.test_sequence(0, False)[0:5] == [1, 17, 92, 227, 864]
	assert clp.test_sequence(499, True)[0:5] == [349, 1, 82, 1, 1]

	all_seq = set()

	for i in range(500):
		ll = clp.test_sequence(i, False)
		assert type(ll) == list
		assert len(ll) >= 1

		for i in ll:
			assert type(i) == int

		all_seq.add(str(ll))

	assert len(all_seq) == 500

	for i in range(500):
		ll = clp.test_sequence(i, True)
		assert type(ll) == list
		assert len(ll) >= 1

		for i in ll:
			assert type(i) == int

		all_seq.add(str(ll))

	assert len(all_seq) == 1000



def test_targets():
	evn = reels.Events()
	assert evn.define_event('e', 'd1', 1.0, 1)
	assert evn.define_event('e', 'd2', 1.0, 2)
	assert evn.define_event('e', 'd3', 1.0, 3)
	assert evn.define_event('e', 'd4', 1.0, 4)
	assert evn.define_event('e', 'd5', 1.0, 5)
	assert evn.define_event('e', 'd6', 1.0, 6)
	assert evn.define_event('e', 'd7', 1.0, 7)

	assert evn.num_events() == 7

	cli = reels.Clients()
	assert cli.add_client_id('cli1')
	assert cli.add_client_id('cli2')
	assert cli.add_client_id('cli3')
	assert cli.add_client_id('cli4')
	assert cli.add_client_id('cli5')
	assert cli.add_client_id('cli6')
	assert not cli.add_client_id('cli5')
	assert not cli.add_client_id('cli4')
	assert not cli.add_client_id('cli1')

	assert cli.num_clients() == 6

	clp = reels.Clips(cli, evn)
	assert clp.scan_event('e', 'd1', 1.0, 'cli1', '2022-06-01 00:00:00')
	assert clp.scan_event('e', 'd2', 1.0, 'cli1', '2022-06-01 01:00:00')
	assert clp.scan_event('e', 'd3', 1.0, 'cli1', '2022-06-01 02:00:00')
	assert clp.scan_event('e', 'd4', 1.0, 'cli1', '2022-06-01 03:00:00')
	assert clp.scan_event('e', 'd5', 1.0, 'cli1', '2022-06-01 04:00:00')
	assert clp.scan_event('e', 'd6', 1.0, 'cli1', '2022-06-01 05:00:00')
	assert clp.scan_event('e', 'd7', 1.0, 'cli1', '2022-06-01 06:00:00')

	assert clp.scan_event('e', 'd2', 1.0, 'cli2', '2022-06-01 00:00:00')
	assert clp.scan_event('e', 'd3', 1.0, 'cli2', '2022-06-01 04:00:00')
	assert clp.scan_event('e', 'd1', 1.0, 'cli2', '2022-06-01 08:00:00')

	assert clp.scan_event('e', 'd4', 1.0, 'cli3', '2022-06-01 05:00:00')
	assert clp.scan_event('e', 'd5', 1.0, 'cli3', '2022-06-01 06:00:00')
	assert clp.scan_event('e', 'd6', 1.0, 'cli3', '2022-06-01 07:00:00')
	assert clp.scan_event('e', 'd7', 1.0, 'cli3', '2022-06-01 10:00:00')

	assert clp.scan_event('e', 'd5', 1.0, 'cli4', '2022-06-01 00:00:00')
	assert clp.scan_event('e', 'd6', 1.0, 'cli4', '2022-06-01 03:00:00')
	assert clp.scan_event('e', 'd7', 1.0, 'cli4', '2022-06-01 06:00:00')

	assert clp.scan_event('e', 'd4', 1.0, 'cli5', '2022-06-01 07:00:00')
	assert clp.scan_event('e', 'd3', 1.0, 'cli5', '2022-06-01 08:00:00')
	assert clp.scan_event('e', 'd2', 1.0, 'cli5', '2022-06-01 09:00:00')
	assert clp.scan_event('e', 'd1', 1.0, 'cli5', '2022-06-01 11:00:00')

	assert clp.scan_event('e', 'd2', 1.0, 'cli6', '2022-06-01 00:00:00')
	assert clp.scan_event('e', 'd2', 1.0, 'cli6', '2022-06-01 03:00:00')
	assert clp.scan_event('e', 'd2', 1.0, 'cli6', '2022-06-01 06:00:00')

	assert not clp.scan_event('e', 'd2', 1.0, 'cli7', '2022-06-01 00:00:00')

	assert clp.num_clips() == 6
	assert clp.num_events() == 24

	trg = reels.Targets(clp, time_format = '%Y%m%d%H')
	assert trg.num_targets() == 0

	st = trg.__str__()
	assert st == trg.__repr__()

	l1, e1, l2, e2, l3 = tuple(st.split('\n'))
	assert e1 == '' and e2 == ''

	assert l1.startswith('reels.Targets')
	assert l1.endswith(' 6 clips')
	assert l2 == 'Has no targets defined.'
	assert l3 == 'Is not fitted.'

	assert trg.insert_target('cli1', '2022060118')
	assert trg.insert_target('cli3', '2022060119')

	assert trg.num_targets() == 2

	assert trg.insert_target('cli4', '2022060120')

	assert trg.num_targets() == 3

	assert trg.describe_tree_node(1) is None
	assert trg.tree_node_children(1) is None

	st = trg.__str__()
	assert st == trg.__repr__()

	l1, e1, l2, e2, l3 = tuple(st.split('\n'))
	assert e1 == '' and e2 == ''

	assert l1.startswith('reels.Targets')
	assert l1.endswith(' 6 clips')
	assert l2 == 'Has 3 targets.'
	assert l3 == 'Is not fitted.'

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(0)

	log = trg.describe_tree()

	assert log.startswith('tree.size() : 1\n\n')
	assert 'num_of_nodes_with_zero_visits : 1' in log
	assert 'num_of_no_targets_one_visit   : 0' in log
	assert 'num_of_has_target_one_visit   : 0' in log
	assert 'num_of_no_targets_more_visits : 0' in log
	assert 'num_of_has_target_more_visits : 0' in log
	assert 'num_of_no_targets_final_node  : 1' in log
	assert 'num_of_has_target_final_node  : 0' in log

	assert n_seen == 0 and n_target == 0 and sum_time_d == 0 and num_codes == 0

	assert trg.tree_node_children(0) is None

	assert trg.fit()

	st = trg.__str__()
	assert st == trg.__repr__()

	l1, e1, l2, e2, l3 = tuple(st.split('\n'))
	assert e1 == '' and e2 == ''

	assert l1.startswith('reels.Targets')
	assert l1.endswith(' 6 clips')
	assert l2 == 'Has 3 targets.'
	assert l3 == 'Is fitted with 6 clips.'

	clp.describe_clip('cli1')		# 1, 2, 3, 4, 5, 6, 7 (T)
	clp.describe_clip('cli2')		# 2, 3, 1
	clp.describe_clip('cli3')		# 4, 5, 6, 7 (T)
	clp.describe_clip('cli4')		# 5, 6, 7 (T)
	clp.describe_clip('cli5')		# 4, 3, 2, 1
	clp.describe_clip('cli6')		# 2, 2, 2

	assert clp.describe_clip('cli7') is None

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(0)

	assert n_seen == 6 and n_target == 3 and sum_time_d > 0 and num_codes == 3

	cc = trg.tree_node_children(0)
	cc.sort()
	assert cc == [1, 2, 7]

	i = trg.tree_node_idx(0, 7)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 3 and n_target == 3 and sum_time_d > 0 and num_codes == 1

	cc = trg.tree_node_children(i)
	assert cc == [6]

	i = trg.tree_node_idx(i, 6)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 3 and n_target == 3 and sum_time_d > 0 and num_codes == 1

	cc = trg.tree_node_children(i)
	assert cc == [5]

	i = trg.tree_node_idx(i, 5)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 3 and n_target == 3 and sum_time_d > 0 and num_codes == 1

	cc = trg.tree_node_children(i)
	assert cc == [4]

	i = trg.tree_node_idx(i, 4)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 2 and n_target == 2 and sum_time_d > 0 and num_codes == 1

	i = trg.tree_node_idx(i, 3)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 1 and sum_time_d > 0 and num_codes == 1

	i = trg.tree_node_idx(i, 2)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 1 and sum_time_d > 0 and num_codes == 1

	i = trg.tree_node_idx(i, 1)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 1 and sum_time_d > 0 and num_codes == 0

	cc = trg.tree_node_children(i)
	assert cc is None

	i = trg.tree_node_idx(0, 2)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 0 and sum_time_d == 0 and num_codes == 1

	cc = trg.tree_node_children(i)
	assert cc == [2]

	i = trg.tree_node_idx(i, 2)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 0 and sum_time_d == 0 and num_codes == 1

	i = trg.tree_node_idx(i, 2)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 0 and sum_time_d == 0 and num_codes == 0

	i = trg.tree_node_idx(0, 1)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 2 and n_target == 0 and sum_time_d == 0 and num_codes == 2

	cc = trg.tree_node_children(i)
	cc.sort()
	assert cc == [2, 3]

	i = trg.tree_node_idx(i, 3)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 0 and sum_time_d == 0 and num_codes == 1

	cc = trg.tree_node_children(i)
	assert cc == [2]

	i = trg.tree_node_idx(i, 2)

	assert i > 0

	n_seen, n_target, sum_time_d, num_codes = trg.describe_tree_node(i)

	assert n_seen == 1 and n_target == 0 and sum_time_d == 0 and num_codes == 0

	log = trg.describe_tree()

	assert log.startswith('tree.size() : 17\n\n')
	assert 'num_of_has_target_final_node  : 1' in log

	cc = trg.tree_node_children(i)
	assert cc is None

	t_cli = list(trg.predict_clients(cli))

	assert len(t_cli) == 6

	t_cl2 = list(trg.predict_clients(['cli1', 'cli2', 'cli3', 'cli4', 'cli5', 'cli6']))

	assert len(t_cl2) == 6

	for x, y in zip(t_cli, t_cl2):
		assert x == y

	t_clp = list(trg.predict_clips(clp))

	assert len(t_clp) == 6

	d_cli = dict(zip(cli.client_hashes(), t_cli))
	d_clp = dict(zip(clp.clips_client_hashes(), t_clp))

	assert d_cli == d_clp

	assert d_clp[cli.hash_client_id('cli1')] < d_clp[cli.hash_client_id('cli2')]
	assert d_clp[cli.hash_client_id('cli1')] < d_clp[cli.hash_client_id('cli5')]
	assert d_clp[cli.hash_client_id('cli1')] < d_clp[cli.hash_client_id('cli6')]

	assert d_clp[cli.hash_client_id('cli3')] < d_clp[cli.hash_client_id('cli2')]
	assert d_clp[cli.hash_client_id('cli3')] < d_clp[cli.hash_client_id('cli5')]
	assert d_clp[cli.hash_client_id('cli3')] < d_clp[cli.hash_client_id('cli6')]

	assert d_clp[cli.hash_client_id('cli4')] < d_clp[cli.hash_client_id('cli2')]
	assert d_clp[cli.hash_client_id('cli4')] < d_clp[cli.hash_client_id('cli5')]
	assert d_clp[cli.hash_client_id('cli4')] < d_clp[cli.hash_client_id('cli6')]

	im = trg.save_as_binary_image()

	trg2 = reels.Targets(reels.Clips(reels.Clients(), reels.Events()), binary_image = im)

	d_cli2 = dict(zip(cli.client_hashes(), trg2.predict_clients(cli)))
	d_clp2 = dict(zip(clp.clips_client_hashes(), trg2.predict_clips(clp)))

	assert d_cli == d_cli2
	assert d_cli == d_clp2

	pkk = pickle.dumps(trg)
	trg3 = pickle.loads(pkk)

	d_cli3 = dict(zip(cli.client_hashes(), trg3.predict_clients(cli)))
	d_clp3 = dict(zip(clp.clips_client_hashes(), trg3.predict_clips(clp)))

	assert d_cli == d_cli3
	assert d_cli == d_clp3

	assert not trg3.load_from_binary_image(im)

	trg4 = reels.Targets(reels.Clips(reels.Clients(), reels.Events()))

	assert not trg4.load_from_binary_image('hi')

	trg4.tr_id += 999

	assert trg4.save_as_binary_image() is None


def test_event_optimizer():
	evn = reels.Events(max_num_events = 10000)
	cli = reels.Clients()
	clp = reels.Clips(cli, evn)

	for i in range(500):
		nt = clp.test_sequence(i, False)
		for code in nt:
			assert evn.insert_row(str(code), '.', 1)

		nt = clp.test_sequence(i, True)
		for code in nt:
			assert evn.insert_row(str(code), '.', 1)

	clp = reels.Clips(cli, evn)
	trg = reels.Targets(clp)

	for i in range(500):
		cl_id = 'cl_%04i' % (i + 1)

		nt = clp.test_sequence(i, False)

		ot = datetime.datetime.strptime('2023-01-01 10:00:00', '%Y-%m-%d %H:%M:%S')

		for j, code in enumerate(nt):
			t = ot + datetime.timedelta(seconds = j)
			assert clp.scan_event(str(code), '.', 1, cl_id, t.strftime('%Y-%m-%d %H:%M:%S'))

	for i in range(500):
		cl_id = 'cl_%04i' % (i + 501)

		nt = clp.test_sequence(i, True)

		ot = datetime.datetime.strptime('2023-01-01 10:00:00', '%Y-%m-%d %H:%M:%S')

		for j, code in enumerate(nt):
			t = ot + datetime.timedelta(seconds = j)
			assert clp.scan_event(str(code), '.', 1, cl_id, t.strftime('%Y-%m-%d %H:%M:%S'))

		t = t + datetime.timedelta(seconds = 1)
		assert trg.insert_target(cl_id, t.strftime('%Y-%m-%d %H:%M:%S'))

	assert clp.num_clips() == 1000
	assert trg.num_targets() == 500

	ori_events = evn.copy()

	l_ori = list(ori_events.describe_events())
	l_evn = list(evn.describe_events())

	assert len(l_ori) == len(l_evn)

	for t_ori, t_evn in zip(l_ori, l_evn):
		assert t_ori == t_evn

	success, dictionary, top_codes, log = evn.optimize_events(clp, trg, 1, 10)

	assert success
	assert type(dictionary) == dict
	assert type(top_codes) == str
	assert type(log) == str

	new_events = ori_events.copy(dictionary)

	l_new = list(new_events.describe_events())
	l_opt = list(evn.describe_events())

	assert len(l_new) == len(l_opt)

	for t_new, t_opt in zip(l_new, l_opt):
		assert t_new == t_opt

	# import pandas as pd
	# from io import StringIO

	# df = pd.read_csv(StringIO(top_codes), sep = '\t')

	# print(df)

	bak_id = clp.cp_id
	clp.cp_id = -999
	success, dictionary, top_codes, log = evn.optimize_events(clp, trg, 1, 10)
	clp.cp_id = bak_id

	assert not success
	assert dictionary is None
	assert top_codes is None
	assert log.startswith('ERROR')


test_events()
test_clients()
test_clips()
test_targets()
test_event_optimizer()
