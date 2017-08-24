function sVer = tascar_version
  [err,sVer] = system('tascar_version');
  if( err ~= 0 )
    error('TASCAR not properly installed');
  end
  sVer = sVer(find((sVer~=10).*(sVer~=13)));
