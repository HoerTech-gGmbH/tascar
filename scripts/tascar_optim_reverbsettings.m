function vRes = tascar_optim_reverbsettings( s_session, ir_ref, fs_ref, fs2, IRlen )
% tascar_optim_reverbsettings - optimize simplefdn settings
%
% Usage:
% tascar_optim_reverbsettings( s_session, ir_ref, fs_ref )
% tascar_optim_reverbsettings( s_session, vt60, drr, fs_ref, irlen )
%
% Method 1: Specify session and impulse response:
% - s_session: name of TASCAR session file
% - ir_ref: reference impulse response. The number of channels in
%           the impulse response must match the number of channels
%           of the receiver in the TASCAR scene
% - fs_ref: sampling rate in Hz of reference impulse response
%
% Method 2: Specify session and T60 in octave bands 250-500, 500-1k, 1k-2k, 2k-4k Hz
% - s_session: name of TASCAR session file
% - vt60: T60 in octave bands (1x4 row vector) in seconds
% - drr : Direct to reverberant ratio in dB
% - fs_ref: sampling rate in Hz
% - IRlen : length of impulse response in samples
%
% optimization criterion:
% T60 in four bands (250-500,500-1000,1000-2000,2000-4000), DRR
%
% Approach:
% 1. At 10 dB gain, find optimal absorption and damping based on T60 error using an interative grid search
% 2. Find optimal gain based on DRR error, using a grid search
% 3. Refine gain using fminsearch and DRR error
% 4. Refine all parameters using fminsearch and a combined error

  if isoctave()
    %% on octave make sure that signal package is loaded:
    pkg('load','signal');
  end
  if nargin == 3
    %% get features of reference IR:
    vRefT60 = get_t60_features( ir_ref, fs_ref );
    vRefDRR = get_drr_features( ir_ref, fs_ref );
    IRlen = size(ir_ref,1);
  else
    vRefT60 = 40*log10(ir_ref);
    vRefDRR = fs_ref;
    fs_ref = fs2;
  end
  % search grid for absorption and damping:
  vAbsorption = [0.14:0.05:0.99];
  vDamping = [0.14:0.05:0.99];
  vRes = [];
  vE = [];
  for a=vAbsorption
    for d=vDamping
      vE(end+1,:) = get_t60_error( s_session, [a,d], vRefT60, IRlen, fs_ref, 10 );
      vRes(end+1,:) = [a,d];
    end
  end
  [tmp,idx] = min(vE);
  vRes = vRes(idx,:);
  % second grid search iteration:
  vAbsorption = unique(min(0.999,max(0.001,vRes(1)+[-0.05:0.01:0.05])));
  vDamping = unique(min(0.999,max(0.001,vRes(2)+[-0.05:0.01:0.05])));
  vRes = [];
  vE = [];
  for a=vAbsorption
    for d=vDamping
      vE(end+1,:) = get_t60_error( s_session, [a,d], vRefT60, IRlen, fs_ref, 10 );
      vRes(end+1,:) = [a,d];
    end
  end
  [tmp,idx] = min(vE);
  vRes = vRes(idx,:);
  vRes = apply_damp_abs_constraints( vRes );
  e1 = vE(idx);
  % grid search for gain:
  vGain = -30:1:30;
  vE = [];
  for g=vGain
    vE(end+1) = get_drr_error(s_session,g,vRefDRR,IRlen,fs_ref,vRes(1),vRes(2));
  end
  [tmp,idx] = min(vE);
  gain = vGain(idx);
  vResGrid = [gain,vRes(1),vRes(2)];
  err_grid = get_combined_error(s_session,vResGrid,vRefDRR,vRefT60,IRlen,fs_ref);
  if nargout == 0
    disp('Grid search result:');
    disp(sprintf('  gain="%1.1f" absorption="%1.2f" damping="%1.3f" t60="0"',gain,vRes(1),vRes(2)));
    disp(sprintf('  (error: T60:%g DRR:%g)',e1,vE(idx)));
  end
  % refine parameters:
  [vResDRR,e] = fminsearch(@(x) get_drr_error(s_session,x,vRefDRR,IRlen,fs_ref,vRes(1),vRes(2)),gain);
  [vRes,e] = fminsearch(@(x) get_combined_error(s_session,x,vRefDRR,vRefT60,IRlen,fs_ref),[vResDRR,vRes]);
  if e > err_grid
    disp('no convergence, using grid results');
    e = err_grid;
    vRes = vResGrid;
  end
  vRes(2:3) = apply_damp_abs_constraints( vRes(2:3) );
  if nargout == 0
    disp('Optimization result:');
    disp(sprintf('  gain="%1.1f" absorption="%1.2f" damping="%1.3f" t60="0"',vRes(1),vRes(2),vRes(3)));
    disp(sprintf('  (error: %g)',e));
    [ir,fs] = tascar_renderir_reverbsettings(s_session, IRlen,fs_ref,'gain',vRes(1),'absorption',vRes(2),'damping',vRes(3),'t60',0);
    disp(sprintf('  T60 = [ %s] s',sprintf('%1.2f ',10.^(get_t60_features(ir,fs)/40))));
    disp(sprintf('  DRR = %1.1f dB',get_drr_features(ir,fs)));
    disp(sprintf('  T60_ref = [ %s] s',sprintf('%1.2f ',10.^(vRefT60/40))));
    disp(sprintf('  DRR_ref = %1.1f dB',vRefDRR));
    clear vRes;
  end
