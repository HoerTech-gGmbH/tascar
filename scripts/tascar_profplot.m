function tascar_profplot( data, labels )
if nargin < 2
    labels = {};
    for k=1:size(data,1)-1
        labels{end+1} = num2str(k);
    end
end
[h,bins] = hist(data(2:end,:)',100);
h = h/size(data,2);
figure
plot(bins,h,'linewidth',2);
legend(labels);