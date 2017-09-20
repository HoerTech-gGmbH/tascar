sCfg.num_intervals = 2;
sCfg.target_intervals = 1:2;
sCfg.labels = {'L > R','R > L'};
sCfg.display_columns = 2;
% additional parameters of interleaved tracks:
sCfg.tracks = {0,-10};
%sCfg.tracks = {0};
%sCfg.tracks = {'green','blue'};
sCfg.interleaved = 1;
sCfg.cb_sound_play = @afc_maa_play_interval;
sCfg.cb_sound_prepare = @afc_maa_audioprepare;
sCfg.cb_sound_release = @afc_maa_audiorelease;
sCfg.msg_begin = 'Start';
sCfg.msg_end = 'Vielen Dank!';
sCfg.indicator = 1;
sCfg.delay_pretrial = 0.2;
sCfg.delay_interval = 0.3;
sCfg.delay_posttrial = 0.1;
sCfg.afc = struct;
% 1up 2down?
sCfg.afc.rule = [1,2];
% initial distance 15 degrees:
sCfg.afc.startvar = 15;
% step sizes of angles: Always divide/multiply by 2, measurement
% phase afer 3 reversals:
sCfg.afc.varstep = [2,2,2,2];
sCfg.afc.stepmode = 'log';
sCfg.afc.maxvar = 45;
% step forward at lower reversals:
sCfg.afc.steprule = 1;
sCfg.afc.reversalnum = 3;
% unlimited number of trials
sCfg.afc.maxtrials = 0;
sCfg.stimulus = 0.1*(rand(22050,2)-0.5);
addpath('/usr/share/tascar/matlab');
javaaddpath('../netutil-1.0.0.jar');
sCfg.hTSC = tascar_ctl('load','maa.tsc');
