function afc_maa_play_interval( is_target_interval, var, param, sUser ) 
% var - adaptively varying parameter
% param - constant parameter
  % target angle:
  target_angle = is_target_interval * var;
  send_osc(sUser.hTSC,'/scene/S/zyxeuler',target_angle,0,0);
  send_osc(sUser.hTSC,'/scene/N/zyxeuler',param,0,0);
  %send_osc(sUser.hTSC,'/scene/N/0/gain',param);
  %send_osc('localhost',9876,'/color',param);
  tascar_jackio( sUser.stimulus, 'output',{'render.scene:S.0','render.scene:N.0'});
