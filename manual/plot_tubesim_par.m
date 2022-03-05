function plot_tubesim_par

  t1 = [0:0.001:1];
  y1 = 0.1*sin(2*pi*t1);
  tmax = 1;
  t2 = [0:(1/40000):tmax];
  y2 = 0.1*sin(2*pi*1000*t2);
  Y2 = realfft(y2');
  f2 = [0:(numel(Y2)-1)]/tmax;
  x = [-1:0.001:1]*0.1;
  x0 = [0:0.05:0.25];
  s = 10.^(0.05*[-40:10:0]);
  fh = figure();
  %%
  subplot(2,3,1);
  vcol = [];
  for x0_ = x0
    h = plot(x, tubesim(x,x0_,s(3)));
    %if x0_ == x0(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    vcol(end+1,:) = get(h,'Color');
    hold on;
  end
  ylim([-0.45,0.45]);
  xlabel('input');
  ylabel('output');
  h = legend(num2str(x0'),'Location','NorthWest');
  set(h,'fontsize',6);
  p = get(h,'Position');
  p = [p(1),p(2)+0.4*p(4),p(3),0.6*p(4)];
  set(h,'Position',p);
  %%
  subplot(2,3,2);
  for x0_ = x0
    h = plot(t1, tubesim(y1,x0_,s(3)));
    %if x0_ == x0(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    hold on;
  end
  ylim([-0.45,0.45]);
  xlabel('input');
  ylabel('output');
  title('offset varied');
  %%
  subplot(2,3,3);
  k = 0;
  for x0_ = x0
    k = k+1;
    Y = 20*log10(abs(realfft(tubesim(y2',x0_,s(3)))));
    Y = Y - max(Y(2:end));
    f = f2*(1+(k-3)*0.02);
    h = plot(f,Y, 'Color',vcol(k,:) );
    hold on;
    idx = 1000*[1:16]+1;
    plot(f(idx), Y(idx),'o', 'Color',vcol(k,:) );
    %if x0_ == x0(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    hold on;
  end
  ylim([-80,5]);
  xlim([900,10000]);
  set(gca,'XScale','log','XTick',1000*2.^[1:3],...
      'XTickLabel',num2str(2.^[1:3]'));
  xlabel('frequency / kHz');
  ylabel('output / dB');
  %%
  subplot(2,3,4);
  for s_ = s
    h = plot(x, tubesim(x,x0(3),s_));
    %if s_ == s(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    hold on;
  end
  ylim([-0.65,0.45]);
  xlabel('input');
  ylabel('output');
  h = legend(num2str(20*log10(s')),'Location','SouthEast');
  set(h,'fontsize',6);
  p = get(h,'Position');
  p = [p(1),p(2),p(3),0.6*p(4)];
  set(h,'Position',p);
  %%
  subplot(2,3,5);
  for s_ = s
    h = plot(t1, tubesim(y1,x0(3),s_));
    %if s_ == s(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    hold on;
  end
  ylim([-0.65,0.45]);
  xlabel('input');
  ylabel('output');
  title('saturation varied');
  %%
  subplot(2,3,6);
  k = 0;
  for s_ = s
    k = k+1;
    Y = 20*log10(abs(realfft(tubesim(y2',x0(3),s_))));
    Y = Y - max(Y(2:end));
    f = f2*(1+(k-3)*0.02);
    h = plot(f,Y, 'Color',vcol(k,:) );
    hold on;
    idx = 1000*[1:16]+1;
    plot(f(idx), Y(idx),'o', 'Color',vcol(k,:) );
    %if s_ == s(3)
    %  set(h,'Color',[0,0,0],'linewidth',1.5);
    %end
    hold on;
  end
  ylim([-80,5]);
  xlim([900,10000]);
  set(gca,'XScale','log','XTick',1000*2.^[1:3],...
      'XTickLabel',num2str(2.^[1:3]'));
  xlabel('frequency / kHz');
  ylabel('output / dB');
  %%
  saveas(gcf,'tubesim.eps','epsc');
  system('epstopdf tubesim.eps');
end

function y = ovdrv( x, s )
  y = x./(x+s);
end

function y = tsim( x, x0 )
  y = max(x+x0,0).^1.5;
end

function y = tubesim( x, x0, s )
  y = ovdrv(tsim(x,x0),s)-ovdrv(tsim(0,x0),s);
end

function y = realfft( x )
  fftlen = size(x,1);
  y = fft(x);
  y = y([1:floor(fftlen/2)+1],:);
end
