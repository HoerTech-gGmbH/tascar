function tascar_profplot( data, labels )
if nargin < 2
    labels = {};
    for k=1:size(data,1)-1
        labels{end+1} = num2str(k);
    end
end
data = data(2:end,:)';
tmax = min(max(data(:)),20*median(data(:)));
dt = max(tmax/100,1e-6);
edges = 0:dt:tmax;
[h,bins] = hist(data,edges);
h = h/size(data,1);
figure
plot(1e6*bins,h,'linewidth',2);
xlim([0,1e6*tmax]);
xlabel('time per cycle / microseconds');
ylabel('relative frequency');
title(['cycle time histogram, ',num2str(size(data,1)),' samples']);
legend(labels);
figure
barh(1e6*max(data));
set(gca,'YTick',1:numel(labels),'YTickLabels',labels,'YDir','reverse');
title('maximum cycle time');
xlabel('time per cycle / microseconds');