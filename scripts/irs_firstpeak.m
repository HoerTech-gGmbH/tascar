function idx = irs_firstpeak( irs, threshold )
% IRS_FIRSTPEAK - return first peak position of impulse responses
%
% Usage:
% idx = irs_firstpeak( irs [, threshold ] )
%
% irs : impulse response
% threshold : detection threshold (default: 0.1 dB)
%
% Method:
% Search for first sample in reverse cumulative energy which is 0.1
% dB below maximum.
% 
%
  if nargin < 2
    threshold = 0.1;
  end
  for k=1:size(irs,2)
    tburst = cumsum(irs(end:-1:1,k).^2);
    tburst = 10*log10(tburst(end:-1:1) ./ max(tburst));
    idx(k) = find(tburst<-threshold,1);
  end
