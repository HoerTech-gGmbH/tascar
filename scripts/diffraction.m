function diffraction( a )
% DIFFRACTION - simple diffraction model (airy disk)
%
% This is the prototype implementation for TASCAR.
%
% Author:
% Giso Grimm, Joanna Luberadzka
% 1/2015
  
  if nargin < 1
    a = 1;
  end
  figure
  theta = [-pi/2:0.001:pi/2];
  f = 500;
  w = airy(f,a,theta);
  plot(180/pi*theta,w,'b-');
  hold on;
  f = 1000;
  w = airy(f,a,theta);
  plot(180/pi*theta,w,'r-');
  xlabel('angular distance from axis / deg');
  ylabel('intensity');
  legend({'500 Hz','1000 Hz'});
  saveas(gcf,'diffraction_intensity.eps','epsc');
  figure
  plot_f( a, [30,45,85]);
  saveas(gcf,'diffraction_filter.eps','epsc');

  
function h = plot_f( a, vdeg )
  c = 340;
  f = [65:1:20000]';
  theta = vdeg*pi/180;
  w = airy(f,a,theta);
  h = plot(f,10*log10(w));
  hold on;
  f0 = 3.8317*c./(2*pi*a*sin(theta))
  m = 20*log10(2*sqrt(2));
  g0 = -13;
  plot(f,g0-m*log2(f*(1./f0)));
  plot([f0;f0],[-90,0],'k-');
  k = 1;
  for vf0=f0
    col = get(h(k),'Color');
    plot(f,lpf(f,vf0),'--','linewidth',2,'Color',col);
    k = k+1;
  end
  set(gca,'XScale','log','XTick',1000*2.^[-3:5],...
	  'XLim',[min(f),max(f)],'YLim',[-60,0]);
  legend(num2str(vdeg'));
  xlabel('frequency / Hz');
  ylabel('gain / dB');
  
function H = lpf( f, f0 )
  fs = 44100;
  A = [1,-exp(-pi*f0/fs)];
  B = 1+A(2);
  %[B,A] = butter(1,0.5*f0/fs)
  B
  A
  irs = zeros(fs,1);
  irs(1) = 1;
  irs = filter(B,A,irs);
  irs = filter(B,A,irs);
  cf = zeros(1+round(0.5*fs/f0),1);
  cf(1) = 1;
  cf(end) = 1;
  %irs = filter(0.5*cf,1,irs);
  H = 20*log10(abs(fft(irs)));
  H = H(round(f)+1);
  
  
function w = airy( f, a, theta )
  c = 340;
  k = 2*pi*f/c;
  x = k*a*sin(theta);
  idx = find(x==0);
  x(idx) = x(idx)+eps;
  w = abs(2*besselj(1,x)./x).^2;
  