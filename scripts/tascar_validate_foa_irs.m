function [d, az, el, ratio] = tascar_validate_foa_irs(ir, fs)
% TASCAR_VALIDATE_FOA_IRS Validates and analyzes B-format impulse responses.
%   This function checks if an impulse response matches the B-format structure
%   and computes various metrics such as distance, direction of arrival, and
%   channel ratios.
%
% Syntax:
%   [d, az, el, ratio] = tascar_validate_foa_irs(ir, fs)
%
% Description:
%   - ir: Impulse response matrix with 4 channels (B-format).
%   - fs: Sampling frequency in Hz.
%   - d: Estimated distance from the sound source to the microphone array.
%   - az: Azimuth angle in degrees (FuMa channel order: wxyz).
%   - el: Elevation angle in degrees (FuMa channel order: wxyz).
%   - ratio: Ratio between the w channel and the xyz channels.
%
% Usage:
%   [d, az, el, ratio] = tascar_validate_foa_irs(ir, fs);
%
% Example:
%   % Load an impulse response
%   load impulse_response.mat;
%   % Validate and analyze the B-format impulse response
%   [d, az, el, ratio] = tascar_validate_foa_irs(ir, fs);
%
% Notes:
%   - The impulse response must have exactly 4 channels.
%   - The function assumes the impulse response is stored in a matrix with
%   time in rows and channels in columns.
%   - Requires the Signal Processing Toolbox or the Octave 'signal' package.
%   - The function displays summary information if called without output arguments.

  % Check if the impulse response has exactly 4 channels
  if size(ir, 2) ~= 4
    error(['Impulse response has ', num2str(size(ir, 2)), ' channels, B-Format requires 4']);
  end

  % Load the signal package if running in GNU Octave
  if isoctave()
    % Ensure the signal package is loaded
    pkg('load', 'signal');
  end

  % Define constants
  c = 340; % Speed of sound in m/s
  l = 0.003; % Averaging time in seconds

  % Design a Butterworth band-pass filter
  [B, A] = butter(1, [80, 500] / (0.5 * fs));
  % Apply the filter to the impulse response
  ir = filter(B, A, ir);

  % Find the peak of the direct sound
  idx = irs_firstpeak(ir);
  % Check the standard deviation of the peak
  if c * std(idx) / fs > 0.2
    error(['The standard deviation of the first peak (', num2str(c * std(idx) / fs), 'm) is larger than a typical microphone diameter']);
  end
  % Round to the nearest sample
  idx = round(median(idx));
  % Calculate the distance
  d = c * (idx - 1) / fs;

  % Define the window for direct sound estimation
  widx = unique(max(1, [-round(l * fs):round(l * fs)] + idx));
  % Select and apply a Hann window to the direct sound
  ir = ir(widx, :) .* repmat(hann(numel(widx)), [1, 4]);

  % Compute intensity-weighted average
  w = ir(:, 1).^2;
  % Avoid division by zero
  ir(find(w == 0), 1) = 1;
  % Estimate the ratio between w and xyz channels
  wxyz_rat = abs(ir(:, 1)) ./ sqrt(max(1e-9, sum(ir(:, 2:4).^2, 2)));
  wxyz_rat_mean = sum(w .* wxyz_rat) / sum(w);

  % Check the ratio against the expected value (sqrt(0.5) for FuMa normalization)
  if abs(wxyz_rat_mean - sqrt(0.5)) > 0.05
    error(['The ratio between w and xyz is ', num2str(wxyz_rat_mean), ', which is not 0.70711 (FuMa normalization)']);
  end

  % Estimate element-wise direction of arrival
  doa = ir(:, 2:4) ./ repmat(ir(:, 1), [1, 3]);
  % Calculate weighted average of direction of arrival
  doa_mean = wxyz_rat_mean * sum(repmat(w, [1, 3]) .* doa) / sum(w);

  % Convert to spherical coordinates (FuMa order: wxyz)
  [th, phi, r] = cart2sph(doa_mean(1), doa_mean(2), doa_mean(3));
  az = 180 / pi * th;
  el = 180 / pi * phi;

  % Convert to spherical coordinates (ACN order: wyzx)
  [th, phi, r] = cart2sph(doa_mean(3), doa_mean(1), doa_mean(2));
  az_acn = 180 / pi * th;
  el_acn = 180 / pi * phi;

  ratio = wxyz_rat_mean;

  % Display results if no output arguments are specified
  if nargout == 0
    disp('Estimated parameters (assuming B-format):');
    disp(sprintf('  Distance = %.2f m', d));
    disp(sprintf('  w/xyz ratio = %.3f', ratio));
    disp(sprintf('  DoA (x, y, z) = %.3f, %.3f, %.3f', doa_mean(1), doa_mean(2), doa_mean(3)));
    disp(sprintf('  Azimuth (FuMa) = %.1f deg', az));
    disp(sprintf('  Elevation (FuMa) = %.1f deg', el));
    disp(sprintf('  Azimuth (ACN) = %.1f deg', az_acn));
    disp(sprintf('  Elevation (ACN) = %.1f deg', el_acn));
    clear d;
  end
end

function b = isoctave()
  % ISOCTAVE Check if the code is running in GNU Octave.
  %   Returns true if running in Octave, false otherwise.
  b = ~isempty(ver('Octave'));
end