end

function vFeat = get_t60_features( ir, fs )
  vFeat = [];
  %for band=1:4
  for band=1:floor(log2(0.5*fs/125))
    vF = 125*2.^(band+[0:1]-0.5);
    [B,A] = butter(2,vF/(0.5*fs));
    irf = filter(B,A,ir);
    t60late = t60(irf,fs,-20,-15,-0.2,2000);
    vFeat(end+1) = 40*log10(mean(t60late));
  end
end

function vFeat = get_drr_features( ir, fs )
  vFeat = mean(drr(ir,fs));
end

function e = get_t60_error( s_session, vPar, vRefT60, irlen, fs, gain )
  try
    [vPar,e0] = apply_damp_abs_constraints( vPar );
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',gain,'absorption',vPar(1),'damping',vPar(2),'t60',0);
    vFeat = get_t60_features( ir, fs );
    e = sum((vFeat-vRefT60).^2) + e0;
  catch
    vPar
    disp(lasterr)
    e = 1e6;
  end
end

function e = get_drr_error( s_session, vPar, vRefDRR, irlen, fs, absorption, damping )
  try
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',vPar,'absorption',absorption,'damping',damping,'t60',0);
    vFeat = get_drr_features( ir, fs );
    e = sum((vFeat-vRefDRR).^2);
  catch
    vPar
    disp(lasterr)
    e = 1e6;
  end
end

function e = get_combined_error( s_session, vPar, vRefDRR, vRefT60, irlen, fs )
  try
    [vPar(2:3),e0] = apply_damp_abs_constraints( vPar(2:3) );
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',vPar(1),'absorption',vPar(2),'damping',vPar(3),'t60',0);
    vFeatDRR = get_drr_features( ir, fs );
    vFeatT60 = get_t60_features( ir, fs );
    e = sum((vFeatDRR-vRefDRR).^2) + sum((vFeatT60-vRefT60).^2)+e0;
  catch
    disp(lasterr)
    e = 1e6;
  end
end

function b = isoctave()
  b = ~isempty(ver('Octave'));
end

function [res,e] = apply_damp_abs_constraints( res )
  e = 0;
  for k=1:numel(res)
    if res(k) < 0.001
      e = e+1;
      res(k) = 0.001;
    end
    if res(k) > 0.999
      e = e+1;
      res(k) = 0.999;
    end
  end
end
