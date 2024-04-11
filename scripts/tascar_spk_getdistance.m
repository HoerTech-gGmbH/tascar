function tascar_spk_getdistance( layoutfile, varargin )
% tascar_spk_getdistance
%
% Usage:
%
% tascar_spk_getdistance( layoutfile [, key, value ] )
%
% layoutfile: tascar speaker layout file
%
% To see a list of optional keys type 'tascar_spk_getdistance help'.
%
% Meansure loudspeaker distance to measurement microphone. Optionally
% estimate elevation angle from distance and vertical loudspeaker
% position.
%
% The updated speaker layout file will be stored as [layoutfile,'.dist'].
  
% 2020-06-09 Giso Grimm

  if (nargin < 1)
    varargin = {'help'};
    help('tascar_spk_getdistance')
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
  sCfg.plot = true;
  sHelp.plot = 'plot 3D speaker layout?';
  sCfg.z2el = false;
  sHelp.z2el = 'convert z attribute into el?';
  sCfg.z0 = 0;
  sHelp.z0 = 'z offset in m';
  sCfg.peak = 0.1;
  sHelp.peak = 'peaklevel (linear) of sweeps';
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
    d = get_attr_num( elem_spk{k}, 'r', 1 );
    if isempty(d)
      d = 1;
    end
    vDistFile(k) = d;
  end
  len = 2^ceil(log2((4*max(vDistFile)/sCfg.c+0.1)*fs + 4*fragsize));

  mIR = zeros(len,numel(elem_spk));
  csLabels= {};
  [B,A] = butter(5,[200,8000]/(0.5*fs));
  for k=1:numel(elem_spk)
    connect = tascar_xml_get_attribute( elem_spk{k}, 'connect' );
    csLabels{k} = tascar_xml_get_attribute( elem_spk{k}, 'label' );
    az = get_attr_num( elem_spk{k}, 'az', 0 );
    el = get_attr_num( elem_spk{k}, 'el', 0 );
    r = get_attr_num( elem_spk{k}, 'r', 1 );
    ir = tascar_ir_measure( 'output', char(connect), ...
			    'input', sCfg.input, ...
			    'len', len,...
			    'gain',sCfg.peak,...
			    'nrep',2);
    ir = filter(B,A,ir);
    mIR(:,k) = ir;
    idx = irs_firstpeak(ir);
    [tmp,idx0] = max(abs(ir(idx+[0:round(0.005*fs)])));
    idx = idx+idx0-1;
    if( abs(ir(idx))/sqrt(mean(ir.^2)) > 5 )
      vDistMeas(k) = sCfg.c*idx/fs;
    else
      vDistMeas(k) = inf;
    end
  end
  if sCfg.plot
      figure
      plot(mIR);
  end
  if( sCfg.dref == 0 )
    sCfg.dref = vDistFile(sCfg.cref);
  end
  vDistFile = vDistMeas-vDistMeas(sCfg.cref)+sCfg.dref;
  vDistFile(find(vDistFile<=0)) = inf;
  for k=1:numel(elem_spk)
    if ~isfinite(vDistFile(k))
      warning(sprintf('invalid distance for channel %d (''%s'')',...
		      k,...
		      csLabels{k}));
    end
    tascar_xml_set_attribute( elem_spk{k}, 'r', vDistFile(k), '%1.2f' );
    if sCfg.z2el
      z = get_attr_num( elem_spk{k}, 'z', 0 ) - sCfg.z0;
      r = vDistFile(k);
      el = 180*asin(z/r)/pi;
      if z > r
	el = 90;
      end
      if z < -r
	el = -90;
      end
      tascar_xml_set_attribute( elem_spk{k}, 'el', el, '%1.1f' );
      javaMethod('removeAttribute',elem_spk{k},'z');
    end
  end
  tascar_xml_save(doc_spk,[layoutfile,'.dist']);
  if( sCfg.plot )
    figure
    plot3([0,1],[0,0],[0,0],'r-');
    hold on;
    plot3([0,0],[0,1],[0,0],'g-');
    plot3([0,0],[0,0],[0,1],'b-');
    plot3(0,0,0,'k-');
    vP = [];
    for k=1:numel(elem_spk)
      az = get_attr_num( elem_spk{k}, 'az', 0 );
      el = get_attr_num( elem_spk{k}, 'el', 0 );
      r = get_attr_num( elem_spk{k}, 'r', 1 );
      if ~isfinite(r)
	msg = sprintf(' invalid %d: %s',k,csLabels{k});
	col = [0.7,0.1,0.1];
	r = 1;
      else
	col = [0.1,0.7,0.1];
	msg = sprintf(' %d: %s',k,csLabels{k});
      end
      [x,y,z] = sph2cart(az*pi/180,el*pi/180,r);
      plot3(x,y,z,'b*','linewidth',3);
      text(x,y,z,msg,'Color',col,'fontweight','bold');
      vP(end+1,:) = [x,y,z];
    end
    plot3(vP(:,1),vP(:,2),vP(:,3),'k-');
    xlim([min(vP(:,1)),max(vP(:,1))]+[-0.5,0.5]);
    ylim([min(vP(:,2)),max(vP(:,2))]+[-0.5,0.5]);
    zlim([min(vP(:,3)),max(vP(:,3))]+[-0.5,0.5]);
    xlabel('x / m');
    xlabel('y / m');
    xlabel('z / m');
    set(gca,'DataAspectRatio',[1,1,1]);
  end

function v = get_attr_num( el, name, def )
  s = tascar_xml_get_attribute( el, name );    
  if isempty(s)
    v = def;
  else
    v = str2num(s);
  end


function b = isoctave()
  b = ~isempty(ver('Octave'));
