function snrdisplay

javaaddpath ../netutil-1.0.0.jar
  addpath('/usr/lib/openmha/mfiles');
addpath /usr/share/tascar/matlab/ % where the useful MATLAB scripts are stored 

  javaaddpath('/usr/lib/openmha/mfiles/mhactl_java.jar');
  sCfg.mha = struct('host','localhost','port',33339);
  fh = figure('NumberTitle','off','MenuBar','none','Name','SNR display');

  %processed/unprocessed button
  sCfg.ui.bypass = uicontrol('Style','togglebutton','Units','normalized',...
      'position',[0,0,1,0.2],'string','hearing aid off',...
      'BackgroundColor',0.7*ones(1,3),'FontSize',32,'callback',@cb_bypass);

  ax1 = axes('Units','normalized','OuterPosition',[0,0.25,0.7,0.75]);
  ax2 = axes('Units','normalized','OuterPosition',[0.7,0.25,0.3,0.75]);

  %sCfg.ui.snr1 = uicontrol('Style','text','Units','normalized','position',[0,0.3,1/3,0.3],'FontSize',32);
  %sCfg.ui.snr2 = uicontrol('Style','text','Units','normalized','position',[1/3,0.3,1/3,0.3],'FontSize',32);
  %sCfg.ui.dsnr = uicontrol('Style','text','Units','normalized','position',[2/3,0.3,1/3,0.3],'FontSize',32,'ForeGroundColor',[0.7,0,0],'fontweight','bold');
  %uicontrol('Style','text','Units','normalized','position',[0,0.7,1/3,0.3],'FontSize',32,'String','input');
  %uicontrol('Style','text','Units','normalized','position',[1/3,0.7,1/3,0.3],'FontSize',32,'String','output');
  %uicontrol('Style','text','Units','normalized','position',[2/3,0.7,1/3,0.3],'FontSize',32,'ForeGroundColor',[0.7,0,0],'fontweight','bold','String','benefit');

  set(fh,'UserData',sCfg);
LEVELS = mha_get(sCfg.mha,'mha.calib_in.rmslevel');
SNR_unproc=LEVELS(1:2)-LEVELS(3:4);
SNR_proc=LEVELS(5:6)-LEVELS(7:8);
BEN=SNR_proc-SNR_unproc;

SNRmatrix=[SNR_unproc;SNR_proc]
hBar = bar(ax1, SNRmatrix);
hBarBenefit = bar(ax2, zeros(2,2));
set(ax1,'Xticklabel',{'unprocessed','processed'})
set(ax2,'Xticklabel',{'benefit'})
legend(ax1,'left ear','right ear','Location','SouthEast')
ylabel(ax1,'SNR [dB]')
set(ax1,'ylim',[-20,10]);
ylabel(ax2,'SNR [dB]')
set(ax2,'ylim',[-5,5],'xlim',[0.5,1.5]);
drawnow
  while ishandle(fh)
    %sData = mha_get(sCfg.mha,'mha.ch');
    %L1 = mha_get(sCfg.mha,'mha.L1.rms');
    %L2 = mha_get(sCfg.mha,'mha.L2.rms');
    %snr1 = L1(3:4)-L1(5:6);
    %snr2 = L2(3:4)-L2(5:6);
    %dsnr = snr2 - snr1;
    %set(sCfg.ui.snr1,'string',sprintf('%1.1f dB ',snr1));
    %set(sCfg.ui.snr2,'string',sprintf('%1.1f dB ',snr2));
    %set(sCfg.ui.dsnr,'string',sprintf('%1.1f dB ',dsnr));
    %pause(0.2);
LEVELS = mha_get(sCfg.mha,'mha.calib_in.rmslevel');
SNR_unproc=LEVELS(1:2)-LEVELS(3:4);
SNR_proc=LEVELS(5:6)-LEVELS(7:8);
BEN=SNR_proc-SNR_unproc;

SNRmatrix=[SNR_unproc;SNR_proc];
set(hBar(1),'YData',SNRmatrix(:,1));
set(hBar(2),'YData',SNRmatrix(:,2));
set(hBarBenefit(1),'YData',BEN(1));
set(hBarBenefit(2),'YData',BEN(2));

drawnow();
  end

function cb_bypass( varargin )
  sCfg = get(gcbf(),'UserData');
  bp = get(sCfg.ui.bypass,'Value');
  if ~bp
    set(gcbo(),'BackgroundColor',[0.7,0.7,0.7],'String','hearing aid off');
    ac2spec.gainA = 1;
    ac2spec.gainB = 0;
  else
    set(gcbo(),'BackgroundColor',[1,1,0],'String','hearing aid on');
    ac2spec.gainA = 0;
    ac2spec.gainB = 1;
  end
send_osc('localhost', 9999, '/unproc/lingain',ac2spec.gainA)
send_osc('localhost', 9999, '/proc/lingain',ac2spec.gainB)
