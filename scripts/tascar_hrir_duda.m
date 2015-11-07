function tascar_hrir_duda( varargin )
% tascar_hrir_duda - create head model HRIR for use in TASCAR.
%
% tascar_hrir_duda( [ param, value ] )
%
% Default values:
%  - fs = 44100;
%  - fftlen = 1024;
%  - n = 8;
%  - radius = 0.08;
%  - nbits = 32;
%  - basename = '';
%  - fmax = 20000;
%  - fmin = 4000;
%
% Reference:
%
% Duda, Richard O. "Modeling head related transfer functions."
% Signals, Systems and Computers, 1993. 1993 Conference Record of The
% Twenty-Seventh Asilomar Conference on. IEEE, 1993.
%
% Implementation with a corrected ITD and an additional simple pinna
% model.  
  
  sCfg = struct();
  sCfg.fs = 44100;
  sCfg.fftlen = 1024;
  sCfg.n = 8;
  sCfg.radius = 0.08;
  sCfg.nbits = 32;
  sCfg.basename = '';
  sCfg.fmax = 20000;
  sCfg.fmin = 4000;
  if mod(numel(varargin),2)
    error('Even number of input arguments expected.');
  end
  for k=1:2:numel(varargin)
    if ~ischar(varargin{k})
      error('parameter name is not a character array.');
    end
    name = lower(varargin{k});
    if ~isfield(sCfg,name)
      error(['invalid parameter name "',name,'".']);
    end
    sCfg.(name) = varargin{k+1};
  end
  if isempty(sCfg.basename)
    sCfg.basename = sprintf('hrir_duda_n%d_r%d_%g_%g',...
			    sCfg.n,round(1000*sCfg.radius),...
			    sCfg.fmin,sCfg.fmax);
  end
  fh_spk = fopen([sCfg.basename,'.spk'],'w');
  fh_tsc = fopen([sCfg.basename,'.tsc'],'w');
  fprintf(fh_spk,'<?xml version="1.0" encoding="UTF-8"?>\n');
  fprintf(fh_tsc,'<?xml version="1.0" encoding="UTF-8"?>\n');
  fprintf(fh_tsc,'<session name="hrir_template">\n  <scene name="example">\n');
  fprintf(fh_tsc,'    <receiver name="out" type="vbap" layout="%s.spk"/>\n',...
	  sCfg.basename);
  fprintf(fh_tsc,'    <src_object name="src"><sound name="0" x="1"/></src_object>\n');
  fprintf(fh_tsc,'  </scene>\n');
  fprintf(fh_spk,'<layout label="%s">\n',sCfg.basename);
  fprintf(fh_tsc,...
	  '  <module name="hrirconv" fftlen="%d" inchannels="%d" outchannels="2" autoconnect="true">\n',...
	  sCfg.fftlen,sCfg.n);
  ir = [];
  for k=1:sCfg.n
    az = (k-1)/sCfg.n*360;
    fprintf(fh_spk,'  <speaker az="%g"/>\n',az);
    for ko=0:1
      fprintf(fh_tsc,'    <entry in="%d" out="%d" file="%s.wav" channel="%d"/>\n',...
	      k-1,ko,sCfg.basename,(1-ko)*(k-1)+ko*mod(sCfg.n-(k-1),sCfg.n));
    end
    ir = [ir,0.5*duda_model(sCfg.fs,sCfg.fftlen,sCfg.radius,az,sCfg.fmin,sCfg.fmax)];
  end
  fprintf(fh_tsc,'  </module>\n</session>\n');
  fprintf(fh_spk,'</layout>\n');
  fclose(fh_spk);
  fclose(fh_tsc);
  wavwrite(ir,sCfg.fs,sCfg.nbits,[sCfg.basename,'.wav']);
  
  
function irs = duda_model( fs, fftlen, a, az, fmin, fmax )
  az0 = 0;
  az0 = az0*pi/180;
  az = az*pi/180;
  c = 340;
  binlen = floor(fftlen/2)+1;
  vF = ([1:binlen]'-1)/fftlen*fs;
  vW = 2*pi*vF;
  alpha = 0.5*(1+sin(az));
  tau = 2.0*a/c;
  Tr = (1-alpha)*tau;
  Tl = alpha*tau;
  H = (1+i*2*alpha*vW*tau)./(1+i*vW*tau).*exp(-i*vW*Tr);
  % extremely simple pinna model, not original:
  kBin = round(((fmax-fmin)*(0.5+0.5*cos(az-az0))+fmin)*fftlen/fs);
  vBin = kBin:binlen;
  vGain = 10.^(0.05*6*log2(vF(kBin)./vF(vBin)));
  H(vBin) = H(vBin).*vGain;
  % end pinna model
  irs = circshift(realifft(H),floor(fftlen/2));
  
  