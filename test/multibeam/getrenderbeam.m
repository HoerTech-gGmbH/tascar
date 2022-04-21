vs = unique([[0:0.05:1],[0:0.1:5]]);
l1 = [];
l2 = [];
l3 = [];
l4 = [];
for s=vs
  l1(end+1) = renderbeam( s, 0 );
  l2(end+1) = renderbeam( s, 34 );
  l3(end+1) = renderbeam( [s,0.5*s], 0 );
  l4(end+1) = renderbeam( [s,0.5*s], 34 );
end
plot(vs,[l1;l2;l3;l4]+60);
grid on
legend({'diffuse sound field approximation','multiple sound sources',...
       '2beam, diffuse sound field approximation','2beam, multiple sound sources'})
xlabel('selectivity');
ylabel('output gain/dB');
saveas(gcf,'multibeam.pdf','pdf');
