function [r,d,alpha] = tascar_alpha2rflt( alpha, freq, fs )
% tascar_alpha2rflt - search for optimal reflector filter coefficients
%
% Usage:
% [r,d,alpha] = tascar_alpha2rflt( alpha, freq, fs )
%
% alpha : absorption coefficients
% freq : absorption coefficient frequencies / Hz
% fs : sampling rate / Hz
%
% Return values:
% r : reflector 'reflectivity'
% d : reflector 'damping'
% alpha : resulting absorption coefficients

  vPar = [0.5,0.5];
  vPar = fminsearch( ...
      (@(x) absorptionerror( x, alpha, freq, fs )), vPar );
  d = exp(-vPar(1).^2);
  r = exp(-vPar(2).^2);
  alpha = tascar_rflt2alpha( r, d, fs, freq );
  
  
function e = absorptionerror( vPar, alpha, freq, fs )
  d = exp(-vPar(1).^2);
  r = exp(-vPar(2).^2);
  alphatest = tascar_rflt2alpha( r, d, fs, freq );
  e = mean((alpha(:)-alphatest(:)).^2);
