function varargout = tascar_ctl(action,varargin)
% This is a function which is responsible for controlling TASCAR
% software using MATLAB. 
% INPUT:
% action          - a string specifing which action should be performed. 
%                   Each action is a different function which will be
%                   called - all the functions are included in this script.  
%                   The following actions are available until now:
%                ---ACTION:---     ---CORRESPONDING FUNCTION:---
%                   'load'           h = tascar_ctl_load(name)
%                   'kill'           tascar_ctl_kill(h)
%
%  Example usage:
%  h = tascar_ctl('load','test_scene.tsc')
%  tascar_ctl('kill','test_scene.tsc')
% -------------------------------------------------------------------------

%Apart of the local functions in this file there are other scripts in: 
%addpath('/home/medi/tascarpro/tascar/scripts/tascar_ctl_functions/');
%addpath('/home/medi/tascarpro/tascar/scripts/external_tools/');

  if nargin < 1
    error('No action name was provided.');
  end
  if ~ischar(action)
    error('Action name is not a string.');
  end
  %find which function to use 
  fun = ['tascar_ctl_',action];
  %create output array for the chosen function
  varargout = cell([1,nargout(fun)]);
  if ~isempty(varargout)
    %evaluate appropriate function with a list of input arguments 
    varargout{:} = feval(fun,varargin{:});
  else
    %for void functions
    feval(fun,varargin{:});
  end
  varargout(nargout+1:end) = [];
  

  %---------------------------------------------------------------------
