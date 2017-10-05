function compare_snd( testno )
  x = audioread(sprintf('expected_%s.wav',testno));
  y = audioread(sprintf('test_%s.wav',testno));
  plot([x,y]);
  legend({'expected','test'});
  