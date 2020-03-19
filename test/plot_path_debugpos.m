function plot_path_debugpos( name, b_render )
% plot_path_debugpos( name, b_render )
if nargin < 2
    b_render = false;
end
if b_render
    sCmd = ['LD_LIBRARY_PATH=../libtascar/build/:../plugins/build/ ' ...
	    'RECTYPE=debugpos ../apps/build/tascar_renderfile'];
    system(sprintf('%s -i zeros.wav -d -f 64 -o %s.wav %s.tsc',sCmd,name,name));
end
d = audioread([name,'.wav']);
d = d(1:64:end,:);
len = size(d,1);
nobj = size(d,2)/4;
xmax = max(max(d(:,1:4:4*nobj)))+1;
xmin = min(min(d(:,1:4:4*nobj)))-1;
ymax = max(max(d(:,2:4:4*nobj)))+1;
ymin = min(min(d(:,2:4:4*nobj)))-1;
zmax = max(max(d(:,3:4:4*nobj)))+1;
zmin = min(min(d(:,3:4:4*nobj)))-1;
hold off
plot3([xmin,xmax],[0,0],[0,0],'r-');
hold on
plot3([0,0],[ymin,ymax],[0,0],'g-');
plot3([0,0],[0,0],[zmin,zmax],'b-');
plot3(d(:,1:4:4*nobj),d(:,2:4:4*nobj),d(:,3:4:4*nobj),'.');
for k=0:4
    l = 1+floor((len-1)*k/4);
    plot3(d(l,1:4:4*nobj),d(l,2:4:4*nobj),d(l,3:4:4*nobj),'ko');
    text(d(l,1:4:4*nobj),d(l,2:4:4*nobj),d(l,3:4:4*nobj),sprintf(' %d',k));
end
set(gca,'DataAspectRatio',[1,1,1],'Xlim',[xmin,xmax],'YLim',[ymin,ymax],'ZLim',[zmin,zmax]);
box('on');