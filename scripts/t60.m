function [t60late,EDT,i50,i250,cburst] = t60( irs, fs, lthreshold, ethreshold, pthreshold, qthreshold )
%%% t60 - measure broadband RT30 based on impulse response
%
% Usage:
% [t60late,EDT,i50,i250,cburst] = t60( irs, fs, lthreshold, ethreshold, pthreshold, qthreshold );
% or:
% [t60late,EDT,i50,i250,cburst] = t60( filename );
%
% irs : impulse response
% fs  : sampling rate
% lthreshold : threshold used for late reverb (default: -30)
% ethreshold : threshold used for early reflections (defaut: -10)
% pthreshold : threshold used for peak (defaut: -0.2)
% qthreshold : fit quality criterion threshold (default: 1)
%
% Method after: Schroeder, Manfred R. "New method of measuring
% reverberation time." The Journal of the Acoustical Society of
% America 37 (1965): 409.
%
% Author: Giso Grimm
% 2012, 2018, 2019, 2021

  if ischar(irs)
    [irs,fs] = audioread(irs);
  end
  if nargin < 3
    lthreshold = -30;
  end
  if nargin < 4
    ethreshold = -10;
  end
  if nargin < 5
    pthreshold = -0.2;
  end
  if nargin < 6
    qthreshold = 1;
  end
  cburst = {};
  for k=1:size(irs,2)
    %% copy IR of channel k:
    lirs = irs(:,k);
    %% remove everything before max:
    [tmp,idx] = max(abs(lirs));
    lirs = lirs(idx:end,:);
    %lirs = lirs / max(abs(lirs));
    %lnoise = 10*log10(mean(lirs(round(0.75*numel(lirs)):end).^2))
    %% trim end to required range, to avoid influence of noise floor:
    tburst = cumsum(lirs(end:-1:1,1).^2);
    tburst = 10*log10(tburst(end:-1:1) ./ max(tburst));
    idx_start = find(tburst<pthreshold,1);
    lstart = tburst(idx_start);
    idx_end = ...
    find(tburst  >= ...
	 lstart + lthreshold+ethreshold-10,...
	 1,'last');
    lirs = lirs(1:idx_end,:);
    %% now get tone burst for calculation ot T60:
    tburst = cumsum(lirs(end:-1:1,1).^2);
    tburst = 10*log10(tburst(end:-1:1) ./ max(tburst));
    idx_start = find(tburst<pthreshold,1);
    lstart = tburst(idx_start);
    idx0 = find(tburst<lstart-0.1,1);
    idx1 = find(tburst<lstart+ethreshold+pthreshold,1);
    idx2 = find(tburst<lstart+lthreshold+ethreshold+pthreshold,1);
    %% get fit quality:
    q_edt = get_fitquality( tburst, idx0, idx1 );
    if q_edt > qthreshold
      warning('poor EDT fit quality. Consider decreasing pthreshold.');
    end
    q_t60 = get_fitquality( tburst, idx1, idx2 );
    if q_t60 > qthreshold
      warning('poor T60 fit quality. Consider increasing lthreshold or ethreshold.');
    end
    EDT(k) = -60/ethreshold*(idx1-idx0)/fs;
    if ~isempty(idx2)
      t60late(k) = -60/lthreshold*(idx2-idx1)/fs;
    else
      rg = ethreshold+pthreshold - tburst(end);
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
    if( (nargout == 0)||(q_edt>qthreshold)||(q_t60>qthreshold) )
      figure
      plot([idx0,idx1,idx2]/fs,tburst([idx0,idx1,idx2]),'kx-','linewidth',5,'markersize',10,'Color',[0.5,0.5,0.5]);
      hold on
      plot([1:numel(tburst)]/fs,tburst);
      plot([1:numel(lirs)]/fs,20*log10(abs(lirs./max(abs(lirs)))))
      text(mean([idx0,idx1])/fs,mean(tburst([idx0,idx1])),...
	   sprintf('  EDT = %1.3fs (q=%1.3g)',EDT(k),q_edt));
      text(mean([idx2,idx1])/fs,mean(tburst([idx2,idx1])),...
	   sprintf('  RT%g = %1.3fs (q=%1.3g)',abs(lthreshold),t60late(k),q_t60));
      ylim([-90,0]);
      title(['channel ',num2str(k)]);
      xlabel('time / s');
    end
  end
end

function q = get_fitquality( tburst, idx0, idx1 )
  x = [idx0:idx1]';
  y = tburst(idx0:idx1,:);
  m = (y(end)-y(1))/numel(y);
  q = y - (x*m - m*x(1) + y(1));
  q = mean(q.^2);
end
