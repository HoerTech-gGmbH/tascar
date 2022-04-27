function [B,A] = tascar_param_eq( f, fs, gain, q)
% tascar_param_eq - parametric equalizer (one biquad)
%
% Usage:
% [B,A] = tascar_param_eq( f, fs, gain, q)
  
  ;
  %% bilinear transformation
  t = 1.0 / tan(pi * f / fs);
  t_sq = t * t;
  Bc = t / q;
  if(gain < 0.0) 
    g = 10.0^(-gain / 20.0);
    inv_a0 = 1.0 / (t_sq + 1.0 + g * Bc);
    A = [1, 2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - g * Bc) * inv_a0];
    B = [(t_sq + 1.0 + Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,...
         (t_sq + 1.0 - Bc) * inv_a0];
  else
    g = 10.0^(gain / 20.0);
    inv_a0 = 1.0 / (t_sq + 1.0 + Bc);
    A = [1, 2.0 * (1.0 - t_sq) * inv_a0, (t_sq + 1.0 - Bc) * inv_a0];
    B = [(t_sq + 1.0 + g * Bc) * inv_a0, 2.0 * (1.0 - t_sq) * inv_a0,...
         (t_sq + 1.0 - g * Bc) * inv_a0];
  end
end
