function plotPolarResponse(H, plane, color, lw, res, dBdynamics, maxdB, X, ...
    name, plotStyle, flagGrid, dBSpacing, DOAspacing)
% plotPolarResponse(H, plane, color, lw, res, dBdynamics, maxdB, X, ...
%    name, plotStyle, flagGrid, dBSpacing, DOAspacing)
% Plots polar response of given H vector
%
% H     : Directional energy histogram, i.e., energy response in linear
%        scale (not in dB)
% plane : 'lateral', 'median' or 'transverse'
% color : color in matlab RGB colors, for example [0 0 1] or 'k'
% lw    : the width of line, for example 2
% res   : resolution
% dbDynamis : dynamic range in dB
% name  : title of the plot
% X     : origin in the visualization, e.g., [0 0]
% flagGrid  : true if grid is visible, false is off
% plotStyle : 'Line' or 'Fill' -style in the visualization 
% dBSpacing : grid of dB values in the visuzalization
% DOAspacing : grid of DOA values in the visuzalization

% SDM toolbox :  plotPolarResponse
% Sakari Tervo & Jukka P����tynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

Scale = 1;

% Dynamics, i.e., limits for the polar plot -mindB to maxdB dB
lims = [maxdB-dBdynamics, maxdB];

% Limiting the data from -mindB to maxdB dB
A = 10*log10(H); % from linear to logarithmic scale
A( A < lims(1)) = lims(1);
A( A > lims(2)) = lims(2);
A = A - lims(1);
A = Scale*A/(abs(diff(lims)));

% --- Plot the polar response ----
switch lower(plotStyle)
    case 'line'
        [x,y] = pol2cart([-pi-2*pi/180*res -pi-pi/180*res -pi:pi/180*res:pi], [A(end-1), A(end), A']);
        if lw <= 0 || isempty(lw)
            warning('plotPolarResponse : linewidth 0 or not specified, using linewidth 1')
            plot(x+X(1),y+X(2),'-','Linewidth',1,'Color', color)
        else
            plot(x+X(1),y+X(2),'-','Linewidth',lw,'Color', color)
        end
    case 'fill'
        [x,y] = pol2cart(-pi:pi/180*res:pi, A');
        % use gray color for the edge if edge is shown
        if lw > 0
            fill(x+X(1),y+X(2), color, 'EdgeColor',[0.7 0.7 0.7],'Linewidth', lw)
        elseif isempty(lw) || lw == 0
            fill(x+X(1),y+X(2), color, 'EdgeColor','none')
        end
end

% --- EOF Plot the polar response ----

% This part adds orientation and bearing description and grid lines to the plot
if flagGrid % is true

    % Bearings and orientation titles
    % Switch selects texts correponding to the selected plane
    % These names may be meaningful only in room acoustics
    switch lower(plane)
        case {'lateral'}
            % lateral
            names = {'Front', 'Right', 'Left', 'Back'};
        case {'median'}
            % median
            names = {'Front', 'Bottom', 'Top', 'Back'};
        case {'transverse'}
            % transverse
            names = {'Right','Bottom','Top','Left'};
    end
    
    if numel(dBSpacing) > 1
        lim = -dBSpacing(end:-1:1);
    else
        lim = 0:dBSpacing:dBdynamics; % dB grid points (at every 10 dB)
        lim = lim(end:-1:1);
    end
    
    
    % Transform the dB scale to radius
    B = maxdB-lim;
    B( B < lims(1)) = lims(1);
    B( B > lims(2)) = lims(2);
    B = B - lims(1);
    B = Scale*B/(abs(diff(lims)));
    
    % Names for the dB grid lines
    bName = cell(size(lim));
    for idx = 1:length(lim)
        bName{idx} = ['-', num2str(lim(idx)),'dB'];
    end
    
    % Draw dB grid lines and add descrption with text()
    for l = 1:length(B)
        [x,y] = pol2cart(-pi:pi/180:pi, B(l)*ones(1,361));
        plot(x,y,':','Color',[0.3 0.3 0.3])
        [x,y] = pol2cart(75/180*pi, B(l));
        text(x,y,bName{l})
    end
    
    % Draw DOA grid lines and add descrption with text()
    % grid points at every 30 degrees
    for a = sort(unique((-180+DOAspacing):DOAspacing:180))
        r = 0:0.01:Scale;
        [x,y] = pol2cart(a/180*pi*ones(size(r)), r);
        plot(x,y,':', 'Color', [0.3 0.3 0.3])
        [x,y] = pol2cart(a/180*pi, Scale);
        text(x,y,[num2str(a) '$^{\circ}$'])
        if a == 0 % Right
            text(x+Scale*0.15,y,names{1})
        end
        if a == -90 % bottom
            text(x,y-Scale*0.15,names{2})
        end
        if a == 90 % top
            text(x,y+Scale*0.15,names{3})
        end
        if a == 180 % left
            text(x-Scale*0.3,y,names{4})
        end
    end
    % Limit the plot to the visible data
    xlim([-Scale Scale])
    ylim([-Scale Scale])    
end
box off
axis off
axis equal

% Name of the plot
title(name);
hold on;