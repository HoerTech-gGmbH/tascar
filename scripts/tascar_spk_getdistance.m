function tascar_spk_getdistance( layoutfile, varargin )
% tascar_spk_calibrate
%
% tascar_spk_calibrate( layoutfile [, key, value ] )
%
% layoutfile: tascar speaker layout file
%
  
% 2020-06-09 Giso Grimm

  if (nargin < 1)
    varargin = {'help'};
  else
    if (nargin < 2)
      if strcmp(layoutfile,'help')
	varargin = {'help'};
      end
    end
  end
  if isoctave()
    % on octave make sure that signal package is loaded:
    pkg('load','signal');
  end
  
  sCfg.input = 'system:capture_1';
  sHelp.input = 'input jack port';
  sCfg.c = 340;
  sHelp.c = 'speed of sound in m/s';
  sCfg.dref = 0;
  sHelp.dref = ['reference distance, or 0 to take value from file,' ...
		' in m'];
  sCfg.cref = 1;
  sHelp.cref = 'reference channel, starting with 1';
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  [y,fs,fragsize] = tascar_jackio(0);

  doc_spk = tascar_xml_open( layoutfile );
  elem_spk = tascar_xml_get_element( doc_spk, 'speaker' );
  vDistFile = zeros(numel(elem_spk),1);
  vDistMeas = zeros(numel(elem_spk),1);
  for k=1:numel(elem_spk)
    vDistFile(k) = get_attr_num( elem_spk{k}, 'r', 1 );
  end
  len = 2^ceil(log2((4*max(vDistFile)/sCfg.c+0.1)*fs + 4*fragsize));

  mIR = zeros(len,numel(elem_spk));
  csLabels= {};
  [B,A] = butter(5,[200,8000]/(0.5*fs));
  for k=1:numel(elem_spk)
    connect = tascar_xml_get_attribute( elem_spk{k}, 'connect' );
    csLabels{k} = connect;
    az = get_attr_num( elem_spk{k}, 'az', 0 );
    el = get_attr_num( elem_spk{k}, 'el', 0 );
    r = get_attr_num( elem_spk{k}, 'r', 1 );
    ir = tascar_ir_measure( 'output', connect, ...
			    'input', sCfg.input, ...
			    'len', len,...
			    'gain',0.1,...
			    'nrep',2);
    ir = filter(B,A,ir);
    mIR(:,k) = ir;
    vDistMeas(k) = sCfg.c*irs_firstpeak(ir)/fs;
  end
  if( sCfg.dref == 0 )
    sCfg.dref = vDistFile(sCfg.cref);
  end
  vDistFile = vDistMeas-vDistMeas(sCfg.cref)+sCfg.dref;
  for k=1:numel(elem_spk)
    tascar_xml_set_attribute( elem_spk{k}, 'r', vDistFile(k) );
  %  tascar_xml_set_attribute( elem_spk{k}, 'compB', mIRcomp(1:sCfg.complen,k) );
  end
  tascar_xml_save(doc_spk,layoutfile);

function v = get_attr_num( el, name, def )
  s = tascar_xml_get_attribute( el, name );    
  if isempty(s)
    v = def;
  else
    v = str2num(s);
  end


function b = isoctave()
  b = ~isempty(ver('Octave'));
