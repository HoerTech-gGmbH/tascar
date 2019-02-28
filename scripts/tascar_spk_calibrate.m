function tascar_spk_calibrate( layoutfile, varargin )
% tascar_spk_calibrate
%
% tascar_spk_calibrate( layoutfile [, key, value ] )
%
% layoutfile: tascar speaker layout file
%

    if (nargin < 2)
        if strcmp(layoutfile,'help')
            varargin = {'help'};
        end
    end
    if isoctave()
        % on octave make sure that signal package is loaded:
        pkg('load','signal');
    end
    
    sCfg.frange = [500,5000];
    sHelp.frange = 'frequency range for compensation in Hz';
    sCfg.gmin = -20;
    sHelp.gmin = 'minimum gain in transfer function in dB';
    sCfg.complen = 32;
    sHelp.complen = 'compensation filter length in taps';
    sCfg.fres = 100;
    sHelp.fres = 'frequency resolution in Hz';
    sCfg.input = 'system:capture_1';
    sHelp.input = 'input jack port';
    sCfg.c = 340;
    sHelp.c = 'speed of sound in m/s';
    sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
    if isempty(sCfg)
        return;
    end

    doc_spk = tascar_xml_open( layoutfile );
    elem_spk = tascar_xml_get_element( doc_spk, 'speaker' );
    tascar_xml_add_comment( doc_spk, elem_spk{1}, ...
                            [' calibrated ',datestr(now),' '] );
    [y,fs,fragsize] = tascar_jackio(0);
    len = fs;
    mIR = zeros(len,numel(elem_spk));
    vDelay = zeros(numel(elem_spk),1);
    vMeasDelay = zeros(numel(elem_spk),1);
    csLabels= {};
    for k=1:numel(elem_spk)
        connect = tascar_xml_get_attribute( elem_spk{k}, 'connect' );
        csLabels{k} = connect;
        az = get_attr_num( elem_spk{k}, 'az', 0 );
        el = get_attr_num( elem_spk{k}, 'el', 0 );
        r = get_attr_num( elem_spk{k}, 'r', 1 );
        ir = tascar_ir_measure( 'output', connect, ...
                                'input', sCfg.input, ...
                                'len', len,...
                                'gain',0.02,...
                                'nrep',2);
        mIR(:,k) = r*ir;
        vDelay(k) = r/sCfg.c;
        vMeasDelay(k) = median(groupdelay( ir ))/fs;
    end
    vDelay = max(0,-(vMeasDelay-vDelay-max(vMeasDelay-vDelay)));
    mIR = smooth_and_truncate_ir( mIR, round(fs/sCfg.fres) );
    H = realfft(mIR);
    vG = median(abs(H(sCfg.frange(1):sCfg.frange(2),:)));
    g = median(vG);
    idx = find(abs(H)<g*10.^(0.05*sCfg.gmin));
    H(idx) = (H(idx)./abs(H(idx)+eps))*g*10.^(0.05*sCfg.gmin);
    Hcomp = g./H;
    Hcomp = smooth_frange( Hcomp, sCfg.frange(1):sCfg.frange(2) );
    mIRcomp = ir2minphaseir( realifft(Hcomp), sCfg.complen );
    for k=1:numel(elem_spk)
        tascar_xml_add_comment( doc_spk, elem_spk{k}, ...
                                sprintf(' measured delay = %g s g = %g',...
                                        vMeasDelay(k),...
                                        20*log10(vG(k)/g)));
        tascar_xml_set_attribute( elem_spk{k}, 'delay', vDelay(k) );
        tascar_xml_set_attribute( elem_spk{k}, 'compB', mIRcomp(1:sCfg.complen,k) );
    end
    tascar_xml_save(doc_spk,[layoutfile,'.calib']);
    figure
    imagesc(mIR(1:4*round(fs/sCfg.fres),:)');
    set(gca,'YTick',1:size(mIR,2),'YTickLabel',csLabels);

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

function ir = ir2minphaseir( ir, nmax )
% ir2minphaseir - generate minimum phase impulse response
%
% Usage:
% ir = ir2minphaseir( ir [, nmax ] );
    
    N = size(ir,1);
    if nargin < 2
        nmax = N;
    end
    H = abs(realfft(ir));
    Nbins = size(H,1);
    P = log(max(1e-10,H));
    P(Nbins+1:N,:) = P(Nbins-1:-1:2,:);
    P = imag(hilbert(P));
    ir = realifft(H.*exp(-i*P(1:Nbins,:)));
    ir(nmax+1:end,:) = [];

function b = isoctave()
    b = ~isempty(ver('Octave'));
