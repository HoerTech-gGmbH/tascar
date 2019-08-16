function trwav( name )
name = ['expected_',name,'.wav'];
[x,fs] = audioread(name);
audiowrite(name,x(1:44100,:),fs,'BitsPerSample',32);