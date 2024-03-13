function [H, doa_vect] = calcPolarResponse(DOA, P, res, plane)
% [H,doa_vect] = calcPolarResp(LOC, W, res, plane)
% Calculates polar response vector H for given LOC matrix and 
% pressure vector P. 
% 
% USAGE:
% 
% H        : polar response of energy (squared pressure) in linear scale
% doa_vect : DOA angles of the polar response, -180:res:180
% 
% DOA      : 3-D locations (Cartesian) of each image-source, from SDMpar
%            Dimensions [N 3] 
% P        : Impulse response in the middle of the array (pressure)
%            sometimes referred to as W in ambisonics, i.e, 
%            0th order harmonic, Dimensions [N 1]
% res      : angular resolution (1 = 1degree, 2=2degree etc)
% plane    : 'lateral', 'median' or 'transverse'
% 
% 
% EXAMPLES:
%
% fs = 48e3; % [Hz]
% DOA = randn(2*fs,3); % in Cartesian coordinates
% P = randn(2*fs,1); % pressure in the middle of the array
% res = 2; % resolution 2 degrees 
% plane = 'lateral'; % lateral plane visualized
%
% [H,doa_vect] = calcPolarResp(LOC, P, res, plane);
%

% SDM toolbox : calcPolarResp
% Sakari Tervo & Jukka P����tynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

% Use these two variables to rotate the plane
az_corr = 0;
el_corr = 0;

% check witch plane
switch lower(plane)
    case {'lateral'}
        % lateral
        [az,el,~] = cart2sph(DOA(:,1),DOA(:,2),DOA(:,3));
        [x,y,z] = sph2cart(az+az_corr,el+el_corr,1);
        [az,el,~] = cart2sph(x,y,z);
       
    case {'median'}
        % median
        [az,el,~] = cart2sph(DOA(:,1),DOA(:,3),-DOA(:,2));
        [x,y,z] = sph2cart(az+az_corr,el+el_corr,1);
        [az,el,~] = cart2sph(x,y,z);
        
    case {'transverse'}
        % transverse
        [az,el,~] = cart2sph(DOA(:,3),DOA(:,2),-DOA(:,1));
        [x,y,z] = sph2cart(az+az_corr+pi/2,el+el_corr,1);
        [az,el,~] = cart2sph(x,y,z);
       
end

% Find the closest direction in the grid for each image-source
AZ = round(az(:)*180/pi/res)*res;
el = el(:);

% Pressure to energy
A2 = P.^2;
A2 = A2(:);

% Doughnut weighting for angles, i.e. cosine weighting
doa_vect = -180:res:180;
H = zeros(length(doa_vect),1);
idx = 1;
for a = doa_vect
    inds = AZ == a;
    H(idx) = nansum(A2(inds).*abs(cos(el(inds))));
    idx = idx + 1;
end

