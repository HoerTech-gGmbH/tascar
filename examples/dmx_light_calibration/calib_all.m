% add TASCAR tools to Matlab/Octave path:
addpath('/usr/share/tascar/matlab/');
% add OSC tool to path:
javaaddpath('netutil-1.0.0.jar');

% calibrate all fixtures defined in "calib.fixtures". A running
% TASCAR instance with corresponding lightctl module is expected
% (see file "lamp_calib.tsc").
%
% The tool "get_rgb.m" is responsible to measure real RGB-values, in
% this example by taking pictures via a camera with adjusted white
% balancing.
%
% "prepare_cam" is called to prepare the camera to a new fixture. In
% this example, the camera sensitivity is changed due to different
% total power output of the lamp types. Additionally, the measurement
% device could be adjusted to point to a lamp, or user interaction can
% be requested to adjust a lamp manually.
sData = tascar_fixtures_calib('calib.fixtures',...
			      'scenename','calib',...
			      'capturefun',@get_rgb,...
			      'preparefun',@prepare_cam);

% plot the input-output functions:
close all
tascar_fixtures_plot_calib(sData);

% See also script "get_hue.m" for a validation of the hue of lamps.