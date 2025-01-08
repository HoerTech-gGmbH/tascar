function gains = tascar_plot_speakerresp( layoutname )
% tascar_plot_speakerresp - plot loudspeaker frequency response stored in layout file
%
% tascar_plot_speakerresp( layoutname )
  doc = tascar_xml_open( layoutname );
  hspk = tascar_xml_get_element( doc, 'speaker' );
  csLabels = {};
  fh = figure();
  vgains = [];
  mgains = [];
  for k=1:numel(hspk)
    vf = str2num(tascar_xml_get_attribute( hspk{k}, 'eqfreq' ));
    vg = str2num(tascar_xml_get_attribute( hspk{k}, 'eqgain' ));
    l = char(tascar_xml_get_attribute( hspk{k}, 'label' ));
    mgains(end+1,:) = vg;
    if isempty(l)
      l = num2str(k);
    end
    csLabels{end+1} = l;
    g = str2num(tascar_xml_get_attribute( hspk{k}, 'gain' ));
    vgains(end+1) = g;
  end
  mgains_mean = mean(mgains);
  mgains_pstd = mgains_mean + std(mgains);
  mgains_mstd = mgains_mean - std(mgains);
  ph = patch([vf,vf(end:-1:1)],[mgains_pstd,mgains_mstd(end:-1:1)],0,'facecolor',[0.9,0.9,0.9]);
  hold on;
  plot(vf,mean(mgains),'-','linewidth',4,'Color',0.7*ones(1,3));
  ph = plot(vf,mgains);
  hold off;
  vxf = round(1000*2.^(-4:1:4));
  xlim([min(vf),max(vf)]);
  grid('on');
  legend(ph, csLabels,'Location','BestOutside');
  set(gca,'XScale','log','XTick',vxf,'XTickLabel',num2str(vxf'));
  xlabel('frequency / Hz');
  ylabel('correction gain / dB');
  [tmp_p,layout_base,layout_ext] = fileparts(layoutname);
  title(layout_base);
  if nargout > 0
    gains = vgains;
  end
end