function tascar_hoa_generate_referencedata 
% tascar_hoa_generate_referencedata - generate test reference data

%% configuration parameters:
order = 5;
duration = 1;
fs = 44100;
fragsize = 64;

%% derived variables:
iduration = ceil(fs*duration/fragsize);
%% generate signals:
vL = tascar_load_speakerlayout( 'lidhan_3d.spk' );
expected_hor_enc = zeros(iduration,(order+1)^2);
expected_hor_hoa3d_pinv = zeros(iduration,size(vL,2));
expected_hor_hoa3d_allrad = zeros(iduration,size(vL,2));
expected_vert_enc = zeros(iduration,(order+1)^2);
expected_vert_hoa3d_pinv = zeros(iduration,size(vL,2));
expected_vert_hoa3d_allrad = zeros(iduration,size(vL,2));
dec = tascar_hoa_decoder_pinv( vL, order );
dec_allrad = tascar_hoa_decoder_allrad( vL, order );
for k=1:iduration
  % trajectory 1:
  az = max(0,(k-2))/(fs*duration/fragsize)*2*pi;
  el = 0;
  B = tascar_hoa_encoder( order, az, el );
  expected_hor_enc(k,:) = B;
  expected_hor_hoa3d_pinv(k,:) = dec*B;
  expected_hor_hoa3d_allrad(k,:) = dec_allrad*B;
  % trajectory 2:
  az = 0;
  el = -(max(0,(k-2))/(fs*duration/fragsize)*2*pi*160/360-pi*80/180);
  B = tascar_hoa_encoder( order, az, el );
  expected_vert_enc(k,:) = B;
  expected_vert_hoa3d_pinv(k,:) = dec*B;
  expected_vert_hoa3d_allrad(k,:) = dec_allrad*B;
end
save_audio('snd_hoa3d_enc_hor',expected_hor_enc,fs,fragsize);
save_audio('snd_hoa3d_pinv_hor',expected_hor_hoa3d_pinv,fs,fragsize);
save_audio('snd_hoa3d_allrad_hor',expected_hor_hoa3d_allrad,fs,fragsize);
save_audio('snd_hoa3d_enc_vert',expected_vert_enc,fs,fragsize);
save_audio('snd_hoa3d_pinv_vert',expected_vert_hoa3d_pinv,fs,fragsize);
save_audio('snd_hoa3d_allrad_vert',expected_vert_hoa3d_allrad,fs,fragsize);

function save_audio( label, x, fs, fragsize )
x = upsample(x,fragsize);
audiowrite(['expected_',label,'.wav'],...
           x(1:fs,:),...
           fs,'BitsPerSample',32);

function x = upsample( x, fragsize )
vT1 = [1:size(x,1)]-1;
vT2 = ([1:(fragsize*size(x,1))])/fragsize;
x = interp1( vT1, x, vT2, 'linear', 'extrap' );