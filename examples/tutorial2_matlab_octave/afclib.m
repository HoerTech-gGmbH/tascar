function d = afclib( mode, varargin )
  d = feval(['afclib_',mode],varargin{:});

function sMultiTrack = afclib_multitrack( sMultiTrack, cUser )
% afclib_multitrack( sMultiTrack, cUser )
  ;
  % set some default values:
  % 'afc' : AFC method configuration
  sMultiTrack = defv( sMultiTrack, 'afc', afclib_examplecfg );
  % 'callback' : display/playback and response callback
  sMultiTrack = defv( sMultiTrack, 'callback', @dummyreply );
  % 'numtracks_min' : minimal number of tracks to remain
  % simultaneously:
  sMultiTrack = defv( sMultiTrack, 'numtracks_min', 1 );
  % interleave tracks? default = yes
  sMultiTrack = defv( sMultiTrack, 'interleaved', 1 );
  %
  num_tracks = prod(size(cUser));
  if num_tracks < sMultiTrack.numtracks_min
    error('Number of tracks is less than ''numtracks_min''.');
  end
  max_closed_tracks = num_tracks - sMultiTrack.numtracks_min;
  % create AFC control variables for all tracks:
  sMultiTrack.tracks = ...
      repmat({afclib_control_init( sMultiTrack.afc )},...
	     size(cUser));
  sMultiTrack.results = ...
      sMultiTrack.tracks;
  if sMultiTrack.interleaved
    while any(~multitrack_vfinished( sMultiTrack.tracks ))
      % create a random tracklist:
      vIdx = randperm(num_tracks);
      % get a list of finished tracks (in random order):
      vFinished = multitrack_vfinished(sMultiTrack.tracks(vIdx));
      % find finished tracks:
      vIdxClosed = find(vFinished);
      % remove finished tracks, but not more than max_closed_tracks:
      if length(vIdxClosed) > max_closed_tracks
	vIdxClosed(max_closed_tracks+1:end) = [];
      end
      vIdx(vIdxClosed) = [];
      % apply AFC method to all open tracks (finished or not):
      for k=vIdx
	sAfc = sMultiTrack.tracks{k};
	sRes = sMultiTrack.results{k};
	sUser = cUser{k};
	% display/play trial:
	[bCorrect,sUser] = sMultiTrack.callback( sAfc, sUser );
	cUser{k} = sUser;
	% AFC method:
	sAfc = afclib_control(sAfc,bCorrect);
	for fn=fieldnames(sAfc)'
	  name = fn{:};
	  if isfield(sRes,name)
	    if (isnumeric(sAfc.(fn{:}))||islogical(sAfc.(fn{:}))) ...
		  && numel(sAfc.(fn{:}))==1
	      sRes.(fn{:})(end+1) = sAfc.(fn{:});
	    end
	  end
	end
	% update control data:
	sMultiTrack.tracks{k} = sAfc;
	sMultiTrack.results{k} = sRes;
      end
    end
  else % non-interleaved:
    for k=1:num_tracks
      sAfc = sMultiTrack.tracks{k};
      sRes = sMultiTrack.results{k};
      sUser = cUser{k};
      while ~sAfc.finished
	% display/play trial:
	[bCorrect,sUser] = sMultiTrack.callback( sAfc, sUser );
	% AFC method:
	sAfc = afclib_control(sAfc,bCorrect);
	for fn=fieldnames(sAfc)'
	  name = fn{:};
	  if isfield(sRes,name)
	    if (isnumeric(sAfc.(name))||islogical(sAfc.(name))) ...
		  && numel(sAfc.(name))==1
	      sRes.(name)(end+1) = sAfc.(name);
	    end
	  end
	end
      end
      cUser{k} = sUser;
      % update control data:
      sMultiTrack.tracks{k} = sAfc;
      sMultiTrack.results{k} = sRes;
    end
  end
  sMultiTrack.var_mean = nan*zeros(size(cUser));
  sMultiTrack.var_std = nan*zeros(size(cUser));
  sMultiTrack.var_N = nan*zeros(size(cUser));
  for k=1:numel(sMultiTrack.results)
    sRtmp = sMultiTrack.results{k};
    idx = find(sRtmp.measurement & ~sRtmp.finished);
    if ~isempty(idx)
      sMultiTrack.var_mean(k) = mean(sRtmp.var(idx));
      sMultiTrack.var_std(k) = std(sRtmp.var(idx));
    end
    sMultiTrack.var_N(k) = numel(idx);
  end
  
function b = multitrack_vfinished( cTracks )
  b = zeros(size(cTracks));
  for k=1:prod(size(b))
    b(k) = cTracks{k}.finished;
  end
  
function bCorrect = dummyreply( sAfc, sUser )
  disp(sAfc);
  disp(sUser);
  s = input('1 or 0? ');
  bCorrect = 0;
  if ~isempty(s)
    bCorrect = (s==1);
  end
  
function sCfg = afclib_examplecfg
  sCfg = struct;
  % [up down]-rule: [1 2] = 1-up 2-down
  sCfg.rule = [1,1];
  % start value of control variable:
  sCfg.startvar = 65;
  % maximum value of control variable:
  sCfg.maxvar = inf;
  % minimum value of control variable:
  sCfg.minvar = -inf;
  % list of step sizes (starting stepsize ... minimum stepsize):
  sCfg.varstep = [10,5,1];
  % stepsize is changed after each upper (-1) or lower (1) reversal:
  sCfg.steprule = -1;
  % stepmode (lin/log) distinguishes between linear (+-) and
  % logarithmic (*/) steps:
  sCfg.stepmode = 'lin';
  % number of reversals in measurement phase:
  sCfg.reversalnum = 8;
  % maximal number of trials (0: no limit):
  sCfg.maxtrials = 0;
  % maximal number of consecutive trials exceeding maxvar (0: no limit):
  sCfg.maxtrials_maxvar = 0;
  % maximal number of consecutive trials exceeding minvar (0: no limit):
  sCfg.maxtrials_minvar = 0;
  
