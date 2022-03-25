function compare_snd( testno, ch )
% compare_snd( testno, ch )
  x = audioread(sprintf('expected_%s.wav',testno));
  y = audioread(sprintf('test_%s.wav',testno));
  if nargin > 1
      x = x(:,ch);
      y = y(:,ch);
  end
  rg1 = [0,max(size(x,1),size(y,1))];
  idx = 1;
  tmax = 0;
  for k=1:size(y,2)
    [tmp,tidx] = max(abs(x(:,k)-y(:,k)));
    if tmp>tmax
      idx = tidx;
    end
  end
  figure();
  rg2 = [-100,100]+idx;
  ax1 = subplot(2,1,1);
  h = plot([x,y]);
  nch = numel(h)/2;
  idx = ones(nch,1)*[1,2];
  idx = idx(:);
  lab = {'expected','test'};
  legend(lab(idx));
  ax2 = subplot(2,1,2);
  plot(x-y);
  sData = struct;
  sData.ax = [ax1,ax2];
  sData.rg1 = rg1;
  sData.rg2 = rg2;
  set(gcf,'UserData',sData);
  uicontrol('Units','normalized','style','pushbutton','position',[0.6,0,0.15,0.05],...
            'Callback',@zoomin,'string','++');
  uicontrol('Units','normalized','style','pushbutton','position',[0.8,0,0.15,0.05],...
            'Callback',@zoomout,'string','[ ]');
end

function zoomin(varargin)
  sData = get(gcf,'UserData');
  set(sData.ax,'XLim',sData.rg2);
end

function zoomout(varargin)
  sData = get(gcf,'UserData');
  set(sData.ax,'XLim',sData.rg1);
end
