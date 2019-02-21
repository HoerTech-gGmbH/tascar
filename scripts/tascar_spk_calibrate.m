function [mIR,mIRcomp,H,Hcomp] = tascar_spk_calibrate( layoutfile, sinput )
frange = 500:5000;
fres = 100;
gmin = -20;
complen = 320;
doc_spk = tascar_xml_open( layoutfile );
elem_spk = tascar_xml_get_element( doc_spk, 'speaker' );
[y,fs,fragsize] = tascar_jackio(0);
len = fs;
mIR = zeros(len,numel(elem_spk));
for k=1:numel(elem_spk)
    connect = tascar_xml_get_attribute( elem_spk{k}, 'connect' );
    az = get_attr_num( elem_spk{k}, 'az', 0 );
    el = get_attr_num( elem_spk{k}, 'el', 0 );
    r = get_attr_num( elem_spk{k}, 'r', 1 );
    ir = tascar_ir_measure( 'output', connect, ...
                            'input', sinput, ...
                            'len', len,...
                            'gain',0.02,...
                            'nrep',2);
    mIR(:,k) = r*ir;
end
mIR = smooth_and_truncate_ir( mIR, round(fs/fres) );
H = realfft(mIR);
g = median(median(abs(H(frange,:))));
idx = find(abs(H)<g*10.^(0.05*gmin));
H(idx) = (H(idx)./abs(H(idx)+eps))*g*10.^(0.05*gmin);
Hcomp = g./H;
Hcomp = smooth_frange( Hcomp, frange );
mIRcomp = smooth_and_truncate_ir( ...
    circshift(realifft(Hcomp),round(0.5*fs)), complen );
for k=1:numel(elem_spk)
    tascar_xml_set_attribute( elem_spk{k}, 'compA', 1 );
    tascar_xml_set_attribute( elem_spk{k}, 'compB', mIRcomp(1:(2*complen),k) );
end
tascar_xml_save(doc_spk,[layoutfile,'.calib']);

function v = get_attr_num( el, name, def )
s = tascar_xml_get_attribute( el, name );    
if isempty(s)
    v = def;
else
    v = str2num(s);
end

function H = smooth_frange( H, frange )
w = ones(size(H,1),1);
i1 = min(frange);
i2 = max(frange);
l1 = round(0.5*i1);
l2 = round(0.5*(numel(w)-max(frange)));
w(1:(i1-l1)) = 0;
w((i1-l1)+[1:l1]) = 0.5-0.5*cos([1:l1]*pi/l1);
w(i2+[1:l2]) = 0.5+0.5*cos([1:l2]*pi/l2);
w((i2+l2):end) = 0;
for k=1:size(H,2)
    H(:,k) = H(:,k) .* w + (1-w).*exp(i*angle(H(:,k)));
end

function ir = smooth_and_truncate_ir( ir, len )
idx = irs_firstpeak( ir );
i_start = floor(median(idx));
i_start2 = floor(median(idx));
wnd = ones(size(ir,1),1);
len2 = min(i_start2,len);
i_start2 = i_start2-len2;
wnd(1:i_start2) = 0;
wnd(i_start2+[1:len2]) = 0.5-0.5*cos([1:len2]*pi/len2);
wnd(i_start+[1:len]) = 0.5+0.5*cos([1:len]*pi/len);
wnd((i_start+len+1):end) = 0;
for k=1:size(ir,2)
    ir(:,k) = ir(:,k) .* wnd;
end
ir = circshift(ir,-i_start2);

%function ir = 