clear all
close all
system('LD_LIBRARY_PATH='''' tascar_renderir gen_ir.tsc -f 48000 -o ir.wav');
[ir,fs] = audioread('ir.wav');
p = createSDMStruct('DefaultArray','Bformat','fs',fs,'winLen',15);
DOA{1} = SDMbf(ir, p);
P{1} = ir(:,1);
set(0,'DefaultTextInterpreter','latex')
v = createVisualizationStruct('DefaultRoom','SmallRoom',...
    'fs',fs,'t',[ 2 5 10 20 50 100 200 1000]);
v.plane = 'lateral';
spatioTemporalVisualization(P, DOA, v)
%v.plane = 'transverse';
%spatioTemporalVisualization(P, DOA, v)
%v.plane = 'median';
%spatioTemporalVisualization(P, DOA, v)
