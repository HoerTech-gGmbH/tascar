function vL = tascar_load_speakerlayout( sName )
%TASCAR_LOAD_SPEAKER_LAYOUT - load a tascar speaker layout file
%
%Usage:
%vL = tascar_load_speaker_layout( sName );
%
% sName : layout file name (e.g., 'lidhan_3d.spk')
% vL  : list of N speaker positions, 3xN
%
%This function requires tools from /usr/share/tascar/matlab.
doc = tascar_xml_open( sName );
cEl = tascar_xml_get_element( doc, 'speaker' );
% return list of row vectors, one row for each speaker:
vL = zeros(3,numel(cEl));
for k=1:numel(cEl)
    az = get_attr_num( cEl{k}, 'az', 0 );
    el = get_attr_num( cEl{k}, 'el', 0 );
    r = get_attr_num( cEl{k}, 'r', 1 );
    [x,y,z] = sph2cart( az*pi/180, el*pi/180, r );
    vL(:,k) = [x;y;z];
end

function v = get_attr_num( el, name, def )
s = char(tascar_xml_get_attribute( el, name ));    
if isempty(s)
    v = def;
else
    v = str2num(s);
end
