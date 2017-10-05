function plot_signal( name, b_render )
if nargin < 2
    b_render = false;
end
if b_render
    sCmd = 'LD_LIBRARY_PATH=../build/ ../build/tascar_renderfile';
    system(sprintf('%s -i zeros.wav -d -f 64 -o %s.wav %s.tsc',sCmd,name,name));
end
d = audioread([name,'.wav']);
plot(d);