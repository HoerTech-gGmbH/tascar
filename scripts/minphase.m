function [x, X, phase] = minphase(x)
    %% Compute the minimum phase signal from the given input signal.
    %
    % Inputs:
    %   x       - Input signal (vector)
    %
    % Outputs:
    %   x       - Minimum phase signal corresponding to the input magnitude
    %   X       - Magnitude of the real part of the FFT of the input signal
    %   phase   - Phase of the minimum phase signal
    %
    % Description:
    % This function computes the minimum phase signal corresponding to the given
    % input signal. The function uses the Hilbert transform to compute the phase
    % and reconstructs the signal using the inverse FFT.

    fftlen = size(x, 1); % Length of the FFT
    X = abs(realfft(x)); % Magnitude of the real part of the FFT of x
    nbins = size(X, 1); % Number of frequency bins
    phase = log(max(1e-10, X)); % Compute initial phase using logarithm (avoid log(0))
    phase(end+1:fftlen, :) = 0; % Extend phase to match fftlen
    phase = -imag(hilbert(phase)); % Compute minimum phase using Hilbert transform
    X = X .* exp(i * phase(1:nbins, :)); % Reconstruct complex FFT from magnitude and phase
    x = realifft(X, fftlen); % Compute inverse FFT to get the minimum phase signal
end