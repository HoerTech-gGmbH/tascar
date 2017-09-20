function sCfg = afcgui( sExperiment, sSubject, sTag )
% SRT_WORD - word-wise speech receiption threshold test
%
% Usage:
% srt_word( sExperiment, sSubject, sTag )
%
% - sExperiment : experiment name (configuration script
%                 ['afccfg_',sExperiment] is called, should fill
%                 structure 'sCfg' with values)
% - sSubject    : subject name (optional if set by 
%                 configuration script)
% - sTag        : tag for data logging (optional, can be set in
%                 configuration script)
%
% Author: Giso Grimm
% Date: Jan 2010

  if nargin < 3
    sTag = '';
  end
  guipos = get(0,'ScreenSize');
  guipos = round([0.25*guipos(3:4),0.5*guipos(3:4)]);
  sCfg = struct;
  eval(['afccfg_',sExperiment]);
  sCfg.experiment = sExperiment;
  sCfg.subject = sSubject;
  sCfg.condition = sTag;
  sCfg.starttime = now;
  rand('state',24*3600*(sCfg.starttime-floor(sCfg.starttime)));
  sCfg.filename = sprintf('afclog_%s_%s_%s_%s.m',...
			  s2fname(sExperiment),...
			  s2fname(sSubject),...
			  s2fname(sTag), ...
			  s2fname(datestr(sCfg.starttime)));
  sCfg.filename_final = sprintf('afc_%s_%s_%s_%s.mat',...
			  s2fname(sExperiment),...
			  s2fname(sSubject),...
			  s2fname(sTag), ...
			  s2fname(datestr(sCfg.starttime)));
  % cb_sound_prepare: called once at beginning of experiment
  sCfg = defv(sCfg,'cb_sound_prepare',@nofun);
  % cb_sound_release: called once at end of experiment
  sCfg = defv(sCfg,'cb_sound_release',@nofun);
  % cb_pre_trial: called before each trial
  sCfg = defv(sCfg,'cb_pre_trial',@nofun_id);
  % cb_sound_play: called for each interval
  sCfg = defv(sCfg,'cb_sound_play',@nofun);
  %
  sCfg = defv(sCfg,'randomize_display',0);
  sCfg = defv(sCfg,'display_columns',1);
  sCfg = defv(sCfg,'num_intervals',3);
  sCfg = defv(sCfg,'labels',1:sCfg.num_intervals);
  sCfg = defv(sCfg,'indicator_labels',1:sCfg.num_intervals);
  sCfg = defv(sCfg,'target_intervals',2:3);
  sCfg = defv(sCfg,'msg_begin','Please press any key to start measurement.');
  sCfg = defv(sCfg,'msg_end','Many thanks for your participation!');
  sCfg = defv(sCfg,'gui_position',guipos);
  sCfg = defv(sCfg,'indicator',0);
  sCfg = defv(sCfg,'delay_pretrial',0.800);
  sCfg = defv(sCfg,'delay_interval',0.200);
  sCfg = defv(sCfg,'delay_posttrial',0.200);
  sCfg = defv(sCfg,'feedback',0);
  sCfg = defv(sCfg,'afc',struct);
  sCfg = defv(sCfg,'roving',0);
  sCfg.afc = defv(sCfg.afc,'rule',[1,1]);
  sCfg.afc = defv(sCfg.afc,'startvar',70);
  sCfg.afc = defv(sCfg.afc,'varstep',[6,3,1]);
  sCfg.afc = defv(sCfg.afc,'steprule',1);
  sCfg.afc = defv(sCfg.afc,'reversalnum',4);
  sCfg.afc = defv(sCfg.afc,'maxtrials',0);
  sCfg.callback = @playtrial;
  %sCfg.callback = @playtrial_rand;
  sCfg.fh = figure('Name',...
		   sprintf('AFC: %s - %s/%s',sExperiment,sSubject,sTag),...
		   'MenuBar','none',...
		   'NumberTitle','off',...
		   'Position',sCfg.gui_position,...
		   'KeyPressFcn',@mha_htl_key);

  cUser = cell(size(sCfg.tracks));
  for k=1:numel(sCfg.tracks)
    [kList,kTarget] = ind2sub(size(sCfg.tracks),k);
    sUser = struct;
    sUser.cfg = sCfg;
    sUser.target = kTarget;
    sUser.list = kList;
    sUser.par = sCfg.tracks{k};
    cUser{k} = sUser;
  end
  sCfg.user = cUser;
  show_msg( sCfg, sCfg.msg_begin );
  b_prepared = false;
  try
    sCfg.cb_sound_prepare(sCfg);
    b_prepared = true;
    log_header(sCfg);
    sCfg = afclib('multitrack',sCfg,cUser);
    log_footer(sCfg);
    sCfg.cb_sound_release(sCfg);
    show_msg( sCfg, sCfg.msg_end );
    close(sCfg.fh);
    sRes = sCfg;
    save(sCfg.filename_final,'sRes');
  catch
    log_footer_error(sCfg);
    if b_prepared
      sCfg.cb_sound_release(sCfg);
    end
    if ishandle(sCfg.fh)
      close(sCfg.fh);
    end
    err = lasterror;
    errordlg(err.message,'Error during measurement');
    for k=1:length(err.stack)
      disp(sprintf('%s:%d %s',err.stack(k).file,...
		   err.stack(k).line,err.stack(k).name));
    end
    rethrow(err);
  end
  
