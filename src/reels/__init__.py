# This file was automatically generated by SWIG (http://www.swig.org).
# Version 4.0.2
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

from sys import version_info as _swig_python_version_info
if _swig_python_version_info < (2, 7, 0):
    raise RuntimeError("Python 2.7 or later required")

# Import the low-level C/C++ module
if __package__ or "." in __name__:
    from . import _py_reels
else:
    import _py_reels

try:
    import builtins as __builtin__
except ImportError:
    import __builtin__

def _swig_repr(self):
    try:
        strthis = "proxy of " + self.this.__repr__()
    except __builtin__.Exception:
        strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)


def _swig_setattr_nondynamic_instance_variable(set):
    def set_instance_attr(self, name, value):
        if name == "thisown":
            self.this.own(value)
        elif name == "this":
            set(self, name, value)
        elif hasattr(self, name) and isinstance(getattr(type(self), name), property):
            set(self, name, value)
        else:
            raise AttributeError("You cannot add instance attributes to %s" % self)
    return set_instance_attr


def _swig_setattr_nondynamic_class_variable(set):
    def set_class_attr(cls, name, value):
        if hasattr(cls, name) and not isinstance(getattr(cls, name), property):
            set(cls, name, value)
        else:
            raise AttributeError("You cannot add class attributes to %s" % cls)
    return set_class_attr


def _swig_add_metaclass(metaclass):
    """Class decorator for adding a metaclass to a SWIG wrapped class - a slimmed down version of six.add_metaclass"""
    def wrapper(cls):
        return metaclass(cls.__name__, cls.__bases__, cls.__dict__.copy())
    return wrapper


class _SwigNonDynamicMeta(type):
    """Meta class to enforce nondynamic attributes (no new attributes) for a class"""
    __setattr__ = _swig_setattr_nondynamic_class_variable(type.__setattr__)



def new_events():
    return _py_reels.new_events()

def destroy_events(id):
    return _py_reels.destroy_events(id)

def events_insert_row(id, p_e, p_d, w):
    return _py_reels.events_insert_row(id, p_e, p_d, w)

def events_define_event(id, p_e, p_d, w, code):
    return _py_reels.events_define_event(id, p_e, p_d, w, code)

def events_optimize_events(id, id_clips, id_targets, num_steps, codes_per_step, threshold, force_include, force_exclude, x_form, agg, p, depth, as_states, exponential_decay, lower_bound_p, log_lift):
    return _py_reels.events_optimize_events(id, id_clips, id_targets, num_steps, codes_per_step, threshold, force_include, force_exclude, x_form, agg, p, depth, as_states, exponential_decay, lower_bound_p, log_lift)

def events_load_block(id, p_block):
    return _py_reels.events_load_block(id, p_block)

def events_save(id):
    return _py_reels.events_save(id)

def events_describe_next_event(id, prev_event):
    return _py_reels.events_describe_next_event(id, prev_event)

def events_set_max_num_events(id, max_events):
    return _py_reels.events_set_max_num_events(id, max_events)

def events_set_store_strings(id, store):
    return _py_reels.events_set_store_strings(id, store)

def events_num_events(id):
    return _py_reels.events_num_events(id)

def new_clients():
    return _py_reels.new_clients()

def destroy_clients(id):
    return _py_reels.destroy_clients(id)

def clients_hash_client_id(id, p_cli):
    return _py_reels.clients_hash_client_id(id, p_cli)

def clients_add_client_id(id, p_cli):
    return _py_reels.clients_add_client_id(id, p_cli)

def clients_hash_by_index(id, idx):
    return _py_reels.clients_hash_by_index(id, idx)

def clients_num_clients(id):
    return _py_reels.clients_num_clients(id)

def clients_load_block(id, p_block):
    return _py_reels.clients_load_block(id, p_block)

def clients_save(id):
    return _py_reels.clients_save(id)

def new_clips(id_clients, id_events):
    return _py_reels.new_clips(id_clients, id_events)

def destroy_clips(id):
    return _py_reels.destroy_clips(id)

def clips_set_time_format(id, fmt):
    return _py_reels.clips_set_time_format(id, fmt)

def clips_scan_event(id, p_e, p_d, w, p_c, p_t):
    return _py_reels.clips_scan_event(id, p_e, p_d, w, p_c, p_t)

def clips_hash_by_previous(id, prev_hash):
    return _py_reels.clips_hash_by_previous(id, prev_hash)

def clips_load_block(id, p_block):
    return _py_reels.clips_load_block(id, p_block)

def clips_save(id):
    return _py_reels.clips_save(id)

def clips_describe_clip(id, client_id):
    return _py_reels.clips_describe_clip(id, client_id)

def clips_num_clips(id):
    return _py_reels.clips_num_clips(id)

def clips_num_events(id):
    return _py_reels.clips_num_events(id)

def clips_test_sequence(seq_num, target):
    return _py_reels.clips_test_sequence(seq_num, target)

def new_targets(id_clips):
    return _py_reels.new_targets(id_clips)

def destroy_targets(id):
    return _py_reels.destroy_targets(id)

def targets_set_time_format(id, fmt):
    return _py_reels.targets_set_time_format(id, fmt)

def targets_insert_target(id, p_c, p_t):
    return _py_reels.targets_insert_target(id, p_c, p_t)

def targets_fit(id, x_form, agg, p, depth, as_states):
    return _py_reels.targets_fit(id, x_form, agg, p, depth, as_states)

def targets_predict_clients(id, id_clients):
    return _py_reels.targets_predict_clients(id, id_clients)

def targets_predict_clips(id, id_clips):
    return _py_reels.targets_predict_clips(id, id_clips)

def targets_load_block(id, p_block):
    return _py_reels.targets_load_block(id, p_block)

def targets_save(id):
    return _py_reels.targets_save(id)

def targets_num_targets(id):
    return _py_reels.targets_num_targets(id)

def targets_tree_node_idx(id, parent_idx, code):
    return _py_reels.targets_tree_node_idx(id, parent_idx, code)

def targets_tree_node_children(id, idx):
    return _py_reels.targets_tree_node_children(id, idx)

def targets_describe_tree_node(id, idx):
    return _py_reels.targets_describe_tree_node(id, idx)

def targets_describe_tree(id):
    return _py_reels.targets_describe_tree(id)

def size_result_iterator(iter_id):
    return _py_reels.size_result_iterator(iter_id)

def next_result_iterator(iter_id):
    return _py_reels.next_result_iterator(iter_id)

def destroy_result_iterator(iter_id):
    return _py_reels.destroy_result_iterator(iter_id)

def size_binary_image_iterator(image_id):
    return _py_reels.size_binary_image_iterator(image_id)

def next_binary_image_iterator(image_id):
    return _py_reels.next_binary_image_iterator(image_id)

def destroy_binary_image_iterator(image_id):
    return _py_reels.destroy_binary_image_iterator(image_id)


# The source version file is <proj>/src/version.py, anything else is auto generated.
__version__ = '1.4.5'

from reels.Clients import Clients
from reels.Events import Events
from reels.Clips import Clips
from reels.Targets import Targets
from reels.Intake import Intake
