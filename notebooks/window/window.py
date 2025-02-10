"""Window functions."""
import numpy as np


# ----------------------------------------------------------------------------
# MARK: Non-Parametric Windows
# ----------------------------------------------------------------------------


def boxcar(N, sym=True):
    """
    Return a Boxcar window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Boxcar window of length N with given symmetry

    """
    return np.ones(N)


def bartlett(N, sym=False):
    """
    Return a Bartlett window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Bartlett window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return (2.0 / N) * ((N / 2) - abs(n - N / 2))


def barthann(N, sym=False):
    """
    Return a Bartlett-Hann window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Bartlett-Hann window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return 0.62 - 0.48 * abs(n / N - 0.5) - 0.38 * np.cos(2 * np.pi * n / N)


def parzen(N, sym=True):
    """
    Return a Parzen window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Parzen window of length N with given symmetry

    """
    n = np.arange(N, dtype=float)
    w = np.zeros(N, dtype=float)
    n = 2 * n / (N-sym) - 1  # [0, N) -> [-1, 1]
    w[abs(n) < 1/2 ] = 1 - 6 * n[abs(n) < 1/2]**2 + 6 * abs(n[abs(n) < 1/2])**3
    w[abs(n) >= 1/2] = 2 * (1 - abs(n[abs(n) >= 1/2]))**3
    return w


def welch(N, sym=False):
    """
    Return a Lanczos window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Lanczos window of length N with given symmetry

    """
    n = np.arange(N)
    return 1 - ((n - (N - 1 - sym) / 2) / ((N + 1 - sym) / 2))**2


def cosine(N, sym=False):
    """
    Return a Cosine window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Cosine window of length N with given symmetry

    """
    n = np.arange(N)
    N = N + (not sym)
#     return -np.cos(np.pi * (n + N/2 + 0.5) / N)
    return np.sin(np.pi * (n + 0.5) / N)


def bohman(N, sym=False):
    """
    Return a Bohman window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Bohman window of length N with given symmetry

    """
    n = np.arange(N)
    N = (N - 1 if sym else N) / 2
    return (1 - abs(n / N - 1)) * np.cos(np.pi * abs(n / N - 1)) + (1.0 / np.pi) * np.sin(np.pi * abs(n / N - 1))


def lanczos(N, sym=False):
    """
    Return a Lanczos window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Lanczos window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    w = np.pi * (2 * n / N - 1)
    w = np.sin(w) / (w + 1e-12)
    w[N//2] = 1
    return w


def hann(N, sym=True):
    """
    Return a Hann window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Hann window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return 0.50 - 0.50 * np.cos(2 * np.pi * n / N)


def hamming(N, sym=False):
    """
    Return a Hamming window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Hamming window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return 0.54 - 0.46 * np.cos(2 * np.pi * n / N)


def blackman(N, sym=False):
    """
    Return a Blackman window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Blackman window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return 0.42 - 0.50 * np.cos(2 * np.pi * n / N) + 0.08 * np.cos(4 * np.pi * n / N)


def blackmanharris(N, sym=False):
    """
    Return a Blackman-Harris window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Blackman-Harris window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return (0.35875
        - 0.48829 * np.cos(2 * np.pi * n / N)
        + 0.14128 * np.cos(4 * np.pi * n / N)
        - 0.01168 * np.cos(6 * np.pi * n / N)
    )


def blackmannuttall(N, sym=False):
    """
    Return a Blackman-Harris window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Blackman-Harris window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return (0.3635819
        - 0.4891775 * np.cos(2 * np.pi * n / N)
        + 0.1365995 * np.cos(4 * np.pi * n / N)
        - 0.0106411 * np.cos(6 * np.pi * n / N)
    )


def kaiserbessel(N, sym=None):
    """
    Return a Kaiser window derived from a Bessel function.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.

    Returns:
        the Kaiser window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - sym
    return (0.402
        - 0.498 * np.cos(2 * np.pi * n / N)
        + 0.098 * np.cos(4 * np.pi * n / N)
        - 0.001 * np.cos(6 * np.pi * n / N)
    )


def flattop(N, sym=False):
    """
    Return a Flattop window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Flattop window of length N with given symmetry

    """
    n = np.arange(N)
    N = N - 1 if sym else N
    return (0.21557895
        - 0.416631580 * np.cos(2 * np.pi * n / N)
        + 0.277263158 * np.cos(4 * np.pi * n / N)
        - 0.083578947 * np.cos(6 * np.pi * n / N)
        + 0.006947368 * np.cos(8 * np.pi * n / N)
    )


# ----------------------------------------------------------------------------
# MARK: Parametric Windows
# ----------------------------------------------------------------------------


def exponential(N, sym=False, alpha=1.0):
    """
    Return a Exponential (Poission) window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Exponential (Poission) window of length N with given symmetry

    """
    n = np.arange(N)
    N = (N - 1 if sym else N) / 2
    return np.exp(-alpha * abs(n - N) / N)


def gaussian(N, sym=False, std=0.25):
    """
    Return a Lanczos window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Lanczos window of length N with given symmetry

    """
    n = np.arange(N)
    N = (N - 1 if sym else N) / 2
    return np.exp(-0.5 * ((n - N)/(std * N))**2)


def hannpoisson(N, sym=False, alpha=1.0):
    """
    Return a Exponential (Poission) window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Exponential (Poission) window of length N with given symmetry

    """
    n = np.arange(N)
    N = (N - 1 if sym else N) / 2
    return 0.5 * (1 - np.cos(np.pi * n / N)) * np.exp(-alpha * abs(n - N) / N)


def tukey(N, sym=False, alpha=0.5):
    """
    Return a Tukey window.

    Args:
        N: Number of points in the output window. 
        If zero or less, an empty array is returned.
        sym: When True (default), generates a symmetric window, 
        for use in filter design. When False, generates a 
        periodic window, for use in spectral analysis.

    Returns:
        the Tukey window of length N with given symmetry

    """
    n = np.arange(N)
    N = (N - 1 if sym else N) / 2
    w = 0.5 * (1 + np.cos(np.pi * (abs(n - N) - alpha * N)/((1 - alpha) * N)))
    w[abs(n - N) < alpha * N] = 1
    return w


# Explicitly define the outward facing API of this module.
__all__ = [
    boxcar.__name__,
    bartlett.__name__,
    barthann.__name__,
    parzen.__name__,
    welch.__name__,
    cosine.__name__,
    bohman.__name__,
    lanczos.__name__,
    hann.__name__,
    hamming.__name__,
    blackman.__name__,
    blackmanharris.__name__,
    blackmannuttall.__name__,
    kaiserbessel.__name__,
    flattop.__name__,
    exponential.__name__,
    gaussian.__name__,
    hannpoisson.__name__,
    tukey.__name__,
]
