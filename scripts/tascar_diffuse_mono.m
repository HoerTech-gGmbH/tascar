function tascar_diffuse_mono( infile, outfile, ofs )
% TASCAR_DIFFUSE_MONO - create diffuse FOA file from mono signal
%
% Usage:
% tascar_diffuse_mono( infile, outfile, ofs )
%
% infile  : input file name
% outfile : output file name
% ofs     : optional directional shift (for intermediate
%           diffuseness)
%
% Example:
%
% Completely diffuse frying pan:
% tascar_diffuse_mono('/usr/share/tascar/examples/sounds/pan.wav','diffpan.wav');
%
% Frying pan, slightly diffuse from frontal direction:
% tascar_diffuse_mono('/usr/share/tascar/examples/sounds/pan.wav','diffpan.wav',2);
%

if nargin < 3
    ofs = [0 0 0];
end
ofs(end+1:3) = 0;
[sig,fs] = audioread( infile );
sig = sig(:,1);
W = realfft(sig);
N = numel(W);
dir = 2*rand(N,3)-1+repmat(ofs,[N,1]);
dir = sqrt(2)*dir ./repmat(sqrt(sum(dir.^2,2)),[1,3]);
X = W.*dir(:,1);
Y = W.*dir(:,2);
Z = W.*dir(:,3);
sig = realifft([W,X,Y,Z]);
audiowrite( outfile, sig, fs );