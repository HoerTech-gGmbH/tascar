function [t60late,EDT,i50,i250,cburst] = t60( irs, fs, lthreshold, ethreshold )
% t60 - measure broadband RT30 based on impulse response
%
% Usage:
% [t60late,EDT,i50,i250,cburst] = t60( irs, fs, lthreshold, ethreshold );
% or:
% [t60late,EDT,i50,i250,cburst] = t60( filename );
%
% irs : impulse response
% fs  : sampling rate
% lthreshold : threshold used for late reverb (default: -30)
% ethreshold : threshold used for early reflections (defaut: -10)
%
% Method after: Schroeder, Manfred R. "New method of measuring
% reverberation time." The Journal of the Acoustical Society of
% America 37 (1965): 409.
%
% Author: Giso Grimm
% 2012, 2018, 2019
  ;
  if ischar(irs)
    [irs,fs] = audioread(irs);
  end
  if nargin < 3
    lthreshold = -30;
  end
  if nargin < 4
    ethreshold = -10;
  end
  cburst = {};
  for k=1:size(irs,2)
      lirs = irs(:,k);
      idx_end = ...
          find(abs(lirs)./max(abs(lirs)) >= ...
               10.^(0.05*(lthreshold+ethreshold-10.1)),...
               1,'last');
      lirs = lirs(1:idx_end,:);
    tburst = cumsum(lirs(end:-1:1,1).^2);
    tburst = 10*log10(tburst(end:-1:1) ./ max(tburst));
    idx0 = find(tburst<-0.1,1);
    idx1 = find(tburst<ethreshold-0.1,1);
    idx2 = find(tburst<lthreshold+ethreshold-0.1,1);
    EDT(k) = -60/ethreshold*(idx1-idx0)/fs;
    if ~isempty(idx2)
      t60late(k) = -60/lthreshold*(idx2-idx1)/fs;
    else
      rg = ethreshold-0.1 - tburst(end);
      idx2 = numel(tburst);
      t60late(k) = 60/rg*(idx2-idx1)/fs;
    end
    if idx0+round(fs*0.05) <= numel(tburst)
      i50(k) = tburst(idx0+round(fs*0.05));
    else
      i50(k) = -inf;
    end
    if idx0+round(fs*0.25) <= numel(tburst)
      i250(k) = tburst(idx0+round(fs*0.25));
    else
      i250(k) = -inf;
    end
    cburst{k} = tburst;
  end
  