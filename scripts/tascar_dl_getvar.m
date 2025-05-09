function data = tascar_dl_getvar( cData, sName )
% tascar_dl_getvar Retrieves a variable from a data structure
% based on its name.
%
% Syntax:
%   data = tascar_dl_getvar(cData, sName)
%
% Description:
%   This function searches through the elements of cData to find
%   the one whose 'name' field matches sName. If found, it returns
%   that element; otherwise, it throws an error.
%
% Inputs:
%   cData - A cell array of structs, each containing a 'name' field.
%   sName - The name to search for in the 'name' fields of cData.
%
% Outputs:
%   data - The struct from cData whose 'name' matches sName.
%
% Errors:
%   Throws an error if no element in cData has a 'name' matching sName.
%
% Example:
%   data = tascar_dl_getvar(cData, 'myVariable');

    dataidx = [];
    for k = 1:numel(cData)  % Iterate over each element in cData
        if strcmp(cData{k}.name, sName)  % Check if the 'name' field
                                         % matches sName
            dataidx = k;  % Store the index of the matching element
            break;  % Exit the loop once the match is found
        end
    end

    if isempty(dataidx)
        error(['Variable "', sName, '" not found in data.']);
        % Throw error if no match found
    end

    data = cData{dataidx}.data;  % Retrieve the matching element from cData
end
