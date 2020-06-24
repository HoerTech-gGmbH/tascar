function [x,fs] = tascar_measure_receiverpattern
n_doc = tascar_xml_doc_new();
% session (root element):
n_session = tascar_xml_add_element(n_doc,n_doc,'session');
n_scene = tascar_xml_add_element(n_doc,n_session,'scene');
% source
n_src = tascar_xml_add_element(n_doc, n_scene, 'source');
n_sound = tascar_xml_add_element(n_doc, n_src, 'sound',[],'x','1');
% receiver:
N = 72;
vaz = arg(exp(i*(2*pi*([1:N]-1)/N + pi)));
type = 'ortf';
%type = 'modortf';
%type = 'hrtf';
for n=1:N
    az = vaz(n);
    switch type
     case 'ortf'
      n_rec = tascar_xml_add_element(n_doc,n_scene,'receiver',[],...
				     'type','ortf',...
				     'name', sprintf('out_%d',n));
     case 'modortf'
      n_rec = tascar_xml_add_element(n_doc,n_scene,'receiver',[],...
				     'type','ortf',...
				     'name', sprintf('out_%d',n),...
				     'angle','140',...
				     'f6db','12000',...
				     'fmin','3000');
     case 'hrtf'
      n_rec = tascar_xml_add_element(n_doc,n_scene,'receiver',[],...
				     'type','hrtf',...
				     'name', sprintf('out_%d',n));
    end
    tascar_xml_add_element(n_doc, n_rec, ...
                           'orientation',sprintf('0 %g 0 0',az*180/pi));
end  
tascar_xml_save( n_doc, 'temp_rec.tsc' );
system(['LD_LIBRARY_PATH='''' ',...
        'tascar_renderir temp_rec.tsc -o temp_rec.wav']);
[x,fs] = audioread('temp_rec.wav');
x = x(:,1:2:end);
cf = 1000*2.^[-4:(1/3):4];
bw = 1/3;
ef_low = round(cf * 2.^(-0.5*bw));
ef_high = round(cf * 2.^(0.5*bw));
H = realfft(x);
m = zeros(size(x,2),numel(cf));
for k=1:numel(cf)
    m(:,k) = 10*log10(mean(abs(H(ef_low(k):ef_high(k),:)).^2));
end
figure
imagesc(m);
colormap('jet');
colorbar;
set(gca,'YTick',1:3:N,'YTickLabel',round(180/pi*vaz(1:3:N)),...
        'XTick',1:3:numel(cf),'XTickLabel',round(cf(1:3:end)),...
	'CLim',[-30,5]);
xlabel('frequency / Hz');
ylabel('azimuth / degree');
title(type);
saveas(gcf,['directivitypattern_',type,'.png'],'png');
%figure
%for k=1:3:numel(cf)
%  c = (max(0,30 + m(:,k))) .* exp(i * vaz(:));
%  plot( real(c), imag(c), '-');
%  hold on
%end
%set(gca,'DataAspectRatio',[1,1,1]);