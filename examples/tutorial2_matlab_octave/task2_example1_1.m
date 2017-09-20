% -------------------------TASCAR WORKSHOP-------------------------- 
% -----------Taskgorup 2: Interfacing TASCAR from MATLAB------------
% -------------------------- EXAMPLE 1 -----------------------------


clc
clear
close all


addpath /usr/share/tascar/matlab/ % where the useful MATLAB scripts are stored 
javaaddpath ../netutil-1.0.0.jar


%% --------------  1. Load an existing scene  -----------------

%load the scene in tascar 
h_temp=tascar_ctl('load', 'task2_basic1.tsc');
pause(5);

%% ---------  2. Modify parameters of the opened scene ---------------
% Here you will see how to modify the TASCAR scene from MATLAB. 
% Parameters of the scene can be modified from MATLAB using function 'send_osc.m'
% You can put a breakpoint after the line where the scene is loaded and 
% when the script stops at the breakpoint go to VIEW-> OSC variables in
% the TASCAR window - you will see a list of parameters which can be changed.
% You can experiment by puting breakpoints after each usage of 'send_osc' 
% and each time see what has changed in the scene or what action was done. 

%Change the position and orientation of the object 
pos_x=1;pos_y=2;pos_z=1;
euler_x=10;euler_y=30;euler_z=0;
send_osc(h_temp,'/scene/src/pos' ,pos_x, pos_y, pos_z, euler_z, euler_y,euler_x)

%start playing back the scene
send_osc(h_temp, '/transport/start')

%Change the gain of an object 
G=10;
send_osc(h_temp,'/scene/src/gain' ,G)

%stop playing back the scene
send_osc(h_temp,'/transport/stop')

%Change the position on the time line
time_point=0;
send_osc(h_temp,'/trasport/locate',time_point);

%% -----------------  3. Play & record  -----------------
% Below you will see how to play back and record a content of the TASCAR scene. 
% It requires specifying the jack audio ports corresponding with the
% sources and receivers. You can again put a breakpoint after the line 
% where the scene is loaded and when the script stops at the breakpoint go
% to the QjackCtl and have a look at the jack port types and their names.
% You can also type 'jack_lsp -p' in the terminal to see the list of
% available jack ports. 

%% --------------  3.1. Specify jack ports  -----------------
% make a cell with the names of writeble clients (e.g. loudspeakers):
c_Write={'render.scene:src.0'};
% make a cell with the names of readable clients (e.g. microphones): 
c_Read={'render.scene:outomni'};


%% --------------  3.2. Use function tascar_jackio  -----------------

% For help type in the command window:
% tascar_jackio help 
% help tascar_jackio

% 3.1.a) Playing back & recording the scene only with its original content
% (from the scene definition file) - for that the test signal has to 
% contain only zeros.

%create a test signal whose size fits the number of sources in the scene 
test_sig= zeros(5*44100,1);
%where on the time line to start play&record
transportStart=5; 
[rec_signal_orig,fs,bufsize,load,xruns,sCfg]=tascar_jackio(test_sig,'output' ,c_Write , 'input',  c_Read, 'starttime', transportStart);

% 3.1.b) Playing back & recording the scene with an arbitrary test sound 
% generated in matlab.

%create a test signal
test_sig= 0.1*(randn(5*44100,1)-0.5);
%where to start play&record
transportStart=5; 
[rec_signal_withnoise,fs,bufsize,load,xruns,sCfg]=tascar_jackio(test_sig,'output' ,c_Write , 'input',  c_Read, 'starttime', transportStart);


%% --------------  4. Close a TASCAR scene  -----------------
tascar_ctl('kill',h_temp);
