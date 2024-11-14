function [mgains,vf,X,ir] = spkgains2minphaseir( layoutname, fs, fftlen )
% spkgains2minphaseir
%
  doc = tascar_xml_open( layoutname );
  hspk = tascar_xml_get_element( doc, 'speaker' );
  csLabels = {};
  vgains = [];
  mgains = [];
  for k=1:numel(hspk)
    vf = str2num(tascar_xml_get_attribute( hspk{k}, 'eqfreq' ));
    vg = str2num(tascar_xml_get_attribute( hspk{k}, 'eqgain' ));
    l = char(tascar_xml_get_attribute( hspk{k}, 'label' ));
    mgains(end+1,:) = vg;
    if isempty(l)
      l = num2str(k);
    end
    csLabels{end+1} = l;
    g = str2num(tascar_xml_get_attribute( hspk{k}, 'gain' ));
    vgains(end+1) = g;
  end
  mgains = mgains';
  vf = vf';
  X = realfft(zeros(fftlen,1));
  nbins = numel(X);
  vflin = linspace(0,0.5*fs,nbins)';
  X = interp1([0;vf;fs],[mgains(1,:);mgains;mgains(end,:)],vflin,'linear','extrap');
  X = 10.^(0.05*X);
  plot(X)
  phase = log(max(1e-10,X));
  phase(end+1:fftlen,:) = 0;
  phase = -imag(hilbert(phase));
  X = X .* exp(i*phase(1:nbins,:));
  ir = realifft(X,fftlen);
end