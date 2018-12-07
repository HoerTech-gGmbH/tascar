function sData = tascar_get_levels_and_pos( varargin )
% tascar_get_levels_and_pos - measure levels and positions of
% sounds
%
% Usage:
% sData = tascar_get_levels_and_pos( keyword, value [, keyword, value ] )
%
% For a list of valid keyword/value pairs, type:
%
% tascar_get_levels_and_pos help
%
% Author: Giso Grimm
% Date: 12/2018

if isempty(varargin)
    help(mfilename);
    error('Invalid usage');
end
% default values:
sCfg = struct;
sCfg.tsc = '';
sHelp.tsc = 'Name of TASCAR file';
sCfg.receiver = 'out';
sHelp.receiver = 'Name of receiver element';
sCfg.tau = 2;
sHelp.tau = 'Time constant of RMS level in seconds';
sCfg.fs = 44100;
sHelp.fs = 'Sampling rate used for measurement in Hz';
sCfg.fragsize = 1024;
sHelp.fragsize = 'Fragment size (= sample rate divisor) in samples';
sCfg.ismorder = -1;
sHelp.ismorder = 'Maximum number ISM model, -1 to use scene settings';
% parse configuration:
sCfg = tascar_parse_keyval( sCfg, sHelp, varargin{:} );
if isempty(sCfg)
    return
end
doc = tascar_xml_open( sCfg.tsc );
root = javaMethod('getDocumentElement',doc);
sduration = char(javaMethod('getAttribute',root,'duration'));
if isempty(sduration)
    duration = 60;
else
    duration = str2num(sduration);
end
sData = {};
audiowrite('silence.wav',zeros(duration*sCfg.fs,1),sCfg.fs);
cScenes = tascar_xml_get_element( root, 'scene' );
for k=1:numel(cScenes)
    scene = struct;
    cSources = [tascar_xml_get_element( cScenes{k}, 'source' ),...
                tascar_xml_get_element( cScenes{k}, 'src_object' )];
    for ksrc=1:numel(cSources)
        sourcename = char(javaMethod('getAttribute',cSources{ksrc},'name'));
        [x,y,z,lev] = render_scene( sCfg, sourcename );
        if isempty(sourcename)
            sourcename = 'in';
        end
        scene.(sourcename).t = ([1:size(x,1)]'-1)/(sCfg.fs/sCfg.fragsize);
        scene.(sourcename).x = x;
        scene.(sourcename).y = y;
        scene.(sourcename).z = z;
        scene.(sourcename).lev = lev;
    end
    sData{k} = scene;
end

function [x,y,z,lev] = render_scene( sCfg, sourcename )
doc = tascar_xml_open( sCfg.tsc );
root = javaMethod('getDocumentElement',doc);

cScenes = tascar_xml_get_element( root, 'scene' );
for k=1:numel(cScenes)
    if sCfg.ismorder >= 0
        javaMethod('setAttribute',cScenes{k},...
                   'mirrororder',num2str(sCfg.ismorder));
    end
    cSources = [tascar_xml_get_element( cScenes{k}, 'source' ),...
                tascar_xml_get_element( cScenes{k}, 'src_object' )];
    for ksrc=1:numel(cSources)
        lsourcename = javaMethod('getAttribute',cSources{ksrc},'name');
        if ~strcmp(lsourcename,sourcename)
            javaMethod('removeChild',cScenes{k},cSources{ksrc});
        end
    end
    cSources = tascar_xml_get_element( cScenes{k}, 'receiver' );
    for ksrc=1:numel(cSources)
        lsourcename = javaMethod('getAttribute',cSources{ksrc},'name');
        if ~strcmp(lsourcename,sCfg.receiver)
            javaMethod('removeChild',cScenes{k},cSources{ksrc});
        else
            javaMethod('setAttribute',cSources{ksrc},'type','debugpos');
            javaMethod('setAttribute',cSources{ksrc},'sources','10');
            javaMethod('setAttribute',cSources{ksrc},'caliblevel','93.9794');
        end
    end
end
oname = ['temp_',char(sourcename),'.tsc'];
tascar_xml_save( doc, oname );
sCmd = sprintf(['tascar_renderfile -i silence.wav ',...
                '-o temp.wav -f %d -d -t 0 %s'],...
               sCfg.fragsize, oname);
[msg,err] = system(sCmd);
if err ~= 0
    error(['Subprocess failed: ',msg]);
end
data = audioread('temp.wav');
delete( oname );
delete( 'temp.wav' );
Nsounds = size(data,2)/4;
activesounds = [];
for k=1:Nsounds
    if any(data(:,4*k)~=0)
        activesounds(end+1) = k;
    end
end
N = size(data,1)/sCfg.fragsize;
x = zeros([N,numel(activesounds)]);
y = zeros([N,numel(activesounds)]);
z = zeros([N,numel(activesounds)]);
lev = zeros([N,numel(activesounds)]);;
[B,A] = butter(1,1./(sCfg.tau*sCfg.fs));
for k=1:numel(activesounds)
    act = activesounds(k);
    x(:,k) = data(1:sCfg.fragsize:end,4*(act-1)+1);
    y(:,k) = data(1:sCfg.fragsize:end,4*(act-1)+2);
    z(:,k) = data(1:sCfg.fragsize:end,4*(act-1)+3);
    I0 = mean(data(1:sCfg.fragsize,4*(act-1)+4).^2);
    audio = filter(B,A,data(:,4*(act-1)+4).^2,I0);
    lev(:,k) = 10*log10(audio(1:sCfg.fragsize:end))+93.9794;
end