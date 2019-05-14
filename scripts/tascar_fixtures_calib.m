function sData = tascar_fixtures_calib( s_layout, varargin )
% tascar_fixtures_calib - calibrate TASCAR light fixtures
  if nargin == 0
    s_layout = [];
    varargin{end+1} = 'help';
    disp(help(mfilename));
  end
  if (nargin == 1) && ischar(s_layout) && strcmp(s_layout, 'help' )
    varargin{end+1} = 'help';
    s_layout = [];
  end
  sCfg = struct;
  sHelp = struct;
  sCfg.scenename = 'lightscene';
  sHelp.scenename = 'Scene name of fixture layout';
  sCfg.capturefun = @internal_capture_enter;
  sHelp.capturefun = 'Capture function handle';
  sCfg.preparefun = @internal_prepare_fixture;
  sHelp.preparefun = 'Fixture prepare function handle';
  sCfg.hostname = 'localhost';
  sHelp.hostname = 'TASCAR hostname';
  sCfg.values = [[0:2:14],[15:4:30],[31:16:255]];
  sHelp.values = 'Calibration input values';
  sCfg.port = 9877;
  sHelp.port = 'TASCAR port number';
  %
  sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
  if isempty(sCfg)
    return;
  end
  %
  send_osc(sCfg.hostname,sCfg.port,['/light/',sCfg.scenename,'/usecalib'],0);
  doc = tascar_xml_open( s_layout );
  cxmlFix = tascar_xml_get_element( doc, 'fixture' );
  sData = struct();
  for kfix=1:numel(cxmlFix)
    xmlFix = cxmlFix{kfix};
    label = tascar_xml_get_attribute( xmlFix, 'label' );
    if isempty(label)
      label = ['fixture',num2str(kfix-1)];
    end
    tascar_xml_rm_element( xmlFix, 'calib' );
    javaMethod('setTextContent',xmlFix,'');
    sCfg.preparefun(label);
    rgb_sent = sCfg.values([1,1,1],:)';
    rgb_measured = rgb_sent;
    for k=1:size(rgb_sent,1)
      rgb = rgb_sent(k,:);
      send_osc(sCfg.hostname,sCfg.port,...
	       ['/light/',sCfg.scenename,'/',label,'/dmx'],...
	       rgb(1),rgb(2),rgb(3));
      rgb_measured(k,:) = sCfg.capturefun();
    end
    create_calib( doc, xmlFix, rgb_sent, rgb_measured );
    send_osc(sCfg.hostname,sCfg.port,...
	     ['/light/',sCfg.scenename,'/',label,'/dmx'],...
	     0,0,0);
    sData(kfix) = struct('sent',rgb_sent,...
			 'measured',rgb_measured,...
			 'label',label);
  end
  tascar_xml_save( doc, s_layout );
  
function rgb = internal_capture_enter
  r = input('red = ');
  g = input('green = ');
  b = input('blue = ');
  rgb = [r,g,b];
  
function internal_prepare_fixture( label )
  r = input(['prepare for fixture "',...
	     label,...
	     '". Type enter to continue.']);
  
function create_calib( doc, xmlFix, rgb_sent, rgb_measured )
  for c=1:size(rgb_sent,2)
    v_meas = rgb_measured(:,c);
    v_sent = rgb_sent(:,c);
    for k=1:size(v_sent,1)
      idx_eq = find(v_meas==v_meas(k));
      [tmp,idx] = min(v_sent(idx_eq));
      if (v_meas(k) > 0) && (idx_eq(idx)==k)
	tascar_xml_add_element(doc, xmlFix,...
			       'calib',[],...
			       'channel',sprintf('%d',c-1),...
			       'in',sprintf('%g',v_meas(k)),...
			       'out',sprintf('%g',v_sent(k)));
      end
    end
  end
