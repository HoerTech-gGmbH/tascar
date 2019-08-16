function Hdecoder = tascar_hoa_decoder_allrad(vL, order)
% tascar_hoa_decoder_allrad
%
% Hdecoder = tascar_hoa_decoder_allrad(vL, order)
%
%ALLRAD computes an "All Round Ambisonic Decoding" (AllRAD) Decoder by
%Zotter and Frank (2012) which is a hybrid Ambisonic-VBAP approach.
%
%Heller, Aaron J., and Eric M. Benjamin. "The Ambisonic Decoder Toolbox: 
%extensions for partial-coverage loudspeaker arrays." Linux Audio 
%Conference. 2014.

%% Array of virtual loudspeakers in the form of an icosahedron
  vL_virtual = gen_icosahedron();
  while size(vL_virtual,2) < 3*(order+1)^2
    vL_virtual = subdivide_mesh( vL_virtual, 1 );
  end
  %Computation of an ambisonics decoder matrix Hdecoder_virtual for the virtual array of
  %loudspeakers
  Hdecoder_virtual = tascar_hoa_decoder_pinv( vL_virtual, order );

  %% computation of VBAP gains for each virtual speaker
  %Project the positions of the real speakers onto the unit sphere.
  vL_n = vector2unitvector(vL);

  %Compute the triangular tessellation of the convex hull of the projected
  %speaker positions.
  vH = convhulln(vL_n');

  %Calculate VBAP gains:
  G_V_R = vbap3d( vH, vL_virtual, vL_n );

  %% decoder matrix
  Hdecoder = G_V_R * Hdecoder_virtual;


function m = gen_icosahedron
  phi = (1+sqrt(5))/2;
  m = [0,1,phi;...
       0,-1,-phi;...
       0,1,-phi;...
       0,-1,phi;...
       1,phi,0;...
       1,-phi,0;...
       -1,-phi,0;...
       -1,phi,0;...
       phi,0,1;...
       -phi,0,1;...
       phi,0,-1;...
       -phi,0,-1]';
  
function m = subdivide_mesh( m, iter )
  for k=1:size(m,2)
    m(:,k) = m(:,k) / sqrt(sum(m(:,k).^2));
  end
  for iteration = 1:iter
    chull = convhulln( m' );
    for k=1:size(chull,1)
      idx = chull(k,:);
      m(:,idx);
      t = mean(m(:,idx),2);
      m(:,end+1) = t;
    end
    for k=1:size(m,2)
      m(:,k) = m(:,k) / sqrt(sum(m(:,k).^2));
    end
  end

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
  
  
function [G_V_R] = vbap3d(vH,vL,speakerlayout_n)
%VBAP3D computes vector based amplitude panning (VBAP) gains for 3D
%loudspeaker arrays
%-vH: triangular tessellation of the convex hull of the projected speaker
%     positions
%-vL: virtual uniform loudspeaker array
%-speakerlayout_n: normalized real loudspeaker positions

% calculate gains for all loudspeaker triangles:
  all_gains = zeros(size(vH));
  G_V_R = zeros(size(speakerlayout_n,2),size(vL,2));

  for ind_virt = 1:size(vL,2)
    for k = 1:size(vH,1)
      triangle = speakerlayout_n(:,vH(k,:));
      all_gains(k,:) = gains_nd(vL(:,ind_virt),triangle);
      % if all gains are >= 0 then this is the right triangle:
      if min(all_gains(k,:))>=0
        kp = k;
      end
      % multiple triangles can only be possible if p is exactly on
      % one of the loudspeakers, but then all gains except for the
      % loudspeaker gain are 0, and all solutions result in the same
      % gain matrix.
    end
    
    % select triangle with the largest minimal gain:
    minimum = zeros(size(all_gains,1),1);
    for k = 1:size(all_gains,1)
      minimum(k) = min(all_gains(k,:));
    end
    % index of selected triangle:
    [~,kp] = max(minimum);
    
    % indices of selected loudspeakers:
    Lp_indices = vH(kp,:);
    % coordinates of selected triangle:
    Lp = speakerlayout_n(:,Lp_indices);
    
    % % plot triangle kp:
    % plot3(Lp(1,[1,2,3,1]),Lp(2,[1,2,3,1]),Lp(3,[1,2,3,1]),...
    %       'g-d','linewidth',2);
    
    % gains of selected triangle:
    g = all_gains(kp,:);
    % scaling (eq. 10 of Pulkki 1997):
    g = g/sqrt(sum(g.^2));
    G_V_R(Lp_indices(1),ind_virt) = g(1);
    G_V_R(Lp_indices(2),ind_virt) = g(2);
    G_V_R(Lp_indices(3),ind_virt) = g(3);
  end



function gains = gains_nd(p,l)

%gains_nd calculates gain factors for a n-dimensional Vector Base Amplitude Panning (VBAP)
%
%input parameters:
%-p: nx1 unit vector defining the direction of the virtual sound source
%-l: nxn invertible matrix defining the directions of the loudspeakers that 
%    belong to the triangle of the virtual source. Each column is a unit vector 
%    containing the x, y(, z) coordinates of a loudspeaker
%
%output parameters:
%-gains: gain factors of the loudspeakers

% % convert l to unit vectors:
% l_unit = zeros(size(l,2),1);
% for k = 1:size(l,2)
%    l_unit(:,k) = l(:,k)./sqrt(sum(l(:,k).^2)); 
% end

  p_transpose = p';
  gains = p_transpose*inv(l');

function plot_convhull( vH, vL )
%PLOT_CONVHULL - plot a convex hull of point list
%
%Usage:
%plot_convhull( vH, vL )
%
% vH : convex hull (index vector, Mx3)
% vL : 3xN list of points

  if isempty( vH )
    vH = convhull( vL' );
  end
  hold off;
  h = trimesh(vH,vL(1,:),vL(2,:),vL(3,:));
  set(h,'Facecolor',[0.9,0.95,1],'EdgeColor',[0,0,0]);
  set(gca,'DataAspectRatio',[1,1,1]);
  hold on;
  plot3(vL(1,:),vL(2,:),vL(3,:),'ro','MarkerSize',6,...
        'MarkerFacecolor',[1,0,0]);
  % plot vertex labels:
  N = size(vL,2);
  for k=1:N
    text(1.05*vL(1,k),1.05*vL(2,k),1.05*vL(3,k),...
         num2str(k),...
         'FontWeight','bold','Color',[0.7,0,0]);
  end
  % plot simplex labels:
  M = size(vH,1);
  for k=1:M
    x = vL(:,vH(k,:));
    xm = mean(x,2);
    text(1.05*xm(1),1.05*xm(2),1.05*xm(3),...
         num2str(k),...
         'FontWeight','bold','Color',[0,0,0.7]);
  end