function jazzpos
% Tool for TASCAR position editing
addpath('/usr/share/tascar/matlab');
javaaddpath('../netutil-1.0.0.jar');
hTSC = struct('host','localhost','port',9877);
csInstruments = {'Voc','Trumpet','Drumset','Guitar','Bass', ...
                 'Keyboards'};
vFig = findobj('tag','jazzpos');
if isempty(vFig)
    fh = figure('tag','jazzpos');
else
    fh = vFig(1);
    close(vFig(2:end));
    figure(fh);
end
hold off;
vPlot = [];
vText = [];
plot([-5,5],[0,0],'k--');
hold on;
plot([0,0],[-5,5],'k--');
for k=1:numel(csInstruments)
    vPlot(k) = plot(k-4,0,'o','MarkerSize',8);
    col = get(vPlot(k),'Color');
    set(vPlot(k),'MarkerFaceColor',col);
    %disp(sprintf('#%02x%02x%02x',round(255*col)));
    vText(k) = text(k-4.1,-0.2,csInstruments{k});
    send_osc(hTSC,['/scene/',csInstruments{k},'/pos'],0,-(k-4),0);
end
sCfg = struct;
sCfg.tsc = hTSC;
sCfg.vp = vPlot;
sCfg.vt = vText;
sCfg.ci = csInstruments;
sCfg.vs = [cell2mat(get(sCfg.vp,'XData')),cell2mat(get(sCfg.vp,'YData'))];
set(fh,'UserData',sCfg);
set(gca,'ButtonDownFcn',@selectfun,'DataAspectRatio',[1,1,1],'XLim',[-6,6],'YLim',[-4,4]);
set(get(gca,'Children'),'ButtonDownFcn',@selectfun);

function selectfun( varargin )
p = get(gca,'CurrentPoint');
p = 0.01*round(100*p(1,1:2));
sCfg = get(gcf,'UserData');
[tmp,idx] = min(sum((sCfg.vs-repmat(p,[size(sCfg.vs,1),1])).^2,2));
set(sCfg.vp(idx),'XData',p(1),'YData',p(2));
set(sCfg.vt(idx),'Position',[p(1)-0.1,p(2)-0.2,0]);
sCfg.vs(idx,:) = p;
set(gcf,'UserData',sCfg);
send_osc(sCfg.tsc,['/scene/',sCfg.ci{idx},'/pos'],p(2),-p(1),0);
disp('<!-- positions -->');
for k=1:numel(sCfg.ci)
    disp(sprintf('  <!-- %s --><position>0 %g %g 0</position>',sCfg.ci{k},sCfg.vs(k,2),-sCfg.vs(k,1)));
end
