function plot_path_debugpos( name, b_render )
if nargin < 2
    b_render = false;
end
if b_render
    sCmd = 'LD_LIBRARY_PATH=../build/ ../build/tascar_renderfile';
    system(sprintf('%s -i zeros.wav -d -f 64 -o %s.wav %s.tsc',sCmd,name,name));
end
d = audioread([name,'.wav']);
d = d(1:64:end,:);
len = size(d,1);
hold off
plot(d(:,1:3:end),d(:,2:3:end),'.');
hold on
for k=0:4
    l = 1+floor((len-1)*k/4);
    plot(d(l,1:3:end),d(l,2:3:end),'ko');
    text(d(l,1:3:end),d(l,2:3:end),sprintf(' %d',k));
end
set(gca,'DataAspectRatio',[1,1,1]);