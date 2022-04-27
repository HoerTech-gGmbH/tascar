function [vF,vG,vQ] = tascar_optim_param_eq( vF, vG, fs )
  f = vF;
  g = vG;
  q = 0.5*ones(size(vF));
  spec = vfg2spec(vF,vG,fs);
  par = fgq2par(f,g,q,fs);
  errfun( par, spec, fs )
  par = fminsearch(@(x) errfun(x,spec,fs), par );
  errfun( par, spec, fs )
  [vF,vG,vQ] = par2fgq(par,fs);
  plot([spec,fgq2spec( vF,vG,vQ, fs )]);
  xt = 1000*2.^[-3:3];
  set(gca,'XLim',[50,20000],'XScale','log','XTick',xt,'XTickLabel',num2str(xt'))
end

function s = vfg2spec( vF, vG, fs )
  vF = [0,vF,fs];
  vG = [vG(1),vG,vG(end)];
  nbins = round(fs/2+1);
  f = [0:(nbins)-1]';
  s = interp1(log(max(1,vF')),vG',log(max(1,f)),'cubic');
end

function par = fgq2par( f, g, q, fs )
  par = [tan((2*f/fs-0.5)*pi),g,tan((q-0.5)*pi)];
end

function [f,g,q] = par2fgq( par, fs )
  N = numel(par)/3;
  f = (atan(par(1:N))/pi+0.5)*0.5*fs;
  g = par(N+[1:N]);
  q = atan(par(2*N+[1:N]))/pi+0.5;
end

function x = filter_fgq( x, f, g, q, fs )
  for k=1:numel(f)
    [B,A] = tascar_param_eq( f(k), fs, g(k), q(k));
    x = filter(B,A,x);
  end
end

function s = fgq2spec( f, g, q, fs )
  dirac = zeros(fs,1);
  dirac(1) = 1;
  s = 20*log10(abs(realfft( filter_fgq( dirac, f, g, q, fs) )));
end

function e = errfun( par, spec, fs )
  [f,g,q] = par2fgq( par, fs );
  H = fgq2spec(f,g,q,fs);
  idx = round(1000*2.^[-4:1/6:4]);
  e = (H(idx)-spec(idx)).^2;
  e = (mean(e(:)));
end
