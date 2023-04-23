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

from . import new_clients, new_clips, new_events
from . import destroy_clips
from . import clips_set_time_format
from . import clips_scan_event
from . import clips_hash_by_previous
from . import clips_load_block
from . import clips_save
from . import clips_describe_clip
from . import clips_num_clips
from . import clips_num_events
from . import clips_test_sequence

from . import size_binary_image_iterator
from . import next_binary_image_iterator
from . import destroy_binary_image_iterator

from . import Events
from . import Clients


class ClipsHashes:
    """Iterator over the hashes of the Clients in a Clips container."""

    def __init__(self, cp_id):
        self.cp_id = cp_id
        self.prev_hash = ''

    def __iter__(self):
        return self

    def __next__(self):
        self.prev_hash = clips_hash_by_previous(self.cp_id, self.prev_hash)

        if self.prev_hash == '':
            raise StopIteration

        return self.prev_hash


class Clips:

    """Interface to the c++ container object to hold clips.

    The purpose of this object is to be filled (via successive scan_event()
    calls) this builds and stores the clips and passed to a Targets object. The
    content can just be serialized as a pickle to make it pythonic, checked
    for size and iterated by calling clips_client_hashes().

    Args:
        clients:      A possibly initialized Clients object to restrict building
                      the clips to a specific set of clients. Pass an empty Clients
                      object for no restriction (all clients).
        events:       An Events object that defines what events have to be considered
                      to build the clips.
        time_format:  An optional definition of the time format that will affect how
                      time is parsed by scan_event() the default is "%Y-%m-%d %H:%M:%S"
                      https://www.gnu.org/software/libc/manual/html_node/Formatting-Calendar-Time.html
        binary_image: An optional binary image (returned by save_as_binary_image())
                      to initialize the object with data copied from another Clips
                      object. You have to pass empty clients and events to use this.
    """

    def __init__(
        self, clients: Clients, events: Events, time_format=None, binary_image=None
    ):
        self.cp_id = new_clips(clients.cl_id, events.ev_id)

        if time_format is not None:
            clips_set_time_format(self.cp_id, time_format)

        if binary_image is not None:
            self.load_from_binary_image(binary_image)

    def __del__(self):
        destroy_clips(self.cp_id)

    def __str__(self):
        return 'reels.Clips object with %i clips totalling %i events' % (self.num_clips(), self.num_events())

    def __repr__(self):
        return self.__str__()

    def __getstate__(self):
        """Used by pickle.dump() (See https://docs.python.org/3/library/pickle.html)"""
        return self.save_as_binary_image()

    def __setstate__(self, state):
        """Used by pickle.load() (See https://docs.python.org/3/library/pickle.html)"""
        self.cp_id = new_clips(new_clients(), new_events())
        self.load_from_binary_image(state)

    def scan_event(self, emitter, description, weight, client, time):
        """Process a row from a transaction file, to add the event to the client's timeline in a Clips object.

        Args:
            emitter:     The "emitter". A string representing "owner of event".
            description: The "description". A string representing "the event".
            weight:      The "weight". A double representing a weight of the event.
            client:      The "client". A string representing "the actor".
            time:        The "time". A timestamp of the event as a string. (The format
                         is given via the time_format argument to the constructor.)

        Returns:
            True on insertion. False usually just means, the event is not in events
            or the client is not in clients. It may be a time parsing error too.
        """
        return clips_scan_event(self.cp_id, emitter, description, weight, client, time)

    def clips_client_hashes(self):
        """Return an iterator to iterate over all the hashed client ids.

        Returns:
            An iterator (a ClipsHashes object)
        """
        return ClipsHashes(self.cp_id)

    def num_clips(self):
        """Return the number of clips in the object.

        Returns:
            The number of clips stored in the object. Clips are indexed by unique
            client hash.
        """
        return clips_num_clips(self.cp_id)

    def num_events(self):
        """Return the number of events counting all the clips stored by the object.

        Returns:
            The number of events stored in the object.
        """
        return clips_num_events(self.cp_id)

    def describe_clip(self, client):
        """Return a list ot the codes in chronological order for a given client.

        Args:
            client: Either a client identifier or the hash of a client identifier returned by Clients.hash_client_id().

        Returns:
            A list of integer codes on success or None on failure.
        """
        s = clips_describe_clip(self.cp_id, client)

        if len(s) != 0:
            return [int(i) for i in s.split('\t')]

    def save_as_binary_image(self):
        """Saves the state of the c++ Clips object as a Python
            list of strings referred to a binary_image.

        Returns:
            The binary_image containing the state of the Clips. There is
            not much you can do with it except serializing it as a Python
            (e.g., pickle) object and loading it into another Clips object.
            Pass it to the constructor to create an initialized object,
        """
        bi_idx = clips_save(self.cp_id)
        if bi_idx == 0:
            return None

        bi = []
        bi_size = size_binary_image_iterator(bi_idx)
        for t in range(bi_size):
            bi.append(next_binary_image_iterator(bi_idx))

        destroy_binary_image_iterator(bi_idx)

        return bi

    def load_from_binary_image(self, binary_image):
        """Load the state of the c++ Clips object from a binary_image
            returned by a previous save_as_binary_image() call.

        Args:
            binary_image: A list of strings returned by save_as_binary_image()

        Returns:
            True on success, destroys, initializes and returns false on failure.
        """
        failed = False

        for binary_image_block in binary_image:
            if not clips_load_block(self.cp_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not clips_load_block(self.cp_id, "")

        if failed:
            destroy_clips(self.cp_id)
            self.cp_id = new_clips(new_clients(), new_events())
            return False

        return True

    def test_sequence(self, seq_num, target):
        """Generates a constant sequence of codes for testing the Event Optimizer.
            This returns one of the 500 non target sequences or one of the 500 target sequences.

        Args:
            seq_num: The sequence id (in range 0.499).
            target:  True for one of the target sequences, false for non target.

        Returns:
            A sequence of integer codes list or None if seq_num is out of range.
        """

        s = clips_test_sequence(seq_num, target)

        if s == '':
            return None

        return [int(i) for i in s.split(',')]
