function [mgains, vf, X, ir] = spkgains2minphaseir(layoutname, fs, fftlen, n)
    %% Compute minimum phase impulse responses from speaker gains in a layout file.
    %
    % Inputs:
    %   layoutname - Name of the XML layout file containing speaker information
    %   fs        - Sampling frequency
    %   fftlen    - Length of the FFT
    %   n         - Length of the impulse response
    %
    % Outputs:
    %   mgains   - Matrix of magnitude gains for each speaker
    %   vf       - Vector of frequencies corresponding to the gains
    %   X        - Complex frequency response in the FFT domain
    %   ir       - Impulse response in the time domain
    %
    % Description:
    % This function reads speaker gain information from an XML layout file,
    % computes the magnitude gains, interpolates them across the frequency spectrum,
    % computes the minimum phase using the Hilbert transform, and reconstructs
    % the impulse response with an optional windowing.

    % Open the XML layout file
    doc = tascar_xml_open(layoutname);

    % Get all speaker elements from the XML
    hspk = tascar_xml_get_element(doc, 'speaker');

    % Initialize arrays to store gains and labels
    csLabels = {};
    vgains = [];
    mgains = [];

    % Process each speaker element
    for k = 1:numel(hspk)
        % Get frequency and gain from the speaker element
        vf = str2num(tascar_xml_get_attribute(hspk{k}, 'eqfreq'));
        vg = str2num(tascar_xml_get_attribute(hspk{k}, 'eqgain'));
        l = char(tascar_xml_get_attribute(hspk{k}, 'label'));

        % Store the magnitude gains
        mgains(end+1,:) = vg;

        % Use default label if not provided
        if isempty(l)
            l = num2str(k);
        end

        % Store the speaker label
        csLabels{end+1} = l;

        % Store the overall gain
        g = str2num(tascar_xml_get_attribute(hspk{k}, 'gain'));
        vgains(end+1) = g;
    end

    % Transpose mgains to match expected dimensions
    mgains = mgains';
    vf = vf';

    % Initialize the frequency response array
    X = realfft(zeros(fftlen, 1));
    nbins = numel(X);

    % Create a linearly spaced frequency vector
    vflin = linspace(0, 0.5*fs, nbins)';

    % Interpolate the magnitude gains across the frequency spectrum
    % Using linear interpolation with extrapolation
    X = interp1(([0.1*min(vf); vf; fs]), [mgains(1,:); mgains; mgains(end,:)], ...
                max(eps, vflin), 'linear', 'extrap');

    % Convert magnitude gains to decibel scale
    X = 10.^(0.05 * X);

    % Compute the phase using the Hilbert transform
    phase = log(max(1e-10, abs(X)));
    phase = -imag(myhilbert(phase));

    % Reconstruct the complex frequency response
    X = X .* exp(i * phase(1:nbins, :));

    % Compute the impulse response using inverse FFT
    ir = realifft(X, fftlen);

    % Apply a cosine window to the impulse response
    wnd = cos(0.5 * [0:(n-1)]' * pi / n);
    ir = ir(1:n, :) .* repmat(wnd, [1, size(X, 2)]);

    % Plot the magnitude response (uncomment to visualize)
    if true
      hold('off');
      plot(interp1(vflin,20 * log10(abs(X)), [0:1:0.5*fs],'linear','extrap'));
      hold('on');
      set(gca,'colororderindex',1);
      tmp_ir = ir;
      tmp_ir(end+1:fs,:) = 0;
      plot(20 * log10(abs(realfft(tmp_ir))),'linewidth',2);
      set(gca,'XScale','log','XLim',[50,0.5*fs]);
    end
end

function x = myhilbert(xr)
    %% Compute the Hilbert transform of a signal.
    %
    % Inputs:
    %   xr - Input signal (vector)
    %
    % Outputs:
    %   x  - Analytic signal obtained from the Hilbert transform
    %
    % Description:
    % This function computes the Hilbert transform of the input signal using
    % FFT-based methods. It handles both even and odd lengths of the input
    % signal appropriately.

    % Ensure the input is a column vector
    if prod(size(xr)) == max(size(xr))
        xr = xr(:);
    end

    % Compute the FFT of the input signal
    X = fft(xr);

    % Get the length of the FFT
    fftlen = size(X, 1);

    % Identify the Nyquist bin
    nyquist_bin = floor(fftlen / 2) + 1;

    % Zero out the negative frequencies
    X(nyquist_bin + 1:end, :) = 0;

    % Handle the DC component
    X(1, :) = 0.5 * X(1, :);

    % Handle the Nyquist frequency if the length is even
    if ~mod(fftlen, 2)
        X(nyquist_bin, :) = 0.5 * X(nyquist_bin, :);
    end

    % Compute the inverse FFT to get the analytic signal
    x = ifft(2 * X);
end
