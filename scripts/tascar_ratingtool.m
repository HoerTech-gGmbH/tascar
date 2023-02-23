function r = tascar_ratingtool( varargin )
% ratingtool - interface for mushra-like rating axes
%
% Usage:
% r = tascar_ratingtool()
% r = tascar_ratingtool('param','value')
% tascar_ratingtool('help')
%
% 7/2019 Giso Grimm

  sCfg.n = 3;
  sHelp.n = 'Number of test stimuli';
  sCfg.item = 'How blue is the sound of TASCAR?';
  sHelp.item = 'Question to be displayed';
  sCfg.label = {'white','','sky blue','','deep blue'};
  sHelp.label = 'Cell array with categories to be displayed';
  sCfg.labelcb = {[],[],[],[],[]};
  sHelp.labelcb = 'Cell array with function handles for each label';
  sCfg.completed = 'Ok';
  sHelp.completed = 'Label on the completed button';
  sCfg.tmin = 4;
  sHelp.tmin = ['Minimum duration of stimulus presentation before' ...
		            ' input is allowed'];
  sCfg.onselect = @donothing;
  sHelp.onselect = ['Callback to be called when a stimulus is selected. First' ...
		                ' input parameter is the stimulus number, second the' ...
		                ' user data.']';
  sCfg.oncompleted = @donothing;
  sHelp.oncompleted = ['Callback to be called when the rating was' ...
                       ' completed. First input parameter is the result,'...
                       ' second the user data.'];
  sCfg.onshow = @donothing;
  sHelp.onshow = 'Callback to be called when the window is shown on screen';
  sCfg.userdata = [];
  sHelp.userdata = 'Data to be passed as second parameter to callbacks';
  sCfg.windowname = 'TASCAR rating tool';
  sHelp.windowname = 'Name of window';
  sCfg.figurehandle = [];
  sHelp.figurehandle = 'Figure handle, or empty to create own window';
  sCfg.infotext = '';
  sHelp.infotext = 'Info text to displayed instead of input fields';
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  if ~isempty(sCfg.infotext)
      sCfg.n = 0;
  end

  csLabel = sCfg.label;
  csLabelCb = sCfg.labelcb;
  csLabelEmpty = csLabel;
  for k=1:numel(csLabelEmpty)
    csLabelEmpty{k} = '';
  end
  csLabelEmptyCb = csLabelCb;
  for k=1:numel(csLabelEmptyCb)
    csLabelEmptyCb{k} = [];
  end
  ssize = get(0,'ScreenSize');
  ssize = round(min(ssize(3:4))*[1.2,0.8]);
  ssize = [round(0.05*ssize),ssize];
  if isempty(sCfg.figurehandle) || ~ishandle(sCfg.figurehandle)
    fh = figure;
    close_fig = true;
  else
    fh = sCfg.figurehandle;
    close_fig = false;
    delete(findobj(fh,'-not','type','figure'));
  end
  set(fh,'Units','pixels','Position',ssize,'MenuBar','none',...
	       'Name',sCfg.windowname,...
	       'NumberTitle','off');
  vAx = [];
  vButton = [];

  sCfg.nfig = max(sCfg.n,2);

  if ~isempty(sCfg.infotext)
      h = uicontrol('style','text','string',sCfg.infotext,'Units',...
                    'normalized','Position', [0.05,0.05, ...
		                              1/(sCfg.nfig+3),0.8],...
                    'HorizontalAlignment','left',...
                    'FontSize',30,'ForegroundColor',[0,0,0]);
  end

  for k=1:sCfg.n

    % Bewertung der Stimuli
    vAx(k) = gui_rating_axes( csLabelEmpty, csLabelEmptyCb, ...
			                        'Position',[(k+0.1)/(sCfg.nfig+3), ...
		                                      0.2,0.8/(sCfg.nfig+3),0.7]);

    % Abspielbuttons
    vButton(k) = uicontrol('style','pushbutton','Units', ...
			                     'normalized','Position',[(k+0.1)/(sCfg.nfig+3), ...
		                                                0.05,0.8/(sCfg.nfig+3),0.1],'String',char(k+ ...
						                                                                               64), ...
			                     'FontSize',20,'UserData',k,'callback', ...
			                     @select_button,'value',0, ...
			                     'backgroundcolor',[0.9,0.9,0.9]);
    axes_set_active( vAx(k), false )
  end
  drawnow();
  sCfg.onshow( sCfg );

  set(vAx,'Xlim',[0,1.1]);

  % Rating labels:
  ax_labels = gui_rating_axes( csLabel, csLabelCb, ...
                               'Position', [(sCfg.nfig+1.1)/(sCfg.nfig+3), ...
		                                        0.2,1.8/(sCfg.nfig+3),0.7],'FontSize',20);

  axes_set_active(ax_labels,false);
  set(ax_labels,'XLim',[-1,20]);

  % Label of rating item:
  hLab = uicontrol('style','text','string',sCfg.item,'Units', ...
		               'normalized','Position',[0.02,0.9,0.96,0.08],'FontSize',32,'HorizontalAlignment','left');
  sUserData = struct;

  % Measurement completed button:
  sUserData.completed = uicontrol('style','pushbutton','Units', ...
				  'normalized','Position',[(sCfg.nfig+1.1)/(sCfg.nfig+3), ...
		                                           0.05,1.8/(sCfg.nfig+3),0.1],'String',sCfg.completed, ...
				  'FontSize',20,'UserData',k,'callback',@rating_completed,'value',0, ...
				  'backgroundcolor',[0.9,0.9,0.9],'enable','off');
  if ~isempty(sCfg.infotext)
      set(sUserData.completed,'enable','on');
  end
  
  %%% Label containing remaining time:
  h_timerlab = uicontrol('style','text','string','','Units',...
			                   'normalized','Position', [0.0,0.05, ...
		                                               1/(sCfg.nfig+3),0.1],'FontSize',30,'ForegroundColor',[0.5,0,0]);

  sUserData.on = 0;
  sUserData.vax = vAx;
  sUserData.vbutton = vButton;
  sUserData.cfg = sCfg;
  sUserData.results = nan*zeros(1,sCfg.n);
  sUserData.timerlab = h_timerlab;
  set(fh,'UserData',sUserData);

  drawnow();

  uiwait(fh);

  if ishandle(fh)
    sUserData = get(fh,'UserData');
    if close_fig
      delete(fh);
    else
      delete(findobj(fh,'-not','type','figure'));
    end
  end
  r = sUserData.results;
