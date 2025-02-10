"""A method to plot and return comparisons of the given windowing functions."""
import matplotlib.pyplot as plt


def compare(N, windowTest, windowReference, sym=False):
    """
    Plot and return comparisons of the given windowing functions.
    
    Args:
        N: the length of the window
        windowTest: the test window function
        windowReference: the reference window function
    """
    sample_idx = list(range(N))
    test = windowTest(N, sym=sym)
    plt.plot(sample_idx, test)
    reference = windowReference(N, sym=sym)
    plt.plot(sample_idx, reference)
    plt.legend(['Test', 'Reference'])
    plt.xlabel('$n$')
    plt.ylabel('w[n]')
    return test, reference


# Explicitly define the outward facing API of this module.
__all__ = [compare.__name__]
