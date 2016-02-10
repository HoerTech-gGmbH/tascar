function [load, xruns]=measure_performance(NrSources,NrSpeakers, ...
					   Period,DelayLine,ReceiverType)
  load = NaN;
  xruns = NaN;
  
  %check if the number of speakers is enough for that receiver            
  if (NrSpeakers<3 && strcmp(ReceiverType,'hoa2d')==1 )|| ...
	(NrSpeakers<2  && strcmp(ReceiverType,'vbap')==1)
    return
  end

  test_filename='scene';

  attempts = 0;
  success = false;
  % try until the next steps were successful:
  while ~success
    try
      %% try to start jack:
      jackpid = tascar_ctl('jackddummy_start',Period);
      success = true;
    catch
      if attempts < 10
	% retry if error occured (success is still false)
	warning(lasterr);
	attempts = attempts + 1;
      else
	% more than 10 attempts, rethrow:
	error(lasterr);
      end
    end
  end

  % create output ports:

  %third order ambisonics horizontal
  if strcmp(ReceiverType, 'amb3h0v')
    channels_Ambi={'.0w','.1x','.1y','.2u', '.2v','.3p','.3q'};
    for kk=1:7
      c_OutPorts{kk}=['render.' ,test_filename,':','out',channels_Ambi{kk}];
    end
    
    %other speaker based methods
  else
    c_OutPorts=cell(NrSpeakers,1);
    for kk=1:NrSpeakers
      c_OutPorts{kk}=['render.' ,test_filename,':','out.',num2str(kk-1)];
    end
    
  end


  % create input ports:
  c_InPorts=cell(NrSources,1);
  for nn=1:NrSources
    c_InPorts{nn}=['render.' ,test_filename,':','src_',num2str(nn),'.0'];
  end

  attempts = 0;
  success = false;
  while ~success
    try

      %generate a scene with a given paramer set
      tascar_ctl('createscene',...
		 'filename', [test_filename,'.tsc'], ...
		 'nrsources',NrSources, 'nrspeakers', NrSpeakers, ...
		 'rec_type',ReceiverType,'maxdist',DelayLine);
      
      
      %load the generate scene in tascar
      h_temp=tascar_ctl('load',[test_filename,'.tsc']);
      pause(5);
      
      
      %measure the cpu load and xruns and store it somewhere
      test_sig=0.1*(randn(4*44100,NrSources)-0.5);

      [y,fs,bufsize,load,xruns,sCfg]=tascar_jackio( test_sig,'output' ,c_InPorts , 'input',  c_OutPorts);
      
      tascar_ctl('kill',h_temp);
      success = true;
    catch
      if attempts < 10
	% retry if error occured:
	warning(lasterr);
	attempts = attempts + 1;
      else
	NrSources
	NrSpeakers
	ReceiverType
	DelayLine
	Period
	system('killall -9 jackd tascar');
	error(lasterr);
      end
    end
  end
  tascar_ctl('killpid',jackpid);
