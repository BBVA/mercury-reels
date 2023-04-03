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

from . import new_clients
from . import destroy_clients
from . import clients_hash_client_id
from . import clients_add_client_id
from . import clients_hash_by_index
from . import clients_num_clients
from . import clients_load_block
from . import clients_save

from . import size_binary_image_iterator
from . import next_binary_image_iterator
from . import destroy_binary_image_iterator


class ClientsHashes:
    """Iterator over the hashes of a Clients container."""

    def __init__(self, cl_id, size):
        self.cl_id = cl_id
        self.size = size
        self.idx = 0

    def __iter__(self):
        return self

    def __next__(self):
        if self.idx < self.size:
            hash = clients_hash_by_index(self.cl_id, self.idx)
            self.idx += 1
            return hash
        else:
            raise StopIteration


class Clients:

    """Interface to the c++ container object to hold clients.

    The purpose of this object is to be filled (via successive add_client_id()
    calls) and passed to a Clips (and indirectly to Targets objects). The
    content can just be serialized as a pickle to make it pythonic, checked
    for size and iterated by calling client_hashes().

    Args:
        binary_image: An optional binary image (returned by save_as_binary_image())
                      to initialize the object with data copied from another Clients
                      object.
    """

    def __init__(self, binary_image=None):
        self.cl_id = new_clients()

        if binary_image is not None:
            self.load_from_binary_image(binary_image)

    def __del__(self):
        destroy_clients(self.cl_id)

    def __str__(self):
        n = self.num_clients()
        if n == 0:
            return 'Empty reels.Clients object (Empty objects select ALL clients.)'
        return 'reels.Clients object selecting %i clients' % n

    def __repr__(self):
        return self.__str__()

    def __getstate__(self):
        """Used by pickle.dump() (See https://docs.python.org/3/library/pickle.html)"""
        return self.save_as_binary_image()

    def __setstate__(self, state):
        """Used by pickle.load() (See https://docs.python.org/3/library/pickle.html)"""
        self.cl_id = new_clients()
        self.load_from_binary_image(state)

    def hash_client_id(self, client):
        """Convert a client id into a hash (as stored buy the internal C** Clients object).

        Args:
            client: The "client". A string representing "the actor".

        Returns:
            The hash as a decimal python string starting with 'h' to prevent numeric conversion.
        """
        return clients_hash_client_id(self.cl_id, client)

    def add_client_id(self, client):
        """Define clients explicitly.

        Args:
            client: The "client". A string representing "the actor".

        Returns:
            True on success.
        """
        return clients_add_client_id(self.cl_id, client)

    def client_hashes(self):
        """Return an iterator to iterate over all the hashed client ids.

        Returns:
            An iterator (a ClientsHashes object)
        """
        return ClientsHashes(self.cl_id, self.num_clients())

    def num_clients(self):
        """Return the number of clients in the object.

        Returns:
            The number of clients stored in the object which may include repeated ids.
            The container will add one client per add_client_id() call and keep the order.
        """
        return clients_num_clients(self.cl_id)

    def save_as_binary_image(self):
        """Saves the state of the c++ Clients object as a Python
            list of strings referred to a binary_image.

        Returns:
            The binary_image containing the state of the Clients. There is
            not much you can do with it except serializing it as a Python
            (e.g., pickle) object and loading it into another Clients object.
            Pass it to the constructor to create an initialized object,
        """
        bi_idx = clients_save(self.cl_id)
        if bi_idx == 0:
            return None

        bi = []
        bi_size = size_binary_image_iterator(bi_idx)
        for t in range(bi_size):
            bi.append(next_binary_image_iterator(bi_idx))

        destroy_binary_image_iterator(bi_idx)

        return bi

    def load_from_binary_image(self, binary_image):
        """Load the state of the c++ Clients object from a binary_image
            returned by a previous save_as_binary_image() call.

        Args:
            binary_image: A list of strings returned by save_as_binary_image()

        Returns:
            True on success, destroys, initializes and returns false on failure.
        """
        failed = False

        for binary_image_block in binary_image:
            if not clients_load_block(self.cl_id, binary_image_block):
                failed = True
                break

        if not failed:
            failed = not clients_load_block(self.cl_id, "")

        if failed:
            destroy_clients(self.cl_id)
            self.cl_id = new_clients()
            return False

        return True
