function prepare_cam( label )
% Adjust camera settings
  [a,b] = system('killall gvfsd-gphoto2 2>&1');
  set_par('/main/imgsettings/imageformat=7');
  set_par('/main/capturesettings/aperture=4');
  if strcmp(label,'d40')
    set_par('/main/capturesettings/shutterspeed=36');
  else
    set_par('/main/capturesettings/shutterspeed=43');
  end
  set_par('/main/imgsettings/iso=1');
  % 0 Auto
  % 1 Daylight
  % 2 Shadow
  % 3 Cloudy
  % 4 Tungsten
  % 5 Fluorescent
  % 6 Flash
  set_par('/main/imgsettings/whitebalance=4');
  

function set_par( spar )
  [a,b] = system(['gphoto2 --set-config ',spar,' 2>&1']);
  if a ~= 0
    error(b);
  end
