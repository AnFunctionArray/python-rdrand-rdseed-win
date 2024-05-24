import _rdrand

from .limits import *
from ._integer import uint_to_bytes
from ._errors import *

from typing import Any
from typing import Sequence
from typing import List


__all__ = [
    'real64',
    'real32',
    'res53',
    'rseed64',
    'rand64',
    'randint',
    'randbelow',
    'randrange',
    'randbytes',
    'choice',
    'shuffle',
]


""" Ensure rdrand/rdseed support after import """
if not all((_rdrand.is_rdrand_supported(),
            _rdrand.is_rdseed_supported())):
    raise InstructionNotSupported(
            "rdrand or rdseed isn't supported by your cpu."
    )
    

def real64() -> float:
    """ Return a 64 bit float random """
    return (_rdrand.rdrand64() + 0.5) * (1.0/4294967296.0)


def real32() -> float:
    """ Return a 32 bit float random """
    return _rdrand.rdrand32() * (1.0/4294967295.0)


def res53() -> float:
    """ Return a random number on [0, 1] with 53-bit resolution """
    a = _rdrand.rdrand32() >> 5
    b = _rdrand.rdrand32() >> 6
    return (a * 67108864.0 + b) * (1.0 / 9007199254740992.0)


def rand64() -> int:
    return _rdrand.rdrand64()


def rseed64() -> int:
    return _rdrand.rdseed()


def randrange(start, stop=None, *, step=1):
    """Choose a random item from range(start, stop[, step])."""

    if not isinstance(start, int):
        raise ValueError(
                f"'start' must be an integer, not {type(start).__name__!r}."
        )

    if stop is None:
        if start > 0:
            return randbelow(start)

        raise ValueError("empty range for randrange()")

    if not isinstance(stop, int):
        raise ValueError(
                f"'stop' must be an integer, not {type(stop).__name__!r}."
        )
    

    if not isinstance(step, int):
        raise ValueError(
                f"'step' must be an integer, not {type(step).__name__!r}."
        )

    width = stop - start

    if step == 1:
        if width > 0:
            return start + randbelow(width)
        raise ValueError("empty range for randrange()")

    if step > 0:
        n = (width + step - 1) // step
    elif step < 0:
        n = (width + step + 1) // step
    else:
        raise ValueError("zero step for randrange()")

    if n <= 0:
        raise ValueError("empty range for randrange()")

    return start + step * randbelow(n)


def randbelow(n: int, *, nolimit=False) -> int:
    """Generate a number in range [0, n]
    
    Args:
        nolimit (bool): Disable 32 bit integers limit. (Default to False)
    """
    
    if not nolimit:
        if n > UINT_MAX - 1:
            raise ValueError(
                "randbelow() does not support integers greater than 32 bits." +
                "\nTo use this without limitation set `nolimit` to True."
            )

    k = n.bit_length()
    r = _rdrand.randbits(k)

    while r >= n:
        r = _rdrand.randbits(k)

    return r


def randint(a: int, b: int) -> int: 
    """Generate a random number in the range [a, b]"""
    return randrange(a, b + 1)


def randbytes(n: int) -> bytes:
    """ Generate 'n' random bytes """
    r = _rdrand.randbits(n * 8)
    #return _rdrand.randbits(n * 8).to_bytes(n, 'big')
    return uint_to_bytes(r)


def choice(seq: Sequence) -> Any:
    """ Choose a random element from a non-empty sequence """
    if not seq:
        raise IndexError("Cannot choose from an empty sequence!")
    return seq[randbelow(len(seq))]


def shuffle(seq: List) -> None:
    """ Shuffle list 'seq' in-place and return None."""
    for i in reversed(range(1, len(seq))):
        j = randbelow(i+1)
        seq[i], seq[j] = seq[j], seq[i]


