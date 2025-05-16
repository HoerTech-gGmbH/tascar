function tascar_plot_speakerlayout(layoutname)
% TASCAR_PLOT_SPEAKERLAYOUT Creates a 3D plot of a speaker layout.
%   This function generates a 3D visualization of a speaker layout defined in an XML file.
%
% Syntax:
%   tascar_plot_speakerlayout(layoutname)
%
% Description:
%   - layoutname: Name of the XML file containing the speaker layout definition.
%   - The function parses the XML file, extracts speaker positions, and plots them in 3D space.
%   - Speakers are represented as blue dots, and their labels are displayed next to them.
%   - The plot includes axes and a connecting line between speakers for better visualization.
%
% Example:
%   tascar_plot_speakerlayout('my_speaker_layout.xml');
%
% Notes:
%   - The XML file must contain 'speaker' elements with 'az', 'el', and 'r' attributes.
%   - The 'gain' attribute is optional and used to scale the marker size.
%   - The plot uses spherical coordinates converted to Cartesian coordinates.
%   - Requires the TASCAR XML functions for parsing the layout file.

  % Open the XML layout file
  doc = tascar_xml_open(layoutname);

  % Get all 'speaker' elements from the XML document
  elem_spk = tascar_xml_get_element(doc, 'speaker');

  % Create a new figure
  figure;

  % Plot the 3D axes
  plot3([0, 1], [0, 0], [0, 0], 'r-'); % X-axis
  hold on;
  plot3([0, 0], [0, 1], [0, 0], 'g-'); % Y-axis
  plot3([0, 0], [0, 0], [0, 1], 'b-'); % Z-axis
  plot3(0, 0, 0, 'k-'); % Origin

  % Initialize arrays to store speaker gains and positions
  vP = [];
  vgains = zeros(numel(elem_spk), 1);

  % Extract gains from speaker elements (if available)
  try
    for k = 1:numel(elem_spk)
      vgains(k) = get_attr_num(elem_spk{k}, 'gain', 0);
    end
  catch
    % Ignore errors if 'gain' attribute is not present
  end

  % Normalize gains to scale marker sizes
  mingain = min(vgains);
  maxgain = max(vgains);
  g = (vgains - mingain) / max(1, (maxgain - mingain));

  % Plot each speaker
  for k = 1:numel(elem_spk)
    % Get speaker attributes
    az = get_attr_num(elem_spk{k}, 'az', 0); % Azimuth angle in degrees
    el = get_attr_num(elem_spk{k}, 'el', 0); % Elevation angle in degrees
    r = get_attr_num(elem_spk{k}, 'r', 1);   % Radius

    % Get speaker label
    l = tascar_xml_get_attribute(elem_spk{k}, 'label');

    % Set color and message based on validity of radius
    if ~isfinite(r)
      msg = sprintf(' invalid %d: %s', k, l);
      col = [0.7, 0.1, 0.1]; % Red for invalid entries
      r = 1; % Default radius for invalid entries
    else
      msg = sprintf(' %d: %s', k, l);
      col = [0.1, 0.7, 0.1]; % Green for valid entries
    end

    % Convert spherical to Cartesian coordinates
    [x, y, z] = sph2cart(az * pi / 180, el * pi / 180, r);

    % Plot the speaker position
    plot3(x, y, z, 'bo', 'LineWidth', 2, 'MarkerSize', 1 + g(k) * 20);
    text(x, y, z, msg, 'Color', col, 'FontWeight', 'bold');

    % Store position for connecting lines
    vP(end + 1, :) = [x, y, z];
  end

  % Plot connecting lines between speakers
  plot3(vP(:, 1), vP(:, 2), vP(:, 3), 'k-', 'Color', 0.7 * ones(1, 3));
  plot3(vP(:, 1), vP(:, 2), vP(:, 3), 'b.');

  % Set axis limits and labels
  xlim([min(vP(:, 1)), max(vP(:, 1))] + [-0.5, 0.5]);
  ylim([min(vP(:, 2)), max(vP(:, 2))] + [-0.5, 0.5]);
  zlim([min(vP(:, 3)), max(vP(:, 3))] + [-0.5, 0.5]);
  xlabel('x / m');
  ylabel('y / m');
  zlabel('z / m');
  set(gca, 'DataAspectRatio', [1, 1, 1]);
end

function v = get_attr_num(el, name, def)
  % GET_ATTR_NUM Safely retrieves a numerical attribute from an XML element.
  %   Returns the default value if the attribute is not found or invalid.
  %
  % Syntax:
  %   v = get_attr_num(el, name, def)
  %
  % Description:
  %   - el: XML element to query.
  %   - name: Name of the attribute to retrieve.
  %   - def: Default value to return if the attribute is not found or invalid.
  %   - v: The numerical value of the attribute or the default value.
  %
  % Example:
  %   v = get_attr_num(element, 'gain', 0);

  s = tascar_xml_get_attribute(el, name);
  if isempty(s)
    v = def;
  else
    v = str2num(s);
  end
end
