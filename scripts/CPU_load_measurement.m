function sData = CPU_load_measurement( tag )

  [a,tascarver] = ...
      system('dpkg-query -W --showformat=''${Architecture} ${Version}'' tascarpro');
  [a,uname] = system('uname -a');
  [a,hostname] = system('hostname');
  [a,cpumodel] = system(...
      ['cat /proc/cpuinfo|',...
       'grep -e "model name"|tail -1|',...
       'sed "s/.*://1"']);

  hostname = strrep(hostname,char(10),'');
  uname = strrep(uname,char(10),'');
  cpumodel = strrep(cpumodel,char(10),'');
  
  if nargin < 1
    tag = hostname;
  end

  for cpu=0:16
    system(sprintf('cpufreq-selector -c %d -g performance',cpu));
  end  

  %This is a script to perform performance measurement (using data menagement
  %tool "libsd" and function  measure_performance.m)

  h = libsd();

  sPar.fields = {'NrSources','NrSpeakers','Period','DelayLine','ReceiverType','cpu','xruns'};

  sPar.values = {...
      [1,10,100],...
      [10,30,100],...
      [64,256,1024],...
      [1,10000] ,...
      {'nsp','vbap','hoa2d'}...
		};

  sData = h.eval(sPar,@measure_performance,'display',true,'nrep',4);

  sData.tascarver = tascarver;
  sData.uanme = uname;
  sData.hostname = hostname;
  sData.cpumodel = cpumodel;

  struct2mfile(sData,['results_',tag,'_',datestr(now(),'YYYYmmDD_HHMMss')]);