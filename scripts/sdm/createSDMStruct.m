function p = createSDMStruct(varargin)
% p = createSDMStruct(varargin)
% Creates a structure of parameters for SDMpar.m
%
% USAGE:
%
% Default values are returned by typing
% p = createSDMStruct('MicLocs',[]);
% The only required parameter for processing is the microphone array.
% It should describe the microphone array coordinates in Cartesian coordinates 
% the center of the array being the origin. They should be in the same
% order as the impulse responses in IR matrix for SDMpar. However, it is 
% also important that correct sampling frequency is set.
%
% Parameter values can be given as an input with pairs 
% 'parameter',parameter_value
% 
% Parameters:
% 'micLocs', matrix : User defined microphone locations [n_mics, 3] 
%                       [no default]
% 'fs', value       : Sampling frequency in Hz [default 48e3]
% 'c',value         : Speed of sound in m/s [default 345]
% 'winLen',value    : Window length in samples [default 0 == minimum]
% 'parFrames',value : Number of parallel frames in SDMpar [default 8192]
% 'showArray',true or false : Display the microphone array in a figure 
%                             [default false]
% 'DefaultArray' : 'GRASVI25','GRASVI50', 'GRASVI100',or 'SPS200','Bformat': 
%               Readily defined microphone arrays [no default]
%
% EXAMPLES:
%
% See demo*.m

% SDM toolbox : createSDMStruct
% Sakari Tervo & Jukka Patynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

% Check if all arguments are valid
listNames = {'fs','c','winLen','parFrames','showArray','micLocs','DefaultArray'};
for i = 1:2:length(varargin)
    if ~any(strcmpi(listNames,varargin{i}))
        error(['SDM toolbox : createVisualizationStruct : ' ...
            'Unknown parameter ' varargin{i}])
    end
end

% Default values
fs = 48e3; % sampling frequency
c = 345; % speed of sound
winLen = 0;
parFrames = 8192; % Number of parallel processes in SDMpar
showArray = false; % Do not draw the applied microphone array

values = [fs,c,winLen,parFrames,showArray];

for i = 1:length(listNames)-2
    j = find(strcmpi(listNames{i},varargin));
    if any(j)
        values(i) = varargin{j+1};        
    end
    eval(['p.' listNames{i} ' = ' num2str(values(i)) ';']);
end

% Custom microphone array
if any(strcmpi('MicLocs',varargin))
    i = find(strcmpi('MicLocs',varargin));
    p.micLocs = varargin{i+1};
else % or Default microphone array
    if any(strcmpi('DefaultArray',varargin))
        i = find(strcmpi('DefaultArray',varargin));
        defaultArray = varargin{i+1};
    else
        error('You must define the microphone array.')
    end
    if any(strcmpi(defaultArray,'GRASVI50'))
        % G.R.A.S. VI50 vector intensity probe microphone array, with 6
        % omni-directional capsules in the corners of a cube
        %
        % This is the default microphone spacing in GRASVI50
        Radius = 0.025;      % Radius of the microphones [m]
        p.micLocs = ...      % Microphone positions in Cartesian coordinates [m]
            [Radius 0 0;
            -Radius 0 0;
            0 Radius 0;
            0 -Radius 0;
            0 0 Radius;
            0 0 -Radius];
    elseif any(strcmpi(defaultArray, 'GRASVI100'))
        % This microphone array was used for example in:
        % J. Patynen, S. Tervo, T. Lokki, "Analysis of concert hall acoustics via
        % visualizations of time-frequency and spatiotemporal responses",
        % In J. Acoustical Society of America, vol. 133, no. 2, pp. 842-857, 2013.
        Radius = 0.05;      % Radius of the microphones [m]
        p.micLocs = ...      % Microphone positions in Cartesian coordinates [m]
            [Radius 0 0;
            -Radius 0 0;
            0 Radius 0;
            0 -Radius 0;
            0 0 Radius;
            0 0 -Radius];
    elseif any(strcmpi(defaultArray,'GRASVI25'))
        % This microphone array was used for example in:
        % S. Tervo, J. Patynen, N. Kaplanis, M. Lydolf, S. Bech, and T. Lokki
        % "Spatial Analysis and Synthesis of Car Audio System and Car Cabin
        % Acoustics with a Compact Microphone Array",
        % Journal of the Audio Engineering Society 63 (11), 914-925
        % and
        % P. Laukkanen
        % "Evaluation of Studio Control Room Acoustics with Spatial Impulse
        % Responses and Auralization",
        % Master's thesis, Aalto University, 2014
        Radius = 0.0125;      % Radius of the microphones [m]
        p.micLocs = ...       % Microphone positions in Cartesian coordinates [m]
            [Radius 0 0;
            -Radius 0 0;
            0 Radius 0;
            0 -Radius 0;
            0 0 Radius;
            0 0 -Radius];
        
    elseif any(strcmpi(defaultArray,'SPS200'))
        % SPS200 Soundfield microphone, with 4 cardioid capsules in the corners
        % of a tetrahedron
        %
        % This microphone array was used for example in:
        % S. Tervo, J. Saarelma, J. Patynen, P. Laukkanen, I. Huhtakallio,
        % "Spatial analysis of the acoustics of rock clubs and nightclubs",
        % In the IOA Auditorium Acoustics, Paris, France, pp. 551-558, 2015
        p.micLocs = ...
            [0    0.0245    0.0173
            0.0245   0   -0.0173
            -0.0245   0   -0.0173
            -0.0000   -0.0245    0.0173];
    elseif any(strcmpi(defaultArray,'Bformat'))
        % B-format microphone signals
        % Microphone geometry is unknown
        p.micLocs = ...
            [NaN NaN];
    else
        error(['Unknown default microphone array : ' defaultArray])
    end
end

disp('User-defined SDM Settings are used :');
disp(p)

