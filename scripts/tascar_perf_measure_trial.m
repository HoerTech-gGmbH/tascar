function [dLoad,dTPrep] = tascar_perf_measure_trial(iNumSources,iNumSpeakers, ...
                                    iFragsize,dMaxdist,sReceiverType)
  dLoad = NaN;
  
  %check if the number of speakers is enough for that receiver            
  if (iNumSpeakers<3 && strcmp(sReceiverType,'hoa2d')==1 )|| ...
          (iNumSpeakers<2  && strcmp(sReceiverType,'vbap')==1)
    return
  end
  
  % create filenames:
  sBase = tempname();
  sTSC = [sBase,'.tsc'];
  sAudioIn = [sBase,'_in.wav'];
  sAudioOut = [sBase,'_out.wav'];
  sCommand = 'tascar_renderfile';
  
  % duration
  dDuration = 10;
  
  % generate a scene with a given paramer set
  tascar_ctl( 'createscene',...
              'filename', sTSC, ...
              'nrsources',iNumSources, 'nrspeakers', iNumSpeakers, ...
              'rec_type',sReceiverType,'maxdist',dMaxdist);
  
  % create the test signal:
  test_sig=0.1*(randn(dDuration*44100,iNumSources)-0.5);
  audiowrite(sAudioIn,test_sig,44100);

  % load the generate scene in tascar
  [err,msg] = system(sprintf('%s -d -f %d -i %s -o %s -v %s 2>/dev/null',...
                             sCommand,iFragsize,sAudioIn,sAudioOut,sTSC));
  tmp = str2num(msg);
  dTPrep = tmp(1);
  dLoad = 100*tmp(2)/dDuration;
  system(['rm -f ',sTSC,' ',sAudioIn,' ',sAudioOut]);
  