function polarplotsig( x )
  phi = [1:size(x,1)]'/size(x,1)*2*pi;
  m = max(abs(x(:)));
  plot(m*cos(phi),m*sin(phi),'k-');
  hold on;
  for ch=1:size(x,2)
    plot(cos(phi).*abs(x(:,ch)),sin(phi).*abs(x(:,ch)));
  end
  hold off;
  set(gca,'DataAspectRatio',[1,1,1],'Xlim',[-m,m],'YLim',[-m,m]);
