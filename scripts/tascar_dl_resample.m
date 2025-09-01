function [d, t] = tascar_dl_resample( data, name, fs, range, nup, maxgapdur )
% TASCAR_DL_RESAMPLE Resamples TASCAR data from data logging module.
%    
%   This function resamples data to a specified sampling frequency, 
%   optionally within a specified time range, and applies a low-pass filter.
%
% Syntax:
%   [d, t] = tascar_dl_resample(data, name, fs, range, nup)
%
% Description:
%   - data: Cell array of logged data (if name is not empty) or matrix with data rows (if name is empty).
%   - name: Variable name to extract from the cell array, or empty to use data directly.
%   - fs: New sampling frequency in Hz.
%   - range: Time range in seconds (default: full range of data).
%   - nup: Upsampling ratio (default: automatic based on fs and original sampling rate).
%   - maxgapdur: maximum gap duration for interpolation
%
% Outputs:
%   - d: Resampled data matrix.
%   - t: Time vector corresponding to the resampled data.
%
% Example:
%   load unnamed_20220502_093828.mat
%   [d, t] = tascar_dl_resample(data, 'ovheadtracker_acc', 25);

    ;
    % Extract the specified variable from the data cell array if name is provided
    if ~isempty(name)
        data = tascar_dl_getvar(data, name);
    end

    if (nargin < 6) || isempty(maxgapdur)
        maxgapdur = 0;
    end

    % Calculate original sampling frequency from the time vector
    t_orig = data(1, :);
    fs_orig = 1 ./ median(diff(t_orig));

    % fill gaps with NaN:
    if maxgapdur > 0
        data = [data(:,1),data,data(:,end)];
        data(:,1) = NaN;
        data(:,end) = NaN;
        data(1,1) = min(data(1,:))-maxgapdur;
        data(1,end) = max(data(1,:))+maxgapdur;
        idx_fill = find(diff(t_orig) > maxgapdur);
        for k=idx_fill(end:-1:1)
            data_new = [mean(t_orig(k:(k+1)));NaN([size(data,1)-1,1])];
            data = [data(:,1:k),data_new,data(:,k+1:end)];
        end
    end
    t_orig = data(1, :);

    % Set default time range if not specified
    if (nargin < 4) || isempty(range)
        range = [min(t_orig), max(t_orig)];
    end

    % Determine upsampling ratio if not provided
    if (nargin < 5) || isempty(nup)
        nup = round(max(16, 2 * fs / fs_orig));
    end

    % Find indices corresponding to the specified time range
    irange_start = find(t_orig <= range(1), 1, 'last');
    if isempty(irange_start)
        irange_start = 1;
    end
    irange_end = find(t_orig >= range(2), 1, 'first');
    if isempty(irange_end)
        irange_end = numel(t_orig);
    end
    irange = irange_start:irange_end;

    % Handle invalid time range
    if isempty(irange)
        error('Invalid time range');
    end

    % Trim data to the specified range
    data = data(:, irange);

    % Remove rows with zero time difference to avoid invalid data points
    data(:, find(diff(data(1, :)) == 0)) = [];

    % Create new time vector for resampling
    t = [range(1) - ((nup - 1) / (nup * fs)) : (1 / (nup * fs)) : range(2)]';

    % Perform linear interpolation to resample data
    d = interp1(data(1, :)', data(2:end, :)', t, ...
                'linear', 'extrap');

    % Apply low-pass filter to smooth the resampled data
    d = lp(d, 2 * nup);

    % Downsample to achieve the desired sampling frequency
    t = t(nup:nup:end, :);
    d = d(nup:nup:end, :);
end

function d = lp(d, n)
% LP Apply low-pass filter to data.
%   This helper function applies a moving average filter to smooth the data.
%
% Syntax:
%   d = lp(d, n)
%
% Description:
%   - d: Input data to be filtered.
%   - n: Filter size (number of points to average).
%
% Output:
%   - d: Filtered data.

    ;
    % Calculate the length of the data
    len = size(d, 1);

    % Determine the number of points to pad at the end
    n2 = ceil(n / 2);

    % Pad the data to handle filter transient
    d(end + [1:n], :) = repmat(d(end, :), [n, 1]);

    % Create the filter kernel (moving average)
    B = ones(n, 1) / n;

    % Apply the filter using FFT-based convolution
    d = fftfilt(B, d);

    % Trim the filtered data to the original length
    d = d(n2 + [1:len], :);
end
