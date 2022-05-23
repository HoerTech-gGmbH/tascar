function [d,t] = tascar_dl_resample( data, name, fs, range, nup )
% tascar_dl_resample - resample TASCAR data from data logging module
%
% [d,t] = tascar_dl_resample( data, name, fs [, range [, nup ] ] )
%
% data : cell array
% name : variable name
% fs   : new sampling rate in Hz
% range : time range in seconds (default: full range)
% nup : upsampling ratio (default: automatic)
%
% Example:
% load unnamed_20220502_093828.mat
% [d,t] = tascar_dl_resample(data,'ovheadtracker_acc', 25);
dataidx = [];
for k=1:numel(data)
    if strcmp(data{k}.name,name)
        dataidx = k;
    end
end
if isempty(dataidx)
    error(['variable "',name,'" not found in data.']);
end
data = data{dataidx};
t_orig = data.data(1,:);
fs_orig = 1./median(diff(t_orig));
if (nargin < 4) || isempty(range)
    range = [min(t_orig),max(t_orig)];
end
if nargin < 5
    nup = round(max(16,2*fs/fs_orig));
end
irange = [find(t_orig<=range(1),1,'last'),...
          find(t_orig>=range(2),1,'first')];
irange = irange(1):irange(2);
if isempty(irange)
    error('invalid time range');
end
data.data = data.data(:,irange);
t = [range(1)-((nup-1)/(nup*fs)):(1/(nup*fs)):range(2)]';
d = interp1(data.data(1,:)',data.data(2:end,:)',t,...
            'linear','extrap');
d = lp( d, 2*nup );
t = t(nup:nup:end,:);
d = d(nup:nup:end,:);

function d = lp( d, n )
len = size(d,1);
n2 = ceil(n/2);
d(end+[1:n],:) = repmat(d(end,:),[n,1]);
B = ones(n,1)/n;
d = fftfilt( B, d );
d = d(n2+[1:len],:);
