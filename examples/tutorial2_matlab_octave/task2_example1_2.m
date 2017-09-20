% -------------------------TASCAR WORKSHOP-------------------------- 
% -----------Taskgorup 2: Interfacing TASCAR from MATLAB------------
% -------------------------- EXAMPLE 2 -----------------------------


clc
clear
close all

addpath /usr/share/tascar/matlab/
javaaddpath ../netutil-1.0.0.jar



%% --------------  1. Generate and load a scene  -----------------
% There is a possibility to generate a simple TASCAR scene from matlab. 
% This simple scene contains one speaker based receiver, and sources and 
% virtual loudspeakers equally distributed on a circle around the receiver. 

% choose parameters
NrSources=4;
NrSpeakers=16;
ReceiverType='hoa2d';
test_filename='my_new_generated';

%generate a scene 
tascar_ctl('createscene',...
    'filename', [test_filename, '.tsc'] , ...
    'nrsources',NrSources, 'nrspeakers', NrSpeakers, ...
    'rec_type',ReceiverType);

%load the generated scene in tascar & wait for a while
h_temp=tascar_ctl('load',[test_filename,'.tsc']);
pause(5);

% connect ports:
for k=1:16
  tscport = sprintf('render.%s:out.%d',test_filename,k-1);
  hwport = sprintf('system:playback_%d',k);
  system(['jack_connect ',tscport,' ',hwport]);
end


%% ---------  2. Modify parameters of the opened scene ---------------



% Do it yourself if you want :-) 



%% -----------------  3. Play & record  -----------------

%% --------------  3.1. Specify jack ports  -----------------

% make a cell with writeble clients names (see QjackCtl  or type 'jack_lsp -p' in terminal):
c_ReadPorts=cell(NrSpeakers,1);
for kk=1:NrSpeakers
    c_ReadPorts{kk}=['render.scene:','out.',num2str(kk-1)];
end
% make a cell with readable clients names (see QjackCtl  or type 'jack_lsp -p' in terminal):
c_WritePorts=cell(NrSources,1);
for nn=1:NrSources
    c_WritePorts{nn}=['render.scene:','src_',num2str(nn),'.0'];
end

%% --------------  3.2. Use function tascar_jackio  -----------------

%create a test signal whose size fits the number of sources in the scene
%test_sig = ????

%Set the time when you want to start the recording
transportStart=2;

%play back the test signal in the scene & record the receiver output
[y,fs,bufsize,load,xruns,sCfg]=tascar_jackio( test_sig,'output' ,c_WritePorts , 'input',  c_ReadPorts, 'starttime', transportStart);

%% --------------  4. Close a TASCAR scene  -----------------
tascar_ctl('kill',h_temp);
