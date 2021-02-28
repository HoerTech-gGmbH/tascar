function c = iacc( irs, fs, trange )
% IACC - interaural cross correlation
%
% Usage:
% c = iacc( irs, fs, trange )
%
% irs    : binaural impulse response
% fs     : sampling rate in Hz
% trange : time range in seconds (optional; default: full length)
%
% Author: Giso Grimm, 2015  
  if nargin < 3
    idx = 1:size(irs,1);
  else
    trange = 1+round(trange*fs);
    if diff(trange) <= 0
      error('trange(2) is not larger than trange(1)');
    end
    i0 = min(irs_firstpeak( irs ));
    if trange(1) ~= 1
      trange(1) = trange(1)+i0;
    end
    trange(2) = trange(2)+i0;
    idx = max(1,trange(1)):min(size(irs,1),trange(2));
  end
  maxlags = round(0.001*fs);
  c = max(xcorr(irs(idx,1),irs(idx,2), maxlags, 'coeff'));