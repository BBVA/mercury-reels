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

from . import new_events
from . import destroy_events
from . import events_insert_row
from . import events_define_event
from . import events_optimize_events
from . import events_load_block
from . import events_save
from . import events_describe_next_event
from . import events_set_max_num_events
from . import events_set_store_strings
from . import events_num_events

from . import size_binary_image_iterator
from . import next_binary_image_iterator
from . import destroy_binary_image_iterator


class EventTuples:
    """Iterator of (emitter, description, weight, code) tuples."""

    def __init__(self, ev_id):
        self.ev_id = ev_id
        self.prev_tuple = ''

    def __iter__(self):
        return self

    def __next__(self):
        self.prev_tuple = events_describe_next_event(self.ev_id, self.prev_tuple)

        if self.prev_tuple == '':
            raise StopIteration

        e, d, w, c = tuple(self.prev_tuple.split('\t'))

        return e, d, float(w), int(c)


class Events:

    """Interface to the c++ container object to hold events.

    The purpose of this object is to be filled (either via successive
    insert_row() or define_event() calls) and passed to a Clips (and
    indirectly to Targets objects). The content can just be serialized as
    a pickle to make it pythonic or checked for size. No other introspection
    methods exist.

    Args:
        max_num_events: The maximum number of events to limit the discovery via
                        insert_row() to the max_num_events more frequent/recent.
        binary_image:   An optional binary image (returned by save_as_binary_image())
                        to initialize the object with data copied from another Events
                        object.
    """

    def __init__(self, max_num_events=1000, binary_image=None):
        self.ev_id = new_events()

        events_set_max_num_events(self.ev_id, max_num_events)
        events_set_store_strings(self.ev_id, True)

        self.max_num_events = max_num_events

        if binary_image is not None:
            self.load_from_binary_image(binary_image)

    def __del__(self):
        destroy_events(self.ev_id)

    def __str__(self):
        return 'reels.Events object with %i events' % self.num_events()

    def __repr__(self):
        return self.__str__()

    def __getstate__(self):
        """Used by pickle.dump() (See https://docs.python.org/3/library/pickle.html)"""
        return self.save_as_binary_image()

    def __setstate__(self, state):
        """Used by pickle.load() (See https://docs.python.org/3/library/pickle.html)"""
        self.ev_id = new_events()
        self.load_from_binary_image(state)

    def insert_row(self, emitter, description, weight):
        """Process a row from a transaction file.

        **Caveat**: insert_row() and define_event() should not be mixed. The former is for
        event discovery and the latter for explicit definition. A set of events is build
        either one way or the other.

        Args:
            emitter:     The "emitter". A C/Python string representing "owner of event".
            description: The "description". A C/Python string representing "the event".
            weight:      The "weight". A double representing a weight of the event.

        Returns:
            True on success.
        """
        return events_insert_row(self.ev_id, emitter, description, weight)

    def define_event(self, emitter, description, weight, code):
        """Define events explicitly.

        **Caveat**: insert_row() and define_event() should not be mixed. The former is for
        event discovery and the latter for explicit definition. A set of events is build
        either one way or the other.

        Args:
            emitter:     The "emitter". A C/Python string representing "owner of event".
            description: The "description". A C/Python string representing "the event".
            weight:      The "weight". A double representing a weight of the event.
            code:        A unique code number identifying the event.

        Returns:
            True on success.
        """
        return events_define_event(self.ev_id, emitter, description, weight, code)

    def num_events(self):
        """Return the number of events in the object.

        Returns:
            The number of events stored in the object.
        """
        return events_num_events(self.ev_id)

    def describe_events(self):
        """Return an iterator of (emitter, description, weight, code) tuples describing all events.

        Returns:
            An iterator of (emitter, description, weight, code) tuple on success or None on failure.
        """
        return EventTuples(self.ev_id)

    def save_as_binary_image(self):
        """Saves the state of the c++ Events object as a Python
            list of strings referred to a binary_image.

        Returns:
            The binary_image containing the state of the Events. There is
            not much you can do with it except serializing it as a Python
            (e.g., pickle) object and loading it into another Events object.
            Pass it to the constructor to create an initialized object,
        """
        bi_idx = events_save(self.ev_id)
        if bi_idx == 0:
            return None

        bi = []
        bi_size = size_binary_image_iterator(bi_idx)
        for t in range(bi_size):
            bi.append(next_binary_image_iterator(bi_idx))

        destroy_binary_image_iterator(bi_idx)

        return bi

    def load_from_binary_image(self, binary_image):
        """Load the state of the c++ Events object from a binary_image
            returned by a previous save_as_binary_image() call.

        Args:
            binary_image: A list of strings returned by save_as_binary_image()

        Returns:
            True on success, destroys, initializes and returns false on failure.
        """
        failed = False

        for binary_image_block in binary_image:
            if not events_load_block(self.ev_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not events_load_block(self.ev_id, "")

        if failed:
            destroy_events(self.ev_id)
            self.ev_id = new_events()
            return False

        return True

    def optimize_events(self, clips, targets, num_steps=10, codes_per_step=5, threshold=0.0001, force_include='',
                        force_exclude='', x_form='linear', agg='longest', p=0.5, depth=1000, as_states=True,
                        exp_decay=0.00693, lower_bound_p=0.95, log_lift=True):
        """Events optimizer.

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

        Args:
            clips:          The id of a clips object with the same codes and clips for a set of clients whose prediction we optimize.
            targets:        The id of a Targets object whose internal TargetMap defines the targets. (Internally a new Targets object
                            will be used to make the predictions we want to optimize.)
            num_steps:      The number of steps to iterate. The method will stop early if no codes are found at a step.
            codes_per_step: The number of codes to be tried from the top of the priority list at each step.
            threshold:      A minimum threshold, below which a score change is not considered improvement.
            force_include:  An optional pointer to a set of codes that must be included before starting.
            force_exclude:  An optional pointer to a set of codes that will excluded and set to the base code.
            x_form:         The x_form argument to fit the internal Targets object prediction model.
            agg:            The agg argument to fit the internal Targets object prediction model.
            p:              The p argument to fit the internal Targets object prediction model.
            depth:          The depth argument to fit the internal Targets object prediction model.
            as_states:      The as_states argument to fit the internal Targets object prediction model.
            exp_decay:      Exponential Decay Factor applied to the internal score in terms of depth. That score selects what
                            codes enter the model. The decay is applied to the average tree depth. 0 is no decay, default
                            value = 0.00693 decays to 0.5 in 100 steps.
            lower_bound_p:  Another p for lower bound, but applied to the scoring process rather than the model.
            log_lift:       A boolean to set if lift (= LB(included)/LB(after inclusion)) is log() transformed or not.

        Returns:
            A tuple (success, dictionary, top_codes, log)
        """

        ret = events_optimize_events(self.ev_id, clips.cp_id, targets.tr_id, num_steps, codes_per_step, threshold, force_include,
                                     force_exclude, x_form, agg, p, depth, as_states, exp_decay, lower_bound_p, log_lift)

        success = ret.startswith('SUCCESS')

        if not success:
            return success, None, None, ret

        txt = ret.split('\n')

        i = next(i for i, s in enumerate(txt) if s.startswith('  Final dictionary ='))
        log = '\n'.join(txt[1:i])
        dictionary = eval(txt[i][20:])

        i = next(i for i, s in enumerate(txt) if s.startswith('code_performance'))
        top_codes = '\n'.join(txt[(i + 1):])

        return success, dictionary, top_codes, log

    def copy(self, dictionary = None):
        """
        Creates a new Events object that is either a copy of the current one or a copy with the codes transformed by a dictionary.

        The typical use of this method is in combination with optimize_events() typically to create a copy of an Events object
        and optimize the copy rather than the original object to leave the latter unmodified. Also, after optimize_events()
        the dictionary returned completely defines the optimization and can be used to transform the original object by applying it
        during the copy.

        NOTE: The copy is identical in terms of recorded events and their codes, but does not have usage frequency statistics since
        it is created via define_event() calls, not by insert_row() over a complete dataset. It is intended to operate in combination
        with either optimize_events() or a Targets object, not to continue populating it with events via insert_row().

        Args:
            dictionary: A dictionary to be applied during the copy. The dictionary must be returned by a previous optimize_events()
                        of an identical object in order to have the same codes defined. Otherwise, the codes not present in the dictionary
                        will not be translated into events.

        Returns:
            A new Events object
        """

        ret = Events(max_num_events = self.max_num_events)

        if dictionary is not None:
            for emitter, description, weight, code in self.describe_events():
                if code in dictionary:
                    code = dictionary[code]
                    ret.define_event(emitter, description, weight, code)

        else:
            for emitter, description, weight, code in self.describe_events():
                ret.define_event(emitter, description, weight, code)

        return ret
