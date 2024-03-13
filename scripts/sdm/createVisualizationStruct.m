function p = createVisualizationStruct(varargin)
% p = createVisualizationStruct(varargin)
% Creates a structure that is used by spatioTemporalVisualization.m
%
% USAGE:
%
% Default values are returned by typing
% p = createVisualizationStruct();
%
% Parameter values can be given as an input with pairs 
% 'parameter',parameter_value
% 
% 'Plane', 'median','transverse',or 'lateral' : visualization plane
%                                               [default 'median']
% 'DOI', 'forward' or 'backward' : Direction of integration
%                                   [default 'forward']
% 'plotStyle','line' or 'fill' : Lines of filled area [default 'line'];
% 'Name', string        : name of the visualization [default 'My Example Room 1']
% 'DOAResolution',value : resolution of DOAs in the visualization [default 1];
% 't', vector          : Time windows of interest in ms 
%                       [default [5 10 15 20 50 80 200 2000]];
% 'Colors'             : array of rgb colors, [default jet(length(t))]
% 'dBSpacing', value   : resolution of the dB grid [default 6]
% 'DOASpacing',  value : resolution of the DOA grid [default 30]
% 'linewidth', value   : vector or single value, the line width to be used 
%                       [default 1]
% 'showGrid', 'true','false' : show the grid in the visualization [default 'true']
% 'dBDynamics', value :   Dynamics of the visualizatio in dB, [default 45];
% 'smoothingMethod', 'average','median','moving' : smoothing method for
%                        polar response [default 'average']
% 'smoothingResolution', single value or vector : smoothing resolution
%                       over the DOA angles in degrees [default 3] degrees.
%                       If vector, then average smoothing is applied with 
%                       the given vector.
%
% EXAMPLES:
%
% Default values, i.e., ready-made settings for visualization
%
% Default values are returned by typing
% p = createVisualizationStruct()
%
% % To use these settings in the visualization, type
% spatioTemporalVisualization(P, DOA, p) % or
%
% %Default settings for different-sized rooms, used in previous research
% % Large room, e.g., concert halls
% p = createVisualizationStruct('DefaultRoom','Large');
%
% % Large rooms, e.g., concert halls or opera houses
% p = createVisualizationStruct('DefaultRoom','Large');
%
% % Medium sized rooms, e.g., small auditoria, theaters, or rock clubs
% p = createVisualizationStruct('DefaultRoom','Medium');
%
% % Small rooms, e.g., living rooms, control rooms, or home theaters
% p = createVisualizationStruct('DefaultRoom','Small');
%
% % Very small rooms, e.g., cars, closets, or bathrooms
% p = createVisualizationStruct('DefaultRoom','VerySmall');
%

% SDM toolbox : createVisualizationStruct
% Sakari Tervo & Jukka P����tynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

if isempty(varargin)
    disp('Using default settings');
end

% Check if all arguments are valid
listNames = {'fs','plane','DOI','plotStyle','name',...
'res','t','colors','dBSpacing','DOASpacing','dBDynamics',...
'linewidth','showGrid','smoothMethod','smoothResolution','DefaultRoom'};
for i = 1:2:length(varargin)
    if ~any(strcmpi(listNames,varargin{i}))
        error(['SDM toolbox : createVisualizationStruct : ' ...
            'Unknown parameter ' varargin{i}])
    end
end

p = {};

