function [data, fs, p_drift] = tascar_dl_dejitter( data )
% The tascar_dl_dejitter function is designed to correct timing jitter or
% synchronization issues in data acquisition systems. It adjusts the
% receive times to align with the send times, compensating for delays and
% drifts in the timing signals.
%
% Function Syntax:
%
% [data, fs, p_drift] = tascar_dl_dejitter(data);
%
% Input Arguments:
%
%  data: A matrix where the first row contains the receive times
%        (t_rec) and the second row contains the send times (t_send).
%
% Output Arguments:
%
%  data: The corrected matrix with adjusted receive times. fs: The sampling
%  frequency calculated from the send times. p_drift: A linear fit
%  representing the drift or trend in the timing data.

;
% Initialize end-to-end delay (currently set to 0)
end2end_delay = 0;

% Extract send and receive times from the input data matrix
t_send = data(2,:);  % Send times are in the second row
t_rec = data(1,:);   % Receive times are in the first row

% Calculate the sampling frequency based on send times
fs = 1/median(diff(t_send));  % Using median to robustly estimate the sampling rate

% Determine the buffer size based on twice the sampling frequency
bufferSize = ceil(2 * fs);

% Check if the number of data points exceeds 4 times the buffer size
if size(data, 2) > 4 * bufferSize
    % Buffer the send times and calculate the median of buffered send times
    bufferedSendTimes = buffer(t_send, bufferSize);
    t_k = median(bufferedSendTimes(:, 2:end-1));  % Exclude the first and last elements to avoid edge effects

    % Buffer the difference between receive and send times
    bufferedSendTimes = buffer(t_rec - t_send, bufferSize);
    y_k = min(bufferedSendTimes(:, 2:end-1)) + t_k;  % Find the minimum adjustment needed

    % Perform a linear polynomial fit to align receive times with send
    % times
    P_fit = polyfit(t_k, y_k, 1);
    t_rec_new = polyval(P_fit, t_send);  % Calculate new receive times

    % Update the receive times in the data matrix
    data(1, :) = t_rec_new - end2end_delay;
else
    % For smaller datasets, apply a simpler synchronization method
    t_rec_new = min(t_rec - t_send) + t_send;
    data(1, :) = t_rec_new - end2end_delay;
end

% Remove any invalid data points where receive times are not increasing
idx = find(diff(data(1, :)) < 0, 1, 'last');
if ~isempty(idx)
    data(:, 1:idx) = [];
end

% Calculate the drift as a linear fit between receive and send times
p_drift = polyfit(data(1, :), data(1, :) - data(2, :), 1);
end
