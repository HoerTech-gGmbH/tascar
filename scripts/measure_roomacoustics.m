function [t60late,t60early,r,irs] = ...
    measure_roomacoustics( srcport, leftport, rightport, len )
  
  [irs,fs] = tascar_ir_measure('output',{srcport},'len',len,'nrep',1,...
			       'gain',0.02,'input',{leftport,rightport});
  [t60late,t60early,r] = roomacoustic_measures(irs,fs);
  