% Example of starting an experiment with TASCAR and LSL and accessing data
% in realtime

% Add paths to TASCAR and LSL Matlab tools
addpath('/usr/share/tascar/matlab');
javaaddpath('../netutil-1.0.0.jar');
addpath('../../../tools/labstreaminglayer/LSL/liblsl-Matlab/');
addpath('../../../tools/labstreaminglayer/LSL/liblsl-Matlab/bin/');
addpath('../../../tools/labstreaminglayer/LSL/liblsl-Matlab/mex/');

% Create LSL inlets:
lib = lsl_loadlib();
result = lsl_resolve_byprop(lib,'name','eog'); % search for stream with same name as in glabsensors module in TASCAR
inlet_eog = lsl_inlet(result{1}); % make an inlet for this stream, so that data can be accessed from Matlab
result = lsl_resolve_byprop(lib,'name','TrackIRaz'); % search for stream with same name as in glabsensors module in TASCAR
inlet_az = lsl_inlet(result{1}); % make an inlet for this stream, so that data can be accessed from Matlab

result = lsl_resolve_byprop(lib,'name','TASCARtime'); % search for stream with the tascar timeline 
% (See lsljacktime module in the scene definition)
inlet_time = lsl_inlet(result{1}); % make an inlet for this stream, so that data can be accessed from Matlab

% Start datalogging:
send_osc('lidhan',7777,'/session_trialid','test'); % name you put in the datalogging window for Trial ID
pause(0.1)
send_osc('lidhan',7777,'/session_start'); % start playing the TASCAR scene and the datalogging
pause(0.5)
% First extract one sample from TASCAR time stream:
[chunk_time,stamps] = inlet_time.pull_chunk(); % clear inlet
[sample_time,stamps] = inlet_time.pull_sample();
while sample_time < 88 % while the TASCAR scene is running
% Collect last measured 0.5s chunk of data:
[chunk_az,stamps] = inlet_az.pull_chunk();
pause(0.5)
[chunk_az,stamps] = inlet_az.pull_chunk();
% Collect last measured sample of data:
% [sample_az,stamps] = inlet_az.pull_sample();
% Check if the subject is facing his head between -45 and +45 deg:
temp_orientation=mean(chunk_az,2)
temp_azimuth=rad2deg(temp_orientation(1))
if temp_azimuth<-45 || temp_azimuth>45 
    send_osc( 'lidhan', 7777, '/notfacingforward', temp_azimuth);
end
% Extract new sample from TASCAR time stream:
[chunk_time,stamps] = inlet_time.pull_chunk(); % clear inlet
[sample_time,stamps] = inlet_time.pull_sample();
end
% Stop the datalogging
send_osc('lidhan',7777,'/session_stop'); 

% If you would like to know how to record videos, have a look at this code:
%{
addpath('/home/medi/gesturelab/gphoto');

% open camera tool, get list of current files on SD card:
hCam = gphoto_open();
csCamFiles = gphoto_listall(hCam)
% start video recording:
gphoto_movie_start(hCam);

% do something
pause(10)

% stop video recording:
gphoto_movie_stop(hCam);
gphoto_close(hCam);

% download new video files from SD card and delete on camera:
hCam = gphoto_open();
csCamFilesNew = gphoto_listall(hCam);
csCamFilesAdded = setdiff(csCamFilesNew,csCamFiles)
gphoto_getfile( hCam, csCamFilesAdded, '');
gphoto_delete( hCam, csCamFilesAdded );
gphoto_close( hCam);
%}