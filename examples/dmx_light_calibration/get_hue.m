addpath('/usr/share/tascar/matlab/');
javaaddpath('netutil-1.0.0.jar');

send_osc('localhost',9877,'/light/calib/usecalib',0);

hsv = [];
for v=150/255
  for h=[[0:1/24:(1-1/24)],2]
    if h==2
      hsv(end+1,:) = [0,0,v];
    else
      hsv(end+1,:) = [h,1,v];
    end
  end
end
rgb = 255*hsv2rgb(hsv);
orgb = rgb;
cFixtures = {'d40','flat18'};
sData = struct;
for kfix=1:numel(cFixtures)
  label = cFixtures{kfix};
  prepare_cam( label );
  for k=1:size(rgb,1)
    set_dmx( label, rgb(k,:) );
    orgb(k,:) = get_rgb();
  end
  set_dmx( label, [0,0,0] );
  sData(kfix) = struct('sent',rgb,...
		       'measured',orgb,...
		       'label',label);
end
tascar_fixtures_plot_calib(sData);
