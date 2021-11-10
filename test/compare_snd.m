function compare_snd( testno, ch )
% compare_snd( testno, ch )
  x = audioread(sprintf('expected_%s.wav',testno));
  y = audioread(sprintf('test_%s.wav',testno));
  if nargin > 1
      x = x(:,ch);
      y = y(:,ch);
  end
  plot([x,y]);
  legend({'expected','test'});
  
