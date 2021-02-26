function g = diffgain( method, ref_irs, sPortSrc, csPortMic, sDiffName, bVerbose)
% DIFFGAIN - adaptively find optimal diffuse reverb gain
%
% The gain of a diffuse reverberation object in TASCAR is
% adaptively varied to create a minimal error compared to a
% binaural reference impulse response. The error is either
% estimated by the interaural cross correlation function (IACC) or
% by the integrated tone burst decay at 250ms.
%
% g = diffgain( method, ref_irs, sPortSrc, csPortMic, sDiffName, bVerbose )
%
% Parameters:
% - method    : 'iacc' or 'i250'
% - ref_irs   : reference impulse response
% - sPortSrc  : jack port name of TASCAR virtual sound source
% - csPortMic : cell array of port names of binaural output
% - sDiffName : OSC path of diffuse reverb object name in TASCAR
% Optional:
% - bVerbose  : print current gain and error during adaptation (false)
%
% Return value: diffuse gain with least error compared to reference
% IRS.
%
% The optimization requires a running TASCAR scene with the named
% objects. TASCAR must be configured to use OSC control on port
% 9999.
%
% Author: Giso Grimm
% Date: 5/2015

  if nargin < 6
    bVerbose = false;
  end
  send_osc('localhost',9999,[sDiffName,'/gain'],0);
  [test_irs,fs] = ...
      tascar_ir_measure('outout',{sPortSrc},...
			'len',2^16,...
			'nrep',1,...
			'gain',0.6,...
			'input',sPortMic);
  [t60l,t60e,i50,i250_ref] = t60(ref_irs,fs,-10);
  [t60l,t60e,i50,i250_test] = t60(test_irs,fs,-10);
  g0 = i250_ref(1)-i250_test(1);
  par = struct;
  par.sPortSrc = sPortSrc;
  par.sPortMic = csPortMic;
  par.sDiffName = sDiffName;
  par.i250_ref = i250_ref;
  par.iacctau = [0,0.25];
  par.iacc = iacc( ref_irs, fs, par.iacctau );
  par.verbose = bVerbose;
  switch method
   case 'i250'
    g = fminsearch(@(x) error_diffgain(x,par),g0);
   case 'iacc'
    g = fminsearch(@(x) error_iacc(x,par),g0);
   otherwise
    error(['invalid optimization method: ',method]);
  end
  
function e = error_diffgain( x, par )
  send_osc('localhost',9999,[par.sDiffName,'/gain'],x);
  [test_irs,fs] = ...
      tascar_ir_measure('outout',{par.sPortSrc},...
			'len',2^16,...
			'nrep',1,...
			'gain',0.6,...
			'input',par.sPortMic);
  [t60l,t60e,i50,i250_test] = t60(test_irs,fs,-10);
  g = par.i250_ref(1)-i250_test(1);
  e = g^2;
  if( par.verbose )
    disp([x,e]);
  end
    
function e = error_iacc( x, par )
  send_osc('localhost',9999,[par.sDiffName,'/gain'],x);
  [test_irs,fs] = ...
      tascar_ir_measure('outout',{par.sPortSrc},...
			'len',2^16,...
			'nrep',1,...
			'gain',0.6,...
			'input',par.sPortMic);
  c = iacc(test_irs,fs, par.iacctau);
  e = (c-par.iacc)^2;
  if( par.verbose )
    disp([x,e]);
  end
    