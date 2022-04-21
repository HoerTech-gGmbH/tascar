vs = unique([[0:0.1:1],[0:0.2:4]]);
l1 = [];
l2 = [];
for s=vs
  l1(end+1) = renderbeam( s, 0 );
  l2(end+1) = renderbeam( s, 32 );
end
plot(vs,[l1;l2]);
grid on