end

function rating_completed( varargin )
  fh = gcbf();
  sUserData = get(fh,'UserData');
  pause(0.01)
  if ~any(isnan(sUserData.results))
    uiresume(fh);
  end
  sUserData.cfg.oncompleted( sUserData.results, sUserData.cfg.userdata );
end

function select_button( varargin )
  fh = gcbf();
  kButton = get(gcbo,'UserData');
  sUserData = get(fh,'UserData');
  for k=1:sUserData.cfg.n
    bActive = k==kButton;
    if(k==kButton)
      set(fh,'UserData',sUserData);
    end
    axes_set_active( sUserData.vax(k),bActive);
    set(sUserData.vbutton(k),'value',bActive,'backgroundcolor',bActive*[0.2,0.8,0.2]+(1-bActive)*[0.9,0.9,0.9]);
  end
  sUserData.cfg.onselect( kButton, sUserData.cfg.userdata );
end

function ax = gui_rating_axes( csLabel, csLabelCb, varargin )
  ylim = [0,length(csLabel)-1];
  dq = 0.03*diff(ylim);
  ax = axes('Visible','on','NextPlot','ReplaceChildren',...
	          'Clipping','off',...
	          'XLim',[0 5],'Ylim',ylim-[dq -dq],...
	          'Box','on','XTick',[],'YTick',[],...
	          'Color',0.9*ones(1,3),...
	          varargin{:});
  q = ylim(1)-ylim(2);
  vhsel = [];
  vhsel(end+1) = ...
      patch([0.05 0.05 1.05 1.05],[ylim(1) ylim(2) ylim(2) ylim(1)],ones(1,3));
  hold on;
  fontsize = get(ax,'FontSize');
  for k=1:length(csLabel)
    %y = (k)/(length(csLabel)+1)*diff(ylim);
    y = (k-1)/(length(csLabel)-1)*diff(ylim);
    if( (k<=numel(csLabelCb)) && (~isempty(csLabelCb{k})) )
      pax = getpixelposition(ax);
      uicontrol('Units','pixel','style','PushButton','String',csLabel{k},...
                'HorizontalAlignment','left',...
                'Position',[pax(1)+0.15*pax(3),pax(2)+0.03*(pax(4)-0.4*pax(4)/length(csLabel))+0.94*(pax(4)-0.4*pax(4)/length(csLabel))*y/diff(ylim),...
                            0.8*pax(3),0.4*pax(4)/length(csLabel)],...
	              'Fontsize',fontsize,'Fontweight','bold',...
                'Callback',@labelcb,'UserData',csLabelCb{k});
    else
      vhsel(end+1) = ...
	        text(1.4,y,csLabel{k},'HorizontalAlignment','left',...
	             'VerticalAlignment','middle',...
	             'Fontsize',fontsize,'Fontweight','bold');
    end
    if (y < ylim(2)) && (y > ylim(1))
      vhsel(end+1) = ...
	        plot([0.05 1],[y y],'-','Color',0.6*ones(1,3),'linewidth', ...
	             2);
    end
    for kmin=1:3
      y = (k-1+kmin/4)/(length(csLabel)-1)*diff(ylim);
      if (y < ylim(2)) && (y > ylim(1))
	      vhsel(end+1) = ...
	          plot([0.05 1],[y y],'--','Color',0.6*ones(1,3),'linewidth', ...
		             1);
      end
    end
  end
  vhsel(end+1) = ax;
  vhsel(end+1) = ...
      plot([0 1 0.5 1 0.5],[q q q+dq q q-dq],...
	         'k-','linewidth',3);
  set(vhsel,'ButtonDownFcn',@gui_rating_axes_select);
  set(ax,'UserData',struct('rating',nan,'arrow',vhsel(end),'ylim',ylim,'vhsel',vhsel));
