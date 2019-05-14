function tascar_fixtures_plot_calib( sData )
% tascar_fixtures_plot_calib - plot fixture calibration data
% structure
%
% tascar_fixtures_plot_calib( sData )
  for k=1:numel(sData)
    figure('Name',sData(k).label);
    subplot(3,1,1);
    hs = sData(k).sent;
    hm = sData(k).measured;
    h1 = plot( hs, '--', 'linewidth', 1);
    hold on;
    h2 = plot( hm, 'linewidth', 2);
    set([h1(1);h2(1)],'Color',[1,0,0]);
    set([h1(2);h2(2)],'Color',[0,1,0]);
    set([h1(3);h2(3)],'Color',[0,0,1]);
    legend(h2,{'R','G','B'},'Location','NorthWest');
    ylim([0,256]);
    subplot(3,1,2);
    hs = rgb2hsv(sData(k).sent/255);
    hm = rgb2hsv(sData(k).measured/255);
    h1 = plot( hs, '--', 'linewidth', 1);
    hold on;
    h2 = plot( hm, 'linewidth', 2);
    set([h1(1);h2(1)],'Color',[0.2,0.7,0.2]);
    set([h1(2);h2(2)],'Color',[1,0,1]);
    set([h1(3);h2(3)],'Color',[0,0,0]);
    legend(h2,{'H','S','V'},'Location','NorthWest');
    subplot(3,1,3);
    imdata = [];
    imdata(1,:,:) = sData(k).sent/255;
    imdata(2,:,:) = sData(k).measured/255;
    image(imdata);
  end