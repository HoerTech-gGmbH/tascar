function [d,az,el,ratio] = tascar_validate_foa_irs( ir, fs )
% tascar_validate_foa_irs - check if an impulse response matches B-format structure
%
% Usage:
% tascar_validate_foa_irs( ir, fs )

  if size(ir,2) ~= 4
    error(['Impulse response has ',num2str(size(ir,2)),' channels, B-Format requires 4']);
  end
  if isoctave()
    %% on octave make sure that signal package is loaded:
    pkg('load','signal');
  end
  %% speed of sound:
  c = 340;
  %% averaging time in seconds:
  l = 0.003;
  [B,A] = butter(1,[80,500]/(0.5*fs));
  ir = filter(B,A,ir);
  %% find peak of direct sound:
  idx = irs_firstpeak( ir );
  if c*std(idx)/fs > 0.2
    error(['The standard deviation of first peak (',num2str(c*std(idx)/fs),'m) is larger than a typical microphone diameter']);
  end
  idx = round(median(idx));
  d = c*idx/fs;
  %% window position for estimation of direct sound direction:
  widx = unique(max(1,[-round(l*fs):round(l*fs)]+idx));
  %% select direct path, with von-Hann window:
  ir = ir(widx,:) .* repmat( hann(numel(widx)), [1,4] );
  %% create intensity-weighted average:
  w = ir(:,1).^2;
  %% avoid division by zero:
  ir(find(w==0),1) = 1;
  %% estimate ratio between w and xyz
  wxyzrat = abs(ir(:,1))./sqrt(max(1e-9,sum(ir(:,2:4).^2,2)));
  wxyzrat_mean = sum(w.*wxyzrat)./sum(w);
  if abs(wxyzrat_mean-sqrt(0.5)) > 0.05
    error(['The ratio between w and xyz is ',num2str(wxyzrat_mean),', which is not 0.70711 (FuMa normalization)']);
  end
  %% estimate element-wise direction of arrival:
  doa = ir(:,2:4) ./ repmat(ir(:,1),[1,3]);
  %% calculate weighted average:
  doa_mean = sum(repmat(w,[1,3]).*doa)./sum(w);
  [th,phi,r] = cart2sph(doa_mean(1),doa_mean(2),doa_mean(3));
  az = 180/pi*th;
  el = 180/pi*phi;
  ratio = wxyzrat_mean;
  if nargout == 0
    disp('Estimated parameters:');
    disp(sprintf('  distance = %1.2f m',d));
    disp(sprintf('  azimuth = %1.1f deg',az));
    disp(sprintf('  elevation = %1.1f deg',el ));
    disp(sprintf('  w/xyz = %1.3f', ratio));
    clear d;
  end
end

function b = isoctave()
  b = ~isempty(ver('Octave'));
end
