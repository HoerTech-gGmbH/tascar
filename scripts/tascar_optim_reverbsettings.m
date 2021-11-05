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
% T60, i50, i250, drr
  if isoctave()
    %% on octave make sure that signal package is loaded:
    pkg('load','signal');
  end
  vRefFeat = get_features( ir_ref, fs_ref );
  vGain = [-20:10:20];
  vAbsorption = [0.25:0.25:1];
  vDamping = [0.3,0.6,0.9];
  vRes = [];
  vE = [];
  for g=vGain
    for a=vAbsorption
      for d=vDamping
        vE(end+1,:) = get_error( s_session, [g,a,d], vRefFeat, size(ir_ref,1), fs_ref );
        vRes(end+1,:) = [g,a,d];
      end
    end
  end
  [tmp,idx] = min(vE);
  vRes = vRes(idx,:);
  [vRes,e] = fminsearch(@(x) get_error(s_session,x,vRefFeat,size(ir_ref,1),fs_ref),vRes);
  if nargout == 0
    disp('Optimization result:');
    disp(sprintf('  gain="%g" absorption="%g" damping="%g"',vRes(1),vRes(2),vRes(3)));
    disp(sprintf('  (error: %g)',e));
    [ir,fs] = tascar_renderir_reverbsettings(s_session, size(ir_ref,1),fs_ref,'gain',vRes(1),'absorption',vRes(2),'damping',vRes(3));
    [t60late,edt,i50,i250] = t60(ir,fs,-30,-10,-0.2,2000);
    disp(sprintf('  T60 = %g',t60late));
    disp(sprintf('  DRR = %g',drr(ir,fs)));
    [t60late,edt,i50,i250] = t60(ir_ref,fs_ref,-30,-10,-0.2,2000);
    disp(sprintf('  T60_ref = %g',t60late));
    disp(sprintf('  DRR_ref = %g',drr(ir_ref,fs_ref)));
    clear vRes;
  end
end

function vFeat = get_features( ir, fs )
  [B1,A1] = butter(1,[250,1000]/(0.5*fs));
  [B2,A2] = butter(1,[1000,4000]/(0.5*fs));
  [t60late,edt,i50,i250] = t60([ir,filter(B1,A1,ir),filter(B2,A2,ir)],fs,-30,-10,-0.2,2000);
  vFeat = [40*log10(t60late),i50,i250,drr(ir,fs)];
end

function e = get_error( s_session, vPar, vRefFeat, irlen, fs )
  try
    [ir,fs] = tascar_renderir_reverbsettings(s_session, irlen,fs,'gain',vPar(1),'absorption',vPar(2),'damping',vPar(3));
    vFeat = get_features( ir, fs );
    e = sum((vFeat-vRefFeat).^2);
  catch
    e = 1e6;
  end
end

function b = isoctave()
  b = ~isempty(ver('Octave'));
end
