addpath('/usr/share/tascar/matlab');

% Install the javaosctomatlab tool from
% https://0110.be/posts/OSC_in_Matlab_on_Windows,_Linux_and_Mac_OS_X_using_Java
% e.g., with
% wget https://0110.be/files/attachments/430/javaosctomatlab.jar
javaaddpath('javaosctomatlab.jar');

% create receiver at port 1032:
receiver =  com.illposed.osc.OSCPortIn(1032);
try
  % Define the OSC method to listen to:
  osc_value_listener = com.illposed.osc.MatlabOSCListener();
  receiver.addListener("/value", osc_value_listener);

  osc_quit_listener = com.illposed.osc.MatlabOSCListener();
  receiver.addListener("/quit", osc_quit_listener);

  receiver.startListening();
  runscript = true;
  while runscript
    msg = osc_value_listener.getMessageArgumentsAsDouble();
    if ~isempty(msg)
      msg
    end
    quitval = osc_quit_listener.getMessageArgumentsAsDouble();
    if ~isempty(quitval)
      if quitval > 0
        runscript = false;
      end
    end
  end
  clear('osc_quit_listener');
  clear('osc_value_listener');
  receiver.stopListening();
  receiver.close();
  clear('receiver');
catch
  clear('osc_quit_listener');
  clear('osc_value_listener');
  receiver.close();
  clear('receiver');
  warning(lasterror);
end