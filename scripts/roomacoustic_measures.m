function [t60late,t60early,r] = roomacoustic_measures( irs, fs )
  [t60late,t60early] = t60( irs, fs );
  r = drr( irs, fs );