function tascar_plot_speakerlayout( layoutname )
% tascar_plot_speakerlayout - Create a 3D plot of a speaker layout.
%
% tascar_plot_speakerlayout( layoutname )
%
%
  doc = tascar_xml_open( layoutname );
  elem_spk = tascar_xml_get_element( doc, 'speaker' );
  figure
  plot3([0,1],[0,0],[0,0],'r-');
  hold on;
  plot3([0,0],[0,1],[0,0],'g-');
  plot3([0,0],[0,0],[0,1],'b-');
  plot3(0,0,0,'k-');
  vP = [];
  vgains = [];
  for k=1:numel(elem_spk)
    vgains(k) = get_attr_num( elem_spk{k}, 'gain', 0 );
  end
  mingain = min(vgains);
  maxgain = max(vgains);
  g = (vgains-mingain)/max(1,(maxgain-mingain));
  for k=1:numel(elem_spk)
    az = get_attr_num( elem_spk{k}, 'az', 0 );
    el = get_attr_num( elem_spk{k}, 'el', 0 );
    r = get_attr_num( elem_spk{k}, 'r', 1 );
    l = tascar_xml_get_attribute( elem_spk{k}, 'label' );
    if ~isfinite(r)
	    msg = sprintf(' invalid %d: %s',k,l);
	    col = [0.7,0.1,0.1];
	    r = 1;
    else
	    col = [0.1,0.7,0.1];
	    msg = sprintf(' %d: %s',k,l);
    end
    [x,y,z] = sph2cart(az*pi/180,el*pi/180,r);
    plot3(x,y,z,'bo','linewidth',2,'markersize',1+g(k)*20);
    text(x,y,z,msg,'Color',col,'fontweight','bold');
    vP(end+1,:) = [x,y,z];
  end
  plot3(vP(:,1),vP(:,2),vP(:,3),'k-','Color',0.7*ones(1,3));
  plot3(vP(:,1),vP(:,2),vP(:,3),'b.');
  xlim([min(vP(:,1)),max(vP(:,1))]+[-0.5,0.5]);
  ylim([min(vP(:,2)),max(vP(:,2))]+[-0.5,0.5]);
  zlim([min(vP(:,3)),max(vP(:,3))]+[-0.5,0.5]);
  xlabel('x / m');
  ylabel('y / m');
  zlabel('z / m');
  set(gca,'DataAspectRatio',[1,1,1]);
end

function v = get_attr_num( el, name, def )
  s = tascar_xml_get_attribute( el, name );    
  if isempty(s)
    v = def;
  else
    v = str2num(s);
  end
end