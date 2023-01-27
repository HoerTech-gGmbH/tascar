function tascar_profplot( data, labels )
if nargin < 2
    labels = {};
    for k=1:size(data,1)-1
        labels{end+1} = num2str(k);
    end
end
data = data(2:end,:)';
tmax = min(max(data(:)),20*median(data(:)));
dt = max(tmax/256,1e-6);
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
barh(1e6*mean(data));
hold on;
for k=1:size(data,2)
  plot([0,1e6]*max(data(:,k)),[k,k],'k-','linewidth',2);
  plot([1e6,1e6]*max(data(:,k)),[k-0.2,k+0.2],'k-','linewidth',2);
end
set(gca,'YTick',1:numel(labels),'YTickLabel',labels,'YDir','reverse',...
   'YLim',[0.5,0.5+size(data,2)]);
title('average/maximum cycle time');
xlabel('time per cycle / microseconds');
end

