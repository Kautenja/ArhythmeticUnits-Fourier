"""A method to plot data from scipy.signal.freqz."""
import numpy as np
import matplotlib.pyplot as plt


def zplot(w, h):
    """
    Plot data from scipy.signal.freqz.

    Args:
        w: the sampled frequencies
        h: the magnitude response

    Returns:
        None

    """
    fig, axs = plt.subplots(1, 2, sharex=True, figsize=(12, 5))
    # magnitude response
    axs[0].plot(w / np.pi, 20 * np.log10(np.abs(h) + 1e-7))
    axs[0].set_xlabel('Normalized Angular Frequency')
    axs[0].set_xlim([0, 1])
    axs[0].set_ylabel('Magnitude Response (dB)')
#     axs[0].set_ylim([-60, 30])
    axs[0].grid()
    # phase response
    YTICKS = np.array([-2.0, -1.75, -1.5, -1.25, -1.0, -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0])
#     # - normalized frequency
#     axs[1].plot(w / np.pi, np.unwrap(np.angle(h)) / np.pi)
#     axs[1].set_xlabel('Normalized Angular Frequency')
#     axs[1].set_xlim([0, 1])
#     axs[1].set_ylabel('Phase Response (Normalized Angular Frequency)')
#     axs[1].set_ylim([-2, 2])
#     axs[1].set_yticks(YTICKS)
#     axs[1].grid()
#     # - degrees
#     axs[1].plot(w / np.pi, np.unwrap(np.angle(h)) * 180 / np.pi)
#     axs[1].set_xlabel('Normalized Frequency')
#     axs[0].set_xlim([0, 1])
#     axs[1].set_ylabel('Phase Response (Degrees)')
#     axs[1].set_ylim([-360, 360])
#     axs[1].set_yticks(180 * YTICKS)
#     axs[1].grid()
    # - radians
    axs[1].plot(w / np.pi, np.unwrap(np.angle(h)))
    axs[1].set_xlabel('Normalized Frequency')
    axs[0].set_xlim([0, 1])
    axs[1].set_ylabel('Phase Response (radians)')
    axs[1].set_ylim([-np.pi, np.pi])
    axs[1].set_yticks(np.pi * YTICKS)
    axs[1].grid()


# Explicitly define the outward facing API of this module.
__all__ = [zplot.__name__]
