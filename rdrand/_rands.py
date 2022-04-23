import _rdrand


__all__ = [
    'randint',
    'randbelow',
    'randrange',
]


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


def randint(a: int, b: int) -> int: 
    """Generate a random number in the range [a, b]"""
    return randrange(a, b + 1)


def randbelow(n: int) -> int:
    """Generate a number in range [0, n]"""

    k = n.bit_length()
    r = _rdrand.randbits(k)

    while r >= n:
        r = _rdrand.randbits(k)

    return r
