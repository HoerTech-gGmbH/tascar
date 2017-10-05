function a_fit=tascar_perf_fitmodel(x, rectype)
  sd = libsd();
  % optionally select a single receiver type:
  if nargin >= 2
    x = sd.restrict(x,'ReceiverType',{rectype});
  end
  %fitOpts = optimset('MaxFunEvals',10000);
  fitOpts = optimset('MaxFunEvals',100000);

  a_initial=ones(5,1);
  a_fit=fminsearch(@(a) funErrorMeasure(a,x), a_initial,fitOpts);
  a_fit = a_fit ./ fitw();

function w = fitw()
    w = [20,20,1000,2000,30000]';
  
function E = funErrorMeasure(a,x)
  if any(a<0)
    E = inf;
    return;
  end
  if a(1) > 50
    E = inf;
    return;
  end
  %E = (1 - corr_model_data( a, x ))^2;
  E = ms_error( a, x );

function e = ms_error( a, x )
  sd = libsd();
  [tmp,kCPU] = sd.getfield(x,'cpu');
  C_measurement = 0.01*x.data(:,kCPU);
  C_mod = model_data( a, x );
  e = mean((C_measurement-C_mod).^2);

function C_mod = model_data( a, x )
    a = a./fitw();
  a_0=a(1);
  a_K=a(2);
  a_KP=a(3);
  a_NP=a(4);
  a_NKP=a(5);
  %a_LKP=a(6);
  
  sd = libsd();
  [tmp,kK] = sd.getfield(x,'NrSources');
  [tmp,kN] = sd.getfield(x,'NrSpeakers');
  [tmp,kP] = sd.getfield(x,'Period');
  % number of sources K:
  K=x.values{kK}(x.data(:,kK));
  % number of speakers N:
  N=x.values{kN}(x.data(:,kN));
  % period size P:
  P=x.values{kP}(x.data(:,kP));
  
  %C_mod = ((a_0 + a_K*K + a_KP*(K.*P) + a_NP*(N.*P) +
  %a_NKP*(N.*K.*P) + a_LKP*(L.*K.*P))./P)';
  
  C_mod = (a_0./P + a_K*(K./P) + a_KP*(K) + a_NP*(N) + a_NKP*(N.*K))';

  %C_mod = (a_0 + a_K*(K) + a_KP*(K.*P) + a_NP*(N.*P) + a_NKP*(N.*K.*P))';

