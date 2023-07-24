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

from . import new_clients, new_clips, new_events, new_targets
from . import destroy_targets
from . import clips_num_clips
from . import targets_set_time_format
from . import targets_insert_target
from . import targets_fit
from . import targets_predict_clients
from . import targets_predict_clips
from . import targets_load_block
from . import targets_save
from . import targets_num_targets
from . import targets_tree_node_idx
from . import targets_tree_node_children
from . import targets_describe_tree_node
from . import targets_describe_tree

from . import size_result_iterator
from . import next_result_iterator
from . import destroy_result_iterator

from . import size_binary_image_iterator
from . import next_binary_image_iterator
from . import destroy_binary_image_iterator

from . import Clients
from . import Clips


class Result:
    """Container holding the results of time predictions.
    It behaves, basically, like an iterator.
    """

    def __init__(self, iter_id):
        self.iter_id = iter_id

    def __del__(self):
        destroy_result_iterator(self.iter_id)

    def __iter__(self):
        return self

    def __next__(self):
        if size_result_iterator(self.iter_id) > 0:
            return next_result_iterator(self.iter_id)
        else:
            raise StopIteration


class Targets:

    """Interface to the c++ container object to hold clips.

    This object fits the model for a set of targets defined via insert_target().
    Once .

    Args:
        clips:        A Clips object containing the clips defining the events for a set of clients.
        time_format:  An optional definition of the time format that will affect how
                      time is parsed by insert_target() the default is "%Y-%m-%d %H:%M:%S"
                      https://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html
        binary_image: An optional binary image (returned by save_as_binary_image())
                      to initialize the object with data copied from another Clips
                      object. You have to pass empty clips to use this.
    """

    def __init__(self, clips: Clips, time_format=None, binary_image=None):
        self.tr_id = new_targets(clips.cp_id)
        self.cp_id = clips.cp_id

        if time_format is not None:
            targets_set_time_format(self.tr_id, time_format)

        if binary_image is not None:
            self.load_from_binary_image(binary_image)

    def __del__(self):
        destroy_targets(self.tr_id)

    def __str__(self):
        n_clips         = clips_num_clips(self.cp_id)
        n_targets       = self.num_targets()
        n_seen, _, _, _ = self.describe_tree_node(0)

        s_targets = 'Has %i targets.' % n_targets if n_targets != 0 else 'Has no targets defined.'
        s_fit     = 'Is fitted with %i clips.' % n_seen if n_seen != 0 else 'Is not fitted.'

        return 'reels.Targets object with %i clips\n\n%s\n\n%s' % (n_clips, s_targets, s_fit)

    def __repr__(self):
        return self.__str__()

    def __getstate__(self):
        """Used by pickle.dump() (See https://docs.python.org/3/library/pickle.html)"""
        return self.save_as_binary_image()

    def __setstate__(self, state):
        """Used by pickle.load() (See https://docs.python.org/3/library/pickle.html)"""
        self.tr_id = new_targets(new_clips(new_clients(), new_events()))
        self.load_from_binary_image(state)

    def insert_target(self, client, time):
        """Define the targets before calling fit() by calling this method once for each client.

        Args:
            client: The "client". A string representing "the actor".
            time:   The "time". A timestamp of the target event as a string. (The format
                    is given via the time_format argument to the constructor.)

        Returns:
            True on new clients. False if the client is already in the object or the time
            is in the wrong format.
        """
        return targets_insert_target(self.tr_id, client, time)

    def fit(self, x_form="log", agg="minimax", p=0.5, depth=8, as_states=False):
        """Fit the prediction model in the object stored after calling insert_target() multiple times.

            Fit can only be called once in the life of a Targets object and predict_*() cannot be called before fit().

        Args:
            x_form:    A possible transformation of the times. (Currently "log" or "linear".)
            agg:       The mechanism used for the aggregation. (Currently "minimax", "mean" or "longest".)
            p:         The width of the confidence interval for the binomial proportion used to calculate the lower bound.
                       (E.g., p = 0.5 will estimate a lower bound of a symmetric CI with coverage of 0.5.)
            depth:     The maximum depth of the tree (maximum sequence length learned).
            as_states: Treat events as states by removing repeated ones from the ClipMap keeping the time of the first instance only.
                       When used, the ClipMap passed to the constructor by reference will be converted to states as a side effect.

        Returns:
            True on success. Error if already fitted, wrong arguments or the id is not found.
        """
        return targets_fit(self.tr_id, x_form, agg, p, depth, as_states)

    def predict_clients(self, clients: Clients):
        """Predict time to target for all the clients in clients whose clips have been used to fit the model.

            predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

            The model can only predict clips, so this will only work for those clients whose clips are stored in the object,
            unlike predict_clips() which is completely general.

        Args:
            clients: The Clients object containing the ids of the clients you want to predict or a list of client ids.

        Returns:
            An iterator object containing the results. (Empty on error.)
        """
        if type(clients) == list:
            cli = Clients()
            for c in clients:
                cli.add_client_id(c)
            return Result(targets_predict_clients(self.tr_id, cli.cl_id))

        return Result(targets_predict_clients(self.tr_id, clients.cl_id))

    def predict_clips(self, clips: Clips):
        """Predict time to target for a set of clients whose clips are given in a Clips object.

            predict() cannot be called before fit() and can be called any number of times in all overloaded forms after that.

        Args:
            clips: The clips you want to predict.

        Returns:
            An iterator object containing the results. (Empty on error.)
        """
        return Result(targets_predict_clips(self.tr_id, clips.cp_id))

    def num_targets(self):
        """Number of target point that have been given to the object.

            This mostly for debugging, verifying that the number of successful insert_target() is as expected.

        Returns:
            The number of target points stored in the internal target object.
        """
        return targets_num_targets(self.tr_id)

    def tree_node_idx(self, parent_idx, code):
        """Returns the index of a tree node by parent node and code.

        Args:
            parent_idx: The index of the parent node which is either 0 for root or returned from a previous tree_node_idx() call.
            code:       The code that leads in the tree from the parent node to the child node.

        Returns:
            On success, i.e, if both the parent index exists and contains the code, it will return the index of the child (-1 otherwise).
        """
        return targets_tree_node_idx(self.tr_id, parent_idx, code)

    def tree_node_children(self, idx):
        """Return a list ot the codes of the children of a node in the tree by index.

        Args:
             idx: The zero-based index identifying the node (0 for root or a value returned by tree_node_idx()).

        Returns:
            A list of integer codes on success or None on failure.
        """
        s = targets_tree_node_children(self.tr_id, idx)

        if len(s) != 0:
            return [int(i) for i in s.split('\t')]

    def describe_tree_node(self, idx):
        """Return a tuple (n_seen, n_target, sum_time_d, num_codes) describing a node in the tree by index.

        Args:
            idx: The zero-based index identifying the node (0 for root or a value returned by tree_node_idx()).

        Returns:
            A tuple (n_seen, n_target, sum_time_d, num_codes) on success or None on failure.
        """
        s = targets_describe_tree_node(self.tr_id, idx)

        if len(s) != 0:
            ns, nt, sd, nc = tuple(s.split('\t'))
            return int(ns), int(nt), float(sd), int(nc)

    def describe_tree(self):
        """Describes some statistics of a fitted tree inside a Targets object.

        Returns:
            On success, it will return a descriptive text with node count numbers for different node content.
        """

        return targets_describe_tree(self.tr_id)

    def save_as_binary_image(self):
        """Saves the state of the c++ Targets object as a Python
            list of strings referred to a binary_image.

        Returns:
            The binary_image containing the state of the Targets. There is
            not much you can do with it except serializing it as a Python
            (e.g., pickle) object and loading it into another Targets object.
            Pass it to the constructor to create an initialized object,
        """
        bi_idx = targets_save(self.tr_id)
        if bi_idx == 0:
            return None

        bi = []
        bi_size = size_binary_image_iterator(bi_idx)
        for t in range(bi_size):
            bi.append(next_binary_image_iterator(bi_idx))

        destroy_binary_image_iterator(bi_idx)

        return bi

    def load_from_binary_image(self, binary_image):
        """Load the state of the c++ Targets object from a binary_image
            returned by a previous save_as_binary_image() call.

        Args:
            binary_image: A list of strings returned by save_as_binary_image()

        Returns:
            True on success, destroys, initializes and returns false on failure.
        """
        failed = False

        for binary_image_block in binary_image:
            if not targets_load_block(self.tr_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not targets_load_block(self.tr_id, "")

        if failed:
            destroy_targets(self.tr_id)
            self.tr_id = new_targets(new_clips(new_clients(), new_events()))
            return False

        return True
