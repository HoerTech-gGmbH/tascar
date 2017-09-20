% -------------------------TASCAR WORKSHOP-------------------------- 
% ---------------Taskgroup 4: room acoustic modeling,---------------
% -----------------diffuse sources and reverberation----------------
% -------------------------- EXAMPLE 1 -----------------------------

% In this example we want to show you how to design materials with a
% desired acoustic properties in TASCAR 

clc
clear
close all

addpath /usr/share/tascar/matlab/
javaaddpath ../netutil-1.0.0.jar

%% Acoustic properties 

% You want to simulate materials with the following absorbtion coefficients:
acoustic_tiles = [0.05 0.22 0.52 0.56 0.45 0.32];
metal_8mm = [0.50 0.35 0.15 0.05 0.05 0.00];
%corresponding frequencies: 
freq = [125,250,500,1000,2000,4000];
% compute reflection filter coefficients (r&d) based on the material 
% properties that you know. 
[r_walls,d_walls,alpha_walls] = tascar_alpha2rflt( acoustic_tiles, freq, 44100);
[r_mirror,d_mirror,alpha_mirror] = tascar_alpha2rflt( metal_8mm, freq, 44100);



%% Changing xml file 
%% Choose a TASCAR scene you would like to edit: 
doc = tascar_xml_open('/home/medi/tascarworkshop/tutorial4/task4_basic.tsc');
%doc = tascar_xml_open('/home/medi/tascarworkshop/tutorial4/task4_basic_HL.tsc');

% Change r&d filter coefficients of the walls
doc = tascar_xml_edit_elements( doc, 'facegroup', 'reflectivity', r_walls,'name','magicchamber');
doc = tascar_xml_edit_elements( doc, 'facegroup', 'damping', d_walls,'name','magicchamber');

%% Save the edited scene definition file
tascar_xml_save( doc, 'task4_basic_modified.tsc' ) 

%% Impulse response rendering (static)
% requires length in samples and output file

system('tascar_renderir task4_basic.tsc -l 2000 -o rendered_IR.wav');

%% Images source model rendering (requires input signal, movement)

x = rand(10*44100,1)-0.5;
audiowrite('input_sig.wav',x,44100)
system('tascar_renderfile -d -1 3 -f 48 -i input_sig.wav -o rendered_ISM.wav task4_basic.tsc ');
%system('tascar_renderfile -d -1 3 -f 48 -i input_sig.wav -o rendered_ISM.wav task4_basic_HL.tsc ');



