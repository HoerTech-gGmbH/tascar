function rgb = get_rgb();
% measure current RGB values by taking a picture via gphoto2
%
% rgb = get_rgb();
%
% The median RGB values over the whole image area are returned.
  [a,b] = system('killall gvfsd-gphoto2 2>&1');
  d = dir('*.JPG');
  n1 = {d.name};
  [a,b] = system('gphoto2 --capture-image-and-download 2>&1');
  d = dir('*.JPG');
  n2 = {d.name};
  n3 = setdiff(n2,n1);
  if isempty(n3)
    error('No picture taken!');
  end
  m = double(imread(n3{1}));
  % in case of over- or under-exposed images, the jpeg coder exports
  % only grayscale images, thus a remapping on RGB is required:
  if size(m,3) == 1
    m = m(:,:,[1,1,1]);
  end
  r = m(:,:,1);
  g = m(:,:,2);
  b = m(:,:,3);
  rgb = [median(r(:)),median(g(:)),median(b(:))];
  %rgb = ([mean(r(:)),mean(g(:)),mean(b(:))]);
  [a,b] = system(['rm -f ',n3{1}]);
