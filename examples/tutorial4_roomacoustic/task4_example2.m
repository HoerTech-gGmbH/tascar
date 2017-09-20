% -------------------------TASCAR WORKSHOP-------------------------- 
% ---------------Taskgroup 4: room acoustic modeling,---------------
% -----------------diffuse sources and reverberation----------------
% -------------------------- EXAMPLE 2 -----------------------------

% In this example we want to show you how to use the offline rendering tools
% and run offline rendering from matlab. Therefore, the tsc files (xml format)
% can be modified from matlab. 


clc
clear
close all

addpath /usr/share/tascar/matlab/
javaaddpath ../netutil-1.0.0.jar

%% Choose a TASCAR scene you would like to edit: 
doc = tascar_xml_open('/home/medi/tascarworkshop/group5/task4_basic_HL.tsc');

%%  Change attributes of elements 
r_walls=0.8;
d_walls=0.2;
ismorder = 2;

%Change mirror order of scene:
doc = tascar_xml_edit_elements( doc, 'scene', 'mirrororder', ismorder);

%Change r&d filter coefficients of the walls
doc = tascar_xml_edit_elements( doc, 'facegroup', 'reflectivity', r_walls,'name','magicchamber');
doc = tascar_xml_edit_elements( doc, 'facegroup', 'damping', d_walls,'name','magicchamber');

%Change size of the shoebox room 
shoebox_size='3 5 3';
doc = tascar_xml_edit_elements( doc, 'facegroup', 'shoebox', shoebox_size ,'name','walls');

%%  Add new elements to  xml file 

%Add a new source with a certain position and spherical interpolation
sceneNode = tascar_xml_get_el_handle( doc,'scene') ;
srcNode = tascar_xml_add_element( doc, sceneNode, 'src_object', [], 'name','new_source'); 
sndfileNode = tascar_xml_add_element( doc, srcNode,'sndfile',[],'name','${HOME}/tascar_scenes/sounds/daps/m4_script4_clean.wav','loop','0');
posNode = tascar_xml_add_element( doc, srcNode, 'position','0 2 0 1 4 5 2 2','interpolation','spherical'); 
soundNode = tascar_xml_add_element( doc, srcNode, 'sound', [], 'name','0','connect','@.0');

%% Removing elements from xml file
sceneNode = tascar_xml_get_el_handle( doc,'scene') ;
positionnNode = tascar_xml_rm_element( doc, sceneNode, 'obstacle', 'name','thewall'); 

%% Save the edited scene definition file
tascar_xml_save( doc, 'task4_basic_modified.tsc' ) 


%% load the scene in tascar 
h_temp=tascar_ctl('load', 'task4_basic_modified.tsc');
pause

%% close a TASCAR scene  -----------------
tascar_ctl('kill',h_temp);



