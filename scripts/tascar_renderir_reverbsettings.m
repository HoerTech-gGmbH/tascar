function [ir,fs] = tascar_renderir_reverbsettings( fname, irlen, fs, varargin )
 %render_ir_with_settings - render with reverb settings
 %
 % Usage:
 %  [ir,fs] = tascar_renderir_reverbsettings( fname, irlen, fs, varargin )
 %
 % Example:
 % [ir,fs] = tascar_renderir_reverbsettings('Buero_simplefdn.tsc',2*44100, 44100, 'absorption',0);
 % plot(ir)
 %
  if mod(numel(varargin),2)==1
    error('expected key/value pair, odd number');
  end
  if ~isempty(strfind(fname,filesep))
    warning('Input session file should be in current directory');
  end
  doc = tascar_xml_open(fname);
  %% remove all sound files:
  doc = xml_remove_sndfile( doc );
  elst = tascar_xml_get_element( doc, 'include' );
  cRMFiles = {};
  for k=1:numel(elst)
    incname = tascar_xml_get_attribute( elst{k}, 'name');
    inctscname = [tempname('./'),'.itsc'];
    tascar_xml_set_attribute( elst{k}, 'name', inctscname );
    incdoc = tascar_xml_open(incname);
    incdoc = xml_remove_sndfile( incdoc );
    tascar_xml_save(incdoc,inctscname);
    clear('incdoc');
    cRMFiles{end+1} = inctscname;
  end
  for k=1:2:numel(varargin)
    key = varargin{k};
    val = varargin{k+1};
    if ~ischar(val)
      val = num2str(val);
    end
    doc = tascar_xml_edit_elements(doc,'reverb',key,val);
  end
  tscname = [tempname('./'),'.tsc'];
  irname = [tempname('./'),'.wav'];
  tascar_xml_save(doc,tscname);
  system(['LD_LIBRARY_PATH='''' tascar_renderir -o ',irname,...
          ' -l ',num2str(irlen),' -f ',num2str(fs),' ',tscname]);
  [ir,fs] = audioread(irname);
  delete(tscname);
  delete(irname);
  for c=cRMFiles
    delete(c{:});
  end
end

function doc = xml_remove_sndfile( doc )
  elst = tascar_xml_get_element( doc, 'sndfile' );
  for k=1:numel(elst)
    parent = javaMethod('getParentNode',elst{k});
    javaMethod('removeChild',parent,elst{k});
  end
  elst = tascar_xml_get_element( doc, 'sndfileasync' );
  for k=1:numel(elst)
    parent = javaMethod('getParentNode',elst{k});
    javaMethod('removeChild',parent,elst{k});
  end
end
