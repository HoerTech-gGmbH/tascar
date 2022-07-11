function varargout = tascar_ctl( action, varargin )
% TASCAR_CTL - control TASCAR processes
%
% This is a function which is responsible for controlling TASCAR
% software using MATLAB/GNU Octave.
%
% Input arguments:
% action: a string specifing which action should be performed.  Each
%         action is a different function which will be called - all
%         the functions are included in this script.  The following
%         actions are available until now:
%
%         'load', 'kill', 'createscene', 'transport',
%         'audioplayercreate', 'audioplayerselect'
%
%  Example usage:
%  h = tascar_ctl( 'load', 'test_scene.tsc' )
%  tascar_ctl( 'transport', h, 'play' )
%  tascar_ctl( 'transport', h, 'locate', 15 )
%  tascar_ctl( 'kill', h )

  if nargin < 1
    error('No action name was provided.');
  end
  if ~ischar(action)
    error('Action name is not a string.');
  end
  % find which function to use
  fun = ['tascar_ctl_',action];
  % create output array for the chosen function
  varargout = cell([1,nargout(fun)]);
  if ~isempty(varargout)
    % evaluate appropriate function with a list of input arguments
    varargout{:} = feval(fun,varargin{:});
  else
    %for void functions
    feval(fun,varargin{:});
  end
  varargout(nargout+1:end) = [];

