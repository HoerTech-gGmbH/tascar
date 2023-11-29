function [x,fs] = tascar_measure_radiationpattern
n_doc = tascar_xml_doc_new();
% session (root element):
n_session = tascar_xml_add_element(n_doc,n_doc,'session');
n_scene = tascar_xml_add_element(n_doc,n_session,'scene');
% source
n_src = tascar_xml_add_element(n_doc, n_scene, 'source');
n_sound = tascar_xml_add_element(n_doc, n_src, 'sound',[],...
                                 'type','cardioidmod',...
                                 'f6db','4000',...
                                 'fmin','500');
% receiver:
N = 72;
vaz = angle(exp(i*(2*pi*([1:N]-1)/N + pi)));
for n=1:N
    az = vaz(n);
    n_rec = tascar_xml_add_element(n_doc,n_scene,'receiver',[],...
                                   'name', sprintf('out_%d',n));
    tascar_xml_add_element(n_doc, n_rec, ...
                           'position',sprintf('0 %g %g 0',cos(az),sin(az)));
end  
tascar_xml_save( n_doc, 'temp_rec.tsc' );
system(['LD_LIBRARY_PATH='''' ',...
        'tascar_renderir temp_rec.tsc -o temp_rec.wav']);
[x,fs] = audioread('temp_rec.wav');
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
colorbar;
set(gca,'YTick',1:3:N,'YTickLabel',round(180/pi*vaz(1:3:N)),...
        'XTick',1:3:numel(cf),'XTickLabel',round(cf(1:3:end)));
figure
size(vaz)
size(m)
for k=1:3:numel(cf)
  c = (max(0,30 + m(:,k))) .* exp(i * vaz(:));
  plot( real(c), imag(c), '-');
  hold on
end
set(gca,'DataAspectRatio',[1,1,1]);