function h = tascar_ctl_load(name)
% This is a function to open a TASCAR scene from MATLAB.
% INPUT :           name            string with tascar scene name
% OUTPUT:           h               handle (structure array) containing following
%                                   information:
%                                   h.pid     - process id 
%                                   
  h = struct();
  sCmd = sprintf('LD_LIBRARY_PATH="" tascar %s 2>%s.err >%s.out & echo $! > %s.pid',name,name,name,name);
  %open tascar scene in tascar, save the error message in a .err file and ?
  %in .out file 
  [a,b] = system(sCmd);
  
  h.pid = textread(sprintf('%s.pid',name));
  if isempty(h.pid)
      [a,msg] = system(sprintf('cat %s.out',name));
      disp(msg);
      [a,msg] = system(sprintf('cat %s.err',name));
      disp(msg);
      error(['unable to start ''',sCmd,'''']);
  end
  pause(1);

  [a,b] = system(sprintf('ps %d >/dev/null',h.pid));
  if a
    error('Uanble to create tascar process.');
  end
  [a,b] = system(sprintf('lsof -a -iUDP -p %d -Fn 2>/dev/null',h.pid));
  if a==0
    [vals,num] = sscanf(b,'p%d n*:%d');
    if num ~= 2
      error('invalid number of entries in lsof output');
    end
    h.oscport = vals(2);
    h.oschost = 'localhost';
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

  if isfield(h,'oscport') && isfield(h,'oschost')
    send_osc(h.oschost,h.oscport,'/unload');
    pause(0.5);
  end
  [a,b] = system(sprintf('kill %d',h.pid));
  
   
function tascar_ctl_createscene( varargin )
% In TASCAR, the virtual acoustic environments are defined in the XML file
% format. This is a function to create and save an XML file using MATLAB 
% (without a need to write an XML file on your own). 
% This function can 
% generate a simple scene with one speaker based receiver, 
% N sources equally distributed on a circle around the receiver and K virtual
% loudspeakers, also equally distributed on a circle around the receiver. 
% The produced XML file can be used as a virtual acoustic scene definition 
% and can be loaded in TASCAR (therefore it has to have a .tsc extension). 
%
% INPUT:         varargin          -list of parameter/value pairs i.e.
% 
%                generate_scene('parameter', value1, 'parameter2', value2)
%                
%                the list contains only the parameters we want to
%                change, if the parameter and its value is not specified,
%                it takes a default value. 
%                                   
% 
% Usage examples:
% generate_scene help    - this will give detailed information about the
%                          parameters and their default values
% generate_scene('filename','my_scene.tsc')  - this will generate an xml
%                                              file my_scene.tsc with default parameters
% generate_scene('r',2)  - this will generate an xml file with a scene with
%                          appropriate parameter values (for example here with
%                          the radius of 2m)

%-------------------------------------------------------------------------


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

  % initialize core objects:
  factory = javaMethod('newInstance', ...
		       'javax.xml.parsers.DocumentBuilderFactory');
  docBuilder = javaMethod('newDocumentBuilder',factory);

  %Create the document node and root element(always):
  docNode = javaMethod('newDocument',docBuilder);

  % session (root element):
  n_session = javaMethod('createElement',docNode,'session');

  javaMethod('appendChild',docNode,n_session); 
  % this line seems to be additional 
  
  [tmpPath,tmpBasename] = fileparts(sCfg.filename);
  % scene, description
  n_scene= addElementHere(docNode,n_session,'scene',[],'name',tmpBasename);

  % sources
  for i = 1:sCfg.nrsources
    
    src_name=['src_',num2str(i)];
    n_src_temp=addElementHere(docNode,n_scene,'src_object',[],'name',src_name);
    %position
    n_pos_src_temp= addElementHere(docNode,n_src_temp,'position',c_src_positions{i});
    %sound
    n_sound_temp=addElementHere(docNode,n_src_temp,'sound',[],'name','0','maxdist',num2str(sCfg.maxdist));
  end

  % receiver:
  n_rec= addElementHere(docNode,n_scene,'receiver',[], 'name', 'out', ...
			'type',sCfg.rec_type);
  for i = 1:sCfg.nrspeakers
    spk_name=['src_',num2str(i)];
    n_spk_temp=addElementHere(docNode,n_rec,'speaker',[],'az',num2str(v_az(i)),'el','0' );
  end  
  
  %% save xml file
  
  dsource = javaObject('javax.xml.transform.dom.DOMSource',docNode);
  transformerFactory = javaMethod('newInstance','javax.xml.transform.TransformerFactory');
  transformer = javaMethod('newTransformer',transformerFactory);
  writer= javaObject('java.io.StringWriter');
  result = javaObject('javax.xml.transform.stream.StreamResult',writer);
  javaMethod('transform',transformer,dsource, result);
  writer.close();
  xml = char(writer.toString());
  
  % makes each row into a cell
  CellArray = strcat(xml); 
  fid = fopen(sCfg.filename,'w');
  for r=1:size(CellArray,1)
    fprintf(fid,'%s\n',CellArray(r,:));
  end
  fclose(fid);
					       
function h_this_node=addElementHere(h_docNode,h_higher_node,s_this_node_name,s_content,varargin) 
%
% This is a function to create ONE element in the xml file, which will be used as TASCAR 
% scene definition file.
% INPUT: 
% docNode                handle of the document node (one for the whole xml file)
% higher_node            handle of the parrent element of the element we want to introduce 
% s_this_node_name       string with name of the element we want to introduce
% s_content              string containing content of the node we want to
%                        introduce. If the node doesnt have a content we put an empty 
%                        matrix here:  []
% varargin               list of attributes followed by their values
% OUTPUT:
% h_this_node            handle to the just introduced node 

% Example usage : 
% n_src_temp=addElementHere(docNode,n_scene,'src_object',[],'name','my_scene','color','#f00000','start','1')
%------------------------------------------------------------------------

%create a new element 
  h_this_node = javaMethod('createElement',h_docNode,s_this_node_name);
  % h_this_node=h_docNode.createElement(s_this_node_name);

  if mod(length(varargin),2)
    error('For each attribute there must be exactly one corresponding value')
  end
  
  %set attributes of the new element   
  if ~isempty(varargin)
    for i = 1 : 2 : length(varargin)  
      javaMethod('setAttribute',h_this_node,varargin{i},varargin{i+1});
      % h_this_node.setAttribute(varargin{i},varargin{i+1});
    end
  end
  
  % specify the parrent node   
  javaMethod('appendChild',h_higher_node,h_this_node);
  %h_higher_node.appendChild(h_this_node);

  % add content to the new node
  if ~isempty(s_content)
    javaMethod('appendChild',h_this_node,javaMethod('createTextNode',h_docNode, s_content));
    %     h_this_node.appendChild(h_docNode.createTextNode(s_content));
  end

function pid = tascar_ctl_jackddummy_start( P )
  sCmd = ['xterm -e "jackd -P86 -p4096 -ddummy -r44100 -p', num2str(P),...
	  '" 2>jackd.err >jackd.out & echo $! > jackd.pid'];
  
  [a,b] = system(sCmd);
  pid = textread('jackd.pid');
  pause(1);
  [a,b] = system(sprintf('ps %d >/dev/null',pid));
  if a
    error('Uanble to create jackd process.');
  end