% If default settings are used
if any(strcmpi('DefaultRoom',varargin))
    i = find(strcmpi('DefaultRoom',varargin));
    defaultRoom = varargin{i+1};
    
    
    if any(strcmpi('Large',defaultRoom))
        % Default values for LARGE ROOMS (10,000 cubic meters or more)
        % These settings were used for example in:
        % J. P����tynen, S. Tervo, T. Lokki, "Analysis of concert hall acoustics via
        % visualizations of time-frequency and spatiotemporal responses",
        % In J. Acoustical Society of America, vol. 133, no. 2, pp. 842-857, 2013.
        
        p.fs = 48e3; % sampling frequency
        p.plane = 'lateral'; % visualization plane
        p.DOI = 'forward'; % direction of integration
        p.plotStyle = 'line'; % plotting style, fill or line
        p.name = 'My Example Large Room'; % title
        p.res = 1; % resolution of the visualization
        p.t = 20:10:200; % time indices
        p.colors = flipud(bone(round(length(p.t)*1.7))); % colormap
        p.colors = p.colors(5:length(p.t)+4,:);
        p.colors(2,:) = 0; p.colors(end,:) = [1 0 0]';
        p.dBSpacing = -12:6:0; % dB grid
        p.DOASpacing = []; % No DOA grid
        p.linewidth = ones(size(p.t)); % the line width to be used
        p.linewidth([2 end]) = 2; % the line width to be used
        p.showGrid = true; % grid in the visualization
        p.dBDynamics = 60; % dB
        
    elseif any(strcmpi('Medium',defaultRoom))
        % Default values for MEDIUM SIZE ROOMS (1,000 cubic meters or so)
        % These settings were used for example in:
        % S. Tervo, J. Saarelma, J. P����tynen, P. Laukkanen, I. Huhtakallio,
        % "Spatial analysis of the acoustics of rock clubs and nightclubs",
        % In the IOA Auditorium Acoustics, Paris, France, pp. 551-558, 2015
        
        p.fs = 48e3; % sampling frequency
        p.plane = 'lateral'; % visualization plane
        p.DOI = 'backward'; % direction of integration
        p.plotStyle = 'fill'; % plotting style, fill or line
        p.name = 'My Example Medium-Size Room'; % title
        p.res = 1; % resolution of the visualization
        p.t = [0 5 20 50 200]; % time indices
        p.colors = jet(length(p.t)); % colormap
        p.dBSpacing = -20:10:0; % resolution of the dB grid
        p.DOASpacing = []; % No DOA grid
        p.linewidth = zeros(size(p.t)); % the line width to be used
        p.showGrid = true; % grid in the visualization
        p.dBDynamics = 60; % dB
        
    elseif any(strcmpi('Small',defaultRoom))
        % Default values for SMALL SIZE ROOMS (100 cubic meters or so)
        % These settings were used for example in:
        % P. Laukkanen
        % "Evaluation of Studio Control Room Acoustics with Spatial Impulse
        % Responses and Auralization",
        % Master's thesis, Aalto University, 2014
        
        p.fs = 48e3; % sampling frequency
        p.plane = 'lateral'; % visualization plane
        p.DOI = 'backward'; % direction of integration
        p.plotStyle = 'line'; % plotting style, fill or line
        p.name = 'My Example Small-Size Room'; % title
        p.res = 1; % resolution of the visualization
        p.t = [0 5 20 50 200]; % time indices
        p.colors = [0 0 0;0 1 0; 0.1 0.1 .8; 1 0.0 0.1; 0.7 0.7 0.7]; % colormap
        p.dBSpacing = -30:15:0; % resolution of the dB grid
        p.DOASpacing = []; % No DOA grid
        p.linewidth = linspace(5,1,length(p.t)); % the line width to be used
        p.showGrid = true; % grid in the visualization
        p.dBDynamics = 45; % dB
        
    elseif any(strcmpi('VerySmall',defaultRoom))
        % Default values for VERY SMALL ROOMS (30 cubic meters or less)
        % These settings were used for example in:
        % S. Tervo, J. P����tynen, N. Kaplanis, M. Lydolf, S. Bech, and T. Lokki
        % "Spatial Analysis and Synthesis of Car Audio System and Car Cabin
        % Acoustics with a Compact Microphone Array",
        % Journal of the Audio Engineering Society 63 (11), 914-925, 2014
        
        p.fs = 48e3; % sampling frequency
        p.plane = 'lateral'; % visualization plane
        p.DOI = 'backward'; % direction of integration
        p.plotStyle = 'fill'; % plotting style, fill or line
        p.name = 'My Example Very Small Room'; % title
        p.res = 1; % resolution of the visualization
        p.t = [0 2 5 20]; % time indices
        p.colors = gray(length(p.t)+1); % colormap
        p.colors = p.colors(1:length(p.t),:);
        p.dBSpacing = -12:6:0; % resolution of the dB grid
        p.DOASpacing = []; % resolution of the DOA grid
        p.linewidth = zeros(size(p.t)); % no line
        p.showGrid = true; % grid in the visualization
        p.dBDynamics = 36; % dB
        
    end
