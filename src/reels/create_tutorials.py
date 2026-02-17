import os, shutil
from importlib.resources import files


def create_tutorials(destination, silent = False):
    """
    Copies mercury.reels tutorial notebooks to `destination`. A folder will be created inside
    destination, named 'reels_tutorials'. The folder `destination` must exist.

    Args:
        destination (str): The destination directory
        silent (bool): If True, suppresses output on success.

    Raises:
        ValueError: If `destination` is equal to source path.

    Examples:
        >>> # copy tutorials to /tmp/reels_tutorials
        >>> from reels import create_tutorials
        >>> create_tutorials('/tmp')

    """
    src = '%s/tutorials' % str(files(__package__))

    # When the packages is not installed and runs from source code, the path is:
    # `<..>/mercury-reels/notebooks` instead of `<..>/mercury-reels/src/reels/tutorials`

    if not os.path.exists(src):
        src = src.replace('/src/reels/tutorials', '/notebooks')

    dst = os.path.abspath(destination)

    assert src != dst, 'Destination (%s) cannot be the same as source.' % src

    assert os.path.isdir(dst), 'Destination (%s) must be a directory.' % dst

    dst = os.path.join(dst, 'reels_tutorials')

    assert not os.path.exists(dst), 'Destination (%s) already exists' % dst

    shutil.copytree(src, dst)

    if not silent:
        print('Tutorials copied to: %s' % dst)
