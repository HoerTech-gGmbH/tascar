function plot_rt30( ir, fs )
% plot_rt30 - plot RT30 as a function of frequency
%
% Usage:
% plot_rt30( ir, fs )
%
% (c) 2019 Giso Grimm
if size(ir,2) > 1
    ir = ir(:,1);
    warning('Using only first channel');
end
vf = 1000*2.^[-4:1/3:(log2(fs/2000)-1/3)];
vT30 = zeros(numel(vf),1);
vEDT = zeros(numel(vf),1);
csLabel = {};
for k=1:numel(vf)
    [B,A] = butter(1,vf(k)*[2.^([-1/6,1/6])]/(0.5*fs));
    ir_f = filter(B,A,ir);
    [vT30(k),vEDT(k)] = t60( ir_f, fs );
    if vf(k) < 1000
        csLabel{k} = sprintf('%1.4g',vf(k));
    else
        csLabel{k} = sprintf('%1.4gk',vf(k)/1000);
    end
end
figure
plot(vf,vT30,'linewidth',1.2);
hold on
plot(vf,vEDT,'linewidth',1.2);
set(gca,'XScale','log','XTick',vf(1:3:end),...
        'XTickLabel',csLabel(1:3:end),...
        'XLim',[vf(1)*2^(-1/6),vf(end)*2^(1/6)]);
legend({'T30','EDT'});
xlabel('frequency / Hz');
ylabel('revereberation time / s');