end
% if user defined values are used
if any(strcmpi('fs',varargin))
    i = find(strcmpi('fs',varargin));
    p.fs = varargin{i+1};
elseif ~isfield(p,'fs')
    p.fs = 48e3; % sampling frequency
end
if any(strcmpi('plane',varargin))
    i = find(strcmpi('plane',varargin));
    p.plane = varargin{i+1};
elseif ~isfield(p,'plane')
    p.plane = 'lateral'; % visualization plane
end
if any(strcmpi('DOI',varargin))
    i = find(strcmpi('DOI',varargin));
    p.DOI = varargin{i+1};
elseif ~isfield(p,'DOI')
    p.DOI = 'forward'; % direction of integration
end
if any(strcmpi('plotStyle',varargin))
    i = find(strcmpi('plotStyle',varargin));
    p.plotStyle = varargin{i+1};
elseif ~isfield(p,'plotStyle')
    p.plotStyle = 'line'; % plotting style, fill or line
end
if any(strcmpi('name',varargin))
    i = find(strcmpi('name',varargin));
    p.name = varargin{i+1};
elseif ~isfield(p,'name')
    p.name = 'My Example Room 1'; % title
end
if any(strcmpi('res',varargin))
    i = find(strcmpi('res',varargin));
    p.res = varargin{i+1};
elseif ~isfield(p,'res')
    p.res = 1; % resolution of the visualization
end
if any(strcmpi('t',varargin))
    i = find(strcmpi('t',varargin));
    p.t = varargin{i+1};
elseif ~isfield(p,'t')
    p.t = [0 5 10 15 20 50 80 200 2000]; % meaningful time indices for a room
end
if any(strcmpi('colors',varargin))
    i = find(strcmpi('colors',varargin));
    p.colors = varargin{i+1};
elseif ~isfield(p,'colors')
    p.colors = jet(length(p.t)); % colormap
end
if any(strcmpi('dBSpacing',varargin))
    i = find(strcmpi('dBSpacing',varargin));
    p.dBSpacing = varargin{i+1};
elseif ~isfield(p,'dBSpacing')
    p.dBSpacing = 6; % resolution of the dB grid
end
if any(strcmpi('DOASpacing',varargin))
    i = find(strcmpi('DOASpacing',varargin));
    p.DOASpacing = varargin{i+1};
elseif ~isfield(p,'DOAspacing')
    p.DOASpacing = 30; % resolution of the DOA grid
end
if any(strcmpi('dBDynamics',varargin))
    i = find(strcmpi('dBDynamics',varargin));
    p.dBDynamics = varargin{i+1};
elseif ~isfield(p,'dBDynamics')
    p.dBDynamics = 45; % dB, dynamics of the magnitude
end
if any(strcmpi('linewidth',varargin))
    i = find(strcmpi('linewidth',varargin));
    p.linewidth = varargin{i+1};
elseif ~isfield(p,'linewidth')
    p.linewidth = ones(size(p.t)); % the line width to be used
end
if any(strcmpi('showGrid',varargin))
    i = find(strcmpi('showGrid',varargin));
    p.showGrid = varargin{i+1};
elseif ~isfield(p,'showGrid')
    p.showGrid = true; % grid in the visualization
end
if any(strcmpi('smoothMethod',varargin))
    i = find(strcmpi('smoothMethod',varargin));
    p.smoothMethod = varargin{i+1};
elseif ~isfield(p,'smoothMethod')
    p.smoothMethod = 'average'; % smoothing method
end
if any(strcmpi('smoothRes',varargin))
    i = find(strcmpi('smoothRes',varargin));
    p.smoothRes = ceil(varargin{i+1}/p.res);
elseif ~isfield(p,'smoothRes')
    p.smoothRes = ceil(3/p.res); % must be an integer
end

if isempty(varargin)
    disp('createVisualizationStruct : Default visualization settings are used:');
    disp(p)
else
    disp('createVisualizationStruct : User-defined visualization settings are used :');
    disp(p)
end


