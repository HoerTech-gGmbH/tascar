function spatioTemporalVisualization(P, DOA, p)%, nameFigure,fc)
% spatioTemporaVisualization(P, DOA, p)
% Calculates and visualizes the spatio-temporal response of P an DOA for given 
% plane and time indices.
%
% USAGE:
% 
% P     : Impulse response (Pressure) in the center of the microphone array
%          Size [N 1]
% DOA   : 3-D locations (Cartesian) of each image-source, from SDMpar
%          Size [N 3]
% p     : struct from createVisualizationStruct.m or can be manually given
% 
% EXAMPLES:
% 
% For further examples for setting the parameters in p, see 
% createVisualizationStruct.m or demo*.m
%
% References
% [1] J. P����tynen, S. Tervo, T. Lokki, "Analysis of concert hall acoustics via
% visualizations of time-frequency and spatiotemporal responses",
% In J. Acoustical Society of America, vol. 133, no. 2, pp. 842-857, 2013.

% SDM toolbox : spatioTemporalVisualization
% Sakari Tervo & Jukka P����tynen, Aalto University, 2011-2016
% Sakari.Tervo@aalto.fi and Jukka.Patynen@aalto.fi

if nargin < 2
    error(['SDM toolbox : spatioTemporalVisualization : At least impulse response'
        ' and microphone array must be defined > SDMPar(IR, micArray)']);
end

disp(['Started spatio-temporal visualization.' ]);tic

ir_threshold = 0.2; % treshold level for the beginning of direct sound

HS = zeros(360/p.res + 1, length(p.t)); 

numOfSources = length(P);

% meaningful time indices for a room
t = round(p.t/1000*p.fs);
    
for s = 1:numOfSources
    
    % Find the direct sound
    ind = find(abs(P{s})/max(abs(P{s})) > ir_threshold,1,'first');
    pre_threshold = round(0.001*p.fs); % go back 1 ms
    t_ds = ind - pre_threshold; % direct sound begins from this sample
    
    % make sure that the time index of direct sound is greater than or equal to 1
    t_ds = max(t_ds, 1);
    
    % make sure the integration does not exceed the vector length
    t_end = length(P{s});

    % Iterate through different time windows
    for k = 1:length(t)
        switch lower(p.DOI)
            case {'backward'} % USEFUL FOR SMALL ROOMS
                % --- BACKWARD INTEGRATION from t to t(end) ----
                t2 = min(t_ds+t(k),t_end);
                H = calcPolarResponse(DOA{s}(t2:t_end,:), ...
                    P{s}(t2:t_end), p.res, p.plane);
                
            case {'forward'} % USEFUL FOR LARGE ROOMS
                % ----- FORWARD INTEGRATION from direct sound to t ------
                t2 = min(t_ds+t(k),t_end);
                H = calcPolarResponse(DOA{s}(t_ds:t2,:), ...
                    P{s}(t_ds:t2), p.res, p.plane);
                
        end
        % Sliding smoothing of the energy distribution
        HS(:,k) = HS(:,k) + circSlidingSmoothing(H, p.smoothRes, p.smoothMethod);
    end
end
% --- Plot the polar responses for each time window ----
% find the maximum for plotting
maxdB = 10*log10(max(HS(:)));

flagGrid = false;
% change this central_coords to obtain off-central visulization
central_coords = [0,0];
fig_stamp = figure;
fig_stamp.WindowState = 'maximized';
hold on
switch lower(p.DOI)
    case {'backward'}
        for k = 1:length(t)
            if k == length(t) && p.showGrid
                flagGrid = true;
            end
            plotPolarResponse(HS(:,k), p.plane, p.colors(k,:), p.linewidth(max(1,k)), p.res, ...
                p.dBDynamics, maxdB, central_coords, p.name, p.plotStyle, flagGrid, ...
                p.dBSpacing, p.DOASpacing);
        end
    case {'forward'}
        HS = HS(:,end:-1:1);
        for k = 1:length(t)
            if k == length(t) && p.showGrid
                flagGrid = true;
            end
            plotPolarResponse(HS(:,k), p.plane, p.colors(length(t)-k+1,:), p.linewidth(max(1,length(t)-k+1)), p.res, ...
                p.dBDynamics, maxdB, central_coords, p.name, p.plotStyle, flagGrid, ...
                p.dBSpacing, p.DOASpacing);
        end
end
% Create names for the legend
legName = cell(1,length(t));

switch lower(p.DOI)
    case {'backward'}
        for k = 1:length(t)
            legName{k} = [num2str(p.t(k)) ' - ' num2str(round(t_end/p.fs*1000)) ' ms'];
        end
    case {'forward'}
        for k = 1:length(t)
            legName{length(t)-k+1} = ['0 - ' num2str(p.t(k)) ' ms'];
        end
end
legend(legName);

hold off
%nameTitle = split(nameFigure,'_');
%fig_stamp = title(strcat(nameTitle(2),'-',nameTitle(3),': ',nameTitle(4),'- ',string(fc),' Hz -',p.plane));

%saveas(fig_stamp,strcat('PolarDiagram-',p.plane,'-fig\',nameFigure,'- ',string(fc),' Hz -',p.plane),'fig')
%pause(1)
%saveas(fig_stamp,strcat('PolarDiagram-',p.plane,'-svg\',nameFigure,'- ',string(fc),' Hz -',p.plane),'svg')
%pause(1)

%disp(['Ended spatio-temporal visualization in ' num2str(toc) ' seconds.' ])

end

function y = circSlidingSmoothing(x,siz, method)
% y = circSlidingSmoothing(x,siz)
% Sliding smoothing for circular data over the first dimension
%
% USAGE:
% 
% y         : smoothed data, same size as x
% x         : circular data, matrix or vector dimensions [N d],
%             where N is 
% method    : 'median','average', and if you have Curve fitting toolbox
%              'moving','lowess','loess','sgolay','rlowess','rloess'
% siz       : size of the smoothing window, single value
%             if vector, then average smoothing is used
%             and the vector is used as a weighting function
%
% EXAMPLES:
% 
% Simple average smoothing over 3 values
% y = circSlidingSmoothing(randn(100,3),3);
% 
% Averaging with a smooth window
% y = circSlidingSmoothing(randn(100,5),hanning(11));

% SDM toolbox : circSlidingSmoothing
% Sakari Tervo & Jukka P����tynen, Aalto University, 2011-2016
% Copyleft

if nargin < 3
    method = 'average';
end
xrep = [x; x; x];

% if window size is a number
if numel(siz) == 1
    win = 1/siz*ones(siz,1);
else
    method = 'average';
    win = siz;
end
% Else define a windowing function, e.g. hanning

switch lower(method)
    case 'median'
        xfilt = medfilt1(xrep,siz);
        y = xfilt(size(x,1)+1-floor(siz/2):2*size(x,1)-floor(siz/2),:);
    case 'average'
        xfilt = filter(win,1,xrep,[],1);
        y = xfilt(size(x,1)+1-floor(siz/2):2*size(x,1)-floor(siz/2),:);
    otherwise
        xfilt = smooth(xrep,siz,method);
        y = xfilt(size(x,1)+1:2*size(x,1),:);
end


end


