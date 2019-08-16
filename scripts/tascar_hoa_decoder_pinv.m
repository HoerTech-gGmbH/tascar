function Hdecoder = tascar_hoa_decoder_pinv( vL, order )
% TASCAR_HOA_DECODER - generate decoder matrix, using pseudo-inverse
%
% Hdecoder = tascar_hoa_decoder( vL, order );
%
% vL: Nx3 matrix containing cartesian position of loudspeakers
% order : Decoder order

  % in cartesian coordinates:
  vLn = vector2unitvector(vL);
  % in spherical coordinates:
  [azimuth, elevation, r] = cart2sph(vLn(1,:),vLn(2,:),vLn(3,:));
  % call encoding function to generate matrix with HOA weights for every
  % loudspeaker position:
  % format: n x (order+1)^2 (n: number of speakers)
  B = tascar_hoa_encoder( order, azimuth, elevation );

  % invert HOA weights matrix:
  Hdecoder = pinv(B);

function p = vector2unitvector( p )
%VECTOR2UNITVECTOR - create a list of unit vectors from vectors
%
%Usage:
%p = vector2unitvector( p )
%
% p : 3xN list of vectors
  for k=1:size(p,2)
    p(:,k) = p(:,k) / sqrt(sum(p(:,k).^2));
  end