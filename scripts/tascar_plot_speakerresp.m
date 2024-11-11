function tascar_plot_speakerresp( layoutname )
% tascar_plot_speakerresp - plot loudspeaker frequency response stored in layout file
%
% tascar_plot_speakerresp( layoutname )
  doc = tascar_xml_open( layoutname );
  hspk = tascar_xml_get_element( doc, 'speaker' );
  csLabels = {};
  fh = figure();
  vgains = [];
  for k=1:numel(hspk)
    vf = str2num(tascar_xml_get_attribute( hspk{k}, 'eqfreq' ));
    vg = str2num(tascar_xml_get_attribute( hspk{k}, 'eqgain' ));
    l = char(tascar_xml_get_attribute( hspk{k}, 'label' ));
    if isempty(l)
      l = num2str(k);
    end
    csLabels{end+1} = l;
    g = str2num(tascar_xml_get_attribute( hspk{k}, 'gain' ));
    vgains(end+1) = g;
    plot(vf,vg);
    hold on;
  end
  hold off;
  vxf = round(1000*2.^(-4:1:4));
  legend(csLabels,'Location','BestOutside');
  set(gca,'XScale','log','XTick',vxf,'XTickLabel',num2str(vxf'));
  xlabel('frequency / Hz');
  ylabel('correction gain / dB');
  vgains
end