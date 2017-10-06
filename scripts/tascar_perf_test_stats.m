function tascar_perf_test_stats( )
  sd = libsd();
  sData = tascar_perf_merge_data();
  nG = numel(sData.values);
  [tmp,kcpu] = sd.getfield(sData,'cpu');
  [tmp,kCPU] = sd.getfield(sData,'CPUmodel');
  [tmp,kRec] = sd.getfield(sData,'ReceiverType');
  [tmp,kPer] = sd.getfield(sData,'Period');
  sData.values{kCPU} = strrep(sData.values{kCPU},'Intel(R)Core(TM)','');
  v = ver();
  v = {v.Name};
  if isempty(strmatch('Octave',v,'exact'))
    disp('MATLAB');
    f_anova = @anova1;
  else
    disp('Octave');
    f_anova = @anova;
  end
  for k=1:nG
      tau_p = 0.01*sData.data(:,kcpu).*sData.data(:,kPer);
    p = f_anova(tau_p,sData.data(:,k));
    if p > 0.05
      disp([sData.fields{k},' is not a significant factor']);
    else
      disp([sData.fields{k},' is a significant factor']);
    end
  end
  disp(' ');
  for k1=1:numel(sData.values{kCPU})
    sTmp1 = sd.restrict(sData,kCPU,k1);
    for k2=1:numel(sTmp1.values{kRec})
      sTmp = sd.restrict(sTmp1,kRec,k2);
      a_fit = tascar_perf_fitmodel(sTmp);
      disp([sprintf('%s & %s ',sData.values{kCPU}{k1},sData.values{kRec}{k2}),...
	    sprintf('& %.2g ',a_fit),...
	    sprintf('& %d & %d\\\\',max_k( 0.90, 1024, 8, a_fit),max_k( 0.90, 1024, 48, a_fit))]);
      plot_example_performance( sTmp, sd, a_fit, sData.values{kCPU}{k1},sData.values{kRec}{k2} );
    end
  end
  
function K = max_k( C, P, N, a )
  K = floor((C-a(1)/P-a(4)*N)/(a(2)/P+a(3)+a(5)*N));

function plot_example_performance( sData, sd, a, sCPU, sRec )
    [tmp,kSrc] = sd.getfield(sData,'NrSources');
    [tmp,kcpu] = sd.getfield(sData,'cpu');
    [tmp,kSpk] = sd.getfield(sData,'NrSpeakers');
    [tmp,kPer] = sd.getfield(sData,'Period');
    P = 1024;
    sData = sd.restrict(sData,kPer,find(sData.values{kPer}==P));
    vx = sData.values{kSrc};
    sPlotPars = struct;
    sPlotPars.colors = {'k'};
    sPlotPars.markers = {'d','o','s'};
    sPlotPars.linestyles = {''};
    [hP,tmp,vp] = sd.plot( sData, kSrc, kcpu, sPlotPars, 'parameter', kSpk );
    hold on;
    %vp = findobj(hP,'markerstyle','d')
    set(vp,'XData',vx)
    set(gca,'XTick',[1,10,100,1000],'XTickLabel',[1,10,100,1000],'XLim',[0.5,2000],'XScale','log');
    %plot([0.5,4.5],[100,100],'k-');
    plot([0.5,2000],[90,90],'k-');
    ylim([0,100]);
    vK = 10.^[0:0.1:3];
    for vN=[8,48,128]
        C = a(1)./P + a(2).*vK./P + a(3).*vK + a(4).*vN + a(5).*vN.*vK;
        plot(vK,100*C,'k-','color',0.7*[1,1,1],'linewidth',3);
    end
    k = max_k( 0.90, P, 8, a);
    plot([k,k],[0,90],'k--')
    k = max_k( 0.90, P, 48, a);
    plot([k,k],[0,90],'k--')
    k = max_k( 0.90, P, 128, a);
    plot([k,k],[0,90],'k--')
    copyobj(vp,gca);
    legend(vp,'Location','West')
    set(hP,'Name',[sCPU,'-',sRec]);
    xlabel('K');
    ylabel('C / %');
    saveas(hP,['cpuload_',sCPU,'_',sRec,'.eps'],'epsc');