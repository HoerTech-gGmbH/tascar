clc
clear 
close all

%create a scene
generate_scene('filename','my_example_scene.tsc','nrsources',3,'r',1, ...
    'nrspeakers', 8, 'rec_type', 'neukom_basic' , 'maxdist', 10);
%load a created scene in tascar
hdl = tascar_ctl('load','my_example_scene.tsc');

%create cell with input port names( depend on nrsources)
c_InPorts=cell(3,1)
for nn=1:3
    c_InPorts{nn}=['render.' ,test_filename,':','src_',num2str(nn),'.0'];
end
%create cell with output port names( depend on nrspeakers)
c_OutPorts=cell(8,1);
for kk=1:8
    c_OutPorts{kk}=['render.' ,test_filename,':','out.',num2str(kk-1)];
end
    

%change the receiver orientation and measure each time  
for i=0:5:360;
   
pause(2);
%change the receiver orientation
send_osc('localhost',9999,['/' scene '/' receiver '/pos'],0,0,0,i,0,0);
%creeate test signal
test_sig= randn(1000,1);
%play back the signal in the scene and measure the signal captured by
%receiver
[y,fs,bufsize,load,xruns,sCfg]=tascar_jackio( test_sig,'output' ,c_InPorts , 'input',  c_OutPorts);
%compute sth, for example the energy of the captured signal
E=mean(y.^2);
end 