function sAFC = afclib_control( sAFC, bCorrect )
% control - the kernel of the AFC method
  ;
  % update answer history:
  sAFC.answers = [sAFC.answers(2:end),bCorrect];
  % get step direction:
  stepdir = 0;
  % up-step, no correct answer in last Nup trials:
  if isequal(sAFC.answers(end-sAFC.cfg.rule(1)+1:end),...
	     zeros([1,sAFC.cfg.rule(1)]))
    stepdir = 1;
    % reset answer group:
    sAFC.answers(end-sAFC.cfg.rule(1)+1:end) = -1;
  end
  % down-step, only correct answers in last Ndown trials:
  if isequal(sAFC.answers(end-sAFC.cfg.rule(2)+1:end),...
	     ones([1,sAFC.cfg.rule(2)]))
    stepdir = -1;
    % reset answer group:
    sAFC.answers(end-sAFC.cfg.rule(2)+1:end) = -1;
  end
  % test for lower (-1) or upper (1) reversal:
  reversal = 0;
  if stepdir
    if sAFC.laststepdir && (sAFC.laststepdir ~= stepdir)
      reversal = -stepdir;
    end
    sAFC.laststepdir = stepdir;
  end
  %vreversal = ...
  %    [isequal(sAFC.answers,[zeros(1,sAFC.cfg.rule(1)),ones(1,sAFC.cfg.rule(2))]),...
  %     isequal(sAFC.answers,[ones(1,sAFC.cfg.rule(2)),zeros(1,sAFC.cfg.rule(1))])];
  % test for desired reversal:
  sAFC.reversal = (reversal == sAFC.cfg.steprule);
  % on reversal, increment varstepidx (i.e., decrease step size):
  sAFC.varstepidx = sAFC.varstepidx + sAFC.reversal;
  % test for measurement phase:
  sAFC.measurement = sAFC.varstepidx > length(sAFC.cfg.varstep);
  % update number of remaining reversals in measurement phase:
  sAFC.reversalnum = sAFC.reversalnum - (sAFC.measurement && sAFC.reversal);
  % test if measurement phase is finished:
  sAFC.finished = sAFC.reversalnum <= 0;
  % update stepsize on reversal (not in measurement phase):
  if sAFC.reversal && (~sAFC.measurement)
    sAFC.stepsize = sAFC.cfg.varstep(sAFC.varstepidx);
  end
  % update control variable:
  if strcmp( sAFC.cfg.stepmode, 'log' )
    sAFC.var = sAFC.var * (sAFC.stepsize.^stepdir);
  else
    sAFC.var = sAFC.var + stepdir * sAFC.stepsize;
  end
  if sAFC.var >= sAFC.cfg.maxvar
    sAFC.var = sAFC.cfg.maxvar;
    sAFC.num_upperlimit_reached = ...
	sAFC.num_upperlimit_reached + 1;
  else
    sAFC.num_upperlimit_reached = 0;
  end
  if sAFC.var <= sAFC.cfg.minvar
    sAFC.var = sAFC.cfg.minvar;
    sAFC.num_lowerlimit_reached = ...
	sAFC.num_lowerlimit_reached + 1;
  else
    sAFC.num_lowerlimit_reached = 0;
  end
  % update trial number:
  sAFC.numtrial = sAFC.numtrial + 1;
  if sAFC.cfg.maxtrials && (sAFC.numtrial >= sAFC.cfg.maxtrials)
    sAFC.maxtrial_exceeded = 1;
    sAFC.finished = 1;
  end
  if sAFC.cfg.maxtrials_maxvar && ...
	(sAFC.num_upperlimit_reached > sAFC.cfg.maxtrials_maxvar)
    sAFC.finished = 1;
  end
  if sAFC.cfg.maxtrials_minvar && ...
	(sAFC.num_lowerlimit_reached > sAFC.cfg.maxtrials_minvar)
    sAFC.finished = 1;
  end
  
function sAFC = afclib_control_init( sCfg )
% control_init - initialize the AFC control structure
  sCfgDef = afclib_examplecfg;
  for fn=fieldnames(sCfgDef)'
    sCfg = defv( sCfg, fn{:}, sCfgDef.(fn{:}) );
  end
  sAFC = struct;
  % keep the original input configuration:
  sAFC.cfg = sCfg;
  % flag, set to 1 in measurement phase:
  sAFC.measurement = 0;
  % counter, counts the remaining reversals in measurement phase:
  sAFC.reversalnum = sCfg.reversalnum;
  % index, selects the appropriate 'varstep' value, and indicates
  % begin of measurement phase:
  sAFC.varstepidx = 1;
  % history of answers:
  sAFC.answers = -ones(1,sum(sCfg.rule));
  % select initial stepsize:
  sAFC.stepsize = sCfg.varstep(1);
  % set start value:
  sAFC.var = sCfg.startvar;
  % track is not finished:
  sAFC.finished = 0;
  % last step direction:
  sAFC.laststepdir = 0;
  % max_trial exceeded:
  sAFC.maxtrial_exceeded = 0;
  % number of trial:
  sAFC.numtrial = 0;
  % number of times of negative answer while maxvar was reached
  sAFC.num_upperlimit_reached = 0;
  % number of times of positive answer while minvar was reached
  sAFC.num_lowerlimit_reached = 0;

function s = defv( s, n, v )
  if ~isfield(s,n)
    s.(n) = v;
  end
  