end

function donothing( varargin )
end

function labelcb( varargin )
  sCfg = get(gcbf,'UserData');
  cb = get(gcbo,'UserData');
  for k=1:sCfg.cfg.n
    bActive = false;
    axes_set_active( sCfg.vax(k),bActive);
    set(sCfg.vbutton(k),'value',bActive,'backgroundcolor',bActive*[0.2,0.8,0.2]+(1-bActive)*[0.9,0.9,0.9]);
  end
  cb(sCfg.cfg.userdata);
end

function axes_set_active( ax, bActive )
  sUD = get(ax, 'UserData');
  if bActive
    set(sUD.vhsel,'ButtonDownFcn',@gui_rating_axes_select);
    sUD.ttic = tic();
    set(ax,'Color',[0.2,0.8,0.2]);
    sFigData = get(gcf,'UserData');
    if ~isfinite(sUD.rating)
      set(sFigData.timerlab,'String',sprintf('%d s',sFigData.cfg.tmin));
    else
      set(sFigData.timerlab,'String','');
    end
  else
    sUD.ttic = [];
    set(sUD.vhsel,'ButtonDownFcn',@donothing);
    set(ax,'Color',0.9*ones(1,3));
  end
  set(ax,'UserData',sUD);
end

function gui_rating_axes_select( varargin )
  ax = gca();
  axp = get(ax,'CurrentPoint');
  sData = get(ax,'UserData');
  sFigData = get(gcf,'UserData');
  if ~isfinite(sData.rating)
    if isempty(sData.ttic)
      return;
    end
    if toc(sData.ttic) < sFigData.cfg.tmin
      set(sFigData.timerlab,'String',sprintf('%d s',ceil(sFigData.cfg.tmin-toc(sData.ttic))));
      return;
    end
  end
  set(sFigData.timerlab,'String','');
  ylim = sData.ylim;
  q = axp(1,2);
  q = min(max(q,ylim(1)),ylim(2));
  h = sData.arrow;
  q0 = get(h,'YData');
  set(h,'YData',q0-q0(1)+q);
  sData.rating = q;
  set(ax,'UserData',sData);
  if isfield(sData,'callback')
    sData.callback();
  end
  update_results();
end

function update_results
  sUserData = get(gcf(),'UserData');
  for k=1:sUserData.cfg.n
    sAD = get(sUserData.vax(k),'UserData');
    sUserData.results(k) = sAD.rating;
  end
  if ~any(isnan(sUserData.results))
    set(sUserData.completed,'enable','on');
  end
  set(gcf(),'UserData',sUserData);
end