function h = show_msg( sCfg, msg, bWait, vCol )
  if nargin < 3
    bWait = 1;
  end
  if nargin < 4
    vCol = 0.7*ones(1,3);
  end
  figure(sCfg.fh);
  delete(get(sCfg.fh,'Children'));
  pos = get(sCfg.fh,'Position');
  %
  % calculate button sizes and positions:
  fig = struct;
  fig.N = [1,1];
  % button size (including margin):
  fig.size = pos(3:4)./fig.N;
  % button margin:
  fig.margin = 0.2*fig.size;
  % final button size:
  fig.size_final = fig.size - 2*fig.margin;
  bpos = round([fig.margin,fig.size_final]);
  h = uicontrol('Position',bpos,...
		'backgroundColor',vCol,...
		'style','PushButton','String',msg,...
		'FontUnits','Pixels','FontSize',pos(4)/30);
  default_ui = uicontrol('KeypressFcn',@mha_htl_key,...
			 'Position',[0 0 1 1],...
			 'BusyAction','cancel');
  if bWait
    set(h,'Callback','uiresume(gcbf);');
    uiwait(sCfg.fh);
    if ishandle(sCfg.fh)
      delete(get(sCfg.fh,'Children'));
    end
  end
  drawnow;
  
function s = s2fname( s )
  for k=' :-+.*'
    s = strrep(s,k,'_');
  end
  s = strrep(s,'__','_');
  s = strrep(s,'__','_');
  
function s = defv( s, n, v )
  if ~isfield(s,n)
    s.(n) = v;
  end
  
function logstr_row(fname,v)
  s = sprintf('%1.40g,',v);
  s(end) = [];
  fh = fopen(fname,'a');
  fprintf(fh,'%s;...\n',s);
  fclose(fh);
  
function log_header( sCfg )
  fh = fopen(sCfg.filename,'a');
  fprintf(fh,'%% ''SRT result file''\n');
  fprintf(fh,'sResult = struct;\n');
  fprintf(fh,'sResult.experiment = ''%s'';\n',sCfg.experiment);
  fprintf(fh,'sResult.subject = ''%s'';\n',sCfg.subject);
  fprintf(fh,'sResult.condition = ''%s'';\n',sCfg.condition);
  %fprintf(fh,'sResult.lists = {...;\n');
  %for kList=1:size(sCfg.lists,1)
  %  sL = '';
  %  for kTarget=1:size(sCfg.lists,2)
  %    sL = sprintf('%s''%s'',',sL,sCfg.lists{kList,kTarget});
  %  end
  %  sL(end) = [];
  %  fprintf(fh,'  %s;...\n',sL);
  %end
  %fprintf(fh,'};\n');
  fprintf(fh,'sResult.data = [...;\n');
  fclose(fh);

function log_footer( sCfg )
  fh = fopen(sCfg.filename,'a');
  fprintf(fh,'];\n');
  fclose(fh);

function log_footer_error( sCfg )
  fh = fopen(sCfg.filename,'a');
  fprintf(fh,'];\n');
  fprintf(fh,'error(''data inclomplete'');\n');
  fclose(fh);
  
function [bCorrect,sUser] = playtrial_rand( sAfc, sUser )
  bCorrect = (rand(1) > 0.42);
  logstr_row(sUser.filename,...
	     [sec_since_start(sUser),sUser.list,sUser.target,bCorrect,sAfc.var,sAfc.measurement,sAfc.finished]);


