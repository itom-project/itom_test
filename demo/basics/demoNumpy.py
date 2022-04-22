"""Numpy
===========

This demo shows an example on how to use ``numpy`` in ``itom``.
"""

import numpy as np
from numpy.linalg import svd
from numpy.typing import ArrayLike


def rank(A: ArrayLike, atol: float = 1e-13, rtol: int = 0) -> int:
    """Estimate the rank (i.e. the dimension of the nullspace) of a matrix.

       The algorithm used by this function is based on the singular value
       decomposition of `A`.
       If both `atol` and `rtol` are positive, the combined tolerance is the
       maximum of the two; that is::
           tol = max(atol, rtol * smax)
       Singular values smaller than `tol` are considered to be zero.

    Args:
        A (ArrayLike): A should be at most 2-D.  A 1-D array with length n will be treated
            as a 2-D with shape (1, n)
        atol (float, optional): The absolute tolerance for a zero singular value.  Singular values
            smaller than `atol` are considered to be zero.
        rtol (int, optional): The relative tolerance.  Singular values less than rtol*smax are
            considered to be zero, where smax is the largest singular value.

    Returns:
        int: The estimated rank of the matrix.

    See also
    --------
    numpy.linalg.matrix_rank
        matrix_rank is basically the same as this function, but it does not
        provide the option of the absolute tolerance."""

    A = np.atleast_2d(A)
    s = svd(A, compute_uv=False)
    tol = max(atol, rtol * s[0])
    rank = int((s >= tol).sum())
    return rank


def nullspace(A: ArrayLike, atol: float = 1e-13, rtol: int = 0) -> ArrayLike:
    """Compute an approximate basis for the nullspace of A.

       The algorithm used by this function is based on the singular value
       decomposition of `A`.

       If both `atol` and `rtol` are positive, the combined tolerance is the
       maximum of the two; that is::
           tol = max(atol, rtol * smax)
       Singular values smaller than `tol` are considered to be zero.

    Args:
        A (ArrayLike): A should be at most 2-D.  A 1-D array with length k will be treated
            as a 2-D with shape (1, k)
        atol (float, optional): The absolute tolerance for a zero singular value.  Singular values
            smaller than `atol` are considered to be zero.
        rtol (int, optional): The relative tolerance.  Singular values less than rtol*smax are
            considered to be zero, where smax is the largest singular value.

    Returns:
        ArrayLike: If `A` is an array with shape (m, k), then `ns` will be an array
            with shape (k, n), where n is the estimated dimension of the
            nullspace of `A`.  The columns of `ns` are a basis for the
            nullspace; each element in numpy.dot(A, ns) will be approximately
            zero.
    """

    A = np.atleast_2d(A)
    u, s, vh = svd(A)
    tol = max(atol, rtol * s[0])
    nnz = (s >= tol).sum()
    ns = vh[nnz:].conj().T
    return ns


def checkit(a):
    print("a:")
    print(a)
    r = rank(a)
    print("rank is", r)
    ns = nullspace(a)
    print("nullspace:")
    print(ns)
    if ns.size > 0:
        res = np.abs(np.dot(a, ns)).max()
        print("max residual is", res)


def demo_numpy():
    print("-" * 25)

    a = np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]])
    checkit(a)

    b = 2

    print("-" * 25)

    a = np.array([[0.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]])
    checkit(a)

    print("-" * 25)

    a = np.array([[0.0, 1.0, 2.0, 4.0], [1.0, 2.0, 3.0, 4.0]])
    checkit(a)

    print("-" * 25)

    a = np.array(
        [
            [1.0, 1.0j, 2.0 + 2.0j],
            [1.0j, -1.0, -2.0 + 2.0j],
            [0.5, 0.5j, 1.0 + 1.0j],
        ]
    )
    checkit(a)

    print("-" * 25)


if __name__ == "__main__":
    demo_numpy()
