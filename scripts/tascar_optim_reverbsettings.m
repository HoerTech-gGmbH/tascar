function vRes = tascar_optim_reverbsettings( s_session, ir_ref, fs_ref )
% tascar_optim_reverbsettings - optimize simplefdn settings
%
% Usage:
% tascar_optim_reverbsettings( s_session, ir_ref, fs_ref )
%
% s_session: name of TASCAR session file
% ir_ref: reference impulse response. Number of channels must match number of channels of receiver
% fs_ref: sampling rate of reference impulse response
%
% optimization criterion:
% T60 in four bands (250-500,500-1000,1000-2000,2000-4000), DRR
%
% Method:
% 1. At 10 dB gain, find optimal absorption and damping based on T60 error using an interative grid search
% 2. Find optimal gain based on DRR error, using a grid search
% 3. Refine gain using fminsearch and DRR error
% 4. Refine all parameters using fminsearch and a combined error

  if isoctave()
    %% on octave make sure that signal package is loaded:
    pkg('load','signal');
  end
  %% get features of reference IR:
  vRefT60 = get_t60_features( ir_ref, fs_ref );
  vRefDRR = get_drr_features( ir_ref, fs_ref );
  % search grid for absorption and damping:
  vAbsorption = [0.14:0.05:0.99];
  vDamping = [0.14:0.05:0.99];
  vRes = [];
  vE = [];
  for a=vAbsorption
    for d=vDamping
      vE(end+1,:) = get_t60_error( s_session, [a,d], vRefT60, size(ir_ref,1), fs_ref, 10 );
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
      vE(end+1,:) = get_t60_error( s_session, [a,d], vRefT60, size(ir_ref,1), fs_ref, 10 );
      vRes(end+1,:) = [a,d];
    end
  end
  [tmp,idx] = min(vE);
  vRes = vRes(idx,:);
  vRes = apply_damp_abs_constraints( vRes );
  e1 = vE(idx);
  % grid search for gain:
  vGain = -20:1:20;
  vE = [];
  for g=vGain
    vE(end+1) = get_drr_error(s_session,g,vRefDRR,size(ir_ref,1),fs_ref,vRes(1),vRes(2));
  end
  [tmp,idx] = min(vE);
  gain = vGain(idx);
  if nargout == 0
    disp('Grid search result:');
    disp(sprintf('  gain="%g" absorption="%g" damping="%g"',gain,vRes(1),vRes(2)));
    disp(sprintf('  (error: T60:%g DRR:%g)',e1,vE(idx)));
  end
  % refine parameters:
  [vResDRR,e] = fminsearch(@(x) get_drr_error(s_session,x,vRefDRR,size(ir_ref,1),fs_ref,vRes(1),vRes(2)),gain);
  [vRes,e] = fminsearch(@(x) get_combined_error(s_session,x,vRefDRR,vRefT60,size(ir_ref,1),fs_ref),[vResDRR,vRes]);
  vRes(2:3) = apply_damp_abs_constraints( vRes(2:3) );
  if nargout == 0
    disp('Optimization result:');
    disp(sprintf('  gain="%g" absorption="%g" damping="%g"',vRes(1),vRes(2),vRes(3)));
    disp(sprintf('  (error: %g)',e));
    [ir,fs] = tascar_renderir_reverbsettings(s_session, size(ir_ref,1),fs_ref,'gain',vRes(1),'absorption',vRes(2),'damping',vRes(3));
    10.^(get_t60_features(ir,fs)/40)
    [t60late,edt,i50,i250] = t60(ir,fs,-30,-10,-0.2,2000);
    disp(sprintf('  T60 = %g',t60late));
    disp(sprintf('  DRR = %g',drr(ir,fs)));
    10.^(get_t60_features(ir_ref,fs_ref)/40)
    [t60late,edt,i50,i250] = t60(ir_ref,fs_ref,-30,-10,-0.2,2000);
    disp(sprintf('  T60_ref = %g',t60late));
    disp(sprintf('  DRR_ref = %g',drr(ir_ref,fs_ref)));
    clear vRes;
  end
end

function vFeat = get_t60_features( ir, fs )
  vFeat = [];
  for band=1:4
    vF = 125*2.^(band+[0:1]);
    [B,A] = butter(1,vF/(0.5*fs));
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
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',gain,'absorption',vPar(1),'damping',vPar(2));
    vFeat = get_t60_features( ir, fs );
    e = sum((vFeat-vRefT60).^2) + e0;
  catch
    disp(lasterr)
    e = 1e6;
  end
end

function e = get_drr_error( s_session, vPar, vRefDRR, irlen, fs, absorption, damping )
  try
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',vPar,'absorption',absorption,'damping',damping);
    vFeat = get_drr_features( ir, fs );
    e = sum((vFeat-vRefDRR).^2);
  catch
    disp(lasterr)
    e = 1e6;
  end
end

function e = get_combined_error( s_session, vPar, vRefDRR, vRefT60, irlen, fs )
  try
    vPar(2:3) = apply_damp_abs_constraints( vPar(2:3) );
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',vPar(1),'absorption',vPar(2),'damping',vPar(3));
    vFeatDRR = get_drr_features( ir, fs );
    vFeatT60 = get_t60_features( ir, fs );
    e = sum((vFeatDRR-vRefDRR).^2) + sum((vFeatT60-vRefT60).^2);
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