function h = tascar_ctl_load( sessionfile, usegui )
% Start TASCAR and load a session file.
%
% Usage
% h = tascar_ctl( 'load', sessionfile [, usegui ] );
%
% Input parameters
%   sessionfile : Name of session file
%   usegui : load TASCAR with GUI? (default: true)
%
% Return value
%   h : handle of type struct with the fields
%       h.pid  : process id
%       h.host : 'localhost'
%       h.port : OSC port number

  if nargin < 2
    usegui = true;
  end
  if usegui
    s_tascar = 'tascar';
  else
    s_tascar = 'tascar_cli';
  end
  h = struct();
  sCmd = sprintf('LD_LIBRARY_PATH="" %s %s 2>%s.err >%s.out & echo $! > %s.pid',s_tascar,sessionfile,sessionfile,sessionfile,sessionfile);
									%open tascar scene in tascar, save the error message in a .err file and ?
									%in .out file
									[a,b] = system(sCmd);

									h.pid = textread(sprintf('%s.pid',sessionfile));
									if isempty(h.pid)
									  [a,msg] = system(sprintf('cat %s.out',sessionfile));
									  disp(msg);
									  [a,msg] = system(sprintf('cat %s.err',sessionfile));
									  disp(msg);
									  error(['unable to start ''',sCmd,'''']);
									end
									pause(1);

									[a,b] = system(sprintf('ps %d >/dev/null',h.pid));
									if a~=0
									  error('Uanble to create tascar process.');
									end
									[a,b] = system(sprintf('lsof -a -iUDP -p %d -Ffn 2>/dev/null',h.pid));
									if a==0
									  [vals,num] = sscanf(b,'p%d f%d n*:%d');
									  if num ~= 3
									    error('invalid number of entries in lsof output');
									  end
									  h.port = vals(3);
									  h.host = 'localhost';
									end

function r = tascar_ctl_killpid( pid )
  [r,b] = system(sprintf('kill %d 2>&1',pid));
  if r > 0
    warning(b);
    return;
  end
  pause(1);
  [a,b] = system(sprintf('ps %d >/dev/null',pid));
  if a==0
    [a,b] = system(sprintf('kill -9 %d',pid));
    pause(1);
    [a,b] = system(sprintf('ps %d >/dev/null',pid));
    if a==0
      error(sprintf('unable to kill process %d',pid));
    end
  end


function tascar_ctl_kill(h)
% This is a function to close a TASCAR scene from MATLAB.
% INPUT :           h            struct with handle obtained when opening a
%                                TASCAR scene from MATLAB

  if isfield(h,'port') && isfield(h,'host')
    send_osc(h,'/tascargui/quit');
    pause(0.5);
  end
  [a,b] = system(sprintf('kill %d 2>/dev/null',h.pid));


function tascar_ctl_createscene( varargin )
% Create a session file with a simple scene
%
% Usage:
% tascar_ctl( 'createscene', parameter, value, ... )
%
% This function generates a session file containing a simple scene
% with one speaker based receiver, N sources equally distributed on a
% circle around the receiver. The receiver renders to K loudspeakers
% equally distributed on a circle. The generated XML file can be used
% as a virtual acoustic scene definition and can be loaded in TASCAR
% (therefore it has to have a .tsc extension).
%
% Usage examples:
%
% Show detailed information about the parameters and their default
% values:
% tascar_ctl('createscene','help')
%
% Generate an xml file 'my_scene.tsc' with default parameters:
% tascar_ctl('createscene','filename','my_scene.tsc')
%
% Generate an xml file with a scene with a source radius of 2m:
% tascar_ctl('createscene','r',2)

  sCfg = struct;
  sHelp = struct;

  % default values:
  sCfg.filename = 'generated.tsc';
  sCfg.nrsources = 1;
  sCfg.r = 1;
  sCfg.rec_type = 'hoa2d';
  sCfg.nrspeakers = 8;
  sCfg.maxdist = 3700;

  % help comments:
  sHelp.filename = 'output file name';
  sHelp.nrsources = 'number of sources in the scene. Sources are equally distributed on a circle' ;
  sHelp.r = 'radius of a circle on which the sources are equally distributed around the receiver (distance from a csource to a receiver)';
  sHelp.rec_type= 'type of a receiver in the tascar scene';
  sHelp.nrspeakers='number of virtual louspeakers used to render the sound';

  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );

  if isempty(sCfg)
    return;
  end

  %% Parameters characteristic for this scene
  % Sources equally spread on a circle:
  v_az = linspace(0,2*pi,sCfg.nrsources+1);
  c_src_positions=cell(length(v_az),1);
  for i=1:length(v_az-1)
    x=sCfg.r*cos(v_az(i));
    y=sCfg.r*sin(v_az(i));
    z=0;
    c_src_positions{i}=['0 ',num2str(x),' ',num2str(y),' ',num2str(z)];
  end
  %Virtual loudspeakers equally spread on a circle
  v_az = linspace(0,2*pi,sCfg.nrspeakers+1);
  v_az=v_az(1:end-1);
  v_az=v_az*180/pi-180;
  %% -------------- Creating an XML scene definition file -----------
  %Create the document node and root element(always):
  docNode = tascar_xml_doc_new();
  % session (root element):
  n_session = tascar_xml_add_element(docNode,docNode,'session',[]);
  [tmpPath,tmpBasename] = fileparts(sCfg.filename);
  % scene, description
  n_scene = tascar_xml_add_element(docNode,n_session,'scene',[],'name',tmpBasename);
  % sources
  for i = 1:sCfg.nrsources
    src_name=['src_',num2str(i)];
    n_src_temp = ...
	tascar_xml_add_element(docNode, n_scene, 'source',[],'name',src_name);
    %position
    n_pos_src_temp = ...
	tascar_xml_add_element(docNode, n_src_temp, 'position',c_src_positions{i});
    %sound
    n_sound_temp = ...
	tascar_xml_add_element(docNode, n_src_temp, 'sound',[],...
			       'maxdist', num2str(sCfg.maxdist));
  end
  % receiver:
  n_rec = tascar_xml_add_element(docNode,n_scene,'receiver',[], 'name', 'out', ...
				 'type',sCfg.rec_type);
  for i = 1:sCfg.nrspeakers
    spk_name=['src_',num2str(i)];
    n_spk_temp = ...
	tascar_xml_add_element(docNode, n_rec, 'speaker',[], ...
			       'az',num2str(v_az(i)),'el','0' );
  end
  %% save xml file:
  tascar_xml_save( docNode, sCfg.filename );

function pid = tascar_ctl_jackddummy_start( P )
  sCmd = ['xterm -e "jackd -P86 -p4096 -ddummy -r44100 -p', num2str(P),...
	  '" 2>jackd.err >jackd.out & echo $! > jackd.pid'];

  [a,b] = system(sCmd);
  pid = textread('jackd.pid');
  pause(1);
  [a,b] = system(sprintf('ps %d >/dev/null',pid));
  if a~=0
    error('Uanble to create jackd process.');
  end

function tascar_ctl_transport( handle, mode, varargin )
  send_osc( handle, ['/transport/',mode], varargin{:} );

function tascar_ctl_help( name )
% provide help on a sub-command.
  eval(['help tascar_ctl>tascar_ctl_',name]);


function h = tascar_ctl_audioplayercreate( cSignals, varargin )
  sCfg = struct;
  sHelp = struct;
  h = [];
  % default values:
  sCfg.connect = '';
  sCfg.level = -20*log10(2e-5);
  sCfg.levelmode = 'calib';
  sCfg.weighting = 'Z';
  sCfg.fs = 44100;
  sCfg.duration = 36000;
  sCfg.port = 9877;
  sCfg.loop = 1;
  sCfg.usegui = false;
  sCfg.rewindonselect = false;
  sCfg.playonselect = false;

  % help comments:
  sHelp.connect = 'connect output to these ports (regexp)';
  sHelp.level = 'playback level' ;
  sHelp.levelmode = ['calibration level mode, see documentation of ' ...
                     'sndfile audio plugin in the manual for details'];
  sHelp.weighting = 'rms Level weighting in case of rms level mode';
  sHelp.fs = 'sampling rate in Hz';
  sHelp.duration = 'session duration (= maximal duration) in seconds';
  sHelp.port = 'port number';
  sHelp.loop = 'loop count of each sound';
  sHelp.usegui = 'open TASCAR with GUI (true) or without (false)';
  sHelp.rewindonselect = 'rewind transport when selecting a sound';
  sHelp.playonselect = 'start transport when selecting a sound';
  if ischar(cSignals)
    if strcmp(cSignals,'help')
      disp('Create a session as audio player');
      disp('');
      disp('Usage:');
      disp('h = tascar_ctl( ''audioplayercreate'', cSignals, parameter, value, ... )');
      disp('');
      disp(' cSignals: cell array with signal vectors or signal names');
      disp('');
      disp('Usage examples:');
      disp('');
      disp('Show detailed information about the parameters and their default');
      disp('values:');
      disp('tascar_ctl(''audioplayercreate'',''help'')');
      disp('');
      tascar_parse_keyval( sCfg, sHelp, 'help' );
      return;
    end
  end
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  fbase = tempname();
  %% Create session file:
  % Create the document node and root element(always):
  docNode = tascar_xml_doc_new();
  % session (root element):
  n_session = tascar_xml_add_element(docNode,docNode,'session',[],...
                                     'license','CC0',...
                                     'duration',num2str(sCfg.duration),...
                                     'srv_port',num2str(sCfg.port));
  n_scene = tascar_xml_add_element(docNode,n_session,'scene');
  n_mod = tascar_xml_add_element(docNode,n_session,'modules');
  % Create wav files:
  for k=1:numel(cSignals)
    if ischar(cSignals{k})
      name = cSignals{k};
      ainf = audioinfo(name);
      nchannels = ainf.NumChannels;
    else
      name = sprintf('%s-%d.wav',fbase,k);
      audiowrite(name,cSignals{k},sCfg.fs,'BitsPerSample',32);
      fh = fopen([name,'.license'],'w');
      fprintf(fh,'CC0\n');
      fclose(fh);
      nchannels = num2str(size(cSignals{k},2));
    end
    n_route = tascar_xml_add_element(docNode,n_mod,...
				     'route',[],...
				     'mute','true',...
				     'name',num2str(k),...
				     'channels',num2str(nchannels),...
				     'connect_out',sCfg.connect);
    n_plugs = tascar_xml_add_element(docNode,n_route,'plugins');
    n_snd = tascar_xml_add_element(docNode,n_plugs,'sndfile',[],...
				   'name',name,...
				   'level',num2str(sCfg.level),...
				   'levelmode',sCfg.levelmode,...
				   'weighting',sCfg.weighting,...
				   'loop',num2str(sCfg.loop),...
				   'resample','true');
  end
  n_time = tascar_xml_add_element(docNode,n_mod,'lsljacktime',[],...
                                  'sendwhilestopped','true');
  %% save xml file:
  tascar_xml_save( docNode, [fbase,'.tsc'] );
  h = tascar_ctl_load( [fbase,'.tsc'], sCfg.usegui );
  h.routes = numel(cSignals);
  h.cfg = sCfg;
  [err,msg] = system(['rm ',fbase,'*']);

function tascar_ctl_audioplayerselect( h, player )
  if ischar(h)
    if strcmp(h,'help')
      disp('Select an active sound file in a player instance');
      disp('');
    end
  end
  if( h.cfg.rewindonselect )
    send_osc(h,'/transport/locate',0);
  end
  if( h.cfg.playonselect )
    send_osc(h,'/transport/start');
  end
  nonplayer = setdiff(1:h.routes,player);
  for np=nonplayer
    send_osc(h,['/',num2str(np),'/mute'],1);
  end
  for np=player
    send_osc(h,['/',num2str(np),'/mute'],0);
  end
