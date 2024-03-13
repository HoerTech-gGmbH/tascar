function DOA = SDMbf(IR_bf, p)
% LOC = SDMPar(IR, p)
% Spatial Decomposition Method [1], implemented for b-format impulse
% response
% Returns locations (or direction of arrival) at each time step of the IR. 
%
% USAGE:
% 
% IR  : A matrix of measured impulse responses from a microphone array 
%       [W X Y Z] of the b-format
% DOA : 3-D Cartesian locations of the image-sources [N 3];
% p   : struct from createSDMStruct.m or can be manually given
% 
% IR matrix includes B-format impulse responses from a microphone array.
% SDM assumes an omni-directional directivity pattern for the pressure,
% i.e. the zeroth order harmonic.
% 
% p.winLen = 0;
% 
% 'fs' : a single value, sampling frequency [Hz] [default
% 48e3]
% 'c' : a single value speed of sound [m/s] [default 345]
%
% 'winLen': a single value, length of the processing window, if empty, 
% minimum size is used. [default minimum size]
% minimum size is winLen = 1 sample
% 
% References
% [1] Spatial decomposition method for room impulse responses
% S Tervo, J Patynen, A Kuusinen, T Lokki - Journal of the Audio
% Engineering Society, vol. 61, no. 1/2, pp. 16-27, 2013

% SDM toolbox : SDMbf
% Sakari Tervo & Jukka Patynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

if nargin < 1
    error(['SDM toolbox : SDMbf : Two inputs are required ' ... 
    ' SDMPar(IR, p)']);
end

disp('Started SDM processing'); tic;

% Choose the frame size
winLen = p.winLen;

% Windowing
W = hanning(winLen);

% --- Frame based processing ---- 
% Convolution of DOA with W -> low-pass filtering the DOA estimates
if winLen >= 5 % Acceptable window length is more than 4.
    X = conv(W,IR_bf(:,1).*IR_bf(:,2));
    Y = conv(W,IR_bf(:,1).*IR_bf(:,3));
    Z = conv(W,IR_bf(:,1).*IR_bf(:,4));
    % Normalize
    DOA = [X Y Z]./repmat(sqrt(X.^2+Y.^2+Z.^2),[1 3]);
    % Truncate
    DOA = DOA(floor(winLen/2)+(1:size(IR_bf,1)),:);
else
    X = IR_bf(:,1).*IR_bf(:,2);
    Y = IR_bf(:,1).*IR_bf(:,3);
    Z = IR_bf(:,1).*IR_bf(:,4);
    % Normalize
    DOA = [X Y Z]./repmat(sqrt(X.^2+Y.^2+Z.^2),[1 3]);
end



% --- EOF Frame based processing ---- 

disp(['Ended SDM processing in ' num2str(toc) ' seconds.' ])
end % <-- EOF SDMpar()
