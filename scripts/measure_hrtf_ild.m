function [ild,vaz,vf] = measure_hrtf_ild( varargin )
  fname = [tempname(),'.tsc'];
  doc = tascar_xml_doc_new();
  e_session = tascar_xml_add_element( doc, doc, 'session', [], 'license','CC0','duration','360' );
  e_scene = tascar_xml_add_element( doc, e_session, 'scene' );
  e_src = tascar_xml_add_element( doc, e_scene, 'source' );
  e_snd = tascar_xml_add_element( doc, e_src, 'sound', [], ...
                                  'x','1','delayline','false',...
                                  'airabsorption','false' );
  e_orient = tascar_xml_add_element( doc, e_src, 'orientation', ...
                                   '0 0 0 0 360 360 0 0');
  e_rec = tascar_xml_add_element( doc, e_scene, 'receiver' , [], ...
                                  'type', 'hrtf', varargin{:});
  tascar_xml_save(doc, fname );
  vaz = [-180:5:180];
  ild = [];
  for az=vaz
    ir = render_ir( 48000, 48000, fname, az );
    H = 20*log10(abs(realfft(ir)));
    ild(:,end+1) = diff(H,[],2);
    %ild(:,end+1) = ir(:,1);
  end
  vf = 1000*2.^[-2:3];
  ild = ild(vf+1,:);
end

function ir = render_ir( fs, irlen, tscname, t )
    irname = ['hrtfir.wav'];
    system(['LD_LIBRARY_PATH='''' tascar_renderir -o ',irname,...
            ' -l ',num2str(irlen),' -f ',num2str(fs),' -t ',...
            num2str(mod(t,360)),' ',tscname]);
    [ir,fs] = audioread(irname);
end
