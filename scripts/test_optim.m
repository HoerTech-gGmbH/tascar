function x = test_optim
  x = fminsearch(@errormeasure,[-10,0.8,0.8,0.8]);
  
function e = errormeasure( params )
  send_osc('localhost',9999,'/cafeteria/fdn/gain',params(1));
  send_osc('localhost',9999,'/cafeteria/tables/reflectivity',params(2));
  send_osc('localhost',9999,'/cafeteria/windows/reflectivity',params(3));
  send_osc('localhost',9999,'/cafeteria/floor_columns/reflectivity',params(4));
  pause(0.1);
  target = [1.5,0.06,-3,-3];
  [T60late,T60early,R] = ...
      measure_roomacoustics('render.cafeteria:f1.0',...
			    'jconvolver:main.L','jconvolver:main.R',...
			    2^16);
  params
  current = [mean(T60late),mean(T60early),R(1),R(2)]
  ve = [log2(target(1))-log2(current(1)),...
	log2(target(2))-log2(current(2)),...
	target(3)-current(3),...
	target(4)-current(4)];
  e = sqrt(sum(ve.^2));
  e
	