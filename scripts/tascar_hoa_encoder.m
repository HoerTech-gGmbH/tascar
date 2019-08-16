function B = tascar_hoa_encoder( order, azimuth, elevation )
% Higher Order Ambisonics Encoding
% B = tascar_hoa_encoder( order, azimuth, elevation )


%% Calculation of HOA channels B(l,m,t)
  index = 1;
  for l = 0:order
    for m = -l:l
      Y = ylmreal( l, m, elevation, azimuth );
      B(index,:) = Y;
      index = index+1;
    end
  end

function y = ylmreal( l, m, elevation, azimuth )
% elevation is from -pi/2 to pi/2, other conventions (including
% wikipedia) use elevation from 0 to pi (with 0 at a pole):
  leg = legendre(l,sin(elevation),'sch');
  P = leg(abs(m)+1,:);
  if m<0
    y = P.*sin(abs(m)*azimuth);
  elseif m==0
    y = P;
  elseif m>0
    y = P.*cos(abs(m)*azimuth);
  end
