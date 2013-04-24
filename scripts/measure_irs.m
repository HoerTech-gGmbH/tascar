function irs = measure_irs()
spk = [1:8];
len = 2^15;
nrep = 8;
sTag = 'FF3_genelec';
gain = 0.01;
sIn = 'system:capture_57';
[irs,fs,x,y] = getirs( spk, len, nrep, sTag, gain, sIn );