function [bCorrect, sUser] = playtrial( sAfc, sUser )
  [tmp,idx_rov] = sort(rand(numel(sUser.cfg.roving),1));
  sUser.cfg.roving = sUser.cfg.roving(idx_rov(1));
  sUser.cfg = sUser.cfg.cb_pre_trial( sUser.cfg, sAfc.var, sUser.par );
  pause(sUser.cfg.delay_pretrial);
  idx = randperm(length(sUser.cfg.target_intervals));
  kTargetInterval = sUser.cfg.target_intervals(idx(1));
  if sUser.cfg.indicator
    [vh,vWord] = drawbuttons(sUser.cfg,sUser.cfg.indicator_labels);
  end
  for kInterval=1:sUser.cfg.num_intervals
    if kInterval > 1
      pause(sUser.cfg.delay_interval);
    end
    if sUser.cfg.indicator
      vcol = get(vh(kInterval),'BackgroundColor');
      set(vh(kInterval),'BackgroundColor',[253,230,12]/255);
      drawnow;
    end
    sUser.cfg.cb_sound_play( kInterval==kTargetInterval, ...
			     sAfc.var, ...
			     sUser.par, ...
			     sUser.cfg );
    if sUser.cfg.indicator
      set(vh(kInterval),'BackgroundColor',vcol);
      drawnow;
    end
  end
  pause(sUser.cfg.delay_posttrial);
  resp = get_reply( sUser.cfg, sUser.cfg.labels, kTargetInterval );
  if isempty(resp)
    error('Invalid answer (window closed) - measurement cancelled.');
  end
  bCorrect = (resp==kTargetInterval);
  
  logstr_row(sUser.cfg.filename,...
	     [sec_since_start(sUser.cfg),sUser.list, ...
	      sUser.target,resp,kTargetInterval,sAfc.var,sAfc.measurement,sAfc.finished]);
  
function t = sec_since_start( sUser )
  t = 24*3600*(now-sUser.starttime);
  %t = round(4*t)/4;
  
function [h,vWord] = drawbuttons( sCfg, csLabels )
  if isnumeric(csLabels)
    csTmp = {};
    for k=1:length(csLabels(:))
      csTmp{end+1} = num2str(csLabels(k));
    end
    csLabels = csTmp;
  end
  figure(sCfg.fh);
  delete(findobj(sCfg.fh,'tag','answerbutton'));
  pos = get(sCfg.fh,'Position');
  %
  if sCfg.randomize_display
    vWord = randperm(length(csLabels));
  else
    vWord = 1:length(csLabels);
  end
  % calculate button sizes and positions:
  fig = struct;
  fig.N = [sCfg.display_columns,ceil(length(csLabels)/sCfg.display_columns)];
  % button size (including margin):
  fig.size = pos(3:4)./fig.N;
  % button margin:
  fig.margin = 0.2*fig.size;
  % final button size:
  fig.size_final = fig.size - 2*fig.margin;
  h = [];
  for k=1:length(csLabels)
    [kx,ky] = ind2sub(fig.N,k);
    bpos = round([[kx-1,fig.N(2)-ky].*fig.size+fig.margin,fig.size_final]);
    h(end+1) = ...
	uicontrol('Position',bpos,...
		  'tag','answerbutton',...
		  'UserData',[vWord(k),k],...
		  'style','PushButton','String',csLabels{vWord(k)},...
		  'FontUnits','Pixels',...
		  'FontSize',min(pos(4)/(fig.N(2)*3),pos(3)/(fig.N(1)*6)));
  end
  default_ui = uicontrol('KeypressFcn',@mha_htl_key,...
			 'Position',[0 0 1 1],...
			 'BusyAction','cancel');
  
function resp = get_reply( sCfg, csList, kCorrect )
  resp = [];
  [vh,vWord] = drawbuttons(sCfg,csList);
  set(vh,'Callback',@cb_get_reply);
  uiwait(sCfg.fh);
  if ishandle(sCfg.fh)
    resp = get(sCfg.fh,'UserData');
    if sCfg.feedback && kCorrect
      set(vh(find(vWord==kCorrect)),'BackgroundColor',[253,230,12]/255);
      drawnow;
      pause(sCfg.feedback);
    end
    %delete(get(sCfg.fh,'Children'));
    delete(findobj(sCfg.fh,'tag','answerbutton'));
    drawnow;
  end
  
function cb_get_reply( varargin )
  repl = get(gcbo,'UserData');
  set(gcbf,'UserData',repl(1));
  uiresume(gcbf);
  
function nofun( varargin )
  
function x = nofun_id( x, varargin )
  
function mha_htl_key( bo, key )
  if isempty(key.Modifier)
    lkey = key.Key;
  else
    lkey = '';
    for mod=key.Modifier
      lkey = [lkey mod{:} '-'];
    end
    lkey = [lkey key.Key];
  end
  vBut = findobj(gcf,'tag','answerbutton');
  ind = double(lkey)-48;
  for k=1:numel(vBut)
    but_ind = get(vBut(k),'UserData');
    if ind == but_ind(2)
      set(gcf,'UserData',but_ind(1));
      uiresume(gcf);
    end
  end
