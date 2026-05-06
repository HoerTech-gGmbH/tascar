function doc = tascar_xml_pretty_print(doc)
% tascar_xml_pretty_print - Modify an XML document in-place to add pretty formatting
%
% Overview:
%   This function takes a Java XML Document object (DOM) and modifies its
%   structure directly to include indentation and newlines. This is useful
%   for debugging or human-readable logging of XML structures without
%   performing a full serialization to a string.
%
% Usage:
%   doc = tascar_xml_pretty_print(doc)
%
% Input:
%   doc - A Java XML Document Object (org.w3c.dom.Document).
%         This object must be valid and instantiated.
%
% Output:
%   doc - The same document object, modified in-place.
%         The DOM tree is updated by inserting text nodes containing
%         whitespace (newlines and spaces) between elements.
%
% Note:
%   This function modifies the DOM structure by inserting text nodes
%   (whitespace) between elements to simulate pretty-printing.
%   It does NOT use XML serialization (e.g., `xmlwrite`).
%
% Algorithm:
%   1. Validation: Checks if the input is a valid Java object.
%   2. Cleanup: Calls `remove_whitespace_inplace` to strip existing
%      whitespace-only text nodes. This ensures a clean slate for formatting.
%   3. Formatting: Calls `add_whitespace` to recursively traverse the DOM
%      and insert new text nodes containing newlines and indentation
%      (2 spaces per level) before every child element.
%
% Example:
%   % Assuming 'xmlDoc' is a valid org.w3c.dom.Document object:
%   xmlDoc = tascar_xml_pretty_print(xmlDoc);
%   % The xmlDoc object now contains whitespace nodes for formatting.
%   % To see the result, you would typically use xmlwrite(xmlDoc).
%
% Dependencies:
%   Requires MATLAB or Octave with Java support enabled.
%
% See also:
%   xmlwrite, org.w3c.dom.Document

    if ~isjava(doc)
        error('Input document must be a Java Document object.');
    end

    %try
    % Start with the root element
    root = doc.getDocumentElement();

    % Step 1: Remove any existing formatting to prevent double-spacing
    % or messy alignment.
    remove_whitespace_inplace(root);

    % Step 2: Recursively add whitespace between nodes.
    % The initial indent is empty ('').
    add_whitespace(doc, root, '');

    %catch err
    %error(['Error while pretty-printing XML in-place: ', err.message]);
    %end

end

function add_whitespace( doc, node, indent )
% add_whitespace - Recursive helper to insert indentation text nodes
%
% This function traverses the DOM tree and inserts text nodes containing
% newlines and spaces before elements to create a visual hierarchy.
%
% Inputs:
%   doc    - The owner document (used to create new text nodes).
%   node   - The current parent node being processed.
%   indent - A string representing the current indentation level (e.g., '  ').

    % Append a newline and the current indent to the parent node.
    % This closes the opening tag of the parent.
    node.appendChild(doc.createTextNode([sprintf('\n'),indent]));
    
    children = node.getChildNodes();
    n = children.getLength();
    
    % Iterate backwards through children. This is done to ensure that
    % when we insert nodes before existing children, the indices of
    % unprocessed children remain stable.
    for k=n:-1:1
        child = children.item(k-1);
        
        if child.getNodeType == child.ELEMENT_NODE
            % Calculate indentation for the next level (add 2 spaces)
            new_indent = [indent, '  '];
            
            % Insert a newline and the new indent *before* the child element.
            % This pushes the child element to the right.
            node.insertBefore(doc.createTextNode([sprintf('\n'),new_indent]),child);
            
            % Recursively process the child element.
            add_whitespace( doc, child, new_indent );
        end
    end
end

% Helper function: recursively remove whitespace text nodes
function remove_whitespace_inplace(node)
% remove_whitespace_inplace - Cleans the DOM by removing existing whitespace
%
% This function recursively traverses the DOM and removes any text nodes
% that contain only whitespace (spaces, tabs, newlines). This is necessary
% to prevent "double formatting" if the document was already partially
% formatted or to clean up parsing artifacts.
%
% Input:
%   node - The current DOM node to process.

    % Get all children of the current node
    children = node.getChildNodes();
    
    % We iterate backwards because removing a node shifts the indices of 
    % subsequent nodes. If we iterated forwards, we would skip nodes.
    for i = children.getLength() - 1 : -1 : 0
        child = children.item(i);
        
        % Check if the child is a text node
        if child.getNodeType() == child.TEXT_NODE
            % Get the text content
            text_content = child.getNodeValue();
            
            % Check if the text is empty or contains only whitespace
            % (matches regex for spaces, tabs, newlines, carriage returns)
            if ~isempty(text_content) && all(isspace(text_content))
                % Remove the node from its parent
                node.removeChild(child);
            end
        elseif child.getNodeType() == child.ELEMENT_NODE
            % If it is an element, recurse into it
            remove_whitespace_inplace(child);
        end
    end
end
